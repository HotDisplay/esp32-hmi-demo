#include "svc_storage.h"

#include <string.h>
#include <dirent.h>
#include "esp_vfs_fat.h"

/* 与 components/sdcard 的 MOUNT_POINT 保持一致 */
#define SD_MOUNT_POINT  "/sdcard"

void svc_storage_get_info(storage_info_t *out)
{
    memset(out, 0, sizeof(*out));

    uint64_t total = 0, free = 0;
    /* ESP-IDF 的 VFS 层不支持 statvfs，容量要用 fatfs 官方接口 esp_vfs_fat_info */
    if (esp_vfs_fat_info(SD_MOUNT_POINT, &total, &free) != ESP_OK) {
        /* 未挂载（SD 卡未插入或挂载失败） */
        out->mounted = false;
        return;
    }

    out->mounted     = true;
    out->total_bytes = total;
    out->free_bytes  = free;
}

void svc_storage_list_files(storage_filelist_t *out)
{
    memset(out, 0, sizeof(*out));

    DIR *dir = opendir(SD_MOUNT_POINT);
    if (dir == NULL) {
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && out->count < SVC_STORAGE_MAX_FILES) {
        if (ent->d_name[0] == '.') {
            continue;   /* 跳过 .  ..  以及隐藏文件 */
        }
        strncpy(out->names[out->count], ent->d_name, SVC_STORAGE_NAME_MAX - 1);
        out->count++;
    }
    closedir(dir);
}
