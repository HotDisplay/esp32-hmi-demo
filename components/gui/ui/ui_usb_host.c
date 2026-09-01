#include "ui_usb_host.h"
#include "ui_common.h"

/* USB Host 页：后续接入 U 盘容量 / 键鼠输入 */
lv_obj_t *ui_usb_host_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("USB Host", back_cb);
    ui_common_add_placeholder(scr, "USB host demo: under construction");
    return scr;
}
