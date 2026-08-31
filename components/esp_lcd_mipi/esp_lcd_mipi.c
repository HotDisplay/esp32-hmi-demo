/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * esp_lcd_mipi — HOTDISPLAY generic MIPI-DSI panel driver.
 *
 * Initialization flow is identical across controller ICs; the only difference
 * between panels is the vendor init command sequence, which is supplied via
 * init_cmds from each panel header under components/panel/panels/dsi/.
 * Sleep Out (0x11) and Display On (0x29) must be included in that sequence.
 */

#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_lcd_mipi.h"

#define DSI_PANEL_VER_MAJOR 1
#define DSI_PANEL_VER_MINOR 0
#define DSI_PANEL_VER_PATCH 0

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_val; // save current value of LCD_CMD_COLMOD register
    const mipi_panel_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int reset_level : 1;
    } flags;
    // To save the original functions of MIPI DPI panel
    esp_err_t (*del)(esp_lcd_panel_t *panel);
    esp_err_t (*init)(esp_lcd_panel_t *panel);
} mipi_panel_t;

static const char *TAG = "esp_lcd_mipi";

static esp_err_t panel_mipi_del(esp_lcd_panel_t *panel);
static esp_err_t panel_mipi_init(esp_lcd_panel_t *panel);
static esp_err_t panel_mipi_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_mipi_disp_on_off(esp_lcd_panel_t *panel, bool on_off);

esp_err_t esp_lcd_new_panel_mipi(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel) {
    ESP_LOGI(TAG, "version: %d.%d.%d", DSI_PANEL_VER_MAJOR, DSI_PANEL_VER_MINOR, DSI_PANEL_VER_PATCH);
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid arguments");
    mipi_panel_vendor_config_t *vendor_config = (mipi_panel_vendor_config_t *)panel_dev_config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor_config && vendor_config->mipi_config.dpi_config && vendor_config->mipi_config.dsi_bus,
        ESP_ERR_INVALID_ARG, TAG, "invalid vendor config");

    esp_err_t ret = ESP_OK;
    mipi_panel_t *dsi = (mipi_panel_t *)calloc(1, sizeof(mipi_panel_t));
    ESP_RETURN_ON_FALSE(dsi, ESP_ERR_NO_MEM, TAG, "no mem for dsi panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        dsi->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        dsi->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        dsi->colmod_val = 0x55;
        break;
    case 18: // RGB666
        dsi->colmod_val = 0x66;
        break;
    case 24: // RGB888
        dsi->colmod_val = 0x77;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    dsi->io = io;
    dsi->init_cmds = vendor_config->init_cmds;
    dsi->init_cmds_size = vendor_config->init_cmds_size;
    dsi->reset_gpio_num = panel_dev_config->reset_gpio_num;
    dsi->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // Create MIPI DPI panel (controller-agnostic base)
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_dpi(vendor_config->mipi_config.dsi_bus, vendor_config->mipi_config.dpi_config, &panel_handle), err, TAG, "create MIPI DPI panel failed");
    ESP_LOGD(TAG, "new MIPI DPI panel @%p", panel_handle);

    // Save original functions of the MIPI DPI panel, then override them
    dsi->del = panel_handle->del;
    dsi->init = panel_handle->init;
    panel_handle->del = panel_mipi_del;
    panel_handle->init = panel_mipi_init;
    panel_handle->reset = panel_mipi_reset;
    panel_handle->disp_on_off = panel_mipi_disp_on_off;
    panel_handle->user_data = dsi;
    *ret_panel = panel_handle;
    ESP_LOGD(TAG, "new dsi panel @%p", dsi);

    return ESP_OK;

err:
    if (dsi) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(dsi);
    }
    return ret;
}

static esp_err_t panel_mipi_del(esp_lcd_panel_t *panel) {
    mipi_panel_t *dsi = (mipi_panel_t *)panel->user_data;

    // Delete MIPI DPI panel
    ESP_RETURN_ON_ERROR(dsi->del(panel), TAG, "del dsi panel failed");
    if (dsi->reset_gpio_num >= 0) {
        gpio_reset_pin(dsi->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del dsi panel @%p", dsi);
    free(dsi);

    return ESP_OK;
}

static esp_err_t panel_mipi_init(esp_lcd_panel_t *panel) {
    mipi_panel_t *dsi = (mipi_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = dsi->io;
    const mipi_panel_init_cmd_t *init_cmds = dsi->init_cmds;
    uint16_t init_cmds_size = dsi->init_cmds_size;

    // uint8_t ID[3] = { 0 };
    // esp_err_t id_ret = esp_lcd_panel_io_rx_param(io, 0x04, ID, 3);
    // if (id_ret == ESP_OK) {
    //     ESP_LOGI(TAG, "LCD ID: %02X %02X %02X", ID[0], ID[1], ID[2]);
    // } else {
    //     ESP_LOGW(TAG, "read LCD ID skipped/failed (0x%x), continue init", id_ret);
    // }

    // Vendor specific initialization; varies between panels / manufacturers.
    // Supplied per-panel through mipi_panel_vendor_config_t.init_cmds.

    if (init_cmds == NULL || init_cmds_size == 0) {
        ESP_LOGW(TAG, "no init command sequence provided for this panel");
    }

    for (int i = 0; i < init_cmds_size; i++) {
        // Send command
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
        ESP_LOGI(TAG, "sending %02X commands %d/%d", init_cmds[i].cmd, i + 1, init_cmds_size);
    }
    ESP_LOGI(TAG, "send init commands success", init_cmds_size);

    ESP_RETURN_ON_ERROR(dsi->init(panel), TAG, "init MIPI DPI panel failed");

    return ESP_OK;
}

static esp_err_t panel_mipi_reset(esp_lcd_panel_t *panel) {
    mipi_panel_t *dsi = (mipi_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = dsi->io;

    // Perform hardware reset
    if (dsi->reset_gpio_num >= 0) {
        gpio_set_level(dsi->reset_gpio_num, !dsi->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(5));
        gpio_set_level(dsi->reset_gpio_num, dsi->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(dsi->reset_gpio_num, !dsi->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(120));
    } else if (io) { // Perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    return ESP_OK;
}

static esp_err_t panel_mipi_disp_on_off(esp_lcd_panel_t *panel, bool on_off) {
    mipi_panel_t *dsi = (mipi_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = dsi->io;
    int command = 0;

    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}
#endif /* SOC_MIPI_DSI_SUPPORTED */
