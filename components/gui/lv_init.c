#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "sdkconfig.h"

#include "esp_timer.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#ifdef SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_mipi_dsi.h"
#endif
#include "esp_lcd_touch.h"
#include "esp_log.h"

#include "lvgl.h"

#include "bsp.h"
#include "lv_demos.h"
#include "app_hmi_demo.h"

static const char *TAG = "LV_PORT";

#define LVGL_DRAW_BUF_LINES         50 // number of display lines in each draw buffer
#define LVGL_TICK_PERIOD_MS         2
#define LVGL_TASK_STACK_SIZE        (12 * 1024)
#define LVGL_TASK_PRIORITY          10
#define LVGL_TASK_MAX_DELAY_MS      500
#define LVGL_TASK_MIN_DELAY_MS      1000 / CONFIG_FREERTOS_HZ
#define LV_COLOR_FORMAT             LV_COLOR_FORMAT_RGB888
#define LV_PIXEL_SIZE               3


// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

#if CONFIG_DISPLAY_INTERFACE_RGB
static bool notify_lvgl_flush_ready_rgb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}
#elif CONFIG_DISPLAY_INTERFACE_MIPI
static bool notify_lvgl_flush_ready_dpi(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}
#endif

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static void increase_lvgl_tick(void *arg) {
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg) {
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of task watch dog timeout
        time_till_next_ms = MAX(time_till_next_ms, LVGL_TASK_MIN_DELAY_MS);
        // in case of lvgl display not ready yet
        time_till_next_ms = MIN(time_till_next_ms, LVGL_TASK_MAX_DELAY_MS);
        usleep(1000 * time_till_next_ms);
    }
}

static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t touchpad_x[1] = { 0 };
    uint16_t touchpad_y[1] = { 0 };
    uint8_t touchpad_cnt = 0;

    esp_lcd_touch_handle_t touch_pad = lv_indev_get_user_data(indev);
    esp_lcd_touch_read_data(touch_pad);
    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_pad, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_init(esp_lcd_panel_handle_t panel, esp_lcd_touch_handle_t touch) {
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // create a lvgl display
    lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
    // associate the rgb panel handle to the display
    lv_display_set_user_data(display, panel);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT);
    // create draw buffers
    // void *buf1 = heap_caps_malloc(LCD_H_RES * LCD_V_RES, MALLOC_CAP_32BIT);
    void *buf1 = NULL;
    void *buf2 = NULL;

    size_t draw_buffer_sz = LCD_H_RES * LCD_V_RES * sizeof(lv_color_t);

    // buf1 = esp_lcd_rgb_alloc_draw_buffer(panel_handle, draw_buffer_sz, MALLOC_CAP_SPIRAM);
    // buf2 = esp_lcd_rgb_alloc_draw_buffer(panel_handle, draw_buffer_sz, MALLOC_CAP_SPIRAM);

    // buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    // buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    // assert(buf1);
    // assert(buf2);


    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
#if CONFIG_DISPLAY_INTERFACE_RGB
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &buf1, &buf2));
#elif CONFIG_DISPLAY_INTERFACE_MIPI
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &buf1, &buf2));
#endif
    // set LVGL draw buffers and direct mode
    lv_display_set_buffers(display, buf1, buf2, LCD_H_RES * LCD_V_RES * LV_PIXEL_SIZE, LV_DISPLAY_RENDER_MODE_DIRECT);

    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, lvgl_flush_cb);

#if CONFIG_DISPLAY_USE_TOUCHPAD
    if (touch != NULL) {
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(indev, display);
        lv_indev_set_user_data(indev, touch);
        lv_indev_set_read_cb(indev, lvgl_touch_cb);
    } else {
        ESP_LOGW(TAG, "No touch handle provided, LVGL runs display-only");
    }
#endif

    ESP_LOGI(TAG, "Register event callbacks");

#if CONFIG_DISPLAY_INTERFACE_RGB
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready_rgb,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel, &cbs, display));
#elif CONFIG_DISPLAY_INTERFACE_MIPI
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready_dpi,
        // .on_refresh_done = example_monitor_refresh_rate,
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(panel, &cbs, display));
#endif

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Display LVGL UI");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    _lock_acquire(&lvgl_api_lock);
    // lv_demo_widgets();
    app_hmi_demo();
    _lock_release(&lvgl_api_lock);
}