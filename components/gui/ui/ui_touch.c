#include "ui_touch.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "src/misc/cache/instance/lv_image_cache.h"

#include <stdlib.h>

typedef struct {
    void *pixels;
    lv_obj_t *canvas;
    lv_obj_t *position;
    lv_point_t last;
    bool drawing;
} touch_ui_t;

static void draw_point(touch_ui_t *ui, const lv_point_t *point)
{
    lv_point_t start = ui->drawing ? ui->last : *point;
    lv_display_t *display = lv_obj_get_display(ui->canvas);
    lv_layer_t layer;
    lv_canvas_init_layer(ui->canvas, &layer);
    if (ui->drawing) {
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = UI_COLOR_PRIMARY;
        line.width = 4;
        line.round_start = 1;
        line.round_end = 1;
        line.p1.x = ui->last.x;
        line.p1.y = ui->last.y;
        line.p2.x = point->x;
        line.p2.y = point->y;
        lv_draw_line(&layer, &line);
    } else {
        lv_draw_rect_dsc_t dot;
        lv_draw_rect_dsc_init(&dot);
        dot.bg_color = UI_COLOR_PRIMARY;
        dot.radius = LV_RADIUS_CIRCLE;
        lv_area_t area = { point->x - 2, point->y - 2, point->x + 1, point->y + 1 };
        lv_draw_rect(&layer, &dot, &area);
    }
    /* Canvas 默认使整个对象失效，这里只提交笔迹附近的区域。 */
    lv_display_enable_invalidation(display, false);
    lv_canvas_finish_layer(ui->canvas, &layer);
    lv_display_enable_invalidation(display, true);
    lv_area_t bounds;
    lv_obj_get_coords(ui->canvas, &bounds);
    lv_area_t dirty = {
        bounds.x1 + LV_MIN(start.x, point->x) - 3,
        bounds.y1 + LV_MIN(start.y, point->y) - 3,
        bounds.x1 + LV_MAX(start.x, point->x) + 3,
        bounds.y1 + LV_MAX(start.y, point->y) + 3,
    };
    lv_obj_invalidate_area(ui->canvas, &dirty);
    ui->last = *point;
    ui->drawing = true;
}

static void canvas_cb(lv_event_t *e)
{
    touch_ui_t *ui = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        lv_image_cache_drop(lv_canvas_get_draw_buf(ui->canvas));
        free(ui->pixels);
        free(ui);
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ui->drawing = false;
        return;
    }
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) {
        return;
    }

    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) {
        return;
    }
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t bounds;
    lv_obj_get_coords(ui->canvas, &bounds);
    lv_point_t local = { point.x - bounds.x1, point.y - bounds.y1 };
    if (local.x < 0 || local.y < 0 || local.x >= lv_obj_get_width(ui->canvas) ||
        local.y >= lv_obj_get_height(ui->canvas)) {
        ui->drawing = false;
        return;
    }
    if (code == LV_EVENT_PRESSED) {
        ui->drawing = false;
    }
    if (ui->drawing && local.x == ui->last.x && local.y == ui->last.y) {
        return;
    }
    draw_point(ui, &local);
    lv_label_set_text_fmt(ui->position, "X: %ld   Y: %ld", (long)point.x, (long)point.y);
}

static void clear_cb(lv_event_t *e)
{
    touch_ui_t *ui = lv_event_get_user_data(e);
    ui->drawing = false;
    lv_canvas_fill_bg(ui->canvas, UI_COLOR_CARD, LV_OPA_COVER);
    lv_label_set_text(ui->position, "Draw with one finger");
}

/* 单指触摸画线：连续采样点连线，抬手或移出画布后开始新笔画。 */
lv_obj_t *ui_touch_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Touch", back_cb);
    lv_display_t *display = lv_obj_get_display(scr);
    int32_t width = lv_display_get_horizontal_resolution(display) - 32;
    int32_t height = lv_display_get_vertical_resolution(display) - 128;
    if (width <= 0 || height <= 0) {
        ui_common_add_placeholder(scr, "Screen too small for drawing");
        return scr;
    }

    touch_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        ui_common_add_placeholder(scr, "Drawing unavailable: not enough memory");
        return scr;
    }
    /* 大画布使用普通堆，避免耗尽 LVGL 的小内存池。 */
    uint32_t stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_RGB565);
    ui->pixels = malloc((size_t)stride * height + LV_DRAW_BUF_ALIGN);
    if (ui->pixels == NULL) {
        free(ui);
        ui_common_add_placeholder(scr, "Drawing unavailable: not enough memory");
        return scr;
    }

    ui->canvas = lv_canvas_create(scr);
    lv_obj_remove_style_all(ui->canvas);
    lv_canvas_set_buffer(ui->canvas, lv_draw_buf_align(ui->pixels, LV_COLOR_FORMAT_RGB565),
                         width, height, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(ui->canvas, 16, 112);
    lv_obj_add_flag(ui->canvas, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(ui->canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui->canvas, canvas_cb, LV_EVENT_ALL, ui);
    lv_canvas_fill_bg(ui->canvas, UI_COLOR_CARD, LV_OPA_COVER);

    ui->position = lv_label_create(scr);
    lv_obj_set_pos(ui->position, 16, 72);
    lv_obj_set_style_text_color(ui->position, UI_COLOR_TEXT, 0);
    lv_label_set_text(ui->position, "Draw with one finger");

    lv_obj_t *clear = lv_button_create(scr);
    lv_obj_set_size(clear, 84, 36);
    lv_obj_align(clear, LV_ALIGN_TOP_RIGHT, -16, 64);
    lv_obj_set_style_bg_color(clear, UI_COLOR_PRIMARY, 0);
    lv_obj_t *label = lv_label_create(clear);
    lv_label_set_text(label, "Clear");
    lv_obj_center(label);
    lv_obj_add_event_cb(clear, clear_cb, LV_EVENT_CLICKED, ui);
    return scr;
}
