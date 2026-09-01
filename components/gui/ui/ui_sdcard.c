#include "ui_sdcard.h"
#include "ui_common.h"

/* SD 卡页：后续接入容量 / 文件列表 / 读写测速 */
lv_obj_t *ui_sdcard_create(lv_event_cb_t back_cb)
{
    lv_obj_t *scr = ui_common_create_screen("SD Card", back_cb);
    ui_common_add_placeholder(scr, "SD card demo: under construction");
    return scr;
}
