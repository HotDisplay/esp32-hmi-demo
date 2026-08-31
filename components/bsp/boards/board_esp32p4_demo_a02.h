#pragma once

/* ===== Board: ESP32P4-DEMO-A02_V0 (ESP32-P4 Driver Board) ===== */

#include "esp_io_expander.h"
#include "driver/gpio.h"

#define BSP_BOARD_NAME          "ESP32P4-DEMO-A02_V0"

/* Backlight */
#define LCD_BACKLIGHT_ON        1
#define LCD_BACKLIGHT_OFF       0
#define BSP_BACKLIGHT_PWM       GPIO_NUM_5
#define BSP_BACKLIGHT_EN        -1

/* Display enable (optional, -1 if not used) */
#define BSP_IOEXP_DISP_RST       IO_EXPANDER_PIN_NUM_3
#define BSP_IOEXP_DISP_EN        IO_EXPANDER_PIN_NUM_2

/* I2C0 (touch) */
#define BSP_I2C0_SDA             GPIO_NUM_27
#define BSP_I2C0_SCL             GPIO_NUM_26

/* I2C1 (IO EXPANDER) */
#define BSP_USE_IO_EXPANDER
#define BSP_I2C1_SDA             GPIO_NUM_53
#define BSP_I2C1_SCL             GPIO_NUM_54

/* Touch interrupt / reset (from FPC) */
#define BSP_CTP_INT             GPIO_NUM_52
#define BSP_IOEXP_CTP_RST       IO_EXPANDER_PIN_NUM_1 //IO expander P01

/* RGB data pins */
#define RGB_LCD_DATA_BITS       16            /* RGB 565 */

#define BSP_LCD_HSYNC           GPIO_NUM_1
#define BSP_LCD_VSYNC           GPIO_NUM_2
#define BSP_LCD_DE              GPIO_NUM_3
#define BSP_LCD_PCLK            GPIO_NUM_4

#define BSP_LCD_D0              GPIO_NUM_10   /* B0 */
#define BSP_LCD_D1              GPIO_NUM_9   /* B1 */
#define BSP_LCD_D2              GPIO_NUM_8    /* B2 */
#define BSP_LCD_D3              GPIO_NUM_7    /* B3 */
#define BSP_LCD_D4              GPIO_NUM_6    /* B4 */

#define BSP_LCD_D5              GPIO_NUM_17   /* G0 */
#define BSP_LCD_D6              GPIO_NUM_16   /* G1 */
#define BSP_LCD_D7              GPIO_NUM_15   /* G2 */
#define BSP_LCD_D8              GPIO_NUM_14   /* G3 */
#define BSP_LCD_D9              GPIO_NUM_13   /* G4 */
#define BSP_LCD_D10             GPIO_NUM_12   /* G5 */

#define BSP_LCD_D11             GPIO_NUM_22   /* R0 */
#define BSP_LCD_D12             GPIO_NUM_21   /* R1 */
#define BSP_LCD_D13             GPIO_NUM_20   /* R2 */
#define BSP_LCD_D14             GPIO_NUM_19   /* R3 */
#define BSP_LCD_D15             GPIO_NUM_18   /* R4 */

/* SD Card pins */
#define BSP_SD1_D0              GPIO_NUM_39
#define BSP_SD1_D1              GPIO_NUM_40
#define BSP_SD1_D2              GPIO_NUM_41
#define BSP_SD1_D3              GPIO_NUM_42
#define BSP_SD1_CLK             GPIO_NUM_43
#define BSP_SD1_CMD             GPIO_NUM_44

/* C6 WIFI MOD */
#define BSP_SD2_D0              GPIO_NUM_48
#define BSP_SD2_D1              GPIO_NUM_49
#define BSP_SD2_D2              GPIO_NUM_50
#define BSP_SD2_D3              GPIO_NUM_51
#define BSP_SD2_CLK             GPIO_NUM_47
#define BSP_SD2_CMD             GPIO_NUM_46
#define BSP_IOEXP_C6_RESET      IO_EXPANDER_PIN_NUM_0

/* ETH RMII IO */
#define ETH_MDC                 GPIO_NUM_31
#define ETH_MDIO                GPIO_NUM_36
#define ETH_PHY_RST             GPIO_NUM_45

#define ETH_RMII_CLK            GPIO_NUM_32
#define ETH_RMII_TX_EN          GPIO_NUM_33
#define ETH_RMII_TXD0           GPIO_NUM_34
#define ETH_RMII_TXD1           GPIO_NUM_35
#define ETH_RMII_CRS_DV         GPIO_NUM_28
#define ETH_RMII_CRS_RXD0       GPIO_NUM_29
#define ETH_RMII_CRS_RXD1       GPIO_NUM_30





