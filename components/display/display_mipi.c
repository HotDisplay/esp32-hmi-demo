#include "display.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "DISPLAY_MIPI";

esp_err_t display_init_mipi(esp_lcd_panel_handle_t *out_panel) {
    ESP_LOGW(TAG, "MIPI DSI not yet implemented (ESP32-P4 only)");
    *out_panel = NULL;
    return ESP_ERR_NOT_SUPPORTED;
}
