#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "bsp.h"

static const char *TAG = "C6_RESET";

/* Local SDIO reset hook, called by the build-directory component patch.
 * The Wi-Fi worker runs this only after bsp_init() has prepared the expander. */
esp_err_t board_wifi_reset(void) {
    ESP_LOGI(TAG, "Reset C6 via IO expander P00");
    ESP_RETURN_ON_ERROR(bsp_wifi_set_reset(1), TAG, "C6 EN release");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(bsp_wifi_set_reset(0), TAG, "C6 EN assert");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(bsp_wifi_set_reset(1), TAG, "C6 EN release");
    vTaskDelay(pdMS_TO_TICKS(CONFIG_ESP_HOSTED_SDIO_RESET_DELAY_MS));
    return ESP_OK;
}
