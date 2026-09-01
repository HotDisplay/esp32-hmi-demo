#include "app_hmi_demo.h"

#include "lvgl.h"
#include "esp_log.h"

#include "ui_theme.h"
#include "ui_launcher.h"
#include "ui_touch.h"
#include "ui_wifi.h"
#include "ui_ethernet.h"
#include "ui_usb.h"
#include "ui_usb_host.h"
#include "ui_sdcard.h"
#include "ui_camera.h"
#include "ui_sysinfo.h"

static const char *TAG = "HMI_APP";

/* 主菜单 screen，返回按钮统一回到这里 */
static lv_obj_t *s_launcher = NULL;

/* 返回主菜单（所有功能页的返回按钮都绑这个回调） */
static void on_back(lv_event_t *e)
{
    (void)e;
    if (s_launcher) {
        lv_scr_load(s_launcher);
    }
}

/* 主菜单卡片点击 → 按 ID 分派到对应功能页 */
static void on_card_click(lv_event_t *e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *scr = NULL;

    switch (id) {
    case APP_SCREEN_TOUCH:    scr = ui_touch_create(on_back);    break;
    case APP_SCREEN_WIFI:     scr = ui_wifi_create(on_back);     break;
    case APP_SCREEN_ETHERNET: scr = ui_ethernet_create(on_back); break;
    case APP_SCREEN_USB:      scr = ui_usb_create(on_back);      break;
    case APP_SCREEN_USB_HOST: scr = ui_usb_host_create(on_back); break;
    case APP_SCREEN_SDCARD:   scr = ui_sdcard_create(on_back);   break;
    case APP_SCREEN_CAMERA:   scr = ui_camera_create(on_back);   break;
    case APP_SCREEN_SYSINFO:  scr = ui_sysinfo_create(on_back);  break;
    default:
        ESP_LOGW(TAG, "unknown screen id: %d", id);
        return;
    }

    if (scr) {
        lv_scr_load(scr);
    }
}

void app_hmi_demo(void)
{
    ESP_LOGI(TAG, "Build HMI demo UI");
    ui_theme_init();
    s_launcher = ui_launcher_create(on_card_click);
    lv_scr_load(s_launcher);
}
