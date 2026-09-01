#include "ui_touch.h"
#include "ui_common.h"

/* 触摸测试页：后续接入画点 / 划线 / 多点触控演示 */
lv_obj_t *ui_touch_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Touch", back_cb);
    ui_common_add_placeholder(scr, "Touch demo: under construction");
    return scr;
}
