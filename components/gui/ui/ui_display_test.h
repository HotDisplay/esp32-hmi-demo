/**
 * @file ui_display_test.h
 * @brief 带自动播放、手动切换和背光调节的全屏显示测试页面接口。
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

/** @brief 创建测试控制页；全屏测试中轻点暂停，控制栏支持切换、播放和停止。 */
lv_obj_t *ui_display_test_create(lv_event_cb_t back_cb);

#ifdef __cplusplus
}
#endif
