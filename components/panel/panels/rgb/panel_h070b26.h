#pragma once
/* H070B26 (7.0 inch / 1024x600 / RGB565 / GT911) — verify timing vs datasheet */

#define BSP_PANEL_NAME          "H070B26 (7.0 inch / 1024x600)"

#define LCD_PIXEL_CLOCK_HZ      (30 * 1000 * 1000)
#define LCD_H_RES               1024
#define LCD_V_RES               600
#define LCD_HSYNC               4
#define LCD_HBP                 160
#define LCD_HFP                 160
#define LCD_VSYNC               4
#define LCD_VBP                 23
#define LCD_VFP                 12
#define LCD_NUM_FB              2
#define LCD_PCLK_ACTIVE_NEG     true

/* Touch controller is chosen by Kconfig: the panel entry `select`s the matching
 * BSP_TOUCH_* symbol, so the chip is never defined here. */
#define BSP_TOUCH_X_MAX          1024
#define BSP_TOUCH_Y_MAX          600
#define BSP_TOUCH_SWAP_XY        0
