<div align="center">

# HOTLCD ESP32 Display Driver

### 深圳市鑫洪泰电子科技有限公司 · HotDisplay

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.0.0-185FA5?style=flat-square)](https://docs.espressif.com/projects/esp-idf/)
[![LVGL](https://img.shields.io/badge/LVGL-9.x-7F77DD?style=flat-square)](https://lvgl.io/)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-✓-3B6D11?style=flat-square)]()
[![ESP32-P4](https://img.shields.io/badge/ESP32--P4-✓-3B6D11?style=flat-square)]()
[![License](https://img.shields.io/badge/License-CC0--1.0-888780?style=flat-square)](LICENSE)

</div>

---

> **HOTLCD display driver firmware** — unified sample for driving HOTLCD TFT panels across the ESP32 family.  
> **HOTLCD 显示屏驱动固件** — ESP32 全系列统一工程，支持 RGB / MIPI / SPI / 8080 多接口。

## Supported Chips & Interfaces

| ESP32 Chip | RGB 16-bit | MIPI DSI | SPI | 8080 |
|:----------:|:---------:|:-------:|:---:|:----:|
| **ESP32-S3** | ✓ | — | — | Planned |
| **ESP32-P4** | ✓ | ✓ | — | Planned |

Built on **LVGL** — Board / Interface / Panel selected independently via `menuconfig`.  
基于 **LVGL** — Board / Interface / Panel 三级独立菜单选配。

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  menuconfig                                             │
│    Chip Target     →  idf.py set-target (S3 / P4)        │
│    BSP Board       →  Board pinout (GPIO)               │
│    Display Interface →  RGB / MIPI / SPI / 8080          │
│    BSP Panel        →  Resolution + timing + touch       │
└─────────────────────────────────────────────────────────┘
              Board × Interface × Panel 独立组合
```

### Project Structure

```
esp-display-driver/
├── main/                       # Application layer (hardware-agnostic)
│   ├── main.c                  # Entry point → calls bsp_init()
│   └── lv_init.c               # LVGL display binding
│
├── components/
│   ├── bsp/                    # Board Support Package (pinout)
│   │   ├── bsp_lcd.h           # Unified BSP interface
│   │   ├── bsp_lcd.c           # Backlight + touch init
│   │   ├── Kconfig.projbuild   # Board selection menu
│   │   └── boards/
│   │       └── board_h050a29_esp_stdrgb.h   # ESP32-S3 RGB board
│   │
│   ├── display/                # Display interface driver layer
│   │   ├── display.h           # Unified interface
│   │   ├── display_rgb.c       # RGB 16-bit parallel
│   │   ├── display_mipi.c      # MIPI DSI (ESP32-P4) [WIP]
│   │   └── Kconfig.projbuild   # Interface selection menu
│   │
│   └── panel/                  # Panel configuration (timing + touch)
│       ├── panel.h             # Panel selector
│       ├── Kconfig.projbuild   # Panel selection menu
│       └── panels/
│           ├── panel_h050a29.h # 5.0" 800×480 GT911
│           └── panel_h070b26.h # 7.0" 1024×600 GT911
│
├── sdkconfig.defaults.esp32s3  # Pre-configured defaults
├── partitions.csv              # Custom partition table
└── README.md
```

---

## Quick Start

### 1. Prerequisites

- **ESP-IDF v6.0.0** ([install guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.0/esp32s3/get-started/))
- ESP32-S3 module with PSRAM (or ESP32-P4)
- One HOTLCD driver board + panel

### 2. Clone & Build

```bash
git clone https://github.com/HotDisplay/esp-display-driver.git
cd esp-display-driver

# Set target chip
idf.py set-target esp32s3      # or: idf.py set-target esp32p4

# Configure: select Board + Interface + Panel
idf.py menuconfig
#   → BSP Board Selection
#   → Display Interface
#   → BSP Panel Selection

# Build, flash, monitor
idf.py build
idf.py -p COM3 flash monitor    # Windows: COM3 / Linux: /dev/ttyUSB0
```

After flashing, the screen displays the **LVGL Widgets Demo**.  
烧录后上电，屏幕显示 LVGL Widgets 示例界面。

---

## Supported Hardware

### Boards

| Board | Chip | RGB | MIPI | Status |
|-------|------|:---:|:----:|:------:|
| HTM-H050A29-ESP-StdRGB_V0 | ESP32-S3 | ✓ | — | Active |
| _more boards coming_ | ESP32-P4 | ✓ | ✓ | Planned |

### Panels

| Panel | Size | Resolution | Interface | Touch | Default |
|-------|:----:|:----------:|:---------:|:-----:|:-------:|
| H050A29 | 5.0" | 800×480 | RGB565 | GT911 | ✓ |
| H070B26 | 7.0" | 1024×600 | RGB565 | GT911 | — |

> Same board FPC connector is compatible with all listed RGB panels.  
> 同一块板 FPC 座兼容以上所有 RGB 屏，仅需屏参不同。

---

## Adding New Hardware

### New Board

| Step | File | Action |
|:----:|------|--------|
| 1 | `components/bsp/boards/board_xxx.h` | Define GPIO pinout |
| 2 | `components/bsp/Kconfig.projbuild` | Add `config BSP_BOARD_XXX` |
| 3 | `components/bsp/bsp_lcd.h` | Add `#elif CONFIG_BSP_BOARD_XXX` |

### New Panel

| Step | File | Action |
|:----:|------|--------|
| 1 | `components/panel/panels/panel_xxx.h` | Define timing + touch |
| 2 | `components/panel/Kconfig.projbuild` | Add `config BSP_PANEL_XXX` |
| 3 | `components/panel/panel.h` | Add `#elif CONFIG_BSP_PANEL_XXX` |

### New Interface

| Step | File | Action |
|:----:|------|--------|
| 1 | `components/display/display_xxx.c` | Implement `display_init_xxx()` |
| 2 | `components/display/Kconfig.projbuild` | Add interface + chip dependency |
| 3 | `components/bsp/bsp_lcd.c` | Add `#elif` dispatch |

> **`main/` never needs changes.** `main/` 一行不用改。

---

## Default SDK Configuration

File: `sdkconfig.defaults.esp32s3`

| Config | Value | Purpose |
|--------|-------|---------|
| SPIRAM | Octal / 80 MHz | Frame buffer in PSRAM |
| Bounce Buffer | Enabled | PCLK stability with PSRAM framebuffer |
| Flash | 16 MB QIO 80 MHz | ESP32-S3-WROOM-1-N16R8 |
| Partition | Custom | `partitions.csv` |
| FreeRTOS | 1000 Hz / dual core | LVGL recommended |
| Main Task Stack | 8192 | LVGL task requirement |

---

## Contact

<div align="center">

**HOTLCD · 深圳市鑫洪泰电子科技有限公司**  
Industrial-grade RGB / MIPI / LVDS TFT LCD display & driver board solutions since 2004

</div>

| | |
|:--|:--|
| **Website 官网** | [cn.display-lcd.com](https://cn.display-lcd.com) |
| **English** | [www.hotlcd.com](https://www.hotlcd.com) |
| **GitHub** | [github.com/HotDisplay](https://github.com/HotDisplay) |
| **Sales 销售** | [lcd@hotlcd.com](mailto:lcd@hotlcd.com) |
| **Samples 样品** | [lcd@hotlcd.com](mailto:lcd@hotlcd.com) |

---

<div align="center">

<sub>Built with ESP-IDF + LVGL · Maintained by HOTLCD R&D Team</sub>

</div>
