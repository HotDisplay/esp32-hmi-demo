#pragma once

/* ===== Board: HTM-H050A29-ESP-StdRGB_V0 (ESP32-S3 RGB Driver Board) ===== */

#include "driver/gpio.h"

#define BSP_BOARD_NAME          "HTM-H050A29-ESP-StdRGB_V0"

/* Backlight */
#define LCD_BACKLIGHT_ON        1
#define LCD_BACKLIGHT_OFF       0
#define BSP_BACKLIGHT_PWM       GPIO_NUM_6
#define BSP_BACKLIGHT_EN        -1

/* Display enable (optional, -1 if not used) */
#define BSP_DISP_EN             -1

/* I2C (touch) */
#define BSP_I2C0_SDA             GPIO_NUM_42
#define BSP_I2C0_SCL             GPIO_NUM_41

/* Touch interrupt / reset (from FPC) */
#define BSP_CTP_INT             GPIO_NUM_2
#define BSP_CTP_RST             GPIO_NUM_1

/* UART1 (optional debug) */
#define BSP_UART1_RX            GPIO_NUM_4
#define BSP_UART1_TX            GPIO_NUM_5

/* RGB data pins */
#define RGB_LCD_DATA_BITS       16            /* RGB 565 */

#define BSP_LCD_HSYNC           GPIO_NUM_17
#define BSP_LCD_VSYNC           GPIO_NUM_16
#define BSP_LCD_DE              GPIO_NUM_15
#define BSP_LCD_PCLK            GPIO_NUM_18

#define BSP_LCD_D0              GPIO_NUM_10   /* B0 */
#define BSP_LCD_D1              GPIO_NUM_9    /* B1 */
#define BSP_LCD_D2              GPIO_NUM_46   /* B2 */
#define BSP_LCD_D3              GPIO_NUM_3    /* B3 */
#define BSP_LCD_D4              GPIO_NUM_8    /* B4 */

#define BSP_LCD_D5              GPIO_NUM_47   /* G0 */
#define BSP_LCD_D6              GPIO_NUM_21   /* G1 */
#define BSP_LCD_D7              GPIO_NUM_14   /* G2 */
#define BSP_LCD_D8              GPIO_NUM_13   /* G3 */
#define BSP_LCD_D9              GPIO_NUM_12   /* G4 */
#define BSP_LCD_D10             GPIO_NUM_11   /* G5 */

#define BSP_LCD_D11             GPIO_NUM_40   /* R0 */
#define BSP_LCD_D12             GPIO_NUM_39   /* R1 */
#define BSP_LCD_D13             GPIO_NUM_38   /* R2 */
#define BSP_LCD_D14             GPIO_NUM_45   /* R3 */
#define BSP_LCD_D15             GPIO_NUM_48   /* R4 */
