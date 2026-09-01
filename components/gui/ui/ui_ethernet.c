#include "ui_ethernet.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "svc_net.h"

#include <stdio.h>

/* 定时刷新回调：读链路状态 / IP 并更新 label */
static void refresh_cb(lv_timer_t *timer)
{
    lv_obj_t *lbl = (lv_obj_t *)lv_timer_get_user_data(timer);
    net_status_t st;
    svc_net_get_status(&st);

    char buf[128];
    if (!st.link_up) {
        snprintf(buf, sizeof(buf), "Link: Down\n\nCheck network cable");
    } else if (!st.has_ip) {
        snprintf(buf, sizeof(buf), "Link: Up\n\nIP: obtaining ...");
    } else {
        snprintf(buf, sizeof(buf),
                 "Link: Up\n\n"
                 "IP:       %s\n"
                 "Gateway:  %s\n"
                 "Netmask:  %s",
                 st.ip, st.gw, st.netmask);
    }
    lv_label_set_text(lbl, buf);
}

lv_obj_t *ui_ethernet_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Ethernet", back_cb);

    lv_obj_t *lbl = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_line_space(lbl, 6, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 64);

    /* 每 1 秒刷新一次。DHCP 异步拿到 IP，故需定时刷新；链路状态也可能变化。 */
    lv_timer_t *timer = lv_timer_create(refresh_cb, 1000, lbl);
    refresh_cb(timer);   /* 立即刷新一次，避免首屏空白 */

    return scr;
}
