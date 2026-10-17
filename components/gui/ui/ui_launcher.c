/**
 * @file ui_launcher.c
 * @brief HOTHMI 品牌首页：展示公司标识及功能卡片，适配不同尺寸和形状的屏幕。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-17
 *
 * 首页只负责界面展示和页面入口回调，不启动外设服务或轮询运行状态。
 */
#include "ui_launcher.h"
#include "ui_theme.h"
#include "ui_brand_assets.h"
#include "app_hmi_demo.h"
#include "sdkconfig.h"

typedef struct {
    const char *title;
    const char *subtitle;
    const char *symbol;
} launcher_item_t;

/** @brief 功能入口描述；数组索引必须与 app_screen_id_t 的枚举值一致。 */
static const launcher_item_t s_items[APP_SCREEN_COUNT] = {
    { "Touch",      "Draw & test",        LV_SYMBOL_EDIT },
    { "Wi-Fi",      "Scan & connect",     LV_SYMBOL_WIFI },
    { "Ethernet",   "Link & IP address",  LV_SYMBOL_SHUFFLE },
    { "USB Device", "Connect to PC",      LV_SYMBOL_USB },
    { "USB Host",   "USB drive / HID",    LV_SYMBOL_KEYBOARD },
    { "SD Card",    "Files & storage",    LV_SYMBOL_SD_CARD },
    { "Camera",     "CSI live preview",   LV_SYMBOL_VIDEO },
    { "System",     "Device information", LV_SYMBOL_SETTINGS },
};

/**
 * @brief 创建纯布局容器，避免装饰元素拦截卡片点击或参与滚动。
 */
