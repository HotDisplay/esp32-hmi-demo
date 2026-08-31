#pragma once

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL and bind it to the given panel / touch handles
 *
 * @param panel  LCD panel handle created by display_init()
 * @param touch  touch handle from bsp_get_touch_handle(), may be NULL
 *               (then LVGL runs display-only, no input device is created)
 */
void lvgl_init(esp_lcd_panel_handle_t panel, esp_lcd_touch_handle_t touch);

#ifdef __cplusplus
}
#endif
