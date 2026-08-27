#include "bsp.h"

#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_io_expander_tca95xx_16bit.h"

static const char *TAG = "BSP";

/* ===== Global panel/touch handles (used by lv_init.c via extern) ===== */
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;

#ifdef BSP_USE_IO_EXPANDER

static esp_io_expander_handle_t s_io_expander = NULL;
static i2c_master_bus_handle_t s_io_expander_i2c_bus = NULL;

static void bsp_io_expander_init(void) {
    const i2c_master_bus_config_t io_expander_i2c_config = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = BSP_I2C1_SDA,
        .scl_io_num = BSP_I2C1_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    i2c_new_master_bus(&io_expander_i2c_config, &s_io_expander_i2c_bus);
    esp_io_expander_new_i2c_tca95xx_16bit(s_io_expander_i2c_bus, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_010, &s_io_expander);
}

static void bsp_tp_reset_by_expander(void) {

    gpio_config_t int_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 1,
        .pin_bit_mask = 1ULL << BSP_CTP_INT,
    };
    gpio_config(&int_gpio_config);
    esp_io_expander_set_dir(s_io_expander, BSP_IOEXP_CTP_RST, IO_EXPANDER_OUTPUT);

    /* rst int output 0 */
    gpio_set_level(BSP_CTP_INT, 0);
    esp_io_expander_set_level(s_io_expander, BSP_IOEXP_CTP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // gpio_set_level(BSP_CTP_INT, 1);
    // vTaskDelay(pdMS_TO_TICKS(1));

    /* rst output 1 */
    esp_io_expander_set_level(s_io_expander, BSP_IOEXP_CTP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    /* int set input */
    int_gpio_config.mode = GPIO_MODE_INPUT;
    int_gpio_config.intr_type = GPIO_INTR_NEGEDGE;
    int_gpio_config.pin_bit_mask = 1ULL << BSP_CTP_INT;
    // int_gpio_config.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&int_gpio_config);

}
#endif /* BSP_USE_IO_EXPANDER */

/* ===== Backlight (internal) ===== */

static void bsp_backlight_init(void) {
#if BSP_BACKLIGHT_EN >= 0
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BSP_BACKLIGHT_EN
    };
    gpio_config(&cfg);
#endif
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
        .duty = 0,
    };
    ledc_channel_config(&ledc_channel);

}

/* 设置背光亮度 */
static void bsp_backlight_set(uint32_t level, uint32_t brightness) {
#if BSP_BACKLIGHT_EN >= 0
    gpio_set_level(BSP_BACKLIGHT_EN, level);
#endif
    uint32_t max_duty = (1U << LEDC_TIMER_10_BIT) - 1;
    uint32_t duty = max_duty * brightness / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ESP_LOGD(TAG, "duty:%d", duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

/* Turn the backlight ON. Call this only after LVGL has finished initializing,
 * so the panel is never lit with an unrendered frame. */
void bsp_backlight_enable(void) {
    bsp_backlight_set(LCD_BACKLIGHT_ON, 30);
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

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = BSP_TOUCH_X_MAX,
        .y_max = BSP_TOUCH_Y_MAX,
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

#ifdef BSP_IOEXP_CTP_RST
    touch_cfg.rst_gpio_num = GPIO_NUM_NC;
    bsp_tp_reset_by_expander();
#else
    touch_cfg.rst_gpio_num = BSP_CTP_RST;
#endif

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &io_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_touch_new_i2c_gt911(io_handle, &touch_cfg, out_touch));
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

    /* 1. IO Expander init (must precede touch reset on expander pins) */
#ifdef BSP_USE_IO_EXPANDER
    bsp_io_expander_init();
#endif

    /* 2. Touch init (display panel is initialized by the display component via
     *    display_init(), called from the application, to avoid a circular
     *    dependency between bsp and display). */
#if CONFIG_DISPLAY_USE_TOUCHPAD
    ESP_RETURN_ON_ERROR(bsp_touch_init_internal(&touch_handle), TAG, "touch init failed");
#endif

    /* 3. Backlight: configure GPIO + PWM, but keep it OFF until LVGL is ready.
     *    The application calls bsp_backlight_enable() after lvgl_init(). */
    bsp_backlight_init();

    /* Output handles */
    if (out_touch) *out_touch = touch_handle;

    ESP_LOGI(TAG, "BSP init complete (io-expander + touch + backlight cfg; display via display_init)");
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


/* ===== Display power / reset control =====
 * The panel RST and SLEEP/EN pins may live on the IO expander (e.g. A02 board)
 * or on direct MCU GPIO (e.g. boards without an expander / with GPIO wiring).
 * These helpers centralize that board-specific wiring so the display component
 * never touches the expander or pin macros directly. */

esp_err_t bsp_display_reset(void) {
#if defined(BSP_IOEXP_DISP_RST)
    if (s_io_expander == NULL) {
        ESP_LOGW(TAG, "display reset requested but no io expander");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(s_io_expander, BSP_IOEXP_DISP_RST, IO_EXPANDER_OUTPUT), TAG, "rst dir");
    esp_io_expander_set_level(s_io_expander, BSP_IOEXP_DISP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    esp_io_expander_set_level(s_io_expander, BSP_IOEXP_DISP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
#elif (BSP_DISP_RST >= 0)
    gpio_set_level(BSP_DISP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BSP_DISP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
#else 

#endif
    return ESP_OK;
}

esp_err_t bsp_display_enable(bool on) {
#if defined(BSP_IOEXP_DISP_EN)
    if (s_io_expander == NULL) {
        ESP_LOGW(TAG, "display enable requested but no io expander");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(s_io_expander, BSP_IOEXP_DISP_EN, IO_EXPANDER_OUTPUT), TAG, "sleep dir");
    /* SLEEP is active-high: high = running, low = sleep */
    esp_io_expander_set_level(s_io_expander, BSP_IOEXP_DISP_EN, on ? 1 : 0);

#elif BSP_DISP_EN >= 0
    gpio_set_level(BSP_DISP_EN, on ? 1 : 0);
#else 

#endif
    return ESP_OK;
}
