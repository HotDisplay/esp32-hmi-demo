#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建主菜单 screen（8 张功能卡片）。
 * 点击卡片时回调 cb，user_data 为 app_screen_id_t。 */
lv_obj_t *ui_launcher_create(lv_event_cb_t card_cb);

#ifdef __cplusplus
}
#endif
