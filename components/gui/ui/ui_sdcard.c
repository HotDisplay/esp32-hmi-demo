#include "ui_sdcard.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "svc_storage.h"

#include <stdio.h>

/* 定时刷新回调：读容量 + 文件列表并更新 label */
static void refresh_cb(lv_timer_t *timer)
{
    lv_obj_t *lbl = (lv_obj_t *)lv_timer_get_user_data(timer);

    storage_info_t info;
    svc_storage_get_info(&info);

    char buf[512];
    if (!info.mounted) {
        snprintf(buf, sizeof(buf), "SD Card: Not mounted\n\nInsert SD card");
    } else {
        int off = snprintf(buf, sizeof(buf),
                           "SD Card: Mounted\n"
                           "Total: %llu MB   Free: %llu MB\n\n"
                           "Files:\n",
                           (unsigned long long)(info.total_bytes / 1024 / 1024),
                           (unsigned long long)(info.free_bytes / 1024 / 1024));

        storage_filelist_t files;
        svc_storage_list_files(&files);
        if (files.count == 0) {
            off += snprintf(buf + off, sizeof(buf) - off, "  (empty)");
        }
        for (int i = 0; i < files.count && off < (int)sizeof(buf); i++) {
            off += snprintf(buf + off, sizeof(buf) - off, "  %s\n", files.names[i]);
        }
    }
    lv_label_set_text(lbl, buf);
}

lv_obj_t *ui_sdcard_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("SD Card", back_cb);

    lv_obj_t *lbl = lv_label_create(scr);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_line_space(lbl, 4, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 64);

    /* 每 1 秒刷新。SD 卡挂载在 lvgl 之后才执行，故首帧可能未挂载，靠定时刷新更新。 */
    lv_timer_t *timer = lv_timer_create(refresh_cb, 1000, lbl);
    refresh_cb(timer);   /* 立即刷新一次 */

    return scr;
}
