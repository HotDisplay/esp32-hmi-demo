#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"


#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#include "sdcard.h"
#include "bsp.h"

#define MOUNT_POINT "/sdcard"

static const char *TAG = "SDCARD_INIT";

esp_err_t sdcard_init(void) {

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    esp_err_t ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return ESP_FAIL;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;

#if BSP_SD_WIDTH != 1 && BSP_SD_WIDTH != 4 && BSP_SD_WIDTH != 8
#error "BSP_SD_WIDTH must be 1, 4, or 8"
#endif
    sdmmc_slot_config_t slot_config = {
        .clk = BSP_SD1_CLK,
        .cmd = BSP_SD1_CMD,
        .d0 = BSP_SD1_D0,
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = BSP_SD_WIDTH,
        .flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP,
    };

#if BSP_SD_WIDTH >= 4
    slot_config.d1 = BSP_SD1_D1;
    slot_config.d2 = BSP_SD1_D2;
    slot_config.d3 = BSP_SD1_D3;
#endif
#if BSP_SD_WIDTH >= 8
    slot_config.d4 = BSP_SD1_D4;
    slot_config.d5 = BSP_SD1_D5;
    slot_config.d6 = BSP_SD1_D6;
    slot_config.d7 = BSP_SD1_D7;
#endif

    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}


