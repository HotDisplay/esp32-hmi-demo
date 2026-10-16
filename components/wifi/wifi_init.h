#pragma once

#include "esp_err.h"
#include "esp_wifi.h"

/* Call from a worker task; C6 transport bring-up may wait for firmware. */
esp_err_t wifi_init(void);

/* Default network remains defined in wifi_init.c; the service owns connecting. */
wifi_config_t wifi_default_config(void);
