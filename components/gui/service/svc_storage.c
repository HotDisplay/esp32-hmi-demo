#include "svc_storage.h"

#include <string.h>
#include <dirent.h>
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/* 与 components/sdcard 的 MOUNT_POINT 保持一致 */
#define SD_MOUNT_POINT  "/sdcard"

static TaskHandle_t s_query_task = NULL;
static SemaphoreHandle_t s_cache_mutex = NULL;
static storage_info_t s_cached_info;
static storage_filelist_t s_cached_files;
static bool s_cache_ready = false;

static void read_info(storage_info_t *out)
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

static void read_files(storage_filelist_t *out)
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
        strlcpy(out->names[out->count], ent->d_name, sizeof(out->names[0]));
        out->count++;
    }
    closedir(dir);
}

static void query_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        storage_info_t info;
        storage_filelist_t files = { 0 };
        read_info(&info);
        if (info.mounted) {
            read_files(&files);
        }

        /* 文件系统操作在锁外执行，UI 只等待短暂的缓存复制。 */
        xSemaphoreTake(s_cache_mutex, portMAX_DELAY);
        s_cached_info = info;
        s_cached_files = files;
        s_cache_ready = true;
        xSemaphoreGive(s_cache_mutex);
    }
}

esp_err_t svc_storage_init(void)
{
    if (s_query_task != NULL) {
        return ESP_OK;
    }

    s_cache_mutex = xSemaphoreCreateMutex();
    if (s_cache_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(query_task, "sd_query", 4096, NULL, 3, &s_query_task) != pdPASS) {
        vSemaphoreDelete(s_cache_mutex);
        s_cache_mutex = NULL;
        s_query_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void svc_storage_request_refresh(void)
{
    if (s_query_task != NULL) {
        xTaskNotifyGive(s_query_task);
    }
}

bool svc_storage_get_snapshot(storage_info_t *info, storage_filelist_t *files)
{
    memset(info, 0, sizeof(*info));
    memset(files, 0, sizeof(*files));
    if (s_cache_mutex == NULL) {
        return false;
    }

    xSemaphoreTake(s_cache_mutex, portMAX_DELAY);
    *info = s_cached_info;
    *files = s_cached_files;
    bool ready = s_cache_ready;
    xSemaphoreGive(s_cache_mutex);
    return ready;
}
