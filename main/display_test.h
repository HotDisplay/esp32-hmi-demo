/**
 * @file display_test.h
 * @brief 上电原生显示测试入口，仅供主程序按配置启动。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date 2026-10-19
 */
#pragma once
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

/** @brief 独占 LCD 循环测试，仅在 CONFIG_DISPLAY_TEST_DEMO 下使用，出错时返回。 */
esp_err_t display_test_run(esp_lcd_panel_handle_t panel);
