# @file CMakeLists.txt
# @brief 注册显示接口驱动和 GUI / 原生测试共用的显示测试资源。
#
# @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
# @author Lee
# @date 2026-10-19

set(srcs "display.c" "display_test_resource.c")

# RGB display init is only compiled when the RGB interface is selected.
if(CONFIG_DISPLAY_INTERFACE_RGB)
    list(APPEND srcs "display_rgb.c")
endif()

# MIPI display init is only compiled when the MIPI interface is selected.
# (On a P4 board the RGB interface is also available, so the target capability
#  flag alone is not enough — guard on the actual interface choice.)
if(CONFIG_DISPLAY_INTERFACE_MIPI)
    list(APPEND srcs "display_mipi.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES esp_lcd bsp panel esp_lcd_mipi esp_partition
)
