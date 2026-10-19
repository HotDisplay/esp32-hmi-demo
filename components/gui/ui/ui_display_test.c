/**
 * @file ui_display_test.c
 * @brief 通过 LVGL 循环显示纯色、16 级灰阶和可选图片，不接管 LCD 驱动回调。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#include "ui_display_test.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "display_test_resource.h"
#include "bsp.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "esp_log.h"
#include <stdlib.h>

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *view;
    lv_obj_t *picture;
    lv_obj_t *brightness_label;
    lv_timer_t *timer;
    display_test_image_t mapped;
    lv_image_dsc_t image;
    unsigned stage;
    bool running;
    lv_opa_t system_layer_opa;
} display_test_ui_t;

/** @brief 拖动滑块时实时更新板级 PWM；设置失败则恢复最近成功的亮度。 */
static void brightness_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    esp_err_t err = bsp_backlight_set_brightness(lv_slider_get_value(slider));
    unsigned brightness = bsp_backlight_get_brightness();
    if (err != ESP_OK) {
        ESP_LOGW("LCD_TEST_UI", "Set brightness failed: %s", esp_err_to_name(err));
        lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
    }
    lv_label_set_text_fmt(ui->brightness_label, "Brightness: %u%%%s", brightness,
                           err == ESP_OK ? "" : " (failed)");
}

/** @brief 按实际显示方向绘制精确的纯色或灰阶条带，不分配整帧画布。 */
static void draw_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    lv_area_t area;
    lv_obj_get_coords(ui->view, &area);
    int32_t width = lv_area_get_width(&area);
    int32_t height = lv_area_get_height(&area);
    bool landscape = width >= height;
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_COVER;
    unsigned bands = ui->stage == DISPLAY_TEST_GRAYSCALE ? 16 : 1;
    for (unsigned i = 0; i < bands; i++) {
        lv_area_t band = area;
        if (landscape) {
            band.x1 += (i * width + bands - 1) / bands;
            band.x2 = area.x1 + ((i + 1) * width + bands - 1) / bands - 1;
        } else {
            band.y1 += (i * height + bands - 1) / bands;
            band.y2 = area.y1 + ((i + 1) * height + bands - 1) / bands - 1;
        }
        rect.bg_color = lv_color_hex(display_test_pattern_color(ui->stage, i));
        lv_draw_rect(lv_event_get_layer(e), &rect, &band);
    }
}

static void show_stage(display_test_ui_t *ui)
{
    if (ui->stage == DISPLAY_TEST_IMAGE) lv_obj_clear_flag(ui->picture, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui->picture, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(ui->view);
    ESP_LOGI("LCD_TEST_UI", "%s", display_test_pattern_names[ui->stage]);
}

static void next_cb(lv_timer_t *timer)
{
    display_test_ui_t *ui = lv_timer_get_user_data(timer);
    unsigned count = ui->mapped.pixels ? DISPLAY_TEST_PATTERN_COUNT : DISPLAY_TEST_IMAGE;
    ui->stage = (ui->stage + 1) % count;
    show_stage(ui);
}

static void stop_test(display_test_ui_t *ui)
{
    lv_timer_pause(ui->timer);
    lv_obj_add_flag(ui->view, LV_OBJ_FLAG_HIDDEN);
    if (ui->running) {
        lv_obj_set_style_opa(lv_display_get_layer_sys(lv_obj_get_display(ui->screen)),
                             ui->system_layer_opa, 0);
    }
    ui->running = false;
}

static void stop_cb(lv_event_t *e)
{
    stop_test(lv_event_get_user_data(e));
}

static void start_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    if (ui->running) return;
    lv_obj_t *system = lv_display_get_layer_sys(lv_obj_get_display(ui->screen));
    /* 系统层没有父对象，当前 LVGL 不支持对其直接清除 HIDDEN 标志。 */
    ui->system_layer_opa = lv_obj_get_style_opa(system, LV_PART_MAIN);
    lv_obj_set_style_opa(system, LV_OPA_TRANSP, 0);
    ui->running = true;
    ui->stage = DISPLAY_TEST_RED;
    lv_obj_clear_flag(ui->view, LV_OBJ_FLAG_HIDDEN);
    show_stage(ui);
    lv_timer_reset(ui->timer);
    lv_timer_resume(ui->timer);
}

/** @brief 页面删除时先停止刷新并撤销图片引用，再释放映射和上下文。 */
static void delete_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    stop_test(ui);
    lv_timer_delete(ui->timer);
    lv_image_set_src(ui->picture, NULL);
    lv_image_cache_drop(&ui->image);
    display_test_image_close(&ui->mapped);
    free(ui);
}

