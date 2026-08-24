/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic MIPI-DSI LCD panel driver (HotDisplay in-house component).
 *
 * This is a controller-agnostic MIPI DPI panel driver extracted from the
 * Espressif ST7703 vendor driver. The only part that differs between panel
 * models is the *initialization command sequence*, which is supplied per-panel
 * through `mipi_panel_vendor_config_t.init_cmds` (defined in
 * components/panel/panels/dsi/panel_h0xxxxx.h).
 *
 * Works for any MIPI-DSI panel whose initialization follows the standard DCS
 * flow (MADCTL / COLMOD + vendor command sequence), e.g. ST7703, ILI9881C,
 * NT36525, JD9365, RM671xx, ...
 */

#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic MIPI-DSI panel initialization command.
 *
 * The layout is identical to Espressif's `st7703_lcd_init_cmd_t`, so existing
 * ST7703 sequences can be copied verbatim into a panel header.
 */
typedef struct {
    int cmd;                /*<! The specific LCD command */
    const void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} mipi_panel_init_cmd_t;

/**
 * @brief Vendor configuration for the generic DSI panel driver.
 *
 * Only `init_cmds` (the per-panel init sequence) varies between models.
 * The array MUST be declared `static const` and live outside any function,
 * e.g. in components/panel/panels/dsi/panel_h0xxxxx.h.
 */
typedef struct {
    const mipi_panel_init_cmd_t *init_cmds;     /*!< Pointer to the panel-specific init sequence */
    uint16_t init_cmds_size;                    /*!< Number of commands in the array above */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;               /*!< MIPI-DSI bus handle */
        const esp_lcd_dpi_panel_config_t *dpi_config;   /*!< MIPI-DPI panel configuration */
    } mipi_config;
} mipi_panel_vendor_config_t;

/**
 * @brief Create a generic MIPI-DSI LCD panel.
 *
 * @param[in]  io                LCD panel IO handle (DBI)
 * @param[in]  panel_dev_config  General panel device configuration
 * @param[out] ret_panel         Returned LCD panel handle
 * @return
 *      - ESP_ERR_INVALID_ARG  if a parameter is invalid
 *      - ESP_OK               on success
 *      - Otherwise            on failure
 */
esp_err_t esp_lcd_new_panel_mipi(const esp_lcd_panel_io_handle_t io,
                                const esp_lcd_panel_dev_config_t *panel_dev_config,
                                esp_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif

#endif /* SOC_MIPI_DSI_SUPPORTED */
