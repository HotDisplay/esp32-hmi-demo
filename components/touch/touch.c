#include "touch.h"

#include "bsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"

/* Only the driver for the selected chip is compiled and linked
 * (see CMakeLists.txt for the matching conditional REQUIRES). */
#if defined(CONFIG_BSP_TOUCH_GT911)
#include "esp_lcd_touch_gt911.h"
#elif defined(CONFIG_BSP_TOUCH_AXS15260D)
#include "esp_lcd_touch_axs15260d.h"
#endif

static const char *TAG = "TOUCH";

static esp_lcd_touch_handle_t s_touch_handle = NULL;

/* ===== Per-chip reset sequences =====================================
 * Each controller latches its I2C address differently, so the sequences are
 * kept completely separate: editing one can never affect the other.
 * The BSP only provides the pin primitives (GPIO or IO expander).         */

#if defined(CONFIG_BSP_TOUCH_GT911)

/**
 * GT911 selects its I2C address by sampling the INT pin while reset is
 * released (0x5D or 0x14 depending on the datasheet option). If INT is left
 * floating or at the wrong level at that instant, the chip comes up on the
 * other address and every transfer NACKs — which is exactly the "wrong
 * address" symptom this separation prevents.
 */
static esp_err_t touch_reset_gt911(void)
{
    /* INT is driven low first: that is the address-select strapping level.
     *
     * NOTE: the delays below are carried over verbatim from the original
     * board-verified sequence. They were tuned on real hardware — do NOT
     * shorten them to "textbook" values, the latch window is what keeps
     * GT911 from coming up on the wrong I2C address. */
    ESP_RETURN_ON_ERROR(bsp_touch_set_int(0), TAG, "gt911: int low");

    ESP_RETURN_ON_ERROR(bsp_touch_set_reset(1), TAG, "gt911: assert rst");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(bsp_touch_set_reset(0), TAG, "gt911: release rst");

    /* Address is latched while INT is held low: do not touch INT in here. */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* hand INT back to input so the driver can use it as an IRQ line */
    ESP_RETURN_ON_ERROR(bsp_touch_release_int(), TAG, "gt911: release int");
    return ESP_OK;
}
#endif /* CONFIG_BSP_TOUCH_GT911 */

#if defined(CONFIG_BSP_TOUCH_AXS15260D)

/**
 * AXS15260D uses a fixed I2C address, so only a clean reset pulse plus enough
 * time for internal initialization is required. Deliberately kept apart from
 * the GT911 sequence above.
 */
static esp_err_t touch_reset_axs15260d(void)
{
    ESP_RETURN_ON_ERROR(bsp_touch_set_reset(1), TAG, "axs: assert rst");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_RETURN_ON_ERROR(bsp_touch_set_reset(0), TAG, "axs: release rst");

    /* wait for internal init before the first I2C transfer */
    vTaskDelay(pdMS_TO_TICKS(200));
    return ESP_OK;
}
#endif /* CONFIG_BSP_TOUCH_AXS15260D */

/* ==================================================================== */

esp_err_t touch_init(esp_lcd_touch_handle_t *out_touch)
{
    if (out_touch != NULL) {
        *out_touch = NULL;
    }

#if !defined(CONFIG_DISPLAY_USE_TOUCHPAD)
    ESP_LOGI(TAG, "Touchpad disabled by menuconfig");
    return ESP_OK;

#elif !defined(CONFIG_BSP_TOUCH_GT911) && !defined(CONFIG_BSP_TOUCH_AXS15260D)
    ESP_LOGW(TAG, "This panel has no touch controller");
    return ESP_OK;

#else
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

    /* IO config and driver data must always come from the SAME chip, otherwise
     * they get mismatched when the panel selection changes. */
#if defined(CONFIG_BSP_TOUCH_GT911)
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    esp_lcd_touch_io_gt911_config_t tp_drv_cfg = { 0 };
#else
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_AXS15260D_CONFIG();
    esp_lcd_touch_io_axs15260d_config_t tp_drv_cfg = { 0 };
#endif
    io_cfg.scl_speed_hz = 400000;
    tp_drv_cfg.dev_addr = io_cfg.dev_addr;

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = BSP_TOUCH_X_MAX,
        .y_max = BSP_TOUCH_Y_MAX,
        .int_gpio_num = BSP_CTP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = BSP_TOUCH_SWAP_XY,
        },
        .driver_data = &tp_drv_cfg,
    };

    /* Run the reset sequence that belongs to the selected chip. When it
     * succeeds the chip is already out of reset, so the driver must not reset
     * it again — otherwise the address strapping is repeated under different
     * INT conditions and may latch the wrong address. */
#if defined(CONFIG_BSP_TOUCH_GT911)
    esp_err_t rst_err = touch_reset_gt911();
#else
    esp_err_t rst_err = touch_reset_axs15260d();
#endif

    if (rst_err == ESP_OK) {
        touch_cfg.rst_gpio_num = GPIO_NUM_NC;
#ifdef BSP_CTP_RST
    } else {
        /* No usable reset pin on this board: let the driver drive it directly */
        ESP_LOGW(TAG, "board reset unavailable (%s), driver drives BSP_CTP_RST",
                 esp_err_to_name(rst_err));
        touch_cfg.rst_gpio_num = BSP_CTP_RST;
#endif
    }

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &io_handle), TAG, "create panel io failed");

    /* A touch failure must not abort the whole boot: the display stays usable,
     * s_touch_handle remains NULL and callers run display-only. */
#if defined(CONFIG_BSP_TOUCH_GT911)
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_touch_new_i2c_gt911(io_handle, &touch_cfg, &s_touch_handle));
#else
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_touch_new_i2c_axs15260d(io_handle, &touch_cfg, &s_touch_handle));
#endif

    if (out_touch != NULL) {
        *out_touch = s_touch_handle;
    }
    return ESP_OK;
#endif
}
