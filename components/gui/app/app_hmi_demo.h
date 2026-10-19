/**
 * @file app_hmi_demo.h
 * @brief HMI 应用入口和首页功能页面枚举，顺序与菜单一致。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 功能页 ID，与主菜单卡片顺序一致 */
typedef enum {
    APP_SCREEN_DISPLAY_TEST = 0,
    APP_SCREEN_TOUCH,
    APP_SCREEN_WIFI,
    APP_SCREEN_ETHERNET,
    APP_SCREEN_USB,
    APP_SCREEN_USB_HOST,
    APP_SCREEN_SDCARD,
    APP_SCREEN_CAMERA,
    APP_SCREEN_SYSINFO,
    APP_SCREEN_ABOUT,
    APP_SCREEN_COUNT,
} app_screen_id_t;

/* HMI 演示应用入口：创建主菜单并启动 UI。
 * 必须在 lvgl_init() 之后、且在持有 LVGL API 锁的状态下调用。 */
void app_hmi_demo(void);

#ifdef __cplusplus
}
#endif
