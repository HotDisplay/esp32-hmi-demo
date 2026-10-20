/**
 * @file display_test.c
 * @brief 配置开启时上电独占 LCD 运行原生测试，不进入 GUI。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-20
 */
#include "sdkconfig.h"

#if CONFIG_DISPLAY_TEST_DEMO
#include <stdint.h>
#include <string.h>
#include "display_test.h"
#include "display_test_resource.h"
#include "bsp.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#if CONFIG_DISPLAY_INTERFACE_RGB
#define TEST_PIXEL_BYTES 2
#else
#include "esp_lcd_mipi_dsi.h"
#define TEST_PIXEL_BYTES 3
#endif

static const char *TAG = "LCD_TEST";

_Static_assert(LCD_NUM_FB == 2, "LCD test requires two frame buffers");

#if CONFIG_DISPLAY_INTERFACE_RGB
static bool IRAM_ATTR frame_done(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t *event, void *ctx)
#else
static bool IRAM_ATTR frame_done(esp_lcd_panel_handle_t panel,
                                esp_lcd_dpi_panel_event_data_t *event, void *ctx)
#endif
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)ctx, &woken);
    return woken == pdTRUE;
}

static esp_err_t present(esp_lcd_panel_handle_t panel, void *fb, SemaphoreHandle_t done)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_H_RES, LCD_V_RES, fb),
                        TAG, "submit frame");
    while (xSemaphoreTake(done, 0) == pdTRUE) {}
    /* Allow both the pending buffer switch and the old DMA/bounce buffer to
     * finish before the previous frame buffer is reused. */
    for (int i = 0; i < 2; i++) {
        ESP_RETURN_ON_FALSE(xSemaphoreTake(done, pdMS_TO_TICKS(1000)) == pdTRUE,
                            ESP_ERR_TIMEOUT, TAG, "frame refresh timeout");
    }
    return ESP_OK;
}

static void fill_pattern(void *fb, unsigned pattern)
{
    unsigned count = pattern == DISPLAY_TEST_GRAYSCALE ? 16 :
                     pattern == DISPLAY_TEST_COLOR_BARS ? DISPLAY_TEST_COLOR_BAR_COUNT :
                     pattern == DISPLAY_TEST_CHECKERBOARD ? 2 : 1;
    uint32_t colors[16];
    for (unsigned i = 0; i < count; i++) colors[i] = display_test_pattern_color(pattern, i);
    for (unsigned y = 0; y < LCD_V_RES; y++) {
#if CONFIG_DISPLAY_INTERFACE_RGB
        uint16_t *row = (uint16_t *)fb + y * LCD_H_RES;
#else
        /* RGB888 is stored as B, G, R bytes in the current DPI framebuffer. */
        uint8_t *row = (uint8_t *)fb + y * LCD_H_RES * TEST_PIXEL_BYTES;
#endif
        for (unsigned x = 0; x < LCD_H_RES; x++) {
            /* Landscape: vertical bands. Portrait: horizontal bands. */
            unsigned level = pattern == DISPLAY_TEST_CHECKERBOARD ?
                (x / DISPLAY_TEST_CHECKER_SIZE + y / DISPLAY_TEST_CHECKER_SIZE) & 1 :
                LCD_H_RES >= LCD_V_RES ? x * count / LCD_H_RES : y * count / LCD_V_RES;
            uint32_t color = colors[level];
            uint8_t r = color >> 16;
            uint8_t g = color >> 8;
            uint8_t b = color;
#if CONFIG_DISPLAY_INTERFACE_RGB
            row[x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
#else
            *row++ = b;
            *row++ = g;
            *row++ = r;
#endif
        }
    }
}

esp_err_t display_test_run(esp_lcd_panel_handle_t panel)
{
    void *fb[2] = {NULL, NULL};
    SemaphoreHandle_t done = xSemaphoreCreateCounting(2, 0);
    ESP_RETURN_ON_FALSE(done, ESP_ERR_NO_MEM, TAG, "frame semaphore");

    esp_err_t err;
#if CONFIG_DISPLAY_INTERFACE_RGB
    err = esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &fb[0], &fb[1]);
    const esp_lcd_rgb_panel_event_callbacks_t callbacks = {.on_frame_buf_complete = frame_done};
    if (err == ESP_OK) {
        err = esp_lcd_rgb_panel_register_event_callbacks(panel, &callbacks, done);
    }
#else
    err = esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &fb[0], &fb[1]);
    const esp_lcd_dpi_panel_event_callbacks_t callbacks = {.on_refresh_done = frame_done};
    if (err == ESP_OK) {
        err = esp_lcd_dpi_panel_register_event_callbacks(panel, &callbacks, done);
    }
#endif
    if (err != ESP_OK) {
        vSemaphoreDelete(done);
        return err;
    }

    display_test_image_t image;
    display_test_image_open(&image);
    unsigned next_fb = 1; /* The driver initially scans fb[0]. */
    bool backlight_on = false;
    while (true) {
        for (unsigned stage = 0; stage < (image.pixels ? DISPLAY_TEST_PATTERN_COUNT : DISPLAY_TEST_IMAGE); stage++) {
            ESP_LOGI(TAG, "%s (%dx%d)", display_test_pattern_names[stage], LCD_H_RES, LCD_V_RES);
            if (stage == DISPLAY_TEST_IMAGE) {
                memcpy(fb[next_fb], image.pixels, LCD_H_RES * LCD_V_RES * TEST_PIXEL_BYTES);
            } else {
                fill_pattern(fb[next_fb], stage);
            }
            err = present(panel, fb[next_fb], done);
            if (err != ESP_OK) {
                display_test_image_close(&image);
                /* The caller aborts on failure; keep the callback context alive. */
                return err;
            }
            next_fb ^= 1;
            if (!backlight_on) {
                bsp_backlight_enable();
                backlight_on = true;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

#endif
