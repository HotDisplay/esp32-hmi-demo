#pragma once

/* ===== Panel: H034A04 (3.4 inch round / 800x800 / RGB565) ===== */

#define BSP_PANEL_NAME          "H034A04 (3.4 inch round / 800x800)"

#define LCD_PIXEL_CLOCK_HZ      (26 * 1000 * 1000)
#define LCD_H_RES               800
#define LCD_V_RES               800
#define LCD_HSYNC               4
#define LCD_HBP                 80
#define LCD_HFP                 80
#define LCD_VSYNC               4
#define LCD_VBP                 40
#define LCD_VFP                 40

#define LCD_NUM_FB              2
#define LCD_PCLK_ACTIVE_NEG     true

/* Touch controller is chosen by Kconfig. */
#define BSP_TOUCH_X_MAX          800
#define BSP_TOUCH_Y_MAX          800
#define BSP_TOUCH_SWAP_XY        0
