/*
 * SPDX-FileCopyrightText: 2015-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 HotDisplay (rewritten to use esp_lcd_panel_io)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rewritten on top of the upstream waveshare component so that every transfer
 * goes through esp_lcd_panel_io (like the GT911 driver does) instead of a raw
 * i2c_master_dev_handle pulled out of config->user_data. The point parsing is
 * unchanged.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_bit_defs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"

#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_axs15260d.h"

static const char *TAG = "AXS15260D";

/* AXS15260D registers */
#define AXS15260D_REG_VERSION           0x0C

#define AXS15260_TOUCH_HEAD_PACKET      (2)  /* Frame header length */
#define AXS15260_TOUCH_POINT_SIZE       (6)  /* Length of one touch point */
#define AXS15260_TOUCH_BUF_SIZE         ((5 * (AXS15260_TOUCH_POINT_SIZE)) + AXS15260_TOUCH_HEAD_PACKET)

/* Point data starts right after the 2-byte header */
#define AXS15260_TOUCH_DATA_OFFSET      AXS15260_TOUCH_HEAD_PACKET

/*******************************************************************************
* Function definitions
*******************************************************************************/
static esp_err_t esp_lcd_touch_axs15260d_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_axs15260d_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength,
                                           uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_axs15260d_del(esp_lcd_touch_handle_t tp);

/* I2C read/write — all through esp_lcd_panel_io */
static esp_err_t touch_axs15260d_i2c_read(esp_lcd_touch_handle_t tp, uint8_t *data, size_t len);
static esp_err_t touch_axs15260d_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data);
static esp_err_t touch_axs15260d_i2c_write_read(esp_lcd_touch_handle_t tp, uint8_t *cmd, uint8_t cmd_len,
                                                uint8_t *data, uint8_t data_len);

static esp_err_t touch_axs15260d_reset(esp_lcd_touch_handle_t tp);
static esp_err_t axs15260_touch_get_version(esp_lcd_touch_handle_t tp);

/*******************************************************************************
* Public API functions
*******************************************************************************/

esp_err_t esp_lcd_touch_new_i2c_axs15260d(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                          esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller io handle can't be NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the touch controller configuration can't be NULL");
    ESP_RETURN_ON_FALSE(out_touch != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the touch controller handle can't be NULL");

    /* Prepare main structure */
    esp_lcd_touch_handle_t esp_lcd_touch_axs15260d = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(esp_lcd_touch_axs15260d, ESP_ERR_NO_MEM, err, TAG, "no mem for AXS15260D controller");

    /* Communication interface */
    esp_lcd_touch_axs15260d->io = io;

    /* Only supported callbacks are set */
    esp_lcd_touch_axs15260d->read_data = esp_lcd_touch_axs15260d_read_data;
    esp_lcd_touch_axs15260d->get_xy = esp_lcd_touch_axs15260d_get_xy;
    esp_lcd_touch_axs15260d->del = esp_lcd_touch_axs15260d_del;

    /* Mutex */
    esp_lcd_touch_axs15260d->data.lock.owner = portMUX_FREE_VAL;

    /* Save config */
    memcpy(&esp_lcd_touch_axs15260d->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch controller reset */
    if (esp_lcd_touch_axs15260d->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(esp_lcd_touch_axs15260d->config.rst_gpio_num)
        };
        ret = gpio_config(&rst_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
    }

    /* Reset controller */
    ret = touch_axs15260d_reset(esp_lcd_touch_axs15260d);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "AXS15260D reset failed");

    /* Prepare pin for touch interrupt */
    if (esp_lcd_touch_axs15260d->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (esp_lcd_touch_axs15260d->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(esp_lcd_touch_axs15260d->config.int_gpio_num)
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");

        /* Register interrupt callback */
        if (esp_lcd_touch_axs15260d->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(esp_lcd_touch_axs15260d,
                                                      esp_lcd_touch_axs15260d->config.interrupt_callback);
        }
    }

    /* Read status and config info */
    ret = axs15260_touch_get_version(esp_lcd_touch_axs15260d);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "AXS15260D init failed");

    *out_touch = esp_lcd_touch_axs15260d;

err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (0x%x)! Touch controller AXS15260D initialization failed!", ret);
        if (esp_lcd_touch_axs15260d) {
            esp_lcd_touch_axs15260d_del(esp_lcd_touch_axs15260d);
        }
    }

    return ret;
}

