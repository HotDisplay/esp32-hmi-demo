idf_component_register(
    SRCS "display_rgb.c" "display_mipi.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd bsp
)
