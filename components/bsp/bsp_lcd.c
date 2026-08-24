#include "bsp_lcd.h"

#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"

#include "driver/ledc.h"
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "BSP";

/* ===== Global panel/touch handles (used by lv_init.c via extern) ===== */
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t  touch_handle = NULL;

/* ===== Backlight (internal) ===== */

static void bsp_backlight_init(void) {
#if BSP_BACKLIGHT_EN >= 0
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BSP_BACKLIGHT_EN
    };
    gpio_config(&cfg);
#endif
}

static void bsp_backlight_pwm_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BSP_BACKLIGHT_PWM,
        .duty = 50,
    };
    ledc_channel_config(&ledc_channel);
}

/* 设置背光亮度 */
static void bsp_backlight_set_level(uint32_t level) {
#if BSP_BACKLIGHT_EN >= 0
    gpio_set_level(BSP_BACKLIGHT_EN, level);
#endif
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 200);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

/* ===== Touch (internal) ===== */

static esp_err_t bsp_touch_init_internal(esp_lcd_touch_handle_t *out_touch) {
#if BSP_TOUCH_TYPE == BSP_TOUCH_GT911
    i2c_master_bus_handle_t i2c_bus = NULL;
    esp_lcd_panel_io_handle_t io_handle = NULL;

    i2c_master_bus_config_t i2c_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C0_SDA,
        .scl_io_num = BSP_I2C0_SCL,
        .i2c_port = I2C_NUM_0,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_cfg, &i2c_bus), TAG, "i2c bus failed");

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.scl_speed_hz = 400000;

    esp_lcd_touch_io_gt911_config_t gt911_cfg = {
        .dev_addr = io_cfg.dev_addr,
    };

    const esp_lcd_touch_config_t touch_cfg = {
        .x_max = BSP_TOUCH_X_MAX,
        .y_max = BSP_TOUCH_Y_MAX,
        .rst_gpio_num = BSP_CTP_RST,
        .int_gpio_num = BSP_CTP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0
        },
        .flags = {
            .swap_xy = BSP_TOUCH_SWAP_XY
        },
        .driver_data = &gt911_cfg,
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &io_handle), TAG, "touch io failed");
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(io_handle, &touch_cfg, out_touch), TAG, "gt911 failed");
#else
    *out_touch = NULL;
    ESP_LOGW(TAG, "Touch not configured");
#endif
    return ESP_OK;
}

/* ===== Public BSP API ===== */

esp_err_t bsp_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch) {
    ESP_LOGI(TAG, "Initializing BSP for board: %s", BSP_BOARD_NAME);
    ESP_LOGI(TAG, "LCD: %dx%d", LCD_H_RES, LCD_V_RES);

    /* 1. Backlight setup */
    bsp_backlight_init();
    bsp_backlight_pwm_init();
    bsp_backlight_set_level(LCD_BACKLIGHT_ON);

    /* 2. Touch (display panel is initialized by the display component via
     *    display_init(), called from the application, to avoid a circular
     *    dependency between bsp and display). */
#if CONFIG_DISPLAY_USE_TOUCHPAD
    ESP_RETURN_ON_ERROR(bsp_touch_init_internal(&touch_handle), TAG, "touch init failed");
#endif

    /* Output handles */
    if (out_touch) *out_touch = touch_handle;

    ESP_LOGI(TAG, "BSP init complete (backlight + touch; display via display_init)");
    return ESP_OK;
}

/* Set the panel handle produced by display_init() so other modules
 * (e.g. lv_init.c) can access it through the extern global. */
void bsp_set_panel_handle(esp_lcd_panel_handle_t panel) {
    panel_handle = panel;
}

const char *bsp_board_name(void) {
    return BSP_BOARD_NAME;
}

const char *bsp_panel_name(void) {
    return BSP_PANEL_NAME;
}

uint16_t bsp_get_screen_width(void) { return LCD_H_RES; }
uint16_t bsp_get_screen_height(void) { return LCD_V_RES; }
