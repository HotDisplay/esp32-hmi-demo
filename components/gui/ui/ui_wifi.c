#include "ui_wifi.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "svc_wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    lv_obj_t *body;
    lv_obj_t *status;
    lv_obj_t *notice;
    lv_obj_t *networks;
    lv_obj_t *ssid;
    lv_obj_t *password;
    lv_obj_t *keyboard;
    lv_obj_t *scan;
    lv_obj_t *connect;
    lv_obj_t *disconnect;
    lv_timer_t *timer;
    svc_wifi_ap_t aps[SVC_WIFI_MAX_APS];
    uint8_t ap_count;
    uint32_t generation;
    int32_t body_height;
} wifi_ui_t;

static void hide_keyboard(wifi_ui_t *ui)
{
    lv_keyboard_set_textarea(ui->keyboard, NULL);
    lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(ui->body, ui->body_height);
}

static void input_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_CLICKED) return;
    wifi_ui_t *ui = lv_event_get_user_data(e);
    lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(ui->body, ui->body_height - lv_obj_get_height(ui->keyboard));
    lv_keyboard_set_textarea(ui->keyboard, lv_event_get_target(e));
    lv_obj_update_layout(ui->body);
    lv_obj_scroll_to_view_recursive(lv_event_get_target(e), LV_ANIM_OFF);
}

static void keyboard_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) hide_keyboard(lv_event_get_user_data(e));
}

static void select_cb(lv_event_t *e)
{
    wifi_ui_t *ui = lv_event_get_user_data(e);
    unsigned selected = lv_dropdown_get_selected(ui->networks);
    if (selected >= ui->ap_count) return;
    hide_keyboard(ui);
    lv_textarea_set_text(ui->ssid, ui->aps[selected].ssid);
    lv_textarea_set_text(ui->password, "");
    lv_label_set_text(ui->notice, !ui->aps[selected].supported ? "This security type is not supported." :
                     ui->aps[selected].password_required ? "Enter the network password." : "Open network: no password required.");
}

static void action_cb(lv_event_t *e)
{
    wifi_ui_t *ui = lv_event_get_user_data(e);
    lv_obj_t *button = lv_event_get_target(e);
    esp_err_t err;
    hide_keyboard(ui);
    if (button == ui->scan) {
        err = svc_wifi_scan();
    } else if (button == ui->disconnect) {
        err = svc_wifi_disconnect();
    } else {
        const char *ssid = lv_textarea_get_text(ui->ssid);
        for (unsigned i = 0; i < ui->ap_count; i++) {
            if (strcmp(ssid, ui->aps[i].ssid) == 0 && !ui->aps[i].supported) {
                lv_label_set_text(ui->notice, "Use an open, WPA2 or WPA3 personal network.");
                return;
            }
        }
        err = svc_wifi_connect(ssid, lv_textarea_get_text(ui->password));
        if (err == ESP_OK) lv_textarea_set_text(ui->password, "");
    }
    lv_label_set_text(ui->notice, err == ESP_OK ? "" : err == ESP_ERR_INVALID_ARG ?
                     "SSID: 1-32 bytes. Password: 8-63 bytes, or 64 hex digits; empty for open Wi-Fi." :
                     err == ESP_ERR_INVALID_STATE ? "Wait for this operation, or disconnect before changing networks." :
                     "Unable to start operation. Please retry.");
}

