/**
 * @file ui_display_test.h
 * @brief 带启动按钮的全屏显示测试页面接口。
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

/** @brief 创建测试控制页；启动后全屏循环，触摸画面停止测试。 */
lv_obj_t *ui_display_test_create(lv_event_cb_t back_cb);

#ifdef __cplusplus
}
#endif
