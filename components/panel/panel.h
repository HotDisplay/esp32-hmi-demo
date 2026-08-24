#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_BSP_PANEL_RGB_H050A29
#include "panel_h050a29.h"
#elif CONFIG_BSP_PANEL_RGB_H070B26
#include "panel_h070b26.h"
#elif CONFIG_BSP_PANEL_MIPI_H028A29
#include "panel_h028a29.h"
#elif CONFIG_BSP_PANEL_MIPI_H035B16
#include "panel_h035b16.h"
#elif CONFIG_BSP_PANEL_MIPI_H043A8
#include "panel_h043a8.h"
#elif CONFIG_BSP_PANEL_MIPI_H050A11
#include "panel_h050a11.h"
#else
#error "No panel selected! Run: idf.py menuconfig -> Display Selection"
#endif
