#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Board selection (pinout) ===== */
#if CONFIG_BSP_BOARD_H050A29_ESP_STDRGB
#include "board_h050a29_esp_stdrgb.h"
#elif CONFIG_BSP_BOARD_ESP32P4_DEMO_A02
#include "board_esp32p4_demo_a02.h"
#else
#error "No BSP board selected! Run: idf.py menuconfig -> BSP Board"
#endif

/* ===== Panel selection (timing + touch) ===== */
#include "panel.h"

/* ===== Uniform BSP Interface ===== */

/**
 * @brief Initialize BSP (backlight + panel + touch)
 * @param[out] out_panel  LCD panel handle
 * @param[out] out_touch  touch handle (NULL if no touch)
 */
esp_err_t bsp_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch);

/* Set the panel handle created by display_init() (display component), so it is
 * visible to other modules through the extern `panel_handle` global. */
void bsp_set_panel_handle(esp_lcd_panel_handle_t panel);

const char *bsp_board_name(void);
const char *bsp_panel_name(void);
uint16_t    bsp_get_screen_width(void);
uint16_t    bsp_get_screen_height(void);

#ifdef __cplusplus
}
#endif
