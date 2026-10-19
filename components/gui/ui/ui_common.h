/**
 * @file ui_common.h
 * @brief 功能页的公共导航栏、内容区域和提示控件接口。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建带顶部标题栏 + 返回按钮的 screen，返回 screen 对象。
 * back_cb 为返回按钮点击回调（通常由应用层传入"回主菜单"的回调）。 */
lv_obj_t *ui_common_create_screen(const char *title, lv_event_cb_t back_cb);

/**
 * @brief 创建带滚动内容区的页面；圆屏导航和内容均位于安全矩形内。
 * @param content 接收采用纵向 Flex 布局的内容容器，由页面管理其生命周期。
 * @return 页面根对象，可用于加载以及放置全屏测试覆盖层。
 */
lv_obj_t *ui_common_create_page(const char *title, lv_event_cb_t back_cb, lv_obj_t **content);

/* 在 screen 中央放一条占位文字（功能尚未接入时使用）。 */
void ui_common_add_placeholder(lv_obj_t *scr, const char *text);

#ifdef __cplusplus
}
#endif
