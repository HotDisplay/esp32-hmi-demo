/**
 * @file display_test_resource.h
 * @brief 原生和 GUI 显示测试共用的画面定义与图片映射接口。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISPLAY_TEST_RED,
    DISPLAY_TEST_GREEN,
    DISPLAY_TEST_BLUE,
    DISPLAY_TEST_WHITE,
    DISPLAY_TEST_GRAYSCALE,
    DISPLAY_TEST_IMAGE,
    DISPLAY_TEST_PATTERN_COUNT,
} display_test_pattern_t;

typedef struct {
    const uint8_t *pixels;
    esp_partition_mmap_handle_t map_handle;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_bytes;
} display_test_image_t;

extern const char *const display_test_pattern_names[DISPLAY_TEST_PATTERN_COUNT];

/** @brief 返回纯色或第 0～15 级灰阶对应的 0xRRGGBB 颜色。 */
uint32_t display_test_pattern_color(unsigned pattern, unsigned gray_level);

/**
 * @brief 映射匹配当前屏参的测试图片；图片缺失或无效时返回 false。
 * @param image 接收映射结果，首次使用前无需初始化；重开前须先关闭旧映射。
 * @note 图片保持只读映射，避免 SPI 分区读取暂停缓存和中断而干扰 RGB 供数。
 */
bool display_test_image_open(display_test_image_t *image);

/** @brief 关闭图片映射并清空句柄，可对未打开的零初始化句柄调用。 */
void display_test_image_close(display_test_image_t *image);

#ifdef __cplusplus
}
#endif
