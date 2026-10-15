#include "svc_camera.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#if CONFIG_GUI_CSI_CAMERA

#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include "bsp.h"
#include "driver/i2c_master.h"
#if CONFIG_GUI_CSI_CAMERA_USE_PPA
#include "driver/ppa.h"
#endif
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "esp_video_ioctl.h"
#include "sc2336.h"
#include "ov5647.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define CAMERA_BUFFERS       3
#define PREVIEW_INTERVAL_US  (1000000 / 15)
#define PREVIEW_SCALE_ROWS   16
#define VIDEO_FLAGS          (ESP_VIDEO_INIT_FLAGS_MIPI_CSI | ESP_VIDEO_INIT_FLAGS_ISP)

struct camera_session {
    SemaphoreHandle_t mutex;
    QueueHandle_t free_frames;
    QueueHandle_t ready_frames;
    atomic_bool stop;
    atomic_bool paused;
    uint16_t max_width;
    uint16_t max_height;
    camera_status_t status;
    void *preview[CAMERA_BUFFERS];
};

static atomic_bool s_busy;
static const char *TAG = "CSI_CAMERA";

static void set_status(camera_session_t session, const camera_status_t *status)
{
    xSemaphoreTake(session->mutex, portMAX_DELAY);
    session->status = *status;
    xSemaphoreGive(session->mutex);
}

static void destroy_session(camera_session_t session)
{
    for (unsigned i = 0; i < CAMERA_BUFFERS; i++) {
        free(session->preview[i]);
    }
    if (session->free_frames) vQueueDelete(session->free_frames);
    if (session->ready_frames) vQueueDelete(session->ready_frames);
    if (session->mutex) vSemaphoreDelete(session->mutex);
    free(session);
    atomic_store(&s_busy, false);
}

static esp_err_t capture(camera_session_t session)
{
    esp_err_t err = ESP_FAIL;
    int fd = -1;
    bool video_initialized = false, streaming = false;
    bool own_i2c = false;
    i2c_master_bus_handle_t bus = NULL;
#if CONFIG_GUI_CSI_CAMERA_USE_PPA
    ppa_client_handle_t ppa = NULL;
#else
    uint16_t *source_columns = NULL;
#endif
    void *capture_buffers[CAMERA_BUFFERS] = { 0 };
    size_t capture_sizes[CAMERA_BUFFERS] = { 0 };
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_status_t status = { 0 };

    /* 借用触摸的总线；触摸关闭时才创建并负责删除 I2C0。 */
    err = i2c_master_get_bus_handle(I2C_NUM_0, &bus);
    if (err == ESP_ERR_INVALID_STATE) {
        const i2c_master_bus_config_t cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = BSP_I2C0_SDA,
            .scl_io_num = BSP_I2C0_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
        };
        err = i2c_new_master_bus(&cfg, &bus);
        own_i2c = err == ESP_OK;
    }
    if (err != ESP_OK) goto cleanup;

    const esp_video_init_csi_config_t csi = {
        .sccb_config = {
            .init_sccb = false,
            .i2c_handle = bus,
            .freq = 400000,
        },
        .reset_pin = GPIO_NUM_NC,
        .pwdn_pin = GPIO_NUM_NC,
    };
    const esp_video_init_config_t video_cfg = { .csi = &csi };
    err = esp_video_init_with_flags(&video_cfg, VIDEO_FLAGS);
    if (err != ESP_OK) goto cleanup;
    video_initialized = true;
    if (atomic_load(&session->stop)) goto cleanup;

    err = ESP_FAIL;
    fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        err = ESP_ERR_NOT_FOUND;
        goto cleanup;
    }

    esp_cam_sensor_id_t id = { 0 };
    struct v4l2_ext_control control = {
        .id = ESP_CAM_SENSOR_IOC_G_CHIP_ID,
        .size = sizeof(id),
        .p_u8 = (uint8_t *)&id,
    };
    struct v4l2_ext_controls controls = {
        .ctrl_class = V4L2_CTRL_CLASS_ESP_CAM_IOCTL,
        .count = 1,
        .controls = &control,
    };
    if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &controls) != 0) goto cleanup;
    snprintf(status.sensor, sizeof(status.sensor), "%s",
             id.pid == SC2336_PID ? "SC2336" : id.pid == OV5647_PID ? "OV5647" : "CSI sensor");

    struct v4l2_format format = { .type = type };
    if (ioctl(fd, VIDIOC_G_FMT, &format) != 0) goto cleanup;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    if (ioctl(fd, VIDIOC_S_FMT, &format) != 0) goto cleanup;
    status.sensor_width = format.fmt.pix.width;
    status.sensor_height = format.fmt.pix.height;
    if (!status.sensor_width || !status.sensor_height ||
        format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) goto cleanup;

    /* 沿用 PPA 的 1/16 缩放精度，保持比例且不超出预览区。 */
    unsigned scale_steps = session->max_width * 16U / status.sensor_width;
    unsigned height_steps = session->max_height * 16U / status.sensor_height;
    if (height_steps < scale_steps) scale_steps = height_steps;
    if (scale_steps > 16) scale_steps = 16;
    if (scale_steps == 0) {
        err = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }
    status.preview_width = status.sensor_width * scale_steps / 16;
    status.preview_height = status.sensor_height * scale_steps / 16;
    size_t preview_size = ((size_t)status.preview_width * status.preview_height * 2 + 63) & ~(size_t)63;
    for (uint8_t i = 0; i < CAMERA_BUFFERS; i++) {
        session->preview[i] = heap_caps_aligned_alloc(64, preview_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!session->preview[i]) {
            err = ESP_ERR_NO_MEM;
            goto cleanup;
        }
        xQueueSend(session->free_frames, &i, 0);
    }
