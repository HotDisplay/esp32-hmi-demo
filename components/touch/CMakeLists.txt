idf_component_register(
    SRC_DIRS "." "axs15260"
    INCLUDE_DIRS "." "axs15260"
    REQUIRES bsp
            esp_lcd_touch
            esp_driver_i2c
            esp_lcd_touch_gt911
)
