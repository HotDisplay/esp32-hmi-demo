set(board_sources "")
set(board_requires "")
if(IDF_TARGET STREQUAL "esp32p4")
    # Dependencies must be known during IDF's early requirements scan.
    set(board_requires bsp)
endif()
if(IDF_TARGET STREQUAL "esp32p4" AND
   CONFIG_ESP_WIFI_REMOTE_ENABLED AND CONFIG_ESP_WIFI_REMOTE_LIBRARY_HOSTED AND
   CONFIG_BSP_BOARD_ESP32P4_DEMO_A02)
    set(board_sources "board_esp32p4_demo_a02_wifi.c")
endif()

idf_component_register(
    SRCS "wifi_init.c" ${board_sources}
    WHOLE_ARCHIVE
    INCLUDE_DIRS "."
    REQUIRES esp_wifi
             esp_event
             esp_netif
             lwip
    PRIV_REQUIRES ${board_requires}
)
