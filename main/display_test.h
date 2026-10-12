#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

/* Runs continuously; only returns on error. */
esp_err_t display_test_run(esp_lcd_panel_handle_t panel);
