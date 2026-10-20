/**
 * @file ui_display_test.c
 * @brief 通过 LVGL 自动或手动显示测试图样，提供背光调节和全屏暂停控制。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-20
 */
#include "ui_display_test.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "display_test_resource.h"
#include "bsp.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "esp_log.h"
#if CONFIG_DISPLAY_INTERFACE_RGB
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#include <stdlib.h>
#include <string.h>

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *view;
    lv_obj_t *picture;
    lv_obj_t *brightness_label;
    lv_obj_t *controls;
    lv_obj_t *pattern_label;
    lv_timer_t *timer;
    display_test_image_t mapped;
    lv_image_dsc_t image;
    unsigned stage;
    bool running;
    bool paused;
    lv_opa_t system_layer_opa;
} display_test_ui_t;

/**
 * @brief 准备原尺寸图片；RGB 分块预装到 PSRAM，避免每次切图突发读取 Flash。
 *
 * RGB 预装期间每复制 4 KiB 让出一个 tick，为 bounce buffer 留出供数余量。
 * 分配失败时跳过图片，不退回 Flash 直接绘制；MIPI 保留原来的只读映射。
 * 成功映射保留至页面删除，避免预装后立即解除映射触发额外的缓存停用窗口。
 */
static void prepare_image(display_test_ui_t *ui)
{
    if (!display_test_image_open(&ui->mapped)) return;
    ui->image.header.magic = LV_IMAGE_HEADER_MAGIC;
    ui->image.header.cf = ui->mapped.pixel_bytes == 2 ? LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB888;
    ui->image.header.w = ui->mapped.width;
    ui->image.header.h = ui->mapped.height;
    ui->image.header.stride = ui->image.header.w * ui->mapped.pixel_bytes;
    ui->image.data_size = ui->image.header.stride * ui->image.header.h;
#if CONFIG_DISPLAY_INTERFACE_RGB
    uint8_t *pixels = heap_caps_malloc(ui->image.data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (pixels == NULL) {
        ESP_LOGW("LCD_TEST_UI", "Not enough PSRAM for test image (%u bytes); skip image",
                   (unsigned)ui->image.data_size);
        display_test_image_close(&ui->mapped);
        return;
    }
    const size_t chunk_size = 4096;
    for (size_t offset = 0; offset < ui->image.data_size; offset += chunk_size) {
        size_t length = ui->image.data_size - offset;
        if (length > chunk_size) length = chunk_size;
        memcpy(pixels + offset, ui->mapped.pixels + offset, length);
        vTaskDelay(1);
    }
    ui->image.data = pixels;
    ESP_LOGI("LCD_TEST_UI", "RGB image prepared in PSRAM (%u bytes)", (unsigned)ui->image.data_size);
#else
    ui->image.data = ui->mapped.pixels;
#endif
}

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

/** @brief 绘制纯色、方向自适应的条带或正方形棋盘格，不分配整帧画布。 */
static void draw_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    if (ui->stage == DISPLAY_TEST_IMAGE) return;
    lv_area_t area;
    lv_obj_get_coords(ui->view, &area);
    int32_t width = lv_area_get_width(&area);
    int32_t height = lv_area_get_height(&area);
    bool landscape = width >= height;
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_COVER;
    if (ui->stage == DISPLAY_TEST_CHECKERBOARD) {
        for (int32_t y = 0; y < height; y += DISPLAY_TEST_CHECKER_SIZE) {
            for (int32_t x = 0; x < width; x += DISPLAY_TEST_CHECKER_SIZE) {
                lv_area_t cell = {
                    .x1 = area.x1 + x,
                    .y1 = area.y1 + y,
                    .x2 = area.x1 + x + DISPLAY_TEST_CHECKER_SIZE - 1,
                    .y2 = area.y1 + y + DISPLAY_TEST_CHECKER_SIZE - 1,
                };
                if (cell.x2 > area.x2) cell.x2 = area.x2;
                if (cell.y2 > area.y2) cell.y2 = area.y2;
                unsigned index = x / DISPLAY_TEST_CHECKER_SIZE + y / DISPLAY_TEST_CHECKER_SIZE;
                rect.bg_color = lv_color_hex(display_test_pattern_color(ui->stage, index));
                lv_draw_rect(lv_event_get_layer(e), &rect, &cell);
            }
        }
        return;
    }
    unsigned bands = ui->stage == DISPLAY_TEST_GRAYSCALE ? 16 :
                     ui->stage == DISPLAY_TEST_COLOR_BARS ? DISPLAY_TEST_COLOR_BAR_COUNT : 1;
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
    unsigned count = ui->image.data ? DISPLAY_TEST_PATTERN_COUNT : DISPLAY_TEST_IMAGE;
    lv_label_set_text_fmt(ui->pattern_label, "%u/%u  %s | %s", ui->stage + 1, count,
                           display_test_pattern_names[ui->stage], ui->paused ? "Paused" : "Auto");
    lv_obj_invalidate(ui->view);
    ESP_LOGI("LCD_TEST_UI", "%s", display_test_pattern_names[ui->stage]);
}