#if CONFIG_GUI_CSI_CAMERA_USE_PPA
    float scale = scale_steps / 16.0f;
    const char *scaling_engine = "PPA";
    const ppa_client_config_t ppa_cfg = { .oper_type = PPA_OPERATION_SRM };
    err = ppa_register_client(&ppa_cfg, &ppa);
    if (err != ESP_OK) goto cleanup;
#else
    const char *scaling_engine = "CPU";
    source_columns = malloc(status.preview_width * sizeof(*source_columns));
    if (!source_columns) {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    for (unsigned x = 0; x < status.preview_width; x++) source_columns[x] = x * 16U / scale_steps;
#endif

    struct v4l2_requestbuffers request = {
        .count = CAMERA_BUFFERS,
        .type = type,
        .memory = V4L2_MEMORY_MMAP,
    };
    err = ESP_FAIL;
    if (ioctl(fd, VIDIOC_REQBUFS, &request) != 0 || request.count != CAMERA_BUFFERS) goto cleanup;
    for (unsigned i = 0; i < CAMERA_BUFFERS; i++) {
        struct v4l2_buffer buffer = { .index = i, .type = type, .memory = V4L2_MEMORY_MMAP };
        if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) != 0) goto cleanup;
        capture_buffers[i] = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
        if (capture_buffers[i] == MAP_FAILED) {
            capture_buffers[i] = NULL;
            goto cleanup;
        }
        capture_sizes[i] = buffer.length;
        if (ioctl(fd, VIDIOC_QBUF, &buffer) != 0) goto cleanup;
    }
    struct timeval timeout = { .tv_usec = 100000 };
    if (ioctl(fd, VIDIOC_S_DQBUF_TIMEOUT, &timeout) != 0) goto cleanup;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) goto cleanup;
    streaming = true;
    status.running = true;
    set_status(session, &status);
    ESP_LOGI(TAG, "%s %ux%u -> %ux%u RGB565, %s scaling, preview up to 15 FPS",
             status.sensor, status.sensor_width, status.sensor_height, status.preview_width, status.preview_height, scaling_engine);

    int64_t last_preview = 0, fps_start = esp_timer_get_time(), last_capture = fps_start;
    unsigned frames = 0;
    while (!atomic_load(&session->stop)) {
        struct v4l2_buffer buffer = { .type = type, .memory = V4L2_MEMORY_MMAP };
        if (ioctl(fd, VIDIOC_DQBUF, &buffer) != 0) {
            if (errno == ETIMEDOUT && esp_timer_get_time() - last_capture < 2000000) continue;
            err = errno == ETIMEDOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
            goto cleanup;
        }
        last_capture = esp_timer_get_time();
        if (buffer.index >= CAMERA_BUFFERS || buffer.bytesused <
            (size_t)status.sensor_width * status.sensor_height * 2) {
            err = ESP_ERR_INVALID_SIZE;
            goto cleanup;
        }
        uint8_t index;
        if (!atomic_load(&session->paused) && last_capture - last_preview >= PREVIEW_INTERVAL_US &&
            xQueueReceive(session->free_frames, &index, 0) == pdTRUE) {
#if CONFIG_GUI_CSI_CAMERA_USE_PPA
            ppa_srm_oper_config_t operation = {
                .in = {
                    .buffer = capture_buffers[buffer.index],
                    .pic_w = status.sensor_width, .pic_h = status.sensor_height,
                    .block_w = status.sensor_width,
                    .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
                },
                .out = {
                    .buffer = session->preview[index], .buffer_size = preview_size,
                    .pic_w = status.preview_width, .pic_h = status.preview_height,
                    .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
                },
                .scale_x = scale, .scale_y = scale,
                .mode = PPA_TRANS_MODE_BLOCKING,
            };
            /* PPA 会在临界区同步缓存。分成 16 行，避免整帧同步阻塞 RGB 供数中断。
             * 缩放比例的小数精度同为 1/16，条带起点保持相同的采样相位。 */
            for (unsigned row = 0; row < status.sensor_height; row += PREVIEW_SCALE_ROWS) {
                if (atomic_load(&session->stop)) {
                    err = ESP_OK;
                    goto cleanup;
                }
                unsigned remaining = status.sensor_height - row;
                operation.in.block_h = remaining < PREVIEW_SCALE_ROWS ? remaining : PREVIEW_SCALE_ROWS;
                if (operation.in.block_h * scale_steps / 16 == 0) break;
                operation.in.block_offset_y = row;
                operation.out.block_offset_y = row * scale_steps / 16;
                err = ppa_do_scale_rotate_mirror(ppa, &operation);
                if (err != ESP_OK) goto cleanup;
            }
#else
            const uint16_t *source = capture_buffers[buffer.index];
            uint16_t *destination = session->preview[index];
            for (unsigned row = 0; row < status.preview_height; row++) {
                if (atomic_load(&session->stop)) {
                    err = ESP_OK;
                    goto cleanup;
                }
                const uint16_t *source_row = source + (size_t)(row * 16U / scale_steps) * status.sensor_width;
                uint16_t *destination_row = destination + (size_t)row * status.preview_width;
                for (unsigned x = 0; x < status.preview_width; x++) destination_row[x] = source_row[source_columns[x]];
                if ((row + 1) % 32 == 0) vTaskDelay(1);
            }
#endif
            /* 整帧完成后才交给 GUI，避免显示尚未写完的帧。 */
            camera_frame_t frame = { session->preview[index], status.preview_width, status.preview_height, index };
            if (xQueueSend(session->ready_frames, &frame, 0) != pdTRUE) {
                xQueueSend(session->free_frames, &index, 0);
            } else {
                frames++;
            }
            last_preview = last_capture;
        }
        if (ioctl(fd, VIDIOC_QBUF, &buffer) != 0) {
            err = ESP_FAIL;
            goto cleanup;
        }
        if (last_capture - fps_start >= 1000000) {
            status.preview_fps = frames * 1000000ULL / (last_capture - fps_start);
            set_status(session, &status);
            frames = 0;
            fps_start = last_capture;
        }
    }
    err = ESP_OK;

