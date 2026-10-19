/**
 * @file bsp.h
 * @brief 提供板级初始化、背光亮度及显示 / 触摸控制引脚接口。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
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

/* ===== Panel selection (timing + touch coordinate limits) ===== */
#include "panel.h"

/* ===== Uniform BSP Interface ===== */

/**
 * @brief Initialize board-level resources (io expander + backlight)
 *
 * Initialization order:
 *   1. IO expander (when the board has one)
 *   2. Backlight GPIO + PWM, kept OFF (see bsp_backlight_enable())
 *
 * Neither the LCD panel nor the touch controller is created here: the
 * application calls display_init() and touch_init() afterwards, which keeps
 * bsp decoupled from both.
 */
esp_err_t bsp_init(void);

/* Touch pin primitives.
 *
 * The BSP only owns WHERE the pins are (plain MCU GPIO or IO expander output);
 * the reset SEQUENCE itself belongs to components/touch, because every touch
 * IC latches its I2C address differently (GT911 samples the INT pin during
 * reset, AXS15260D does not). Keeping the timing out of the BSP is what stops
 * one chip's sequence from silently breaking another's. */

/**
 * @brief Drive the touch reset pin
 *
 * @param level  1 = hold the chip in reset (low), 0 = release (high)
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED when this board has no touch reset pin
 *      - ESP_ERR_NOT_FOUND when the pin needs an expander that is not ready
 */
esp_err_t bsp_touch_set_reset(int level);

/**
 * @brief Drive the touch INT pin while it acts as an address strapping input
 *
 * Must be called before releasing reset on controllers that sample INT to
 * select their I2C address.
 */
esp_err_t bsp_touch_set_int(int level);

/**
 * @brief Release the INT pin back to input so the driver can use it as IRQ
 */
esp_err_t bsp_touch_release_int(void);

/* Turn the backlight ON. Call this after LVGL init completes, so the panel is
 * only lit once a frame has been rendered. bsp_init() only configures the
 * backlight GPIO/PWM but keeps it OFF. */
void bsp_backlight_enable(void);

/**
 * @brief 设置背光 PWM 亮度，需在 bsp_init() 后调用。
 *
 * @param brightness 亮度百分比，范围 0-100；0 关闭背光。
 * @return ESP_OK 表示成功，ESP_ERR_INVALID_ARG 表示超出范围，其他错误由驱动返回。
 */
esp_err_t bsp_backlight_set_brightness(uint32_t brightness);

/** @brief 返回最近一次成功设置的亮度百分比；背光初始关闭时为 0。 */
uint32_t bsp_backlight_get_brightness(void);

/* Hardware-reset the display panel (RST pin). No-op when the panel has no
 * dedicated reset wiring (the panel driver then performs a software reset). On
 * expander-based boards this toggles the expander RST pin; on GPIO boards it
 * toggles the MCU GPIO. */
esp_err_t bsp_display_reset(void);

/* Enable/disable panel power (SLEEP/EN pin). true = out of sleep / powered on,
 * false = sleep. No-op when the board has no such pin. On expander-based boards
 * SLEEP is active-high (low = sleep). */
esp_err_t bsp_display_enable(bool on);

/* Set C6 EN through the existing IO expander: low resets, high releases. */
esp_err_t bsp_wifi_set_reset(int level);

const char *bsp_board_name(void);
const char *bsp_panel_name(void);
uint16_t    bsp_get_screen_width(void);
uint16_t    bsp_get_screen_height(void);

#ifdef __cplusplus
}
#endif
