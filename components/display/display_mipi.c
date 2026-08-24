#include "display.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"

#include "bsp_lcd.h"

#include "esp_lcd_mipi.h"

static const char *TAG = "DISPLAY_MIPI";

esp_err_t display_init_mipi(esp_lcd_panel_handle_t *out_panel) {

    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;

    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");

    // create MIPI DSI bus
    esp_lcd_dsi_bus_handle_t dsi_bus_handle;
    esp_lcd_dsi_bus_config_t dsi_bus_config = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .lane_bit_rate_mbps = 600,
    };

    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&dsi_bus_config, &dsi_bus_handle));

    esp_lcd_panel_io_handle_t dsi_io_handle;
    esp_lcd_dbi_io_config_t dsi_io_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus_handle, &dsi_io_config, &dsi_io_handle));

    esp_lcd_dpi_panel_config_t dpi_config = {
        .num_fbs = LCD_NUM_FB,
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = LCD_FREQ_MHZ,
#if PANEL_MIPI_COLOR_BITS == 16
        .in_color_format = LCD_COLOR_FMT_RGB565,
#elif PANEL_MIPI_COLOR_BITS == 18
        .in_color_format = LCD_COLOR_FMT_RGB666,
#else
        .in_color_format = LCD_COLOR_FMT_RGB888,
#endif
        .video_timing = {
            .h_size = LCD_H_RES,
            .v_size = LCD_V_RES,
            .hsync_back_porch = LCD_HBP,
            .hsync_pulse_width = LCD_HSYNC,
            .hsync_front_porch = LCD_HFP,
            .vsync_back_porch = LCD_VBP,
            .vsync_pulse_width = LCD_VSYNC,
            .vsync_front_porch = LCD_VFP,
        },
    };

    mipi_panel_vendor_config_t vendor_config = {
        .init_cmds = PANEL_MIPI_INIT_CMDS,
        .init_cmds_size = PANEL_MIPI_INIT_CMDS_SIZE,
        .mipi_config = {
            .dsi_bus = dsi_bus_handle,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = PANEL_MIPI_COLOR_BITS,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_mipi(dsi_io_handle, &lcd_dev_config, out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*out_panel));

    ESP_LOGI(TAG, "MIPI panel ready");
    return ESP_OK;
}
