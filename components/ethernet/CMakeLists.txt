idf_component_register(SRCS "ethernet_init.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_netif esp_eth bsp
                    )
