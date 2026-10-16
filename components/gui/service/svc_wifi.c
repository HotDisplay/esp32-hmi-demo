#include "svc_wifi.h"

#include <ctype.h>
#include <string.h>
#include "sdkconfig.h"

#if CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_WIFI_REMOTE_ENABLED
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "wifi_init.h"

typedef enum { SCAN, CONNECT, DISCONNECT, SCAN_DONE, ASSOCIATED, DISCONNECTED, GOT_IP, LOST_IP } request_type_t;
typedef struct {
    request_type_t type;
    wifi_config_t config;
    esp_netif_ip_info_t ip;
    uint16_t reason;
    uint32_t scan_status;
    char ssid[33];
} wifi_request_t;

static const char *TAG = "WIFI_DEMO";
static SemaphoreHandle_t s_mutex;
static QueueHandle_t s_requests;
static TaskHandle_t s_task;
static svc_wifi_status_t s_status;
static bool s_ready;
static bool s_want_connection;
static bool s_disconnect_queued;
static unsigned s_retries;
static int64_t s_deadline;

static void set_error(esp_err_t error)
{
    s_want_connection = false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_status.state = SVC_WIFI_ERROR;
    s_status.pending = false;
    s_status.error = error;
    xSemaphoreGive(s_mutex);
    s_deadline = 0;
    ESP_LOGW(TAG, "operation failed: %s", esp_err_to_name(error));
}

static void event_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    wifi_request_t request = { 0 };
    if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        request.type = SCAN_DONE;
        request.scan_status = ((wifi_event_sta_scan_done_t *)data)->status;
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        request.type = ASSOCIATED;
        wifi_event_sta_connected_t *event = data;
        memcpy(request.ssid, event->ssid, event->ssid_len > 32 ? 32 : event->ssid_len);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        request.type = DISCONNECTED;
        request.reason = ((wifi_event_sta_disconnected_t *)data)->reason;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        request.type = GOT_IP;
        request.ip = ((ip_event_got_ip_t *)data)->ip_info;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP) {
        request.type = LOST_IP;
    } else {
        return;
    }
    /* Event callbacks only enqueue data; RPCs run outside the event task. */
    if (xQueueSend(s_requests, &request, 0) != pdTRUE) ESP_LOGW(TAG, "event queue full");
}

static esp_err_t ensure_ready(void)
{
    if (s_ready) return ESP_OK;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_status.state = SVC_WIFI_STARTING;
    xSemaphoreGive(s_mutex);
    esp_err_t err = wifi_init();
    s_ready = err == ESP_OK;
    return err;
}