cleanup:
    if (streaming && ioctl(fd, VIDIOC_STREAMOFF, &type) != 0) ESP_LOGW(TAG, "stream stop failed");
    for (unsigned i = 0; i < CAMERA_BUFFERS; i++) {
        if (capture_buffers[i]) munmap(capture_buffers[i], capture_sizes[i]);
    }
    if (fd >= 0) close(fd);
#if CONFIG_GUI_CSI_CAMERA_USE_PPA
    if (ppa) ppa_unregister_client(ppa);
#else
    free(source_columns);
#endif
    if (video_initialized) {
        esp_err_t cleanup_err = esp_video_deinit_with_flags(VIDEO_FLAGS);
        if (cleanup_err != ESP_OK) ESP_LOGW(TAG, "video cleanup failed: %s", esp_err_to_name(cleanup_err));
    }
    if (own_i2c) i2c_del_master_bus(bus);
    status.running = false;
    status.error = err;
    set_status(session, &status);
    return err;
}

static void camera_task(void *arg)
{
    camera_session_t session = arg;
    esp_err_t err = capture(session);
    if (err != ESP_OK && !atomic_load(&session->stop)) {
        ESP_LOGW(TAG, "camera unavailable: %s (0x%x)", esp_err_to_name(err), err);
    }
    /* 错误状态和最后一帧保留到页面退出，避免 GUI 使用已释放的像素。 */
    while (!atomic_load(&session->stop)) vTaskDelay(pdMS_TO_TICKS(100));
    destroy_session(session);
    vTaskDelete(NULL);
}

