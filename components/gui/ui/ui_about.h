/**
 * @file ui_about.h
 * @brief 公司信息、联系方式和固件版权页面接口。
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

/** @brief 创建 About 页面，返回按钮调用 back_cb。 */
lv_obj_t *ui_about_create(lv_event_cb_t back_cb);

#ifdef __cplusplus
}
#endif
