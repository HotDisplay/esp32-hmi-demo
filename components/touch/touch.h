#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the independently selected touch controller
 *
 * Select the touch driver in menuconfig -> Touch Selection independently of
 * the display panel. Coordinate limits follow the selected panel resolution.
 *
 * Must be called after bsp_init(), because the reset line may live on the
 * board's IO expander.
 *
 * @param[out] out_touch  Touch handle. Set to NULL when touch is disabled by
 *                        menuconfig, when no touch driver is selected,
 *                        or when initialization failed (boot is not aborted
 *                        in that case — the device stays display-only).
 * @return
 *      - ESP_OK on success, including the "no touch" cases described above
 *      - Other error codes for bus/IO failures
 */
esp_err_t touch_init(esp_lcd_touch_handle_t *out_touch);

#ifdef __cplusplus
}
#endif
