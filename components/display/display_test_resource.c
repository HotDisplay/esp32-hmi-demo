/**
 * @file display_test_resource.c
 * @brief GUI 与原生显示测试共用的颜色定义和只读图片映射。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#include "sdkconfig.h"
#include <stdint.h>
#include <string.h>
#include "display_test_resource.h"
#include "bsp.h"
#include "esp_partition.h"
#include "esp_log.h"

#if CONFIG_DISPLAY_INTERFACE_RGB
_Static_assert(RGB_LCD_DATA_BITS == 16, "LCD test requires RGB565");
#define TEST_PIXEL_BYTES 2
#else
_Static_assert(PANEL_MIPI_COLOR_BITS == 24, "LCD test requires MIPI RGB888");
#define TEST_PIXEL_BYTES 3
#endif

static const char *TAG = "LCD_TEST";

typedef struct {
    char magic[8];
    uint32_t width;
    uint32_t height;
    uint32_t pixel_bytes;
    uint32_t payload_bytes;
} image_header_t;

_Static_assert(sizeof(image_header_t) == 24, "Invalid image header layout");

const char *const display_test_pattern_names[DISPLAY_TEST_PATTERN_COUNT] = {
    "red", "green", "blue", "white", "grayscale", "image",
};

uint32_t display_test_pattern_color(unsigned pattern, unsigned gray_level)
{
    static const uint32_t colors[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffffff};
    if (pattern < DISPLAY_TEST_GRAYSCALE) {
        return colors[pattern];
    }
    if (gray_level > 15) gray_level = 15;
    return (gray_level * 255 / 15) * 0x010101U;
}

bool display_test_image_open(display_test_image_t *image)
{
    if (image == NULL) return false;
    memset(image, 0, sizeof(*image));
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "lcd_image");
    const size_t image_size = LCD_H_RES * LCD_V_RES * TEST_PIXEL_BYTES;
    esp_partition_mmap_handle_t map_handle = 0;
    if (!partition || partition->size < sizeof(image_header_t) + image_size) {
        ESP_LOGI(TAG, "No test image; skip image stage");
        return false;
    }

    /* SPI1 reads suspend cache/interrupts and starve RGB bounce buffers.
     * Keep the image mapped so copying uses the cache without that suspension. */
    const void *mapped = NULL;
    esp_err_t err = esp_partition_mmap(partition, 0, sizeof(image_header_t) + image_size,
                                      ESP_PARTITION_MMAP_DATA, &mapped, &map_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Cannot map test image: %s; skip image stage", esp_err_to_name(err));
        return false;
    }
    const image_header_t *header = mapped;
    if (memcmp(header->magic, "LCDTEST1", 8) != 0 || header->payload_bytes == 0) {
        esp_partition_munmap(map_handle);
        ESP_LOGI(TAG, "No test image; skip image stage");
        return false;
    }
    if (header->width != LCD_H_RES || header->height != LCD_V_RES ||
        header->pixel_bytes != TEST_PIXEL_BYTES || header->payload_bytes != image_size) {
        esp_partition_munmap(map_handle);
        ESP_LOGW(TAG, "Image does not match panel; rebuild and run idf.py flash");
        return false;
    }
    image->pixels = (const uint8_t *)mapped + sizeof(image_header_t);
    image->map_handle = map_handle;
    image->width = header->width;
    image->height = header->height;
    image->pixel_bytes = header->pixel_bytes;
    return true;
}

void display_test_image_close(display_test_image_t *image)
{
    if (image == NULL) return;
    if (image->pixels != NULL) esp_partition_munmap(image->map_handle);
    memset(image, 0, sizeof(*image));
}
