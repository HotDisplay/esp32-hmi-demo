idf_component_register(
    SRCS "bsp.c"
    INCLUDE_DIRS "." "boards"
    REQUIRES esp_lcd
            esp_lcd_touch
            esp_driver_ledc
            esp_driver_gpio
            esp_driver_i2c
            esp_io_expander
            esp_io_expander_tca95xx_16bit
            driver
            panel
)