lv_obj_t *ui_display_test_create(lv_event_cb_t back_cb)
{
    lv_obj_t *body;
    lv_obj_t *scr = ui_common_create_page("Display Test", back_cb, &body);
    display_test_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        ui_common_add_placeholder(scr, "Test unavailable: not enough memory");
        return scr;
    }
    ui->screen = scr;
    ui->timer = lv_timer_create(next_cb, 1000, ui);
    if (ui->timer == NULL) {
        free(ui);
        ui_common_add_placeholder(scr, "Test unavailable: no timer");
        return scr;
    }
    lv_timer_pause(ui->timer);
    display_test_image_open(&ui->mapped);
    lv_obj_t *instructions = lv_label_create(body);
    lv_obj_set_width(instructions, LV_PCT(100));
    lv_obj_set_style_text_color(instructions, UI_COLOR_TEXT, 0);
    lv_label_set_text(instructions,
        "Red > Green > Blue > White > Grayscale > Image\n\n"
        "1 second per screen, repeat continuously.\n"
        "Landscape: vertical grayscale bands.\n"
        "Portrait: horizontal grayscale bands.\n\n"
        "Touch anywhere on the full-screen test to stop.");
    lv_obj_t *status = lv_label_create(body);
    lv_obj_set_width(status, LV_PCT(100));
    lv_obj_set_style_text_color(status, UI_COLOR_TEXT_MUTED, 0);
    lv_label_set_text(status, ui->mapped.pixels ? "Image: ready" : "Image: unavailable (skipped)");

    lv_obj_t *brightness = lv_obj_create(body);
    lv_obj_set_size(brightness, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(brightness, UI_COLOR_CARD, 0);
    lv_obj_set_style_border_color(brightness, UI_COLOR_BORDER, 0);
    lv_obj_set_style_pad_all(brightness, 16, 0);
    lv_obj_set_style_pad_row(brightness, 16, 0);
    lv_obj_set_flex_flow(brightness, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(brightness, LV_OBJ_FLAG_SCROLLABLE);
    ui->brightness_label = lv_label_create(brightness);
    lv_obj_set_style_text_color(ui->brightness_label, UI_COLOR_TEXT, 0);
    unsigned current_brightness = bsp_backlight_get_brightness();
    lv_label_set_text_fmt(ui->brightness_label, "Brightness: %u%%", current_brightness);
    lv_obj_t *slider = lv_slider_create(brightness);
    lv_obj_set_size(slider, LV_PCT(100), 10);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, current_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, UI_COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightness_cb, LV_EVENT_VALUE_CHANGED, ui);

    lv_obj_t *start = lv_button_create(body);
    lv_obj_set_size(start, 160, 48);
    lv_obj_set_style_bg_color(start, UI_COLOR_PRIMARY, 0);
    lv_obj_t *label = lv_label_create(start);
    lv_label_set_text(label, "Start Test");
    lv_obj_center(label);
    lv_obj_add_event_cb(start, start_cb, LV_EVENT_CLICKED, ui);

    lv_display_t *display = lv_obj_get_display(scr);
    ui->view = lv_obj_create(scr);
    lv_obj_remove_style_all(ui->view);
    lv_obj_set_size(ui->view, lv_display_get_horizontal_resolution(display),
                    lv_display_get_vertical_resolution(display));
    lv_obj_add_flag(ui->view, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui->view, draw_cb, LV_EVENT_DRAW_MAIN, ui);
    lv_obj_add_event_cb(ui->view, stop_cb, LV_EVENT_CLICKED, ui);
    ui->picture = lv_image_create(ui->view);
    lv_obj_clear_flag(ui->picture, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    if (ui->mapped.pixels != NULL) {
        ui->image.header.magic = LV_IMAGE_HEADER_MAGIC;
        ui->image.header.cf = ui->mapped.pixel_bytes == 2 ? LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB888;
        ui->image.header.w = ui->mapped.width;
        ui->image.header.h = ui->mapped.height;
        ui->image.header.stride = ui->image.header.w * ui->mapped.pixel_bytes;
        ui->image.data_size = ui->image.header.stride * ui->image.header.h;
        ui->image.data = ui->mapped.pixels;
        lv_image_set_src(ui->picture, &ui->image);
        lv_obj_set_size(ui->picture, lv_display_get_horizontal_resolution(display),
                        lv_display_get_vertical_resolution(display));
        lv_image_set_inner_align(ui->picture, LV_IMAGE_ALIGN_STRETCH);
    }
    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);
    return scr;
}