static void refresh_cb(lv_timer_t *timer)
{
    wifi_ui_t *ui = lv_timer_get_user_data(timer);
    svc_wifi_status_t status;
    svc_wifi_get_status(&status);
    static const char *states[] = { "Off", "Starting / waiting for C6", "Ready", "Scanning",
                                   "Connecting / obtaining IP", "Connected", "Error", "Disconnecting" };
    char text[192];
    if (status.state == SVC_WIFI_ONLINE) {
        snprintf(text, sizeof(text), "%s  |  %d dBm\nIP: %s   Gateway: %s",
                 status.ssid, status.rssi, status.ip, status.gateway);
    } else if (status.state == SVC_WIFI_ERROR) {
        if (status.error == ESP_ERR_TIMEOUT) {
            snprintf(text, sizeof(text), "Timed out. Check C6 SDIO firmware or network availability.");
        } else if (status.disconnect_reason) {
            snprintf(text, sizeof(text), "Connection failed (reason %u). Check the network and password.",
                     status.disconnect_reason);
        } else {
            snprintf(text, sizeof(text), "Wi-Fi unavailable (%s). Check C6 firmware / SDIO, then Scan.",
                     esp_err_to_name(status.error));
        }
    } else {
        snprintf(text, sizeof(text), "%s%s%s", states[status.state], status.ssid[0] ? "  |  " : "", status.ssid);
    }
    if (strcmp(lv_label_get_text(ui->status), text) != 0) lv_label_set_text(ui->status, text);
    bool busy = status.pending || status.state == SVC_WIFI_STARTING ||
                status.state == SVC_WIFI_SCANNING || status.state == SVC_WIFI_CONNECTING || status.state == SVC_WIFI_DISCONNECTING;
    if (busy || status.connected) lv_obj_add_state(ui->scan, LV_STATE_DISABLED);
    else lv_obj_remove_state(ui->scan, LV_STATE_DISABLED);
    if (busy || status.connected) lv_obj_add_state(ui->connect, LV_STATE_DISABLED);
    else lv_obj_remove_state(ui->connect, LV_STATE_DISABLED);
    if (status.state != SVC_WIFI_SCANNING && status.state != SVC_WIFI_DISCONNECTING && (status.connected || status.state == SVC_WIFI_CONNECTING))
        lv_obj_remove_state(ui->disconnect, LV_STATE_DISABLED);
    else lv_obj_add_state(ui->disconnect, LV_STATE_DISABLED);

    if (ui->generation == status.scan_generation) return;
    ui->generation = status.scan_generation;
    ui->ap_count = status.ap_count;
    memcpy(ui->aps, status.aps, sizeof(ui->aps));
    char options[SVC_WIFI_MAX_APS * 80];
    size_t used = 0;
    options[0] = 0;
    for (unsigned i = 0; i < ui->ap_count; i++) {
        char ssid[33];
        memcpy(ssid, ui->aps[i].ssid, sizeof(ssid));
        /* Dropdown uses newline as a separator; keep the original SSID for connecting. */
        for (unsigned j = 0; ssid[j]; j++) if ((unsigned char)ssid[j] < 32) ssid[j] = ' ';
        int length = snprintf(options + used, sizeof(options) - used, "%s%s  (%d dBm, %s)",
                              i ? "\n" : "", ssid, ui->aps[i].rssi,
                              !ui->aps[i].supported ? "unsupported" : ui->aps[i].password_required ? "secure" : "open");
        if (length < 0 || (size_t)length >= sizeof(options) - used) break;
        used += (size_t)length;
    }
    lv_dropdown_set_options(ui->networks, ui->ap_count ? options : "No networks found");
    if (ui->ap_count) lv_obj_remove_state(ui->networks, LV_STATE_DISABLED);
    else lv_obj_add_state(ui->networks, LV_STATE_DISABLED);
    /* Keep manually entered SSID and password when a scan completes. */
}

static lv_obj_t *add_button(wifi_ui_t *ui, lv_obj_t *parent, const char *text)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_width(button, 1);
    lv_obj_set_height(button, 38);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_bg_color(button, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_color(button, UI_COLOR_CARD, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(button, UI_COLOR_TEXT_MUTED, LV_STATE_DISABLED);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, action_cb, LV_EVENT_CLICKED, ui);
    return button;
}

static void delete_cb(lv_event_t *e)
{
    wifi_ui_t *ui = lv_event_get_user_data(e);
    lv_timer_delete(ui->timer);
    lv_keyboard_set_textarea(ui->keyboard, NULL);
    lv_obj_remove_event_cb_with_user_data(ui->ssid, input_cb, ui);
    lv_obj_remove_event_cb_with_user_data(ui->password, input_cb, ui);
    free(ui);
}

