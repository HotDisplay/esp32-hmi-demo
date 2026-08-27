#pragma once

#include "sdkconfig.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the display interface (RGB / MIPI / SPI / 8080)
 *
 * Internally dispatches to the driver selected via menuconfig
 * (CONFIG_DISPLAY_IFACE_RGB / CONFIG_DISPLAY_IFACE_MIPI / ...).
 *
 * All pinout and timing parameters are taken from the BSP board &
 * panel headers which are already included by bsp.h.
 */
esp_err_t display_init(esp_lcd_panel_handle_t *out_panel);

/** Per-interface init functions (called by display_init dispatch) */
#if CONFIG_DISPLAY_INTERFACE_RGB
esp_err_t display_init_rgb(esp_lcd_panel_handle_t *out_panel);
#elif CONFIG_DISPLAY_INTERFACE_MIPI
esp_err_t display_init_mipi(esp_lcd_panel_handle_t *out_panel);
#endif

#ifdef __cplusplus
}
#endif
