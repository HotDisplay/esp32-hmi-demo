# @file display_test_image.cmake
# @brief 为 GUI 和原生显示测试生成图片分区，完整烧录时更新图片内容。
#
# @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
# @author Lee
# @date 2026-10-19

if(CONFIG_DISPLAY_INTERFACE_RGB)
    set(image_interface RGB)
    set(image_panel_dir rgb)
    set(image_format rgb565)
else()
    set(image_interface MIPI)
    set(image_panel_dir dsi)
    set(image_format bgr888)
endif()

file(GLOB panel_headers CONFIGURE_DEPENDS
    "${PROJECT_DIR}/components/panel/panels/${image_panel_dir}/panel_*.h")
foreach(header IN LISTS panel_headers)
    get_filename_component(model "${header}" NAME_WE)
    string(REGEX REPLACE "^panel_" "" model "${model}")
    string(TOUPPER "${model}" model)
    if(CONFIG_BSP_PANEL_${image_interface}_${model})
        set(image_panel_header "${header}")
    endif()
endforeach()
if(NOT image_panel_header)
    message(FATAL_ERROR "LCD test: could not find the selected panel header")
endif()

file(GLOB test_images CONFIGURE_DEPENDS
    "${PROJECT_DIR}/main/images/test.png" "${PROJECT_DIR}/main/images/test.jpg"
    "${PROJECT_DIR}/main/images/test.jpeg" "${PROJECT_DIR}/main/images/test.bmp")
list(LENGTH test_images image_count)
if(image_count GREATER 1)
    message(FATAL_ERROR "Keep only one test.png, test.jpg, test.jpeg or test.bmp in main/images")
endif()
idf_build_get_property(image_python PYTHON)
set(image_binary "${CMAKE_BINARY_DIR}/display_test_image.bin")
add_custom_command(OUTPUT "${image_binary}"
    COMMAND "${image_python}" "${PROJECT_DIR}/tools/convert_test_image.py"
        --panel-header "${image_panel_header}" --image "${test_images}"
        --format "${image_format}" --output "${image_binary}"
    DEPENDS "${PROJECT_DIR}/tools/convert_test_image.py" "${image_panel_header}" ${test_images}
    VERBATIM)
add_custom_target(display_test_image ALL DEPENDS "${image_binary}")
add_dependencies(flash display_test_image)
esptool_py_flash_to_partition(flash lcd_image "${image_binary}")
