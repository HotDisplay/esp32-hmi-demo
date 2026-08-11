idf_component_register(
    SRCS "bsp_lcd.c"
    INCLUDE_DIRS "." "boards"
    REQUIRES esp_lcd
            esp_driver_ledc
            esp_driver_gpio
            esp_driver_i2c
            esp_lcd_touch
            esp_lcd_touch_gt911
            driver
            panel
            display
)
