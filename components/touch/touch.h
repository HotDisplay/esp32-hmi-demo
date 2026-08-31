#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the touch controller attached to the selected panel
 *
 * The touch chip is NOT configured separately: every panel entry in
 * menuconfig `select`s the matching BSP_TOUCH_* symbol (see
 * components/display/Kconfig.projbuild), so panel and touch chip can never
 * drift apart. Adding a new panel only requires one extra `select` line.
 *
 * Must be called after bsp_init(), because the reset line may live on the
 * board's IO expander.
 *
 * @param[out] out_touch  Touch handle. Set to NULL when touch is disabled by
 *                        menuconfig, when the panel has no touch controller,
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
