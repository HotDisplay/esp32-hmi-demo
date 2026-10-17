/**
 * @file ui_launcher.h
 * @brief HMI 演示首页的创建接口及功能入口回调约定。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-17
 */
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建带自适应布局的 HMI 功能首页。
 * @param card_cb 卡片的 LV_EVENT_CLICKED 回调，可为 NULL。
 *                回调的 user_data 携带对应的 app_screen_id_t 枚举值。
 * @return 首页 screen 对象，由调用方加载并管理其生命周期。
 * @note 必须在 LVGL 任务中或持有 LVGL API 锁时调用。
 */
lv_obj_t *ui_launcher_create(lv_event_cb_t card_cb);

#ifdef __cplusplus
}
#endif
