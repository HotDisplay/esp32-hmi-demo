#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建单指画线测试页，支持坐标显示、清空和返回时释放画布。 */
lv_obj_t *ui_touch_create(lv_event_cb_t back_cb);

#ifdef __cplusplus
}
#endif
