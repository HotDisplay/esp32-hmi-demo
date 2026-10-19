/**
 * @file ui_brand.c
 * @brief 在深色页面上显示无底板的彩色 HOTHMI 图标和浅色字标。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#include "ui_brand.h"
#include "ui_brand_assets.h"
#include "ui_theme.h"

lv_obj_t *ui_brand_create_logo(lv_obj_t *parent)
{
    lv_obj_t *brand = lv_image_create(parent);
    lv_obj_clear_flag(brand, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_src(brand, &ui_logo_compact);
    lv_obj_set_size(brand, 128, 37);
    lv_image_set_inner_align(brand, LV_IMAGE_ALIGN_CONTAIN);
    lv_image_set_antialias(brand, true);

    /* 裁剪起点位于图标和字标的空隙，只将深色字标覆盖为浅色。 */
    lv_obj_t *wordmark = lv_obj_create(brand);
    lv_obj_remove_style_all(wordmark);
    lv_obj_clear_flag(wordmark, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(wordmark, 36, 0);
    lv_obj_set_size(wordmark, 92, 37);
    lv_obj_t *image = lv_image_create(wordmark);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_src(image, &ui_logo_compact);
    lv_obj_set_pos(image, -36, 0);
    lv_obj_set_size(image, 128, 37);
    lv_image_set_inner_align(image, LV_IMAGE_ALIGN_CONTAIN);
    lv_image_set_antialias(image, true);
    lv_obj_set_style_image_recolor(image, UI_COLOR_TEXT, 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    return brand;
}
