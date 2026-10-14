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
| 0 | **Touch 触摸** | 单指画点 / 划线 / 坐标显示 / 清空 | 已实现，待上板验证 |
| 1 | **Wi-Fi 无线** | 扫描 AP / 连接 / IP | 占位 |
| 2 | **Ethernet 以太网** | 链路状态 / IP / 网关 / 掩码 | ✅ 已接通 |
| 3 | **USB Device** | 插入电脑枚举状态 | 占位 |
| 4 | **USB Host** | U 盘容量 / 键鼠 | 占位 |
| 5 | **SD Card** | 挂载状态 / 容量 / 文件列表 | ✅ 已接通 |
| 6 | **Camera 摄像头** | 实时预览 / 拍照 | 占位 |
| 7 | **System Info 系统信息** | 板名 / 芯片 / IDF / 固件 / 分辨率 / 内存 | ✅ 已接通 |

> 以太网与 SD 卡页采用 **1 秒定时刷新**（DHCP 与挂载均为异步过程，因此数据会动态更新）。

触摸页在标题栏下方提供画布，按住并移动手指可画线，抬手后开始新笔画。
`X / Y` 显示触摸点在屏幕中的坐标；`Clear` 清空画面，`Back` 返回主页并释放画布。
画布使用 RGB565，大小跟随显示分辨率；触摸驱动仍在 `Touch Selection` 中独立选择。
带中断的触摸设备由独立任务采集，按顺序缓存原始点，绘图只刷新笔迹附近区域。
保留真实点连线，不做曲线平滑；抬手日志 `TOUCH_INPUT` 输出每笔的平均、最大采集间隔。
队列满时会警告并重新起笔，避免跨过丢失点连线；无中断设备沿用组件默认读取方式。

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

Built on **LVGL 9.5** — Board / Interface / Panel / Touch selected independently via `menuconfig`.
基于 **LVGL 9.5** — Board / Interface / Panel / Touch 独立菜单选配。

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  menuconfig                                             │
│    Chip Target     →  idf.py set-target (S3 / P4)        │
│    BSP Board       →  Board pinout (GPIO)               │
│    Display Interface →  RGB / MIPI                       │
│    BSP Panel        →  Resolution + timing               │
│    Touch Controller →  GT911 / AXS15260D / disabled       │
└─────────────────────────────────────────────────────────┘
              Board × Interface × Panel × Touch 独立组合
```

显示屏在 `Display Selection` 中选择，触摸驱动在 `Touch Selection` 中单独选择。
例如 H028A29 可搭配 GT911 或 AXS15260D；切换显示屏不会改变已选触摸驱动。
触摸坐标范围跟随显示屏分辨率，触摸的 I²C、RST 和 INT 引脚由板型定义。

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

# 配置：选择 Board + Interface + Panel + Touch
idf.py menuconfig
#   → BSP Board Selection（板型）
#   → Display Selection → Display Interface（RGB / MIPI）
#   → Display Selection → Panel（屏型号）
#   → Touch Selection → Enable Touch Panel（启用触摸，默认关闭）
#   → Touch Selection → Touch Controller（GT911 / AXS15260D）

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

| Panel | Size | Resolution | Interface |
|-------|:----:|:----------:|:---------:|
| H021A04 | 2.1" | 540×540 | RGB565 |
| H034A04 | 3.4" 圆屏 | 800×800 | RGB565 |
| H048A02 | 4.8" | 480×1120 | RGB565 |
| **H050A29** | 5.0" | 800×480 | RGB565 |
| H070B26 | 7.0" | 1024×600 | RGB565 |
| H028A29 | 2.8" | 540×540 | MIPI DSI |
| H035B16 | 3.5" | 640×480 | MIPI DSI |
| H043A8 | 4.3" | 480×800 | MIPI DSI |
| H050A11 | 5.0" | 480×854 | MIPI DSI |

> 同一块板的 FPC 座兼容同接口的多种屏，仅需切换屏参。  
> 触摸驱动独立选择，请根据实际连接的触摸芯片配置；未连接触摸时关闭 `Enable Touch Panel`。

---

## Development Guide

### LVGL 移植

`components/gui/idf_component.yml` 使用 Registry 的 `espressif/esp_lvgl_port ^2.9.0`，
`dependencies.lock` 固定本次解析为 2.9.0；LVGL 继续使用 9.5.0。
`lv_init.c` 仅负责绑定现有显示、触摸句柄及创建 UI，任务、时钟、输入和刷新由组件管理。
RGB 使用 RGB565、MIPI 使用 RGB888，均使用驱动的双帧缓冲、DIRECT 模式和防撕裂同步；
RGB 的 `bb_mode` 对应 `display_init_rgb()` 中已启用的 bounce buffer。

ESP32-P4 + IDF 6.0.0 存在 DPI 回调字段兼容问题，
`cmake/esp_lvgl_port_compat.cmake` 只在构建目录生成修正副本，
将 MIPI 的 `cbs.on_frame_buf_complete` 改为 `cbs.on_refresh_done`，保留 RGB 回调。
不要手工修改 `managed_components`。组件版本、目标赋值或构建布局不匹配时配置会报错，
需核对新版源码后再更新或移除修正。

从其他任务调用 LVGL API 时必须使用 `lvgl_port_lock(0)` / `lvgl_port_unlock()`。
现有 LVGL 事件及定时器回调已在组件任务的锁内执行。触摸句柄为空或注册失败时保持显示运行。
上板验证应覆盖连续滑动、抬手、页面切换及长时间运行；编译成功不代表硬件防撕裂效果已验证。

### 原生 LCD 测试

`idf.py menuconfig → Display Test Demo → Run native LCD test instead of LVGL` 启用测试；
关闭后恢复原有 LVGL 界面。测试模式不启动触摸、LVGL、SD 卡或网络。
按红、绿、蓝、白、16 级灰阶、可选图片循环，每个画面停留 1 秒。
灰阶根据屏幕宽高自动切换：横屏显示竖条（左黑右白），竖屏显示横条（上黑下白）。
沿用所选 panel 的分辨率、时序和双帧缓冲；RGB 使用 RGB565，MIPI 使用 RGB888，
提交画面后等待对应接口的刷新回调再复用旧帧缓冲。

图片放在 `main/images/`，命名为 `test.png`、`test.jpg`、`test.jpeg` 或 `test.bmp`，只保留一个。
建议按所选屏幕的原始分辨率准备图片；尺寸不一致时编译会拉伸到全屏。
没有图片时自动跳过，有图片时转换工具需要 ESP-IDF Python 环境中的 Pillow
（`python -m pip install Pillow`）。新增 `lcd_image` 数据分区占用 4 MiB 空闲 Flash，
原有 NVS、PHY 和应用分区的位置及大小不变，Flash 容量至少为 8 MB。
添加、替换、删除图片或切换 panel 后执行 `idf.py build flash`；
图片分区会随完整烧录更新，单独烧录应用 bin 不会更新图片。
运行时通过 `esp_partition_mmap()` 映射图片分区，再复制到空闲帧缓冲，
避免循环中 `esp_partition_read()` 暂停 Cache/中断而使 RGB bounce buffer 供数不足。

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
| 2 | `components/display/Kconfig.projbuild` | 加 `config BSP_PANEL_XXX`，不绑定触摸驱动 |
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