static void next_cb(lv_timer_t *timer)
{
    display_test_ui_t *ui = lv_timer_get_user_data(timer);
    if (!ui->running || ui->paused) return;
    unsigned count = ui->image.data ? DISPLAY_TEST_PATTERN_COUNT : DISPLAY_TEST_IMAGE;
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
    ui->paused = false;
}

static void stop_cb(lv_event_t *e)
{
    stop_test(lv_event_get_user_data(e));
}

/** @brief 轻点画面暂停并切换控制栏；隐藏控制栏后仍保持手动停留。 */
static void controls_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    ui->paused = true;
    lv_timer_pause(ui->timer);
    if (lv_obj_has_flag(ui->controls, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    }
    show_stage(ui);
}

static void start_test(display_test_ui_t *ui, bool manual)
{
    if (ui->running) return;
    lv_obj_t *system = lv_display_get_layer_sys(lv_obj_get_display(ui->screen));
    /* 系统层没有父对象，当前 LVGL 不支持对其直接清除 HIDDEN 标志。 */
    ui->system_layer_opa = lv_obj_get_style_opa(system, LV_PART_MAIN);
    lv_obj_set_style_opa(system, LV_OPA_TRANSP, 0);
    ui->running = true;
    ui->paused = manual;
    ui->stage = DISPLAY_TEST_RED;
    if (manual) lv_obj_clear_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->view, LV_OBJ_FLAG_HIDDEN);
    show_stage(ui);
    lv_timer_reset(ui->timer);
    if (!manual) lv_timer_resume(ui->timer);
}

static void start_cb(lv_event_t *e)
{
    start_test(lv_event_get_user_data(e), false);
}

static void manual_cb(lv_event_t *e)
{
    start_test(lv_event_get_user_data(e), true);
}

/** @brief 手动切换时暂停计时，并按图片是否可用循环选择测试图样。 */
static void step_manual(display_test_ui_t *ui, int direction)
{
    if (!ui->running) return;
    ui->paused = true;
    lv_timer_pause(ui->timer);
    unsigned count = ui->image.data ? DISPLAY_TEST_PATTERN_COUNT : DISPLAY_TEST_IMAGE;
    ui->stage = (ui->stage + count + direction) % count;
    show_stage(ui);
}

static void previous_cb(lv_event_t *e)
{
    step_manual(lv_event_get_user_data(e), -1);
}

static void next_manual_cb(lv_event_t *e)
{
    step_manual(lv_event_get_user_data(e), 1);
}

static void play_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    ui->paused = false;
    lv_obj_add_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    show_stage(ui);
    lv_timer_reset(ui->timer);
    lv_timer_resume(ui->timer);
}

static void hide_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    lv_obj_add_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
}

static void control_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, display_test_ui_t *ui)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 0, 42);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_bg_color(button, UI_COLOR_PRIMARY, 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, ui);
}

