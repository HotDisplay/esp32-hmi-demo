#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 最多列出的文件数 */
#define SVC_STORAGE_MAX_FILES  16
/* 单个文件名最大长度 */
#define SVC_STORAGE_NAME_MAX   32

/* SD 卡容量信息 */
typedef struct {
    bool     mounted;        /* 是否已挂载 */
    uint64_t total_bytes;    /* 总容量（字节） */
    uint64_t free_bytes;     /* 剩余容量（字节） */
} storage_info_t;

/* SD 卡根目录文件列表 */
typedef struct {
    int   count;                                    /* 文件/目录数量 */
    char  names[SVC_STORAGE_MAX_FILES][SVC_STORAGE_NAME_MAX];
} storage_filelist_t;

/* 首次进入 SD 页面时启动一个后台查询任务，后续调用复用该任务。 */
esp_err_t svc_storage_init(void);

/* 请求后台更新容量和根目录条目；不等待文件系统操作。 */
void svc_storage_request_refresh(void);

/* 复制同一次查询的缓存结果；首次查询完成前返回 false。 */
bool svc_storage_get_snapshot(storage_info_t *info, storage_filelist_t *files);

#ifdef __cplusplus
}
#endif