static void read_scan_results(void)
{
    wifi_ap_record_t records[SVC_WIFI_MAX_APS];
    uint16_t count = SVC_WIFI_MAX_APS;
    esp_err_t err = esp_wifi_scan_get_ap_records(&count, records);
    if (err != ESP_OK) {
        esp_wifi_clear_ap_list();
        set_error(err);
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_status.ap_count = 0;
    for (unsigned i = 0; i < count; i++) {
        if (records[i].ssid[0] == 0) continue;
        bool duplicate = false;
        for (unsigned j = 0; j < s_status.ap_count; j++) {
            if (strncmp(s_status.aps[j].ssid, (char *)records[i].ssid, 32) == 0) duplicate = true;
        }
        if (duplicate) continue;
        svc_wifi_ap_t *ap = &s_status.aps[s_status.ap_count++];
        memcpy(ap->ssid, records[i].ssid, 32);
        ap->ssid[32] = 0;
        ap->rssi = records[i].rssi;
        ap->password_required = records[i].authmode != WIFI_AUTH_OPEN;
        ap->supported = records[i].authmode == WIFI_AUTH_OPEN || records[i].authmode == WIFI_AUTH_WPA2_PSK ||
                        records[i].authmode == WIFI_AUTH_WPA_WPA2_PSK || records[i].authmode == WIFI_AUTH_WPA3_PSK ||
                        records[i].authmode == WIFI_AUTH_WPA2_WPA3_PSK;
    }
    s_status.scan_generation++;
    s_status.pending = false;
    s_status.state = s_status.ip[0] ? SVC_WIFI_ONLINE : SVC_WIFI_READY;
    s_status.error = ESP_OK;
    xSemaphoreGive(s_mutex);
    s_deadline = 0;
}

static void process_request(wifi_request_t *request)
{
    svc_wifi_status_t status;
    svc_wifi_get_status(&status);
    esp_err_t err = ESP_OK;
    if (request->type == SCAN || request->type == CONNECT) {
        err = ensure_ready();
        if (err != ESP_OK) {
            set_error(err);
            return;
        }
    }
    switch (request->type) {
    case SCAN: {
        wifi_scan_config_t cfg = { .show_hidden = false };
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_status.state = SVC_WIFI_SCANNING;
        s_status.error = ESP_OK;
        xSemaphoreGive(s_mutex);
        s_deadline = esp_timer_get_time() + 20000000;
        err = esp_wifi_scan_start(&cfg, false);
        break;
    }
    case CONNECT:
        s_want_connection = true;
        s_retries = 0;
        s_deadline = esp_timer_get_time() + 20000000;
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        memcpy(s_status.ssid, request->config.sta.ssid, 32);
        s_status.ssid[32] = 0;
        s_status.state = SVC_WIFI_CONNECTING;
        s_status.disconnect_reason = 0;
        s_status.error = ESP_OK;
        s_status.ip[0] = s_status.gateway[0] = 0;
        xSemaphoreGive(s_mutex);
        err = esp_wifi_set_config(WIFI_IF_STA, &request->config);
        if (err == ESP_OK) err = esp_wifi_connect();
        if (err != ESP_OK) s_want_connection = false;
        break;
    case DISCONNECT:
        s_want_connection = false;
        s_deadline = esp_timer_get_time() + 5000000;
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_disconnect_queued = false;
        s_status.state = SVC_WIFI_DISCONNECTING;
        xSemaphoreGive(s_mutex);
        err = esp_wifi_disconnect();
        if (err == ESP_ERR_WIFI_NOT_CONNECT) {
            err = ESP_OK;
            s_deadline = 0;
            xSemaphoreTake(s_mutex, portMAX_DELAY);
            s_status.connected = false;
            s_status.ip[0] = s_status.gateway[0] = 0;
            s_status.state = SVC_WIFI_READY;
            s_status.pending = false;
            xSemaphoreGive(s_mutex);
        }
        break;
    case SCAN_DONE:
        if (status.state != SVC_WIFI_SCANNING) {
            esp_wifi_clear_ap_list();
            break;
        }
        if (request->scan_status != 0) {
            esp_wifi_clear_ap_list();
            err = ESP_FAIL;
        } else {
            read_scan_results();
        }
        break;
    case ASSOCIATED:
        if (!s_want_connection || strcmp(status.ssid, request->ssid) != 0) break;
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_status.connected = true;
        xSemaphoreGive(s_mutex);
        break;
    case GOT_IP:
        if (!s_want_connection || !status.connected) break;
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        esp_ip4addr_ntoa(&request->ip.ip, s_status.ip, sizeof(s_status.ip));
        esp_ip4addr_ntoa(&request->ip.gw, s_status.gateway, sizeof(s_status.gateway));
        s_status.state = SVC_WIFI_ONLINE;
        s_status.pending = false;
        s_status.error = ESP_OK;
        xSemaphoreGive(s_mutex);
        s_retries = 0;
        s_deadline = 0;
        ESP_LOGI(TAG, "connected; IP=" IPSTR, IP2STR(&request->ip.ip));
        break;
    case LOST_IP:
        if (s_want_connection && status.connected) {
            xSemaphoreTake(s_mutex, portMAX_DELAY);
            s_status.state = SVC_WIFI_CONNECTING;
            s_status.ip[0] = s_status.gateway[0] = 0;
            xSemaphoreGive(s_mutex);
            s_deadline = esp_timer_get_time() + 20000000;
        }
        break;
    case DISCONNECTED:
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_status.connected = false;
        s_status.ip[0] = s_status.gateway[0] = 0;
        s_status.disconnect_reason = request->reason;
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "disconnected, reason=%u", request->reason);
        if (s_want_connection && s_retries < 3 && request->reason != WIFI_REASON_AUTH_FAIL &&
            request->reason != WIFI_REASON_HANDSHAKE_TIMEOUT && request->reason != WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
            s_retries++;
            if (s_deadline == 0) s_deadline = esp_timer_get_time() + 20000000;
            xSemaphoreTake(s_mutex, portMAX_DELAY);
            s_status.state = SVC_WIFI_CONNECTING;
            xSemaphoreGive(s_mutex);
            err = esp_wifi_connect();
        } else if (s_want_connection) {
            s_want_connection = false;
            err = ESP_FAIL;
        } else if (status.state == SVC_WIFI_DISCONNECTING) {
            s_deadline = 0;
            xSemaphoreTake(s_mutex, portMAX_DELAY);
            s_status.state = SVC_WIFI_READY;
            s_status.pending = false;
            s_status.disconnect_reason = 0;
            xSemaphoreGive(s_mutex);
        }
        break;
    }
    if (err != ESP_OK) set_error(err);
}

