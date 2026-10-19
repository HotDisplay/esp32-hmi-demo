/**
 * @file ui_brand.h
 * @brief 创建首页和 About 共用的透明 HOTHMI 标识。
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

/** @brief 创建 128x37 标识，保留图标原色并使用浅色字标。 */
lv_obj_t *ui_brand_create_logo(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
