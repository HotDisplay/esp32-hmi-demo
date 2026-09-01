#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 功能页 ID，与主菜单卡片顺序一致 */
typedef enum {
    APP_SCREEN_TOUCH = 0,
    APP_SCREEN_WIFI,
    APP_SCREEN_ETHERNET,
    APP_SCREEN_USB,
    APP_SCREEN_USB_HOST,
    APP_SCREEN_SDCARD,
    APP_SCREEN_CAMERA,
    APP_SCREEN_SYSINFO,
    APP_SCREEN_COUNT,
} app_screen_id_t;

/* HMI 演示应用入口：创建主菜单并启动 UI。
 * 必须在 lvgl_init() 之后、且在持有 LVGL API 锁的状态下调用。 */
void app_hmi_demo(void);

#ifdef __cplusplus
}
#endif
