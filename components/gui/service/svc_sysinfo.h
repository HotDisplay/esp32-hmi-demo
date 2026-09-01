#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 系统信息（供系统信息页展示） */
typedef struct {
    char     board[32];      /* 板子名称 */
    char     panel[32];      /* 屏型号 */
    char     chip[16];       /* 芯片型号 */
    char     idf_ver[16];    /* ESP-IDF 版本 */
    char     fw_ver[16];     /* 固件版本 */
    uint16_t width;          /* 屏宽 */
    uint16_t height;         /* 屏高 */
    uint32_t free_heap;      /* 剩余堆（字节） */
} sysinfo_t;

/* 填充系统信息。UI 层只依赖本函数，不直接触碰 esp_* 系统 API。 */
void svc_sysinfo_get(sysinfo_t *out);

#ifdef __cplusplus
}
#endif