static lv_obj_t *create_container(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

/** @brief 创建功能卡片，将页面枚举值作为点击回调的用户数据。 */
static lv_obj_t *create_card(lv_obj_t *parent, const launcher_item_t *item,
                             int id, lv_event_cb_t cb, bool compact, int32_t width)
{
    lv_obj_t *card = lv_btn_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, UI_COLOR_HOME_CARD, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, compact ? 12 : 16, 0);
    lv_obj_set_style_pad_gap(card, compact ? 10 : 14, 0);
    lv_obj_set_style_bg_color(card, lv_color_mix(UI_COLOR_ACCENT, UI_COLOR_HOME_CARD, LV_OPA_20), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, UI_COLOR_BRAND_GREEN, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, UI_COLOR_BRAND_GREEN, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(card, UI_COLOR_BRAND_GREEN, LV_STATE_FOCUS_KEY);
    lv_obj_set_flex_flow(card, compact ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                          compact ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t *icon = create_container(card);
    lv_obj_set_size(icon, compact ? 32 : 40, compact ? 32 : 40);
    lv_obj_set_style_bg_color(icon, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(icon, LV_OPA_10, 0);
    lv_obj_set_style_radius(icon, 10, 0);
    lv_obj_t *symbol = create_label(icon, item->symbol, UI_COLOR_ACCENT, UI_FONT_CARD);
    lv_obj_center(symbol);

    lv_obj_t *body = create_container(card);
    lv_obj_set_height(body, LV_SIZE_CONTENT);
    lv_obj_set_width(body, compact ? 0 : LV_PCT(100));
    if (compact) {
        lv_obj_set_flex_grow(body, 1);
    }
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 6, 0);
    const lv_font_t *title_font = compact && width < 220 ? UI_FONT_NORMAL : UI_FONT_CARD;
    lv_obj_t *title = create_label(body, item->title, UI_COLOR_TEXT, title_font);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_height(title, lv_font_get_line_height(title_font));
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_t *sub = create_label(body, item->subtitle, UI_COLOR_TEXT_MUTED, UI_FONT_NORMAL);
    lv_obj_set_width(sub, LV_PCT(100));
    lv_obj_set_height(sub, lv_font_get_line_height(UI_FONT_NORMAL));
    lv_label_set_long_mode(sub, LV_LABEL_LONG_DOT);

    if (!compact) {
        lv_obj_t *arrow = create_label(card, LV_SYMBOL_RIGHT, UI_COLOR_TEXT_MUTED, UI_FONT_NORMAL);
        lv_obj_add_flag(arrow, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_align(arrow, LV_ALIGN_TOP_RIGHT, 0, 10);
    }
    if (cb) {
        lv_obj_add_event_cb(card, cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
    }
    return card;
}

lv_obj_t *ui_launcher_create(lv_event_cb_t card_cb)
{
    lv_display_t *display = lv_display_get_default();
    int32_t width = lv_display_get_horizontal_resolution(display);
    int32_t height = lv_display_get_vertical_resolution(display);
    int32_t content_width = width;
    int32_t content_height = height;
#if CONFIG_BSP_PANEL_RGB_H034A04
    /* 圆屏使用圆内安全矩形，不能仅凭正方形分辨率判断屏幕形状。 */
    content_width = width * 70 / 100;
    content_height = height * 70 / 100;
#endif
    int32_t pad = content_width >= 900 && content_height >= 550 ? 28 : 16;
    int32_t gap = pad == 28 ? 16 : 12;
    if (content_height < 360 || content_width < 360) {
        pad = 12;
        gap = 10;
    }
    int32_t columns = content_width >= 760 && width > height ? 4 : 2;
    if (content_width < 400) {
        columns = 1;
    }
    int32_t rows = (APP_SCREEN_COUNT + columns - 1) / columns;
    bool show_description = content_height >= 580 && content_width >= 400;
    int32_t header_height = 40;
    int32_t intro_height = show_description ? 56 : 34;
    int32_t grid_height = content_height - pad * 2 - header_height - intro_height - 16 - gap * 3;
    int32_t min_grid_height = rows * 80 + (rows - 1) * gap;
    bool scroll = grid_height < min_grid_height;
    if (scroll) {
        grid_height = min_grid_height;
    }
    int32_t card_width = (content_width - pad * 2 - (columns - 1) * gap - (scroll ? 6 : 0)) / columns;
    bool compact = (grid_height - (rows - 1) * gap) / rows < 140 || card_width < 180;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COLOR_HOME_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_text_font(scr, UI_FONT_NORMAL, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cont = create_container(scr);
    lv_obj_set_size(cont, content_width, content_height);
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, pad, 0);
    lv_obj_set_style_pad_row(cont, gap, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *header = create_container(cont);
    lv_obj_set_size(header, LV_PCT(100), header_height);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 保留原图透明通道和图标颜色，不增加底板。 */
    lv_obj_t *brand = lv_image_create(header);
    lv_obj_clear_flag(brand, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_src(brand, &ui_logo_compact);
    lv_obj_set_size(brand, 128, 37);
    lv_image_set_inner_align(brand, LV_IMAGE_ALIGN_CONTAIN);
    lv_image_set_antialias(brand, true);

    /* 裁剪起点位于图标和字标的空隙，只将深色字标覆盖为浅色。 */
    lv_obj_t *wordmark = create_container(brand);
    lv_obj_set_pos(wordmark, 36, 0);
    lv_obj_set_size(wordmark, 92, 37);
    lv_obj_t *wordmark_image = lv_image_create(wordmark);
    lv_obj_clear_flag(wordmark_image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_src(wordmark_image, &ui_logo_compact);
    lv_obj_set_pos(wordmark_image, -36, 0);
    lv_obj_set_size(wordmark_image, 128, 37);
    lv_image_set_inner_align(wordmark_image, LV_IMAGE_ALIGN_CONTAIN);
    lv_image_set_antialias(wordmark_image, true);
    lv_obj_set_style_image_recolor(wordmark_image, UI_COLOR_TEXT, 0);
    lv_obj_set_style_image_recolor_opa(wordmark_image, LV_OPA_COVER, 0);

    lv_obj_t *device = create_container(header);
    lv_obj_set_size(device, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(device, content_width < 400 ? 8 : 12, 0);
    lv_obj_set_style_pad_ver(device, 8, 0);
    lv_obj_set_style_border_width(device, 1, 0);
    lv_obj_set_style_border_color(device, UI_COLOR_BORDER, 0);
    lv_obj_set_style_radius(device, 8, 0);
#if CONFIG_IDF_TARGET_ESP32P4
    const char *chip = "ESP32-P4";
#elif CONFIG_IDF_TARGET_ESP32S3
    const char *chip = "ESP32-S3";
#else
    const char *chip = CONFIG_IDF_TARGET;
#endif
    lv_obj_t *device_label = create_label(device, chip, UI_COLOR_TEXT_MUTED, UI_FONT_NORMAL);
    if (content_width >= 760) {
        lv_label_set_text_fmt(device_label, "%s   /   %ld x %ld", chip, (long)width, (long)height);
    }

    lv_obj_t *intro = create_container(cont);
    lv_obj_set_size(intro, LV_PCT(100), intro_height);
    lv_obj_set_flex_flow(intro, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(intro, 6, 0);
    create_label(intro, "HOTHMI Demo",
                 UI_COLOR_TEXT, content_width < 400 ? UI_FONT_CARD : UI_FONT_HEADING);
    if (show_description) {
        create_label(intro, "Display / Touch / Connectivity", UI_COLOR_BRAND_GREEN, UI_FONT_NORMAL);
    }

    /* 窄屏保留足够的触摸区域，在网格区域内滚动查看全部入口。 */
    lv_obj_t *viewport = create_container(cont);
    lv_obj_set_width(viewport, LV_PCT(100));
    lv_obj_set_height(viewport, 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_pad_right(viewport, scroll ? 6 : 0, 0);
    lv_obj_set_scrollbar_mode(viewport, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(viewport, UI_COLOR_BORDER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(viewport, 3, LV_PART_SCROLLBAR);
    if (scroll) {
        lv_obj_add_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(viewport, LV_DIR_VER);
    }

    lv_obj_t *grid = create_container(viewport);
    lv_obj_set_size(grid, LV_PCT(100), grid_height);
    lv_obj_set_style_pad_gap(grid, gap, 0);
    static const int32_t cols_1[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t cols_2[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t cols_4[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t rows_2[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t rows_4[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t rows_8[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, columns == 4 ? cols_4 : columns == 2 ? cols_2 : cols_1,
                              rows == 2 ? rows_2 : rows == 4 ? rows_4 : rows_8);
    for (int i = 0; i < APP_SCREEN_COUNT; i++) {
        lv_obj_t *card = create_card(grid, &s_items[i], i, card_cb, compact, card_width);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % columns, 1,
                             LV_GRID_ALIGN_STRETCH, i / columns, 1);
    }

    lv_obj_t *footer = create_container(cont);
    lv_obj_set_size(footer, LV_PCT(100), 16);
    lv_obj_t *hint = create_label(footer, scroll ? "Swipe to explore all modules" : "Select a module to get started",
                                  UI_COLOR_TEXT_MUTED, UI_FONT_NORMAL);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_label_set_long_mode(hint, LV_LABEL_LONG_DOT);
    return scr;
}
