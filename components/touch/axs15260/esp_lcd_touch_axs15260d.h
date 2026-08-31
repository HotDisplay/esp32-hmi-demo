/*
 * SPDX-FileCopyrightText: 2022-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 HotDisplay (rewritten to use esp_lcd_panel_io)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: AXS15260D (panel_io flavour)
 *
 * This is a drop-in replacement for the upstream waveshare component. The
 * public API is identical, but all transfers go through esp_lcd_panel_io
 * instead of a raw i2c_master_dev_handle, so it can be swapped with any other
 * esp_lcd_touch driver (e.g. GT911) without touching the caller.
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new AXS15260D touch driver
 *
 * @param io        LCD/Touch panel IO handle (from esp_lcd_new_panel_io_i2c)
 * @param config    Touch configuration
 * @param out_touch Touch instance handle
 * @return
 *      - ESP_OK         on success
 *      - ESP_ERR_NO_MEM if there is no memory for the main structure
 */
esp_err_t esp_lcd_touch_new_i2c_axs15260d(const esp_lcd_panel_io_handle_t io,
                                          const esp_lcd_touch_config_t *config,
                                          esp_lcd_touch_handle_t *out_touch);

/**
 * @brief I2C address of the AXS15260D controller
 */
#define ESP_LCD_TOUCH_IO_I2C_AXS15260D_ADDRESS          (0x3B)

/**
 * @brief AXS15260D Configuration Type
 */
typedef struct {
    uint8_t dev_addr;  /*!< I2C device address */
} esp_lcd_touch_io_axs15260d_config_t;

/**
 * @brief Touch IO configuration structure
 *
 * lcd_cmd_bits is 8 so that a register read sends exactly one register address
 * byte. Reading the raw point stream needs no command byte at all — the driver
 * passes lcd_cmd = -1 for that, which makes the panel_io layer issue a bare
 * i2c_master_receive (see panel_io_i2c_rx_buffer in ESP-IDF).
 */
#define ESP_LCD_TOUCH_IO_I2C_AXS15260D_CONFIG()             \
    {                                                       \
        .scl_speed_hz = 100000,                             \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_AXS15260D_ADDRESS, \
        .control_phase_bytes = 1,                           \
        .dc_bit_offset = 0,                                 \
        .lcd_cmd_bits = 8,                                  \
        .flags =                                            \
        {                                                   \
            .disable_control_phase = 1,                     \
        }                                                   \
    }

#ifdef __cplusplus
}
#endif
