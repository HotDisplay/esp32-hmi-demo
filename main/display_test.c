#include "sdkconfig.h"

#if CONFIG_DISPLAY_TEST_DEMO

#include <stdint.h>
#include <string.h>
#include "display_test.h"
#include "bsp.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#if CONFIG_DISPLAY_INTERFACE_RGB
_Static_assert(RGB_LCD_DATA_BITS == 16, "LCD test requires RGB565");
#define TEST_PIXEL_BYTES 2
#else
#include "esp_lcd_mipi_dsi.h"
_Static_assert(PANEL_MIPI_COLOR_BITS == 24, "LCD test requires MIPI RGB888");
#define TEST_PIXEL_BYTES 3
#endif

_Static_assert(LCD_NUM_FB == 2, "LCD test requires two frame buffers");

static const char *TAG = "LCD_TEST";

typedef struct {
    char magic[8];
    uint32_t width;
    uint32_t height;
    uint32_t pixel_bytes;
    uint32_t payload_bytes;
} image_header_t;

_Static_assert(sizeof(image_header_t) == 24, "Invalid image header layout");

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
    static const uint8_t colors[][3] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255},
    };
    for (unsigned y = 0; y < LCD_V_RES; y++) {
#if CONFIG_DISPLAY_INTERFACE_RGB
        uint16_t *row = (uint16_t *)fb + y * LCD_H_RES;
#else
        /* RGB888 is stored as B, G, R bytes in the current DPI framebuffer. */
        uint8_t *row = (uint8_t *)fb + y * LCD_H_RES * TEST_PIXEL_BYTES;
#endif
        for (unsigned x = 0; x < LCD_H_RES; x++) {
            /* Landscape: vertical bands. Portrait: horizontal bands. */
            uint8_t gray = (LCD_H_RES >= LCD_V_RES ? x * 16 / LCD_H_RES : y * 16 / LCD_V_RES) * 255 / 15;
            uint8_t r = pattern == 4 ? gray : colors[pattern][0];
            uint8_t g = pattern == 4 ? gray : colors[pattern][1];
            uint8_t b = pattern == 4 ? gray : colors[pattern][2];
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

static const uint8_t *find_image(esp_partition_mmap_handle_t *map_handle)
{
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "lcd_image");
    const size_t image_size = LCD_H_RES * LCD_V_RES * TEST_PIXEL_BYTES;
    *map_handle = 0;
    if (!partition || partition->size < sizeof(image_header_t) + image_size) {
        ESP_LOGI(TAG, "No test image; skip image stage");
        return NULL;
    }

    /* SPI1 reads suspend cache/interrupts and starve RGB bounce buffers.
     * Keep the image mapped so copying uses the cache without that suspension. */
    const void *mapped = NULL;
    esp_err_t err = esp_partition_mmap(partition, 0, sizeof(image_header_t) + image_size,
                                      ESP_PARTITION_MMAP_DATA, &mapped, map_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Cannot map test image: %s; skip image stage", esp_err_to_name(err));
        return NULL;
    }
    const image_header_t *header = mapped;
    if (memcmp(header->magic, "LCDTEST1", 8) != 0 || header->payload_bytes == 0) {
        esp_partition_munmap(*map_handle);
        *map_handle = 0;
        ESP_LOGI(TAG, "No test image; skip image stage");
        return NULL;
    }
    if (header->width != LCD_H_RES || header->height != LCD_V_RES ||
        header->pixel_bytes != TEST_PIXEL_BYTES || header->payload_bytes != image_size) {
        esp_partition_munmap(*map_handle);
        *map_handle = 0;
        ESP_LOGW(TAG, "Image does not match panel; rebuild and run idf.py flash");
        return NULL;
    }
    return (const uint8_t *)mapped + sizeof(image_header_t);
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

    esp_partition_mmap_handle_t image_map = 0;
    const uint8_t *image = find_image(&image_map);
    const char *names[] = {"red", "green", "blue", "white", "grayscale", "image"};
    unsigned next_fb = 1; /* The driver initially scans fb[0]. */
    bool backlight_on = false;
    while (true) {
        for (unsigned stage = 0; stage < (image ? 6 : 5); stage++) {
            ESP_LOGI(TAG, "%s (%dx%d)", names[stage], LCD_H_RES, LCD_V_RES);
            if (stage == 5) {
                memcpy(fb[next_fb], image, LCD_H_RES * LCD_V_RES * TEST_PIXEL_BYTES);
            } else {
                fill_pattern(fb[next_fb], stage);
            }
            err = present(panel, fb[next_fb], done);
            if (err != ESP_OK) {
                if (image) {
                    esp_partition_munmap(image_map);
                }
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
