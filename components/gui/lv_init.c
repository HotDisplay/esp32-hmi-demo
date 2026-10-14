#include "lv_init.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#include "bsp.h"
#include "lv_demos.h"
#include "app_hmi_demo.h"

static const char *TAG = "LV_PORT";

#define LVGL_TICK_PERIOD_MS         2
#define LVGL_TASK_STACK_SIZE        (12 * 1024)
#define LVGL_TASK_PRIORITY          10
#define LVGL_TASK_MAX_DELAY_MS      500
#if CONFIG_DISPLAY_INTERFACE_RGB
#define LV_COLOR_FORMAT             LV_COLOR_FORMAT_RGB565
#elif CONFIG_DISPLAY_INTERFACE_MIPI
#define LV_COLOR_FORMAT             LV_COLOR_FORMAT_RGB888
#else
#error "Unsupported display interface"
#endif

_Static_assert(LCD_NUM_FB >= 2, "LVGL anti-tearing requires two LCD frame buffers");

void lvgl_init(esp_lcd_panel_handle_t panel, esp_lcd_touch_handle_t touch) {
    ESP_ERROR_CHECK(panel != NULL ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_LOGI(TAG, "Initialize ESP LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = LVGL_TASK_PRIORITY;
    port_cfg.task_stack = LVGL_TASK_STACK_SIZE;
    port_cfg.task_max_sleep_ms = LVGL_TASK_MAX_DELAY_MS;
    port_cfg.timer_period_ms = LVGL_TICK_PERIOD_MS;
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t display_cfg = {
        .panel_handle = panel,
        .buffer_size = LCD_H_RES * LCD_V_RES, // pixels, not bytes
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .color_format = LV_COLOR_FORMAT,
        .flags = {
            .buff_spiram = true,
            .direct_mode = true,
        },
    };

#if CONFIG_DISPLAY_INTERFACE_RGB
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true, // display_init_rgb() enables bounce buffers
            .avoid_tearing = true,
        },
    };
    lv_display_t *display = lvgl_port_add_disp_rgb(&display_cfg, &rgb_cfg);
#elif CONFIG_DISPLAY_INTERFACE_MIPI
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags.avoid_tearing = true,
    };
    lv_display_t *display = lvgl_port_add_disp_dsi(&display_cfg, &dsi_cfg);
#endif
    ESP_ERROR_CHECK(display != NULL ? ESP_OK : ESP_FAIL);

#if CONFIG_DISPLAY_USE_TOUCHPAD
    if (touch != NULL) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = display,
            .handle = touch,
        };
        if (lvgl_port_add_touch(&touch_cfg) == NULL) {
            ESP_LOGW(TAG, "Touch registration failed, LVGL runs display-only");
        }
    } else {
        ESP_LOGW(TAG, "No touch handle provided, LVGL runs display-only");
    }
#endif

    ESP_LOGI(TAG, "Display LVGL UI");
    ESP_ERROR_CHECK(lvgl_port_lock(0) ? ESP_OK : ESP_ERR_TIMEOUT);
    // lv_demo_widgets();
    app_hmi_demo();
    lvgl_port_unlock();
}
