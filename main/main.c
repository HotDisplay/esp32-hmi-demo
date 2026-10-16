#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "bsp.h"
#include "display.h"
#include "display_test.h"
#include "touch.h"
#include "lv_init.h"
#include "svc_wifi.h"
#include "ethernet_init.h"
#include "sdcard.h"

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

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    /* Board resources: IO expander + backlight PWM (backlight stays off).
     * Must run first — the touch reset line may live on the IO expander. */
    ESP_ERROR_CHECK(bsp_init());

    /* Initialize display interface (RGB / MIPI, dispatched by menuconfig) */
    ESP_ERROR_CHECK(display_init(&panel));

#if CONFIG_DISPLAY_TEST_DEMO
    ESP_ERROR_CHECK(display_test_run(panel));
    return;
#endif

    /* Touch controller is selected independently in menuconfig. */
    ESP_ERROR_CHECK(touch_init(&touch));

    /* Wi-Fi and Ethernet share the TCP/IP stack and default event loop. */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize Slot 0 before the Wi-Fi worker opens C6 SDIO on Slot 1. */
    sdcard_init();

    /* Start LVGL. Panel and touch handles are passed in explicitly, so nothing
     * depends on module-global variables. */
    lvgl_init(panel, touch);

    /* After LVGL fully initialized (frame rendered): now turn on the backlight */
    bsp_backlight_enable();

#ifdef CONFIG_SOC_EMAC_SUPPORTED
    ethernet_init();
#endif
}
