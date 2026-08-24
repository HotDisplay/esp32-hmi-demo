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

#ifdef BSP_USE_IO_EXPANDER
    esp_io_expander_handle_t io_expander = NULL;
    i2c_master_bus_handle_t io_expander_i2c_bus = NULL;

    const i2c_master_bus_config_t io_expander_i2c_config = {
    .i2c_port = I2C_NUM_1,
    .sda_io_num = BSP_I2C1_SDA,
    .scl_io_num = BSP_I2C1_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    };

    i2c_new_master_bus(&io_expander_i2c_config, &io_expander_i2c_bus);
    esp_io_expander_new_i2c_tca95xx_16bit(io_expander_i2c_bus, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_010, &io_expander);
#endif 

    /* Initialize BSP (backlight + touch) */
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(bsp_init(&panel, &touch));

    /* Initialize display interface (RGB / MIPI, dispatched by menuconfig) */
    ESP_ERROR_CHECK(display_init(&panel));
    bsp_set_panel_handle(panel);

    /* Start LVGL */
    lvgl_init();

    // esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_2, IO_EXPANDER_OUTPUT);
    // // Print state
    // esp_io_expander_print_state(io_expander);

    // for (int i = 0; i < 5; i++) {
    //     // Set level to 0
    //     ESP_LOGI(TAG, "Set level to 0");
    //     esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     // Set level to 1
    //     ESP_LOGI(TAG, "Set level to 1");
    //     esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 1);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    /* Init Wi-Fi*/

#ifdef CONFIG_ESP_WIFI_ENABLED
    wifi_init();
#endif
}
