#pragma once

#include "lvgl.h"
#include "esp_lcd_touch.h"

/* 单个触摸设备：中断采集原始坐标，LVGL 按顺序消费缓存。 */
lv_indev_t *gui_touch_add(lv_display_t *display, esp_lcd_touch_handle_t touch);
