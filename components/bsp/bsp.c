#include "bsp.h"

#include "esp_lcd_touch.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_io_expander_tca95xx_16bit.h"

static const char *TAG = "BSP";

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

#endif /* BSP_USE_IO_EXPANDER */

/* Touch pin primitives.
 *
 * The BSP only knows WHERE these pins are (plain MCU GPIO or IO expander
 * output). The reset SEQUENCE lives in components/touch, because each
 * controller latches its I2C address differently — GT911 samples the INT pin
 * while reset is released, AXS15260D does not. Keeping the timing out of the
 * BSP is what stops one chip's sequence from breaking another's.
 *
 * Every function returns ESP_ERR_NOT_SUPPORTED when the board has no such
 * pin, so the caller can fall back gracefully. */

esp_err_t bsp_touch_set_reset(int level) {
#if defined(BSP_IOEXP_CTP_RST)
    if (s_io_expander == NULL) {
        ESP_LOGW(TAG, "touch reset requested but no io expander");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(s_io_expander, BSP_IOEXP_CTP_RST, IO_EXPANDER_OUTPUT),
                        TAG, "ctp rst dir");
    ESP_RETURN_ON_ERROR(esp_io_expander_set_level(s_io_expander, BSP_IOEXP_CTP_RST, level ? 0 : 1),
                        TAG, "ctp rst level");
    return ESP_OK;
#elif defined(BSP_CTP_RST) && (BSP_CTP_RST >= 0)
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << BSP_CTP_RST,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "ctp rst cfg");
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_CTP_RST, asserted ? 0 : 1), TAG, "ctp rst level");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_touch_set_int(int level) {
#if defined(BSP_CTP_INT) && (BSP_CTP_INT >= 0)
    gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << BSP_CTP_INT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "ctp int cfg");
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_CTP_INT, level ? 1 : 0), TAG, "ctp int level");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_touch_release_int(void) {
#if defined(BSP_CTP_INT) && (BSP_CTP_INT >= 0)
    gpio_config_t cfg = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = 1ULL << BSP_CTP_INT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "ctp int release");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

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

/* Touch controller initialization lives in the `touch` component
 * (components/touch/touch.c). Call touch_init() after bsp_init(). */

/* ===== Public BSP API ===== */

esp_err_t bsp_init(void) {
    ESP_LOGI(TAG, "Initializing BSP for board: %s", BSP_BOARD_NAME);
    ESP_LOGI(TAG, "LCD: %dx%d", LCD_H_RES, LCD_V_RES);

    /* 1. IO Expander init (must precede touch reset on expander pins) */
#ifdef BSP_USE_IO_EXPANDER
    bsp_io_expander_init();
#endif

    /* 2. Backlight: configure GPIO + PWM, but keep it OFF until LVGL is ready.
     *    The application calls bsp_backlight_enable() after lvgl_init(). */
    bsp_backlight_init();

    /* The touch controller is initialized separately by the application via
     * touch_init() (components/touch), which must run AFTER bsp_init(), because
     * its reset line may sit on the IO expander configured above. */
    ESP_LOGI(TAG, "BSP init complete (io-expander + backlight cfg; display via display_init)");
    return ESP_OK;
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
