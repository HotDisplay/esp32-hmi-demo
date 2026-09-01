#include "ui_launcher.h"
#include "ui_theme.h"
#include "app_hmi_demo.h"

typedef struct {
    const char *title;
    const char *subtitle;
} launcher_item_t;

/* 卡片顺序必须与 app_screen_id_t 一致 */
static const launcher_item_t s_items[APP_SCREEN_COUNT] = {
    { "Touch",      "Touch test" },
    { "Wi-Fi",      "Scan / Connect" },
    { "Ethernet",   "Link / IP" },
    { "USB Device", "Connect to PC" },
    { "USB Host",   "USB drive / HID" },
    { "SD Card",    "Storage" },
    { "Camera",     "CSI preview" },
    { "System",     "About" },
};

static lv_obj_t *create_card(lv_obj_t *parent, const launcher_item_t *item,
                             int id, lv_event_cb_t cb)
{
    lv_obj_t *card = lv_btn_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 4, 0);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, item->title);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);

    lv_obj_t *sub = lv_label_create(card);
    lv_label_set_text(sub, item->subtitle);
    lv_obj_set_style_text_color(sub, UI_COLOR_TEXT_MUTED, 0);

    lv_obj_add_event_cb(card, cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
    return card;
}

lv_obj_t *ui_launcher_create(lv_event_cb_t card_cb)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_BG, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 内容容器：纵向排列（标题 + 卡片网格） */
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, 24, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 16, 0);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "HMI Demo");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);

    /* 卡片网格：2 列 4 行，等分铺满 */
    lv_obj_t *grid = lv_obj_create(cont);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_column(grid, 12, 0);
    lv_obj_set_style_pad_row(grid, 12, 0);

    static int32_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                 LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    for (int i = 0; i < APP_SCREEN_COUNT; i++) {
        lv_obj_t *card = create_card(grid, &s_items[i], i, card_cb);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 2, 1,
                             LV_GRID_ALIGN_STRETCH, i / 2, 1);
    }

    return scr;
}
