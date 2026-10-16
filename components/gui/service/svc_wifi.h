#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SVC_WIFI_MAX_APS 16

typedef enum {
    SVC_WIFI_OFF,
    SVC_WIFI_STARTING,
    SVC_WIFI_READY,
    SVC_WIFI_SCANNING,
    SVC_WIFI_CONNECTING,
    SVC_WIFI_ONLINE,
    SVC_WIFI_ERROR,
    SVC_WIFI_DISCONNECTING,
} svc_wifi_state_t;

typedef struct {
    char ssid[33];
    int8_t rssi;
    bool password_required;
    bool supported;
} svc_wifi_ap_t;

typedef struct {
    svc_wifi_state_t state;
    bool pending;
    bool connected;
    esp_err_t error;
    uint16_t disconnect_reason;
    char ssid[33];
    char ip[16];
    char gateway[16];
    int8_t rssi;
    uint32_t scan_generation;
    uint8_t ap_count;
    svc_wifi_ap_t aps[SVC_WIFI_MAX_APS];
} svc_wifi_status_t;

/* Requests return immediately; the persistent worker owns Wi-Fi APIs. */
esp_err_t svc_wifi_init(void);
esp_err_t svc_wifi_scan(void);
esp_err_t svc_wifi_connect(const char *ssid, const char *password);
esp_err_t svc_wifi_disconnect(void);
void svc_wifi_get_status(svc_wifi_status_t *out);
