#include "ui_camera.h"
#include "ui_common.h"

/* 摄像头页：后续接入 CSI 实时预览 / 拍照 */
lv_obj_t *ui_camera_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Camera", back_cb);
    ui_common_add_placeholder(scr, "Camera demo: under construction");
    return scr;
}
