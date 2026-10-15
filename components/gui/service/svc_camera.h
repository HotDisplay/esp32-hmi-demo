#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct camera_session *camera_session_t;

typedef struct {
    bool running;
    esp_err_t error;
    uint16_t sensor_width;
    uint16_t sensor_height;
    uint16_t preview_width;
    uint16_t preview_height;
    unsigned preview_fps;
    char sensor[16];
} camera_status_t;

typedef struct {
    const void *pixels;
    uint16_t width;
    uint16_t height;
    uint8_t index;
} camera_frame_t;

/* 初始化和采集在后台执行；INVALID_STATE 表示上一会话正在退出。 */
esp_err_t svc_camera_start(uint16_t max_width, uint16_t max_height, camera_session_t *out);
bool svc_camera_get_status(camera_session_t session, camera_status_t *out);
bool svc_camera_take_frame(camera_session_t session, camera_frame_t *out);
void svc_camera_release_frame(camera_session_t session, const camera_frame_t *frame);
void svc_camera_pause(camera_session_t session, bool paused);
/* 调用前释放 UI 持有的帧；调用后不再访问 session。后台负责释放资源。 */
void svc_camera_stop(camera_session_t session);
