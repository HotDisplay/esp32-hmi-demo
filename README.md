<div align="center">

# HOTLCD ESP32 Display Driver

### 深圳市鑫洪泰电子科技有限公司 · HotDisplay

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.0.0-185FA5?style=flat-square)](https://docs.espressif.com/projects/esp-idf/)
[![LVGL](https://img.shields.io/badge/LVGL-9.5-7F77DD?style=flat-square)](https://lvgl.io/)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-✓-3B6D11?style=flat-square)]()
[![ESP32-P4](https://img.shields.io/badge/ESP32--P4-✓-3B6D11?style=flat-square)]()
[![License](https://img.shields.io/badge/License-CC0--1.0-888780?style=flat-square)](LICENSE)

</div>

---

> **HOTLCD display driver firmware** — unified sample for driving HOTLCD TFT panels across the ESP32 family, with a built-in **HMI demo application**.  
> **HOTLCD 显示屏驱动固件** — ESP32 全系列统一工程，支持 RGB / MIPI 多接口，内置 **HMI 功能演示应用**。

## What's New · 最新进展

本工程已从"单纯的点屏驱动"升级为**带 HMI 演示应用的完整固件**：

- 烧录上电后，屏幕显示 **HMI 功能演示主菜单**（不再是 LVGL Widgets 示例）
- 客户拿到板子即可通过触摸操作，直观体验板载各功能（以太网、SD 卡、系统信息等）
- 采用模块化三层架构（应用编排 / UI / 服务），便于 UI 设计师与工程师并行协作

## HMI Demo Application · HMI 演示应用

### 主菜单与功能页

主菜单为 8 张功能卡片，点击进入对应演示页，页面左上角 **Back** 键返回主菜单。

| # | 页面 | 演示内容 | 状态 |
|:-:|------|---------|:---:|
| 0 | **Touch 触摸** | 画点 / 划线 | 占位 |
| 1 | **Wi-Fi 无线** | 扫描 AP / 连接 / IP | 占位 |
| 2 | **Ethernet 以太网** | 链路状态 / IP / 网关 / 掩码 | ✅ 已接通 |
| 3 | **USB Device** | 插入电脑枚举状态 | 占位 |
| 4 | **USB Host** | U 盘容量 / 键鼠 | 占位 |
| 5 | **SD Card** | 挂载状态 / 容量 / 文件列表 | ✅ 已接通 |
| 6 | **Camera 摄像头** | 实时预览 / 拍照 | 占位 |
| 7 | **System Info 系统信息** | 板名 / 芯片 / IDF / 固件 / 分辨率 / 内存 | ✅ 已接通 |

> 以太网与 SD 卡页采用 **1 秒定时刷新**（DHCP 与挂载均为异步过程，因此数据会动态更新）。

### 应用架构（三层）

```
┌──────────────────────────────────────────────────────┐
│  应用编排层  app/        app_hmi_demo.c               │
│               导航中枢：app_screen_id_t 分派页面       │
└──────────────────────────────────────────────────────┘
                     │ 创建页面 / 处理返回
┌──────────────────────────────────────────────────────┐
│  UI 层  ui/          8 页面 + theme + common + launcher│
│              纯界面 + 事件回调，不触碰底层驱动          │
└──────────────────────────────────────────────────────┘
                     │ 只调用 svc_* 接口
┌──────────────────────────────────────────────────────┐
│  服务层  service/    svc_sysinfo / svc_net / svc_storage│
│              调用驱动、回传数据给 UI                    │
└──────────────────────────────────────────────────────┘
                     │ 调用驱动
┌──────────────────────────────────────────────────────┐
│  框架层  bsp / display / panel / touch / lv_init       │
│  外设组件  ethernet / sdcard / wifi                    │
└──────────────────────────────────────────────────────┘
```

**核心约定**：UI 文件不出现 `esp_*` / `sdmmc_*` 等底层调用，只调 `svc_*`；服务层才碰驱动。  
这样 UI 设计师可独立改 `ui/`，不影响底层逻辑。

### 给 UI 设计师

完整的架构说明、页面清单、交付要求与 LVGL 能力边界见工程文档  
**`GUI架构与流程说明.md`**（UI 设计师交接文档）

> 当前界面文案为英文：LVGL 默认字体（montserrat 14）不含中文字形，上中文需先引入中文字体并替换 `ui_theme.h` 的 `UI_FONT_NORMAL`。

---

## Supported Chips & Interfaces

| ESP32 Chip | RGB 16-bit | MIPI DSI | SPI | 8080 |
|:----------:|:---------:|:-------:|:---:|:----:|
| **ESP32-S3** | ✓ | — | — | Planned |
| **ESP32-P4** | ✓ | ✓ | — | Planned |

Built on **LVGL 9.5** — Board / Interface / Panel selected independently via `menuconfig`.  
基于 **LVGL 9.5** — Board / Interface / Panel 三级独立菜单选配。

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  menuconfig                                             │
│    Chip Target     →  idf.py set-target (S3 / P4)        │
│    BSP Board       →  Board pinout (GPIO)               │
│    Display Interface →  RGB / MIPI                       │
│    BSP Panel        →  Resolution + timing + touch       │
└─────────────────────────────────────────────────────────┘
              Board × Interface × Panel 独立组合
```

触摸芯片无需单独配置：每个面板选项通过 Kconfig `select` 自动选定匹配的触摸芯片，面板与触摸不会配错。

---

## Project Structure

```
esp32-display-driver/
├── main/                           # 应用入口（只做编排，不含业务逻辑）
│   ├── main.c                      # bsp → display → touch → lvgl → 外设
│   ├── CMakeLists.txt
│   └── idf_component.yml
│
├── components/
│   ├── bsp/                        # 板级支持包（GPIO 引脚定义）
│   │   ├── bsp.h / bsp.c           # 背光、IO 扩展器等
│   │   ├── Kconfig.projbuild       # 板型选择菜单
│   │   └── boards/
│   │       ├── board_h050a29_esp_stdrgb.h    # ESP32-S3 RGB 板
│   │       └── board_esp32p4_demo_a02.h      # ESP32-P4 演示板
│   │
│   ├── display/                    # 显示接口驱动层
│   │   ├── display.h / display.c   # 统一接口
│   │   ├── display_rgb.c           # RGB 16-bit 并行
│   │   ├── display_mipi.c          # MIPI DSI（ESP32-P4）
│   │   └── Kconfig.projbuild       # 接口 + 面板选择菜单
│   │
│   ├── panel/                      # 面板参数（时序 + 分辨率）
│   │   ├── panel.h                 # 面板选择器
│   │   └── panels/
│   │       ├── rgb/                # RGB 屏：H021A04 / H050A29 / H070B26
│   │       └── dsi/                # MIPI 屏：H028A29 / H035B16 / H043A8 / H050A11
│   │
│   ├── touch/                      # 触摸控制器（GT911 / AXS15260D）
│   ├── esp_lcd_mipi/               # 自研 MIPI 初始化组件
│   ├── ethernet/                   # 以太网（ESP32-P4 内置 EMAC + RMII PHY）
│   ├── sdcard/                     # SD 卡（SDMMC 4-bit，挂载到 /sdcard）
│   ├── wifi/                       # Wi-Fi STA
│   │
│   └── gui/                        # ★ HMI GUI 应用
│       ├── lv_init.c / .h          # LVGL 移植（显示 + 触摸绑定）
│       ├── app/                    # 应用编排（导航中枢）
│       ├── ui/                     # UI 层（页面 / 主题 / 复用控件 / 主菜单）
│       └── service/                # 服务层（svc_sysinfo / svc_net / svc_storage）
│
├── sdkconfig.defaults.esp32s3      # S3 默认配置
├── sdkconfig.defaults.esp32p4      # P4 默认配置
├── partitions.csv                  # 分区表
└── README.md
```

---

## Quick Start

### 1. Prerequisites

- **ESP-IDF v6.0.0** ([install guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.0/esp32s3/get-started/))
- ESP32-S3 module with PSRAM，或 ESP32-P4
- 一块 HOTLCD 驱动板 + 显示屏

### 2. Build & Flash

```bash
git clone https://github.com/HotDisplay/esp-display-driver.git
cd esp32-display-driver

# 选择目标芯片
idf.py set-target esp32s3      # 或: idf.py set-target esp32p4

# 配置：选择 Board + Interface + Panel
idf.py menuconfig
#   → BSP Board Selection（板型）
#   → Display Selection → Display Interface（RGB / MIPI）
#   → Display Selection → Panel（屏型号）

# 编译、烧录、监视
idf.py build
idf.py -p COM3 flash monitor    # Windows: COM3 / Linux: /dev/ttyUSB0
```

### 3. 上电效果

烧录后屏幕显示 **HMI 功能演示主菜单**（8 张功能卡片），点击卡片进入对应演示页，Back 键返回。

> 以太网与 SD 卡页的数据为 1 秒定时刷新；若未插入网线 / SD 卡，页面显示 `Not mounted` / `Link: Down` 属正常。

---

## Supported Hardware

### Boards

| Board | Chip | RGB | MIPI | SD | ETH | Status |
|-------|------|:---:|:----:|:--:|:---:|:------:|
| HTM-H050A29-ESP-StdRGB_V0 | ESP32-S3 | ✓ | — | — | — | Active |
| ESP32-P4 Demo A02 | ESP32-P4 | ✓ | ✓ | ✓ | ✓ | Active |

### Panels

| Panel | Size | Resolution | Interface | Touch |
|-------|:----:|:----------:|:---------:|:-----:|
| H021A04 | 2.1" | 540×540 | RGB565 | AXS15260D |
| **H050A29** | 5.0" | 800×480 | RGB565 | GT911 |
| H070B26 | 7.0" | 1024×600 | RGB565 | GT911 |
| H028A29 | 2.8" | 540×540 | MIPI DSI | AXS15260D |
| H035B16 | 3.5" | 640×480 | MIPI DSI | GT911 |
| H043A8 | 4.3" | 480×800 | MIPI DSI | GT911 |
| H050A11 | 5.0" | 480×854 | MIPI DSI | GT911 |

> 同一块板的 FPC 座兼容同接口的多种屏，仅需切换屏参。  
> 触摸芯片由面板选项自动 `select`，无需单独配置。

---

## Development Guide

### 新增一个 HMI 功能页

| Step | File | Action |
|:----:|------|--------|
| 1 | `components/gui/ui/ui_xxx.h/.c` | 实现 `lv_obj_t *ui_xxx_create(lv_event_cb_t back_cb)` |
| 2 | `components/gui/service/svc_xxx.h/.c` | （如需数据）实现 `svc_xxx` 服务，封装驱动调用 |
| 3 | `components/gui/app/app_hmi_demo.h` | 在 `app_screen_id_t` 枚举末尾加一项 |
| 4 | `components/gui/app/app_hmi_demo.c` | 在 `on_card_click` 的 switch 里加分派 |
| 5 | `components/gui/ui/ui_launcher.c` | 在 `s_items[]` 末尾加卡片标题/副标题 |
| 6 | `components/gui/CMakeLists.txt` | SRCS 加 `ui/ui_xxx.c`（如新增服务则加 `service/svc_xxx.c`） |

> 卡片顺序必须与 `app_screen_id_t` 枚举顺序一致。

### 新增硬件

| Step | File | Action |
|:----:|------|--------|
| **新板型** 1 | `components/bsp/boards/board_xxx.h` | 定义 GPIO 引脚 |
| 2 | `components/bsp/Kconfig.projbuild` | 加 `config BSP_BOARD_XXX` |
| 3 | `components/bsp/bsp.h` | 加 `#elif CONFIG_BSP_BOARD_XXX` |
| **新面板** 1 | `components/panel/panels/{rgb,dsi}/panel_xxx.h` | 定义时序 + 分辨率 |
| 2 | `components/display/Kconfig.projbuild` | 加 `config BSP_PANEL_XXX` + `select BSP_TOUCH_*` |
| 3 | `components/panel/panel.h` | 加 `#elif CONFIG_BSP_PANEL_XXX` |
| **新接口** 1 | `components/display/display_xxx.c` | 实现 `display_init_xxx()` |
| 2 | `components/display/Kconfig.projbuild` | 加接口 + 芯片依赖 |
| 3 | `components/bsp/bsp.c` | 加 `#elif` 分派 |

> **`main/` 不需要改动** —— main 只做编排，硬件差异全部收敛在组件层。

---

## Default SDK Configuration

Files: `sdkconfig.defaults.esp32s3` / `sdkconfig.defaults.esp32p4`

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
