/**
 * @file ui_theme.h
 * @brief HMI 界面的共享配色、字体选择及主题初始化接口。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-17
 *
 * 正文使用 Montserrat 14；首页优先使用已启用的 20 / 28 字号，未启用时回退到正文。
 */
#pragma once

#include "lvgl.h"

/* ===== 全局配色（深色工业风，主色沿用 HMI 设计规范 #1F6FEB） =====
 * UI 设计师统一从这里取色，不要在页面文件里写死颜色。 */
#define UI_COLOR_BG          lv_color_hex(0x0D1117)   /* 页面背景 */
#define UI_COLOR_CARD        lv_color_hex(0x1B222C)   /* 卡片 / 面板底色 */
#define UI_COLOR_PRIMARY     lv_color_hex(0x1F6FEB)   /* 主色 / 标题栏 */
#define UI_COLOR_TEXT        lv_color_hex(0xE6EDF3)   /* 主文字 */
#define UI_COLOR_TEXT_MUTED  lv_color_hex(0x8B949E)   /* 次要文字 */
#define UI_COLOR_BORDER      lv_color_hex(0x2B3747)   /* 首页卡片边框 */
#define UI_COLOR_ACCENT      lv_color_hex(0x1FC2F4)   /* 公司标识青蓝色 */
#define UI_COLOR_BRAND_GREEN lv_color_hex(0xA3D416)   /* 公司标识黄绿色 */
#define UI_COLOR_BRAND_PAPER lv_color_hex(0xFFFFFF)   /* 品牌栏背景 */
#define UI_COLOR_BRAND_INK   lv_color_hex(0x263442)   /* 品牌栏文字 */
#define UI_COLOR_HOME_BG     lv_color_hex(0x0C1624)   /* 首页深蓝灰背景 */
#define UI_COLOR_HOME_CARD   lv_color_hex(0x17283B)   /* 首页功能卡片 */

/* 正文字体保持 14 号，首页标题按配置选择较大的字号。 */
#define UI_FONT_NORMAL       (&lv_font_montserrat_14)

#if LV_FONT_MONTSERRAT_28
#define UI_FONT_HEADING      (&lv_font_montserrat_28)
#else
#define UI_FONT_HEADING      UI_FONT_NORMAL
#endif

#if LV_FONT_MONTSERRAT_20
#define UI_FONT_CARD         (&lv_font_montserrat_20)
#else
#define UI_FONT_CARD         UI_FONT_NORMAL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 全局主题初始化。骨架阶段仅占位，后续可在此集中注册共享样式。 */
void ui_theme_init(void);

#ifdef __cplusplus
}
#endif
