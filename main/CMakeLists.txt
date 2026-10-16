idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS "."
                       REQUIRES
                       nvs_flash
                       esp_partition
                       esp_netif
                       esp_event
                       bsp
                       display
                       touch
                       gui
                       wifi
                       ethernet
                       sdcard
                       )