lv_obj_t *ui_wifi_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("Wi-Fi", back_cb);
    esp_err_t err = svc_wifi_init();
    if (err != ESP_OK) {
        ui_common_add_placeholder(scr, err == ESP_ERR_NOT_SUPPORTED ? "Wi-Fi is disabled on this board" : "Wi-Fi unavailable: not enough resources");
        return scr;
    }
    wifi_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        ui_common_add_placeholder(scr, "Wi-Fi unavailable: not enough memory");
        return scr;
    }
    int32_t height = lv_display_get_vertical_resolution(lv_obj_get_display(scr));
    ui->body_height = height - 64;
    ui->body = lv_obj_create(scr);
    lv_obj_set_pos(ui->body, 12, 56);
    lv_obj_set_size(ui->body, lv_pct(100), ui->body_height);
    lv_obj_set_width(ui->body, lv_display_get_horizontal_resolution(lv_obj_get_display(scr)) - 24);
    lv_obj_set_style_bg_color(ui->body, UI_COLOR_CARD, 0);
    lv_obj_set_flex_flow(ui->body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ui->body, 10, 0);

    ui->status = lv_label_create(ui->body);
    lv_obj_set_width(ui->status, lv_pct(100));
    lv_obj_set_style_text_color(ui->status, UI_COLOR_TEXT, 0);
    ui->networks = lv_dropdown_create(ui->body);
    lv_obj_set_width(ui->networks, lv_pct(100));
    lv_dropdown_set_options(ui->networks, "Tap Scan to find networks");
    lv_obj_add_state(ui->networks, LV_STATE_DISABLED);
    lv_obj_add_event_cb(ui->networks, select_cb, LV_EVENT_VALUE_CHANGED, ui);

    ui->ssid = lv_textarea_create(ui->body);
    lv_textarea_set_one_line(ui->ssid, true);
    lv_textarea_set_max_length(ui->ssid, 32);
    lv_textarea_set_placeholder_text(ui->ssid, "SSID (select above or enter manually)");
    lv_obj_set_width(ui->ssid, lv_pct(100));
    lv_obj_add_event_cb(ui->ssid, input_cb, LV_EVENT_FOCUSED, ui);
    lv_obj_add_event_cb(ui->ssid, input_cb, LV_EVENT_CLICKED, ui);
    ui->password = lv_textarea_create(ui->body);
    lv_textarea_set_one_line(ui->password, true);
    lv_textarea_set_max_length(ui->password, 64);
    lv_textarea_set_password_mode(ui->password, true);
    lv_textarea_set_placeholder_text(ui->password, "Password (empty for open Wi-Fi)");
    lv_obj_set_width(ui->password, lv_pct(100));
    lv_obj_add_event_cb(ui->password, input_cb, LV_EVENT_FOCUSED, ui);
    lv_obj_add_event_cb(ui->password, input_cb, LV_EVENT_CLICKED, ui);

    lv_obj_t *actions = lv_obj_create(ui->body);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, lv_pct(100), 38);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 8, 0);
    ui->scan = add_button(ui, actions, "Scan");
    ui->connect = add_button(ui, actions, "Connect");
    ui->disconnect = add_button(ui, actions, "Disconnect");
    ui->notice = lv_label_create(ui->body);
    lv_obj_set_width(ui->notice, lv_pct(100));
    lv_obj_set_style_text_color(ui->notice, UI_COLOR_TEXT_MUTED, 0);

    ui->keyboard = lv_keyboard_create(scr);
    lv_obj_set_size(ui->keyboard, lv_pct(100), height * 2 / 5);
    lv_obj_align(ui->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ui->keyboard, keyboard_cb, LV_EVENT_ALL, ui);
    ui->timer = lv_timer_create(refresh_cb, 500, ui);
    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);
    refresh_cb(ui->timer);
    return scr;
}
