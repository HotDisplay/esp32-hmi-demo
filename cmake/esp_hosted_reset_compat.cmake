# DEMO-A02 resets C6 through expander P00, unsupported by Hosted 2.12.3.
# Compile a patched SDIO source in the build directory; do not edit dependencies.
function(apply_esp_hosted_reset_compat)
    if(NOT IDF_TARGET STREQUAL "esp32p4" OR NOT CONFIG_BSP_BOARD_ESP32P4_DEMO_A02 OR
       NOT CONFIG_ESP_WIFI_REMOTE_ENABLED OR NOT CONFIG_ESP_WIFI_REMOTE_LIBRARY_HOSTED)
        return()
    endif()
    if(NOT CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE OR CONFIG_ESP_HOSTED_HOST_POWER_SAVE_ENABLED)
        message(FATAL_ERROR "DEMO-A02 P00 reset patch requires SDIO with Hosted host power save disabled.")
    endif()
    idf_component_get_property(hosted_version espressif__esp_hosted COMPONENT_VERSION)
    if(NOT hosted_version VERSION_EQUAL "2.12.3")
        message(FATAL_ERROR "Review DEMO-A02 P00 reset patch for Hosted ${hosted_version}.")
    endif()
    idf_component_get_property(hosted_dir espressif__esp_hosted COMPONENT_DIR)
    idf_component_get_property(hosted_lib espressif__esp_hosted COMPONENT_LIB)
    set(relative_source "host/drivers/transport/sdio/sdio_drv.c")
    set(original_source "${hosted_dir}/${relative_source}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original_source}")
    file(READ "${original_source}" source_code)
    string(REPLACE "\r\n" "\n" source_code "${source_code}")

    string(REGEX MATCH "static esp_err_t transport_gpio_reset\\([^}]+\\}\n" old_reset "${source_code}")
    set(old_pin_setup [=[
	gpio_pin_t reset_pin = { .port = H_GPIO_PORT_RESET, .pin = H_GPIO_PIN_RESET };

	if (ESP_TRANSPORT_OK != esp_hosted_transport_get_reset_config(&reset_pin)) {
		ESP_LOGE(TAG, "Unable to get RESET config for transport");
		return -1;
	}

	assert(reset_pin.pin != -1);

	release_slave_reset_gpio_post_wakeup();
]=])
    set(old_log "ESP_LOGW(TAG, \"Reset slave using GPIO[%u]\", reset_pin.pin);")
    string(FIND "${source_code}" "${old_pin_setup}" pin_offset)
    string(FIND "${source_code}" "${old_log}" log_offset)
    string(REGEX MATCHALL "transport_gpio_reset\\(bus_handle, reset_pin\\)" reset_calls "${source_code}")
    list(LENGTH reset_calls reset_count)
    if(old_reset STREQUAL "" OR pin_offset LESS 0 OR log_offset LESS 0 OR NOT reset_count EQUAL 2)
        message(FATAL_ERROR "Hosted SDIO reset source changed; review the DEMO-A02 P00 patch.")
    endif()
    string(REPLACE "${old_reset}" "/* Local board hook; no native GPIO reset on DEMO-A02. */\nextern esp_err_t board_wifi_reset(void);\n" source_code "${source_code}")
    string(REPLACE "${old_pin_setup}" "" source_code "${source_code}")
    string(REGEX REPLACE "(\t+)transport_gpio_reset\\(bus_handle, reset_pin\\);" "\\1res = board_wifi_reset();\n\\1if (res != ESP_OK) return res;" source_code "${source_code}")
    string(REPLACE "${old_log}" "ESP_LOGW(TAG, \"Reset slave via board hook (P00)\");" source_code "${source_code}")
    # Reject leftover GPIO-reset paths instead of silently compiling a partial patch.
    if(source_code MATCHES "reset_pin|transport_gpio_reset|release_slave_reset_gpio_post_wakeup")
        message(FATAL_ERROR "Hosted SDIO still references native reset GPIO; review the P00 patch.")
    endif()

    set(patched_source "${CMAKE_BINARY_DIR}/esp_hosted_reset_compat/sdio_drv.c")
    file(CONFIGURE OUTPUT "${patched_source}" CONTENT "${source_code}" @ONLY)
    get_target_property(hosted_sources ${hosted_lib} SOURCES)
    set(replaced_sources "")
    set(replaced_count 0)
    foreach(source IN LISTS hosted_sources)
        if(source STREQUAL relative_source OR source STREQUAL original_source OR
           source STREQUAL "./${relative_source}")
            list(APPEND replaced_sources "${patched_source}")
            math(EXPR replaced_count "${replaced_count} + 1")
        else()
            list(APPEND replaced_sources "${source}")
        endif()
    endforeach()
    if(NOT replaced_count EQUAL 1)
        message(FATAL_ERROR "Hosted source layout changed; review the DEMO-A02 P00 patch.")
    endif()
    set_property(TARGET ${hosted_lib} PROPERTY SOURCES "${replaced_sources}")
    message(STATUS "Applied Hosted 2.12.3 DEMO-A02 P00 reset hook")
endfunction()

apply_esp_hosted_reset_compat()
