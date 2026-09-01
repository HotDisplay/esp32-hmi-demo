#include "ui_wifi.h"
#include "ui_common.h"

/* Wi-Fi 页：后续接入 C6 无线扫描 / 连接 / IP 显示 */
lv_obj_t *ui_wifi_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Wi-Fi", back_cb);
    ui_common_add_placeholder(scr, "Wi-Fi demo: under construction");
    return scr;
}
