#include "ui_usb.h"
#include "ui_common.h"

/* USB 设备页：后续接入插电脑后的枚举状态 */
lv_obj_t *ui_usb_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("USB Device", back_cb);
    ui_common_add_placeholder(scr, "USB device demo: under construction");
    return scr;
}
