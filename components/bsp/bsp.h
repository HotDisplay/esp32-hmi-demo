#pragma once

#include <stdbool.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"
#include "esp_io_expander.h"

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
 * @brief Initialize BSP (backlight + touch + io expander)
 * @param[out] out_panel  LCD panel handle
 * @param[out] out_touch  touch handle (NULL if no touch)
 */
esp_err_t bsp_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch);

/* Set the panel handle created by display_init() (display component), so it is
 * visible to other modules through the extern `panel_handle` global. */
void bsp_set_panel_handle(esp_lcd_panel_handle_t panel);

/* Turn the backlight ON. Call this after LVGL init completes, so the panel is
 * only lit once a frame has been rendered. bsp_init() only configures the
 * backlight GPIO/PWM but keeps it OFF. */
void bsp_backlight_enable(void);

/* Hardware-reset the display panel (RST pin). No-op when the panel has no
 * dedicated reset wiring (the panel driver then performs a software reset). On
 * expander-based boards this toggles the expander RST pin; on GPIO boards it
 * toggles the MCU GPIO. */
esp_err_t bsp_display_reset(void);

/* Enable/disable panel power (SLEEP/EN pin). true = out of sleep / powered on,
 * false = sleep. No-op when the board has no such pin. On expander-based boards
 * SLEEP is active-high (low = sleep). */
esp_err_t bsp_display_enable(bool on);

const char *bsp_board_name(void);
const char *bsp_panel_name(void);
uint16_t    bsp_get_screen_width(void);
uint16_t    bsp_get_screen_height(void);

#ifdef __cplusplus
}
#endif
