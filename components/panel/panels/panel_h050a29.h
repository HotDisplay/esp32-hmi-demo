#pragma once

/* ===== Panel: H050A29 (5.0 inch / 800x480 / RGB565 / GT911) ===== */

#define BSP_PANEL_NAME          "H050A29 (5.0 inch / 800x480)"

#define LCD_PIXEL_CLOCK_HZ      (18 * 1000 * 1000)
#define LCD_H_RES               800
#define LCD_V_RES               480
#define LCD_HSYNC               4
#define LCD_HBP                 8
#define LCD_HFP                 8
#define LCD_VSYNC               4
#define LCD_VBP                 8
#define LCD_VFP                 8
#define LCD_NUM_FB              2
#define LCD_PCLK_ACTIVE_NEG     true

#define BSP_TOUCH_NONE           0
#define BSP_TOUCH_GT911          1
#define BSP_TOUCH_TYPE           BSP_TOUCH_GT911
#define BSP_TOUCH_X_MAX          800
#define BSP_TOUCH_Y_MAX          480
#define BSP_TOUCH_SWAP_XY        0
