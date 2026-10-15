#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CSI 实时预览：暂停/继续、重新检测，页面退出时停止后台采集。 */
lv_obj_t *ui_camera_create(lv_event_cb_t back_cb);

#ifdef __cplusplus
}
#endif
