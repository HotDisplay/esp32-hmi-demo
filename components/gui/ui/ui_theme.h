#pragma once

#include "lvgl.h"

/* ===== 全局配色（深色工业风，主色沿用 HMI 设计规范 #1F6FEB） =====
 * UI 设计师统一从这里取色，不要在页面文件里写死颜色。 */
#define UI_COLOR_BG          lv_color_hex(0x0D1117)   /* 页面背景 */
#define UI_COLOR_CARD        lv_color_hex(0x1B222C)   /* 卡片 / 面板底色 */
#define UI_COLOR_PRIMARY     lv_color_hex(0x1F6FEB)   /* 主色 / 标题栏 */
#define UI_COLOR_TEXT        lv_color_hex(0xE6EDF3)   /* 主文字 */
#define UI_COLOR_TEXT_MUTED  lv_color_hex(0x8B949E)   /* 次要文字 */

/* 默认字体。第 1 步统一用 LVGL 默认字体（montserrat 14，一定存在）。
 * 后续引入中文字体 / 更大字号时，在 menuconfig 开启对应选项后替换这里。 */
#define UI_FONT_NORMAL       (&lv_font_montserrat_14)

#ifdef __cplusplus
extern "C" {
#endif

/* 全局主题初始化。骨架阶段仅占位，后续可在此集中注册共享样式。 */
void ui_theme_init(void);

#ifdef __cplusplus
}
#endif
