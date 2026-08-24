#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/i2c_master.h"
#include "esp_io_expander_tca95xx_16bit.h"

#include "bsp_lcd.h"
#include "display.h"
#include "lv_init.h"
#include "wifi_init.h"

static const char *TAG = "Main";

void app_main(void) {
    // 初始化nvs flash
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // If NVS has no free pages or a new version is found, erase and reinitialize NVS
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_LOGI(TAG, "Board: %s", bsp_board_name());
    ESP_LOGI(TAG, "Panel: %s", bsp_panel_name());
    ESP_LOGI(TAG, "Screen: %dx%d", bsp_get_screen_width(), bsp_get_screen_height());



    /* Initialize BSP (backlight + touch) */
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(bsp_init(&panel, &touch));

    /* Initialize display interface (RGB / MIPI, dispatched by menuconfig) */
    ESP_ERROR_CHECK(display_init(&panel));
    bsp_set_panel_handle(panel);

    /* Start LVGL */
    lvgl_init();

    /* Init Wi-Fi*/

#ifdef CONFIG_ESP_WIFI_ENABLED
    wifi_init();
#endif
}
