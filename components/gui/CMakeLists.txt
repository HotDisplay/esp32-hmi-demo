idf_component_register(
    SRCS "lv_init.c"
         "app/app_hmi_demo.c"
         "ui/ui_theme.c"
         "ui/ui_common.c"
         "ui/ui_launcher.c"
         "ui/ui_touch.c"
         "ui/ui_wifi.c"
         "ui/ui_ethernet.c"
         "ui/ui_usb.c"
         "ui/ui_usb_host.c"
         "ui/ui_sdcard.c"
         "ui/ui_camera.c"
         "ui/ui_sysinfo.c"
         "service/svc_sysinfo.c"
         "service/svc_net.c"
    INCLUDE_DIRS "."
                 "app"
                 "ui"
                 "service"
    REQUIRES bsp
             esp_lcd
             esp_lcd_touch
             esp_netif
             esp_system
             esp_timer
             lvgl
)
