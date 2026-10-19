/**
 * @file ui_about.c
 * @brief 展示公司标识、业务简介、官网邮箱、实际固件版本和版权信息。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#include "ui_about.h"
#include "ui_common.h"
#include "ui_brand.h"
#include "ui_theme.h"
#include "esp_app_desc.h"
#include "esp_system.h"

static lv_obj_t *add_text(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_color(label, color, 0);
    lv_label_set_text(label, text);
    return label;
}

lv_obj_t *ui_about_create(lv_event_cb_t back_cb)
{
    lv_obj_t *body;
    lv_obj_t *scr = ui_common_create_page("About", back_cb, &body);
    ui_brand_create_logo(body);
    lv_obj_t *title = add_text(body, "HOTLCD / HOTHMI", UI_COLOR_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_CARD, 0);
    add_text(body, "Hot Display Technology Co.,Ltd", UI_COLOR_TEXT);
    add_text(body, "LCD modules, touch displays and HMI solutions.", UI_COLOR_TEXT_MUTED);
    add_text(body, "Website: https://cn.display-lcd.com\nEmail: lcd@hotlcd.com", UI_COLOR_ACCENT);
    const esp_app_desc_t *app = esp_app_get_description();
    lv_obj_t *version = add_text(body, "", UI_COLOR_TEXT_MUTED);
    lv_label_set_text_fmt(version, "ESP32 HMI Demo\nFirmware: %s\nESP-IDF: %s",
                          app->version, esp_get_idf_version());
    add_text(body, "Copyright (c) 2026\nHot Display Technology Co.,Ltd\nAuthor: Lee",
             UI_COLOR_TEXT_MUTED);
    return scr;
}
