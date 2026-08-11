#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "bsp_lcd.h"
#include "lv_init.h"

static const char *TAG = "Main";

void app_main(void) {
    ESP_LOGI(TAG, "Board: %s", bsp_board_name());
    ESP_LOGI(TAG, "Panel: %s", bsp_panel_name());
    ESP_LOGI(TAG, "Screen: %dx%d", bsp_get_screen_width(), bsp_get_screen_height());

    /* Initialize BSP (backlight + LCD panel + touch) */
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(bsp_init(&panel, &touch));

    /* Start LVGL */
    lvgl_init();
}
