#pragma once

/* ===== Panel: H021A04 (2.1 inch / 540x540 / RGB565 / AXS15260D) ===== */

#define BSP_PANEL_NAME          "H021A04 (2.1 inch / 540x540)"

#define LCD_PIXEL_CLOCK_HZ      (28 * 1000 * 1000)
#define LCD_H_RES               540
#define LCD_V_RES               540
#define LCD_HSYNC               10
#define LCD_HBP                 100
#define LCD_HFP                 100
#define LCD_VSYNC               10
#define LCD_VBP                 40
#define LCD_VFP                 60

#define LCD_NUM_FB              2
#define LCD_PCLK_ACTIVE_NEG     true

/* Touch controller is chosen by Kconfig: the panel entry `select`s the matching
 * BSP_TOUCH_* symbol, so the chip is never defined here. */
#define BSP_TOUCH_X_MAX          540
#define BSP_TOUCH_Y_MAX          540
#define BSP_TOUCH_SWAP_XY        0
