#include "svc_sysinfo.h"

#include <string.h>

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_idf_version.h"
#include "bsp.h"

/* 固件版本：先硬编码，后续可换 esp_app_get_description() 或构建注入 */
#define APP_FW_VERSION  "0.1.0"

void svc_sysinfo_get(sysinfo_t *out)
{
    memset(out, 0, sizeof(*out));

    strncpy(out->board,   bsp_board_name(),        sizeof(out->board) - 1);
    strncpy(out->panel,   bsp_panel_name(),        sizeof(out->panel) - 1);
    strncpy(out->chip,    CONFIG_IDF_TARGET,       sizeof(out->chip) - 1);
    strncpy(out->idf_ver, esp_get_idf_version(),   sizeof(out->idf_ver) - 1);
    strncpy(out->fw_ver,  APP_FW_VERSION,          sizeof(out->fw_ver) - 1);

    out->width     = bsp_get_screen_width();
    out->height    = bsp_get_screen_height();
    out->free_heap = esp_get_free_heap_size();
}
