#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_event.h"
#include "sdkconfig.h"

#include "bsp.h"

static const char *TAG = "Ethernet_Init";

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
    uint8_t mac_addr[6] = { 0 };
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

static esp_err_t ethernet_install(esp_eth_handle_t *eth_handle_out) {
    if (eth_handle_out == NULL) {
        ESP_LOGE(TAG, "invalid argument: eth_handle_out cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    // Update PHY config based on board specific configuration
    phy_config.phy_addr = ESP_ETH_PHY_ADDR_AUTO;
    phy_config.reset_gpio_num = ETH_PHY_RST;
    // #if CONFIG_EXAMPLE_ETH_PHY_RST_TIMING_EN
    phy_config.hw_reset_assert_time_us = 10000;
    phy_config.post_hw_reset_delay_ms = 100;
    phy_config.reset_timeout_ms = 1000;
    // #endif // CONFIG_EXAMPLE_ETH_PHY_RST_TIMING_EN

        // Init vendor specific MAC config to default
    eth_esp32_emac_config_t esp32_emac_config = {
        .smi_gpio = {
            .mdc_num = ETH_MDC,
            .mdio_num = ETH_MDIO
        },
        .interface = EMAC_DATA_INTERFACE_RMII,
        .clock_config = {
            .rmii = {
                .clock_mode = EMAC_CLK_EXT_IN,
                .clock_gpio = ETH_RMII_CLK
            }
        },
        .dma_burst_len = ETH_DMA_BURST_LEN_32,
        .intr_priority = 0,
        .mdc_freq_hz = 0,
        .emac_dataif_gpio = {
            .rmii = {
                .tx_en_num = ETH_RMII_TX_EN,
                .txd0_num = ETH_RMII_TXD0,
                .txd1_num = ETH_RMII_TXD1,
                .crs_dv_num = ETH_RMII_CRS_DV,
                .rxd0_num = ETH_RMII_CRS_RXD0,
                .rxd1_num = ETH_RMII_CRS_RXD1
            }
        },
        .clock_config_out_in = {
            .rmii = {
                .clock_mode = EMAC_CLK_EXT_IN,
                .clock_gpio = -1
            }
        },
    };

    // Create new ESP32 Ethernet MAC instance
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    if (mac == NULL) {
        ESP_LOGE(TAG, "create MAC instance failed");
        return ESP_FAIL;
    }

    // Create new generic PHY instance
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "create PHY instance failed");
        mac->del(mac);
        return ESP_FAIL;
    }

    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    if (esp_eth_driver_install(&config, &eth_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Ethernet driver install failed");
        mac->del(mac);
        phy->del(phy);
        return ESP_FAIL;
    }

    *eth_handle_out = eth_handle;

    return ESP_OK;
}

esp_err_t ethernet_init(void) {
    // Initialize Ethernet driver
    esp_eth_handle_t eth_handle;
    ethernet_install(&eth_handle);

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create instance of esp-netif for Ethernet
    // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
    // default esp-netif configuration parameters.
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, eth_netif_glue));

    // Register user defined event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    return ESP_OK;
}