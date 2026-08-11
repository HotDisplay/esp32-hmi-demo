#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_BSP_PANEL_H050A29
#include "panel_h050a29.h"
#elif CONFIG_BSP_PANEL_H070B26
#include "panel_h070b26.h"
#else
#error "No panel selected! Run: idf.py menuconfig -> BSP Panel"
#endif
