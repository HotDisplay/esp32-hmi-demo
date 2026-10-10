#pragma once

/* ===== Panel: H048A02 (4.8 inch / 480x1120 / RGB565 / GT911) ===== */

#define BSP_PANEL_NAME          "H048A02 (4.8 inch / 480x1120)"

#define LCD_PIXEL_CLOCK_HZ      (32 * 1000 * 1000)
#define LCD_H_RES               480
#define LCD_V_RES               960
#define LCD_HSYNC               8
#define LCD_HBP                 40
#define LCD_HFP                 40
#define LCD_VSYNC               8
#define LCD_VBP                 20
#define LCD_VFP                 20

#define LCD_NUM_FB              2
#define LCD_PCLK_ACTIVE_NEG     true

/* Touch controller is chosen by Kconfig: the panel entry `select`s the matching
 * BSP_TOUCH_* symbol, so the chip is never defined here. */
#define BSP_TOUCH_X_MAX         480
#define BSP_TOUCH_Y_MAX         1120
#define BSP_TOUCH_SWAP_XY       0