esp_err_t svc_camera_start(uint16_t max_width, uint16_t max_height, camera_session_t *out)
{
    if (!out || !max_width || !max_height) return ESP_ERR_INVALID_ARG;
    *out = NULL;
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_busy, &expected, true)) return ESP_ERR_INVALID_STATE;
    camera_session_t session = calloc(1, sizeof(*session));
    if (!session) {
        atomic_store(&s_busy, false);
        return ESP_ERR_NO_MEM;
    }
    atomic_init(&session->stop, false);
    atomic_init(&session->paused, false);
    session->max_width = max_width;
    session->max_height = max_height;
    session->mutex = xSemaphoreCreateMutex();
    session->free_frames = xQueueCreate(CAMERA_BUFFERS, sizeof(uint8_t));
    session->ready_frames = xQueueCreate(1, sizeof(camera_frame_t));
    if (!session->mutex || !session->free_frames || !session->ready_frames ||
        xTaskCreate(camera_task, "csi_camera", 6144, session, 4, NULL) != pdPASS) {
        destroy_session(session);
        return ESP_ERR_NO_MEM;
    }
    *out = session;
    return ESP_OK;
}

bool svc_camera_get_status(camera_session_t session, camera_status_t *out)
{
    if (!session || !out || xSemaphoreTake(session->mutex, 0) != pdTRUE) return false;
    *out = session->status;
    xSemaphoreGive(session->mutex);
    return true;
}

bool svc_camera_take_frame(camera_session_t session, camera_frame_t *out)
{
    return session && out && xQueueReceive(session->ready_frames, out, 0) == pdTRUE;
}

void svc_camera_release_frame(camera_session_t session, const camera_frame_t *frame)
{
    if (session && frame && frame->pixels) xQueueSend(session->free_frames, &frame->index, 0);
}

void svc_camera_pause(camera_session_t session, bool paused)
{
    if (session) atomic_store(&session->paused, paused);
}

void svc_camera_stop(camera_session_t session)
{
    if (session) {
        atomic_store(&session->stop, true);
    }
}

#else

esp_err_t svc_camera_start(uint16_t max_width, uint16_t max_height, camera_session_t *out)
{
    (void)max_width;
    (void)max_height;
    if (out) *out = NULL;
    return ESP_ERR_NOT_SUPPORTED;
}
bool svc_camera_get_status(camera_session_t session, camera_status_t *out) { (void)session; (void)out; return false; }
bool svc_camera_take_frame(camera_session_t session, camera_frame_t *out) { (void)session; (void)out; return false; }
void svc_camera_release_frame(camera_session_t session, const camera_frame_t *frame) { (void)session; (void)frame; }
void svc_camera_pause(camera_session_t session, bool paused) { (void)session; (void)paused; }
void svc_camera_stop(camera_session_t session) { (void)session; }

#endif