static esp_err_t esp_lcd_touch_axs15260d_read_data(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[AXS15260_TOUCH_BUF_SIZE] = {0};

    memset(buf, 0xff, sizeof(buf));
    esp_err_t err = touch_axs15260d_i2c_read(tp, buf, AXS15260_TOUCH_BUF_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read error!");
        return ESP_OK;
    }

    uint8_t gesture = buf[0];
    uint8_t point_byte = buf[1];
    uint8_t point_num = point_byte & 0x0F;

    if (gesture > 0x0F || point_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        portENTER_CRITICAL(&tp->data.lock);
        tp->data.points = 0;
        tp->data.coords[0].track_id = 0;
        portEXIT_CRITICAL(&tp->data.lock);
        return ESP_OK;
    }

    uint8_t esd_flag = point_byte >> 4;
    if (esd_flag && esd_flag != 0x08 && esd_flag != 0x04) {
        portENTER_CRITICAL(&tp->data.lock);
        tp->data.points = 0;
        portEXIT_CRITICAL(&tp->data.lock);
        return ESP_OK;
    }

    uint8_t event = buf[AXS15260_TOUCH_DATA_OFFSET] >> 4;

    portENTER_CRITICAL(&tp->data.lock);

    tp->data.points = point_num;

    for (int i = 0; i < point_num; i++) {
        uint8_t step = AXS15260_TOUCH_DATA_OFFSET + (AXS15260_TOUCH_POINT_SIZE * i);

        if ((event != 0x08) && (event != 0)) {
            tp->data.points--;
            continue;
        }

        uint16_t x = ((buf[step] & 0x0F) << 8) | buf[step + 1];
        uint16_t y = ((buf[step + 2] & 0x0F) << 8) | buf[step + 3];

        tp->data.points = point_num;
        tp->data.coords[i].x = x;
        tp->data.coords[i].y = y;
        tp->data.coords[i].track_id = buf[step + 2] >> 4;
        tp->data.coords[i].strength = buf[step + 4];
        // area = buf[step + 5] >> 4;
    }

    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool esp_lcd_touch_axs15260d_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength,
                                           uint8_t *point_num, uint8_t max_point_num)
{
    ESP_RETURN_ON_FALSE(tp != NULL, false, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(x != NULL, false, TAG, "Pointer to the x coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(y != NULL, false, TAG, "Pointer to the y coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(point_num != NULL, false, TAG, "Pointer to number of touch points can't be NULL");
    ESP_RETURN_ON_FALSE(max_point_num > 0, false, TAG, "Array size must be equal or larger than 1");

    portENTER_CRITICAL(&tp->data.lock);

    /* Count of points */
    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t esp_lcd_touch_axs15260d_del(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }

    /* Reset GPIO pin settings */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);

    return ESP_OK;
}

/*******************************************************************************
* Private API function
*******************************************************************************/

/* Reset controller */
static esp_err_t touch_axs15260d_reset(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level error!");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level error!");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_OK;
}

static esp_err_t axs15260_touch_get_version(esp_lcd_touch_handle_t tp)
{
    uint8_t cmd = AXS15260D_REG_VERSION;
    uint8_t data[2] = {0};

    esp_err_t ret = touch_axs15260d_i2c_write_read(tp, &cmd, sizeof(uint8_t), data, sizeof(data));
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ESP_ERR_INVALID_RESPONSE, TAG, "Failed to read version information");

    ESP_LOGI(TAG, "Firmware version number is 0x%02x%02x", data[0], data[1]);

    return ESP_OK;
}

/**
 * Bare read: no register address is sent, the chip streams its point buffer
 * straight away. In the panel_io layer a negative lcd_cmd means "receive only",
 * which maps to i2c_master_receive (see panel_io_i2c_rx_buffer in ESP-IDF).
 */
static esp_err_t touch_axs15260d_i2c_read(esp_lcd_touch_handle_t tp, uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "Pointer to the data array can't be NULL");

    return esp_lcd_panel_io_rx_param(tp->io, -1, data, len);
}

/**
 * Register read: send `cmd_len` register address bytes, then read back.
 * The AXS15260D only ever uses a single address byte, which matches
 * lcd_cmd_bits = 8 in ESP_LCD_TOUCH_IO_I2C_AXS15260D_CONFIG().
 */
static esp_err_t touch_axs15260d_i2c_write_read(esp_lcd_touch_handle_t tp, uint8_t *cmd, uint8_t cmd_len,
                                                uint8_t *data, uint8_t data_len)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(cmd != NULL && cmd_len == 1, ESP_ERR_INVALID_ARG, TAG,
                        "AXS15260D register read expects exactly one address byte");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "Pointer to the data array can't be NULL");

    return esp_lcd_panel_io_rx_param(tp->io, cmd[0], data, data_len);
}

static __unused esp_err_t touch_axs15260d_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    // *INDENT-OFF*
    return esp_lcd_panel_io_tx_param(tp->io, reg, (uint8_t[]){data}, 1);
    // *INDENT-ON*
}
