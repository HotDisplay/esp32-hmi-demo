#include "display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

/* BSP board + panel macros come via bsp.h chain */
#include "bsp.h"

static const char *TAG = "DISPLAY_RGB";

esp_err_t display_init_rgb(esp_lcd_panel_handle_t *out_panel) {
    ESP_LOGI(TAG, "Init RGB panel: %dx%d PCLK=%lu Hz", LCD_H_RES, LCD_V_RES, LCD_PIXEL_CLOCK_HZ);

    esp_lcd_rgb_panel_config_t cfg = {
        .data_width = RGB_LCD_DATA_BITS,
        .dma_burst_size = 64,
        .num_fbs = LCD_NUM_FB,
        .bounce_buffer_size_px = 10 * LCD_H_RES, //Bounce buffer
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = -1, // DISP/EN is driven by the BSP (expander on some boards)
        .pclk_gpio_num = BSP_LCD_PCLK,
        .vsync_gpio_num = BSP_LCD_VSYNC,
        .hsync_gpio_num = BSP_LCD_HSYNC,
        .de_gpio_num = BSP_LCD_DE,
        .data_gpio_nums = {
            BSP_LCD_D0,  BSP_LCD_D1,  BSP_LCD_D2,  BSP_LCD_D3,
            BSP_LCD_D4,  BSP_LCD_D5,  BSP_LCD_D6,  BSP_LCD_D7,
            BSP_LCD_D8,  BSP_LCD_D9,  BSP_LCD_D10, BSP_LCD_D11,
            BSP_LCD_D12, BSP_LCD_D13, BSP_LCD_D14, BSP_LCD_D15,
#if RGB_LCD_DATA_BITS > 16
            BSP_LCD_D16, BSP_LCD_D17, BSP_LCD_D18, BSP_LCD_D19,
            BSP_LCD_D20, BSP_LCD_D21, BSP_LCD_D22, BSP_LCD_D23,
#endif
        },
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_back_porch = LCD_HBP,
            .hsync_front_porch = LCD_HFP,
            .hsync_pulse_width = LCD_HSYNC,
            .vsync_back_porch = LCD_VBP,
            .vsync_front_porch = LCD_VFP,
            .vsync_pulse_width = LCD_VSYNC,
            .flags = {.pclk_active_neg = LCD_PCLK_ACTIVE_NEG },
        },
        .flags.fb_in_psram = true,
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&cfg, out_panel), TAG, "new rgb panel");

    /* Panel RST / EN may live on the IO expander (e.g. ESP32P4-DEMO-A02); let
     * the BSP handle them so this component stays board-agnostic. On boards
     * without such wiring these are no-ops and the panel driver resets itself. */
    ESP_RETURN_ON_ERROR(bsp_display_enable(true), TAG, "display enable");
    ESP_RETURN_ON_ERROR(bsp_display_reset(), TAG, "display reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(*out_panel), TAG, "panel init");

    ESP_LOGI(TAG, "RGB panel ready");
    return ESP_OK;
}
