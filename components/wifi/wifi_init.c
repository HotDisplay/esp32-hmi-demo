#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "wifi_init.h"

#define ESP_WIFI_SSID           "MI14"
#define ESP_WIFI_PASS           "456123123"

#if CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_WIFI_REMOTE_ENABLED
static const char *TAG = "WIFI_INIT";
#endif


wifi_config_t wifi_default_config(void) {
    return (wifi_config_t) {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = sizeof(ESP_WIFI_PASS) > 1 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN,
            .pmf_cfg.capable = true,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
}

/* Standard APIs use wifi_remote on P4 and the native Wi-Fi driver on S3. */
esp_err_t wifi_init(void) {
#if CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_WIFI_REMOTE_ENABLED
    static esp_netif_t *sta_netif;
    static bool ready;
    if (ready) return ESP_OK;
    if (sta_netif == NULL) {
        sta_netif = esp_netif_create_default_wifi_sta();
        if (sta_netif == NULL) return ESP_ERR_NO_MEM;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        esp_wifi_deinit();
        ESP_LOGE(TAG, "Wi-Fi driver init: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err == ESP_OK) err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err != ESP_OK) esp_wifi_deinit();
    ready = err == ESP_OK;
    return err;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
