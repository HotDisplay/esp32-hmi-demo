# HOTLCD ESP32 Display Driver

## 深圳市鑫洪泰电子科技有限公司 · HotDisplay

**HOTLCD display driver firmware** — unified sample for driving HOTLCD TFT panels across the ESP32 family.  
**HOTLCD 显示屏驱动固件** — ESP32 全系列统一工程，支持 RGB / MIPI / SPI / 8080 多接口。

| ESP32 Chip | RGB | MIPI DSI | SPI | 8080 |
|------------|-----|----------|-----|------|
| ESP32-S3 | YES | — | — | Future |
| ESP32-P4 | YES | YES | — | Future |

Built on **LVGL**, board + interface + panel selected independently via `menuconfig`.  
基于 **LVGL**，Board / Interface / Panel 三级独立菜单选配。

---

## Architecture / 架构

```
menuconfig
  |-- Chip Target      →  idf.py set-target (ESP32-S3 / ESP32-P4)
  |-- BSP Board        →  选板子型号 (决定引脚 GPIO)
  |-- Display Interface →  选显示接口 (RGB / MIPI / SPI / 8080)
  |-- BSP Panel        →  选屏幕型号 (决定分辨率 + 时序 + 触摸)

Board + Interface + Panel 三者独立组合。
换接口 = menuconfig 换，换屏 = menuconfig 换，main/ 不动。
```

```
components/
  bsp/                   # 板子引脚定义
    Kconfig              # Board 选板菜单
    bsp_lcd.h / bsp_lcd.c
    boards/
      board_h050a29_esp_stdrgb.h    # ESP32-S3 板 (RGB)
      board_xxx_p4.h                # ESP32-P4 板 (RGB+MIPI) [未来]

  display/               # 显示接口驱动层 (NEW)
    Kconfig              # Interface 选接口菜单
    display.h            # 统一接口
    display_rgb.c        # RGB parallel (S3 / P4)
    display_mipi.c       # MIPI DSI (P4 only) [待实现]

  panel/                 # 屏幕时序 + 触摸参数
    Kconfig              # Panel 选屏菜单
    panel.h
    panels/
      panel_h050a29.h    # 5.0" 800x480 GT911
      panel_h070b26.h    # 7.0" 1024x600 GT911

  main/                  # 应用层 (不依赖硬件)
    main.c / lv_init.c
```

---

## Quick Start / 快速上手

### Prerequisites / 环境

- **ESP-IDF v6.0.0**
- ESP32-S3 module with PSRAM (or ESP32-P4)

```bash
git clone <repo-url>
cd <repo>

# Set target chip
idf.py set-target esp32s3      # or esp32p4

# Select Board / Interface / Panel via menuconfig
idf.py menuconfig

idf.py build
idf.py -p COMx flash monitor
```

---

## Supported Hardware / 支持硬件

### Boards / 板卡

| Board | Chip | RGB | MIPI |
|-------|------|-----|------|
| HTM-H050A29-ESP-StdRGB_V0 | ESP32-S3 | YES | — |

### Panels / 屏幕

| Panel | Size | Resolution | Interface | Touch |
|-------|------|-----------|-----------|-------|
| H050A29 | 5.0 inch | 800x480 | RGB565 | GT911 |
| H070B26 | 7.0 inch | 1024x600 | RGB565 | GT911 |

---

## Adding New Hardware / 新增硬件

### New Board / 新板卡

1. `components/bsp/boards/board_xxx.h` — GPIO pinout
2. `components/bsp/Kconfig.projbuild` — add board entry
3. `components/bsp/bsp_lcd.h` — add `#elif` include

### New Panel / 新屏幕

1. `components/panel/panels/panel_xxx.h` — timing + touch
2. `components/panel/Kconfig.projbuild` — add panel entry
3. `components/panel/panel.h` — add `#elif` include

### New Interface / 新接口

1. `components/display/display_xxx.c` — implement `display_init_xxx()`
2. `components/display/Kconfig.projbuild` — add interface entry (with chip dependency)
3. `components/bsp/bsp_lcd.c` — add `#elif CONFIG_DISPLAY_IFACE_XXX` dispatch

**No changes to `main/` needed.** `main/` 不动。

---

## Default SDK Config / 默认配置

`sdkconfig.defaults.esp32s3` — pre-configured defaults:

- **PSRAM** — Octal mode, 80 MHz (required for RGB frame buffer)
- **Bounce Buffer** — improves PCLK stability
- **Partition** — custom 16 MB for ESP32-S3-WROOM-1-N16R8
- **FreeRTOS** — 1000 Hz tick, dual core

---

## Contact / 联系我们

**HOTLCD — 深圳市鑫洪泰电子科技有限公司**

| Channel | Info |
|---------|------|
| Website 官网 | [https://cn.display-lcd.com](https://cn.display-lcd.com) |
| English 英文站 | [https://www.hotlcd.com](https://www.hotlcd.com) |
| GitHub | [https://github.com/HotDisplay](https://github.com/HotDisplay) |
| Sales 销售 | [lcd@hotlcd.com](mailto:lcd@hotlcd.com) |

**HOTLCD — 工业级 RGB / MIPI / LVDS TFT LCD 显示屏与驱动板方案 · Since 2004**
