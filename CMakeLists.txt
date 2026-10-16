cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
# idf_build_set_property(MINIMAL_BUILD ON)
project(esp32-hmi-demo)

include(cmake/esp_lvgl_port_compat.cmake)
include(cmake/esp_hosted_reset_compat.cmake)
include(cmake/display_test_image.cmake)
