#include "display.h"

esp_err_t display_init(esp_lcd_panel_handle_t *out_panel) {
#if CONFIG_DISPLAY_INTERFACE_RGB
    return display_init_rgb(out_panel);
#elif CONFIG_DISPLAY_INTERFACE_MIPI
    return display_init_mipi(out_panel);
#else
    #error "No display interface selected! Run: idf.py menuconfig -> Display Selection"
#endif
}