/** @brief 页面删除时先停止刷新并撤销图片引用，再释放映射和上下文。 */
static void delete_cb(lv_event_t *e)
{
    display_test_ui_t *ui = lv_event_get_user_data(e);
    stop_test(ui);
    lv_timer_delete(ui->timer);
    lv_image_set_src(ui->picture, NULL);
    lv_image_cache_drop(&ui->image);
#if CONFIG_DISPLAY_INTERFACE_RGB
    heap_caps_free((void *)ui->image.data);
#endif
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
    prepare_image(ui);
    lv_obj_t *instructions = lv_label_create(body);
    lv_obj_set_width(instructions, LV_PCT(100));
    lv_obj_set_style_text_color(instructions, UI_COLOR_TEXT, 0);
    lv_label_set_text(instructions,
        "Red > Green > Blue > White > Grayscale > Color bars > Checkerboard > Image\n\n"
        "Auto: 1 second per screen, repeat continuously.\n"
        "Manual: switch and hold each pattern.\n"
        "Landscape: vertical bands. Portrait: horizontal bands.\n\n"
        "Tap the test picture to pause and show/hide controls.\n"
        "Play resumes auto; Stop returns to this page.");
    lv_obj_t *status = lv_label_create(body);
    lv_obj_set_width(status, LV_PCT(100));
    lv_obj_set_style_text_color(status, UI_COLOR_TEXT_MUTED, 0);
    lv_label_set_text(status, ui->image.data ? "Image: ready" : "Image: unavailable (skipped)");

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
    lv_obj_t *manual = lv_button_create(body);
    lv_obj_set_size(manual, 160, 48);
    lv_obj_set_style_bg_color(manual, UI_COLOR_PRIMARY, 0);
    label = lv_label_create(manual);
    lv_label_set_text(label, "Manual Test");
    lv_obj_center(label);
    lv_obj_add_event_cb(manual, manual_cb, LV_EVENT_CLICKED, ui);

    lv_display_t *display = lv_obj_get_display(scr);
    ui->view = lv_obj_create(scr);
    lv_obj_remove_style_all(ui->view);
    lv_obj_set_size(ui->view, lv_display_get_horizontal_resolution(display),
                    lv_display_get_vertical_resolution(display));
    lv_obj_add_flag(ui->view, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui->view, draw_cb, LV_EVENT_DRAW_MAIN, ui);
    lv_obj_add_event_cb(ui->view, controls_cb, LV_EVENT_CLICKED, ui);
    ui->picture = lv_image_create(ui->view);
    lv_obj_clear_flag(ui->picture, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    if (ui->image.data != NULL) {
        lv_image_set_src(ui->picture, &ui->image);
        lv_obj_set_size(ui->picture, lv_display_get_horizontal_resolution(display),
                        lv_display_get_vertical_resolution(display));
        lv_image_set_inner_align(ui->picture, LV_IMAGE_ALIGN_STRETCH);
    }

    ui->controls = lv_obj_create(ui->view);
    lv_obj_set_width(ui->controls, LV_PCT(94));
    lv_obj_set_style_max_width(ui->controls, 600, 0);
    lv_obj_set_height(ui->controls, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(ui->controls, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(ui->controls, LV_OPA_90, 0);
    lv_obj_set_style_pad_all(ui->controls, 12, 0);
    lv_obj_set_style_pad_row(ui->controls, 8, 0);
    lv_obj_set_flex_flow(ui->controls, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(ui->controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->controls, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
#if CONFIG_BSP_PANEL_RGB_H034A04
    lv_obj_set_width(ui->controls, LV_PCT(70));
    lv_obj_align(ui->controls, LV_ALIGN_BOTTOM_MID, 0, -lv_display_get_vertical_resolution(display) * 15 / 100);
#else
    lv_obj_align(ui->controls, LV_ALIGN_BOTTOM_MID, 0, -16);
#endif
    ui->pattern_label = lv_label_create(ui->controls);
    lv_obj_set_width(ui->pattern_label, LV_PCT(100));
    lv_obj_set_style_text_color(ui->pattern_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_align(ui->pattern_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *buttons = lv_obj_create(ui->controls);
    lv_obj_remove_style_all(buttons);
    lv_obj_set_size(buttons, LV_PCT(100), 42);
    lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(buttons, 6, 0);
    lv_obj_clear_flag(buttons, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    control_button(buttons, "Prev", previous_cb, ui);
    control_button(buttons, "Play", play_cb, ui);
    control_button(buttons, "Next", next_manual_cb, ui);
    control_button(buttons, "Hide", hide_cb, ui);
    control_button(buttons, "Stop", stop_cb, ui);
    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);
    return scr;
}
