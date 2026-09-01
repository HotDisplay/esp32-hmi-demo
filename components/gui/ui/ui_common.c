#include "ui_common.h"
#include "ui_theme.h"

static void screen_set_bg(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, UI_COLOR_BG, 0);
}

lv_obj_t *ui_common_create_screen(const char *title, lv_event_cb_t back_cb)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    screen_set_bg(scr);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部标题栏 */
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_PCT(100), 48);
    lv_obj_set_style_bg_color(bar, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar, 12, 0);
    lv_obj_set_style_pad_left(bar, 8, 0);

    /* 返回按钮 */
    lv_obj_t *back = lv_btn_create(bar);
    lv_obj_set_size(back, 72, 34);
    lv_obj_set_style_bg_color(back, UI_COLOR_CARD, 0);
    lv_obj_set_style_radius(back, 8, 0);
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_set_style_text_color(back_lbl, UI_COLOR_TEXT, 0);
    lv_obj_center(back_lbl);
    if (back_cb) {
        lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    }

    /* 标题 */
    lv_obj_t *title_lbl = lv_label_create(bar);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_white(), 0);

    return scr;
}

void ui_common_add_placeholder(lv_obj_t *scr, const char *text)
{
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_center(lbl);
}
