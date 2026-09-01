#pragma once

#include <stdbool.h>
#include <stdint.h>

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

/* 查询 SD 卡容量信息。UI 层只依赖本函数，不直接碰 VFS / fatfs。 */
void svc_storage_get_info(storage_info_t *out);

/* 列出 /sdcard 根目录条目（最多 SVC_STORAGE_MAX_FILES 个，跳过 . 开头项）。 */
void svc_storage_list_files(storage_filelist_t *out);

#ifdef __cplusplus
}
#endif
