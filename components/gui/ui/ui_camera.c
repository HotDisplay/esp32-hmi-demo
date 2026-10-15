#include "ui_camera.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "svc_camera.h"
#include "src/misc/cache/instance/lv_image_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    camera_session_t camera;
    camera_frame_t frame;
    lv_image_dsc_t image;
    lv_obj_t *preview;
    lv_obj_t *status;
    lv_obj_t *pause;
    lv_obj_t *pause_label;
    lv_obj_t *restart;
    lv_timer_t *timer;
    uint16_t max_width;
    uint16_t max_height;
    bool paused;
    esp_err_t start_error;
} camera_ui_t;

static void close_camera(camera_ui_t *ui)
{
    lv_image_cache_drop(&ui->image);
    lv_image_set_src(ui->preview, NULL);
    svc_camera_release_frame(ui->camera, &ui->frame);
    memset(&ui->frame, 0, sizeof(ui->frame));
    svc_camera_stop(ui->camera);
    ui->camera = NULL;
}

static void start_camera(camera_ui_t *ui)
{
    ui->paused = false;
    lv_label_set_text(ui->pause_label, "Pause");
    lv_obj_add_state(ui->pause, LV_STATE_DISABLED);
    ui->start_error = svc_camera_start(ui->max_width, ui->max_height, &ui->camera);
    if (ui->start_error == ESP_ERR_NOT_SUPPORTED) {
        lv_label_set_text(ui->status, "CSI camera disabled or unavailable on this target");
        lv_obj_add_state(ui->restart, LV_STATE_DISABLED);
    } else if (ui->start_error == ESP_ERR_NO_MEM) {
        lv_label_set_text(ui->status, "Camera unavailable: not enough memory");
    } else {
        lv_label_set_text(ui->status, "Starting CSI camera ...");
    }
}

static void refresh_cb(lv_timer_t *timer)
{
    camera_ui_t *ui = lv_timer_get_user_data(timer);
    if (ui->camera == NULL) {
        if (ui->start_error == ESP_ERR_INVALID_STATE) start_camera(ui);
        return;
    }
    camera_status_t status;
    if (!svc_camera_get_status(ui->camera, &status)) return;
    char text[160];
    if (status.error != ESP_OK) {
        const char *message = status.error == ESP_ERR_NOT_FOUND ? "Camera not detected. Check cable / power." :
                              status.error == ESP_ERR_TIMEOUT ? "No camera frames. Check CSI cable / power." :
                              status.error == ESP_ERR_NO_MEM ? "Camera unavailable: not enough memory." :
                              "Camera failed. Check console logs, then Restart.";
        snprintf(text, sizeof(text), "%s", message);
        lv_obj_add_state(ui->pause, LV_STATE_DISABLED);
    } else if (!status.running) {
        snprintf(text, sizeof(text), "Starting CSI camera ...");
    } else {
        snprintf(text, sizeof(text), "%s  %ux%u  |  Preview %ux%u  |  %s",
                 status.sensor, status.sensor_width, status.sensor_height,
                 status.preview_width, status.preview_height, ui->paused ? "Paused" : "Live");
        size_t used = strlen(text);
        if (!ui->paused) snprintf(text + used, sizeof(text) - used, "  %u FPS", status.preview_fps);
        lv_obj_clear_state(ui->pause, LV_STATE_DISABLED);
    }
    if (strcmp(lv_label_get_text(ui->status), text) != 0) lv_label_set_text(ui->status, text);

    camera_frame_t next;
    if (!svc_camera_take_frame(ui->camera, &next)) return;
    if (ui->paused) {
        svc_camera_release_frame(ui->camera, &next);
        return;
    }
    lv_image_cache_drop(&ui->image);
    ui->image.header.magic = LV_IMAGE_HEADER_MAGIC;
    ui->image.header.cf = LV_COLOR_FORMAT_RGB565;
    ui->image.header.w = next.width;
    ui->image.header.h = next.height;
    ui->image.header.stride = next.width * 2;
    ui->image.data_size = (size_t)next.width * next.height * 2;
    ui->image.data = next.pixels;
    lv_image_set_src(ui->preview, &ui->image);
    lv_obj_center(ui->preview);
    /* 新帧已经替换图像源，旧帧才交还采集任务。 */
    svc_camera_release_frame(ui->camera, &ui->frame);
    ui->frame = next;
}

static void pause_cb(lv_event_t *e)
{
    camera_ui_t *ui = lv_event_get_user_data(e);
    ui->paused = !ui->paused;
    svc_camera_pause(ui->camera, ui->paused);
    lv_label_set_text(ui->pause_label, ui->paused ? "Resume" : "Pause");
}

static void restart_cb(lv_event_t *e)
{
    camera_ui_t *ui = lv_event_get_user_data(e);
    close_camera(ui);
    start_camera(ui);
}

static void delete_cb(lv_event_t *e)
{
    camera_ui_t *ui = lv_event_get_user_data(e);
    lv_timer_delete(ui->timer);
    close_camera(ui);
    free(ui);
}

lv_obj_t *ui_camera_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("CSI Camera", back_cb);
    lv_display_t *display = lv_obj_get_display(scr);
    int32_t width = lv_display_get_horizontal_resolution(display) - 32;
    int32_t height = lv_display_get_vertical_resolution(display) - 160;
    if (width <= 0 || height <= 0) {
        ui_common_add_placeholder(scr, "Screen too small for camera preview");
        return scr;
    }
    camera_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        ui_common_add_placeholder(scr, "Camera unavailable: not enough memory");
        return scr;
    }
    ui->max_width = width;
    ui->max_height = height;
    ui->status = lv_label_create(scr);
    lv_obj_set_pos(ui->status, 16, 56);
    lv_obj_set_width(ui->status, width);
    lv_obj_set_style_text_color(ui->status, UI_COLOR_TEXT, 0);

    lv_obj_t *viewport = lv_obj_create(scr);
    lv_obj_remove_style_all(viewport);
    lv_obj_set_pos(viewport, 16, 100);
    lv_obj_set_size(viewport, width, height);
    lv_obj_set_style_bg_color(viewport, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_COVER, 0);
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    ui->preview = lv_image_create(viewport);

    ui->pause = lv_button_create(scr);
    lv_obj_set_size(ui->pause, 100, 36);
    lv_obj_align(ui->pause, LV_ALIGN_BOTTOM_MID, -58, -10);
    lv_obj_set_style_bg_color(ui->pause, UI_COLOR_PRIMARY, 0);
    ui->pause_label = lv_label_create(ui->pause);
    lv_label_set_text(ui->pause_label, "Pause");
    lv_obj_center(ui->pause_label);
    lv_obj_add_event_cb(ui->pause, pause_cb, LV_EVENT_CLICKED, ui);

    ui->restart = lv_button_create(scr);
    lv_obj_set_size(ui->restart, 100, 36);
    lv_obj_align(ui->restart, LV_ALIGN_BOTTOM_MID, 58, -10);
    lv_obj_set_style_bg_color(ui->restart, UI_COLOR_CARD, 0);
    lv_obj_t *label = lv_label_create(ui->restart);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);
    lv_obj_add_event_cb(ui->restart, restart_cb, LV_EVENT_CLICKED, ui);

    ui->timer = lv_timer_create(refresh_cb, 33, ui);
    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);
    start_camera(ui);
    return scr;
}