static void wifi_task(void *arg)
{
    (void)arg;
    wifi_request_t request = { .type = CONNECT, .config = wifi_default_config() };
    process_request(&request);
    memset(&request, 0, sizeof(request));
    int64_t last_rssi = 0;
    for (;;) {
        if (xQueueReceive(s_requests, &request, pdMS_TO_TICKS(250)) == pdTRUE) {
            process_request(&request);
            /* Credentials are never retained in the service snapshot or logs. */
            memset(&request, 0, sizeof(request));
        }
        int64_t now = esp_timer_get_time();
        if (s_deadline && now >= s_deadline) {
            svc_wifi_status_t status;
            svc_wifi_get_status(&status);
            s_want_connection = false;
            if (status.state == SVC_WIFI_SCANNING) esp_wifi_scan_stop();
            else esp_wifi_disconnect();
            set_error(ESP_ERR_TIMEOUT);
        }
        svc_wifi_status_t status;
        svc_wifi_get_status(&status);
        if (status.state == SVC_WIFI_ONLINE && now - last_rssi >= 5000000) {
            wifi_ap_record_t ap;
            esp_err_t err = esp_wifi_sta_get_ap_info(&ap);
            if (err == ESP_OK) {
                xSemaphoreTake(s_mutex, portMAX_DELAY);
                s_status.rssi = ap.rssi;
                xSemaphoreGive(s_mutex);
            } else if (err == ESP_ERR_WIFI_NOT_CONNECT) {
                wifi_request_t disconnected = { .type = DISCONNECTED, .reason = WIFI_REASON_UNSPECIFIED };
                process_request(&disconnected);
            } else {
                esp_wifi_disconnect();
                xSemaphoreTake(s_mutex, portMAX_DELAY);
                s_status.connected = false;
                s_status.ip[0] = s_status.gateway[0] = 0;
                xSemaphoreGive(s_mutex);
                set_error(err);
            }
            last_rssi = now;
        }
    }
}

esp_err_t svc_wifi_init(void)
{
    if (s_task != NULL) return ESP_OK;
    esp_err_t err = ESP_ERR_NO_MEM;
    s_mutex = xSemaphoreCreateMutex();
    s_requests = xQueueCreate(8, sizeof(wifi_request_t));
    if (s_mutex == NULL || s_requests == NULL) goto no_memory;
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL);
    if (err != ESP_OK) goto no_memory;
    err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL);
    if (err != ESP_OK) {
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, event_cb);
        goto no_memory;
    }
    s_status.state = SVC_WIFI_STARTING;
    if (xTaskCreate(wifi_task, "wifi_demo", 8192, NULL, 4, &s_task) == pdPASS) return ESP_OK;
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, event_cb);
    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, event_cb);
    err = ESP_ERR_NO_MEM;
no_memory:
    if (s_mutex) vSemaphoreDelete(s_mutex);
    if (s_requests) vQueueDelete(s_requests);
    s_mutex = NULL;
    s_requests = NULL;
    s_task = NULL;
    return err;
}

static esp_err_t submit(wifi_request_t *request)
{
    if (s_requests == NULL) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool busy = s_disconnect_queued || s_status.pending || s_status.state == SVC_WIFI_STARTING || s_status.state == SVC_WIFI_SCANNING ||
                s_status.state == SVC_WIFI_CONNECTING || s_status.state == SVC_WIFI_DISCONNECTING;
    bool cancel = request->type == DISCONNECT && s_status.state == SVC_WIFI_CONNECTING && !s_disconnect_queued;
    if ((!cancel && busy) || ((request->type == CONNECT || request->type == SCAN) && s_status.connected)) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_status.pending = true;
    if (xQueueSend(s_requests, request, 0) != pdTRUE) {
        s_status.pending = false;
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NO_MEM;
    }
    if (request->type == DISCONNECT) s_disconnect_queued = true;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t svc_wifi_scan(void)
{
    wifi_request_t request = { .type = SCAN };
    return submit(&request);
}

esp_err_t svc_wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) return ESP_ERR_INVALID_ARG;
    size_t ssid_len = strnlen(ssid, 33), password_len = strnlen(password, 65);
    if (!ssid_len || ssid_len > 32 || password_len > 64 || (password_len && password_len < 8)) return ESP_ERR_INVALID_ARG;
    if (password_len == 64) {
        for (unsigned i = 0; i < 64; i++) if (!isxdigit((unsigned char)password[i])) return ESP_ERR_INVALID_ARG;
    }
    wifi_request_t request = { .type = CONNECT };
    memcpy(request.config.sta.ssid, ssid, ssid_len);
    memcpy(request.config.sta.password, password, password_len);
    request.config.sta.threshold.authmode = password_len ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    request.config.sta.pmf_cfg.capable = true;
    request.config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    esp_err_t err = submit(&request);
    memset(&request, 0, sizeof(request));
    return err;
}

esp_err_t svc_wifi_disconnect(void)
{
    wifi_request_t request = { .type = DISCONNECT };
    return submit(&request);
}

void svc_wifi_get_status(svc_wifi_status_t *out)
{
    memset(out, 0, sizeof(*out));
    if (s_mutex == NULL) return;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_status;
    xSemaphoreGive(s_mutex);
}
#else
esp_err_t svc_wifi_init(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t svc_wifi_scan(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t svc_wifi_connect(const char *ssid, const char *password) { (void)ssid; (void)password; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t svc_wifi_disconnect(void) { return ESP_ERR_NOT_SUPPORTED; }
void svc_wifi_get_status(svc_wifi_status_t *out) { memset(out, 0, sizeof(*out)); }
#endif
