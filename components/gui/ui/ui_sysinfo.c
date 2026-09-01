#include "ui_sysinfo.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "svc_sysinfo.h"

#include <stdio.h>

/* 系统信息页：显示板名 / 屏型号 / 芯片 / IDF 版本 / 分辨率 / 剩余堆。
 * UI 只调 svc_sysinfo_get()，不直接触碰 esp_* 系统 API。 */
lv_obj_t *ui_sysinfo_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("System Info", back_cb);

    sysinfo_t info;
    svc_sysinfo_get(&info);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Board: %s\n"
             "Panel: %s\n"
             "Chip: %s\n"
             "IDF: %s\n"
             "Firmware: %s\n"
             "Resolution: %ux%u\n"
             "Free heap: %lu KB",
             info.board, info.panel, info.chip, info.idf_ver, info.fw_ver,
             (unsigned)info.width, (unsigned)info.height,
             (unsigned long)(info.free_heap / 1024));

    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_line_space(lbl, 8, 0);
    /* 标题栏高 48，内容区从下方留边距开始 */
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 64);

    return scr;
}
