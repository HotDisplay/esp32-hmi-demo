#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建带顶部标题栏 + 返回按钮的 screen，返回 screen 对象。
 * back_cb 为返回按钮点击回调（通常由应用层传入"回主菜单"的回调）。 */
lv_obj_t *ui_common_create_screen(const char *title, lv_event_cb_t back_cb);

/* 在 screen 中央放一条占位文字（功能尚未接入时使用）。 */
void ui_common_add_placeholder(lv_obj_t *scr, const char *text);

#ifdef __cplusplus
}
#endif
