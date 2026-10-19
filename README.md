# HOTLCD ESP32 HMI

HOTLCD HMI 演示工程，支持 ESP32-S3 的 RGB 显示和 ESP32-P4 的 RGB / MIPI DSI 显示。
可用于测试显示屏、触摸屏及板载外设，也可在现有 LVGL 界面基础上开发应用。

## 快速开始

### 准备环境

- 安装并激活 **ESP-IDF v6.0** 开发环境。
- 使用带 PSRAM、Flash 容量至少 8 MB 的对应板型，连接匹配的显示屏。
- 首次编译需要联网下载组件；Flash / PSRAM 的容量、类型和频率应与实际硬件一致。

在工程根目录执行：

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p COM3 flash monitor
```

ESP32-S3 板将第一条命令改为 `idf.py set-target esp32s3`。将 `COM3` 替换为实际串口；退出监视器使用 `Ctrl+]`。
切换目标芯片会重新生成 `sdkconfig`，需要重新确认板型、屏幕和触摸配置。

### 选择板型、屏幕和触摸

在 `idf.py menuconfig` 中设置：

| 菜单 | 配置内容 |
|------|----------|
| `BSP Board Selection → Board Select` | 选择实际板型 |
| `Display Selection → Display Interface` | 选择 RGB 或 MIPI DSI |
| `Display Selection → RGB LCD Panel / MIPI DSI LCD Panel` | 选择屏幕型号 |
| `Touch Selection → Enable Touch Panel` | 有触摸时启用，默认关闭 |
| `Touch Selection → Touch Controller` | 根据实际芯片选择 GT911 或 AXS15260D |
| `Display Test Demo → Run native LCD test instead of LVGL` | 关闭时运行 HMI，开启时运行纯色 / 灰阶测试 |

显示屏和触摸驱动分别选择。例如 H028A29 可搭配 GT911 或 AXS15260D，切换屏幕型号不会自动切换触摸驱动。
烧录后可从开机日志的 `Board`、`Panel`、`Screen` 确认配置。

## 支持的硬件

### 板型

| 板型 | 主控 | 显示接口 | 其他外设 |
|------|------|----------|----------|
| HTM-H050A29-ESP-StdRGB_V0 | ESP32-S3 | RGB 16-bit | 原生 Wi-Fi、触摸 |
| ESP32P4-DEMO-A02_V0 | ESP32-P4 | RGB 16-bit、MIPI DSI | C6 Wi-Fi、以太网、SD 卡、CSI 摄像头、触摸 |

引脚定义位于 `components/bsp/boards/`。更换显示屏时应核对 FPC 引脚、电源和所需转接板；菜单中的屏幕选项负责选择驱动参数。

### 屏幕型号

| 接口 | 型号 | 分辨率 |
|------|------|--------|
| RGB | H021A04 | 540 × 540 |
| RGB | H034A04（圆屏） | 800 × 800 |
| RGB | H050A29 | 800 × 480 |
| RGB | H070B26 | 1024 × 600 |
| MIPI DSI | H028A29 | 540 × 540 |
| MIPI DSI | H035B16 | 640 × 480 |
| MIPI DSI | H043A8 | 480 × 800 |
| MIPI DSI | H050A11 | 480 × 854 |
| MIPI DSI | H055A01 | 720 × 1280 |
| MIPI DSI | H055A05 | 720 × 1280 |

## HMI 功能

关闭原生 LCD 测试后，启动进入功能菜单。点击卡片进入页面，点击 `Back` 返回。

| 页面 | 使用说明 |
|------|----------|
| Touch | 按住并移动手指画线，显示 X / Y 坐标；`Clear` 清空。保留真实采样轨迹，不做曲线平滑 |
| Wi-Fi | 查看连接状态、IP 和信号强度，扫描和切换无线网络 |
| Ethernet | 查看链路状态、IP、网关和掩码；连接网线后通过 DHCP 获取地址 |
| SD Card | 查看挂载状态、容量和根目录文件列表；需先启用 SD 卡初始化 |
| CSI Camera | 自动检测摄像头并预览；支持暂停、恢复和重新检测，仅适用于 P4 |
| System Info | 查看板型、芯片、固件、IDF 版本、分辨率和内存信息 |
| USB Device | 页面占位，功能尚未实现 |
| USB Host | 页面占位，功能尚未实现 |
| About | 查看公司标识、业务简介、官网、邮箱、固件版本和版权信息 |

当前界面使用英文字体，中文 SSID 等内容可能无法完整显示。

## 无线网络

S3 使用内置 Wi-Fi；P4 通过 `esp_wifi_remote 1.5.1` 和 `esp_hosted 2.12.3` 使用板载 C6。
应用统一调用标准 `esp_wifi_*` API，无需单独启用 Wi-Fi Demo 开关。

当前代码在**首次进入 Wi-Fi 页面时**启动无线服务，尝试连接 `components/wifi/wifi_init.c` 中
`ESP_WIFI_SSID` / `ESP_WIFI_PASS` 定义的默认网络。

切换网络时，先点击 `Disconnect`，再点击 `Scan`，选择网络、输入密码后点击 `Connect`。
隐藏网络可直接填写 SSID；开放网络的密码留空。支持 2.4 GHz 开放网络及 WPA2 / WPA3 Personal。
退出页面后保持连接；GUI 输入的网络信息仅在本次运行中使用，重启后恢复代码中的默认网络。

### P4 与 C6 配置

C6 需烧录与 P4 配套的 **Hosted 2.12.3 SDIO 固件**。DEMO-A02 默认使用 SDMMC Slot 1、4-bit、20 MHz：

| 信号 | P4 引脚（BSP_SD2） | C6 引脚 |
|------|-------------------|---------|
| CLK | GPIO47 | GPIO19 |
| CMD | GPIO46 | GPIO18 |
| D0 / D1 | GPIO48 / GPIO49 | GPIO20 / GPIO21 |
| D2 / D3 | GPIO50 / GPIO51 | GPIO22 / GPIO23 |
| 复位 | IO 扩展器 P00 | EN |

配置入口为 `Component config → ESP-Hosted`，默认值位于 `sdkconfig.defaults.esp32p4`。
P00 复位由工程附带的组件补丁支持，需保持 Hosted 主机深睡眠功能关闭。

在已激活的 ESP-IDF 终端中，先完成 P4 工程编译以下载依赖，再从工程根目录生成并烧录 C6 固件：

```powershell
python tools/build_c6_hosted.py
cd build/c6_hosted_2_12_3
idf.py -B out -p COM7 flash
cd ../..
```

将 `COM7` 替换为 C6 的串口，并将 C6 置于下载模式。脚本按 C6 的 4 MB Flash 配置构建。
使用独立烧录工具时，可将 `build/c6_hosted_2_12_3/out/esp_hosted_2.12.3_esp32c6_sdio_stream.bin` 从地址 `0x0` 烧录。
已有配套固件时无需重复烧录 C6，回到工程根目录烧录 P4 即可。

## 摄像头与 SD 卡

### CSI 摄像头

P4 支持官方 EV Board 配套的 **SC2336 / OV5647** 摄像头，进入页面后自动检测型号。
在 `CSI Camera Demo → Enable CSI camera preview` 中启用，需要 PSRAM。

DEMO-A02 的摄像头与触摸共用 I2C0：**SDA = GPIO27，SCL = GPIO26**。
摄像头接到板上的 CSI 接口，供电和差分线按模块定义连接。

- `Pause` 冻结预览，`Resume` 恢复。
- `Restart` 停止当前会话并重新检测摄像头。
- `Back` 返回主页并释放摄像头资源。

未检测到摄像头或采集超时时，页面显示错误，串口 `CSI_CAMERA` 日志提供具体信息。

### SD 卡

DEMO-A02 使用 BSP_SD1 / SDMMC Slot 0，挂载目录为 `/sdcard`，与 C6 的 Slot 1 分开。
使用前插入 FAT 格式的 SD 卡。

`main/main.c` 在启动时调用 `sdcard_init()` 挂载 SD 卡，SD Card 页面只查询挂载状态和文件。
插卡后重新启动板子，再进入页面查看；当前 S3 板型未定义 SD 卡引脚，不提供该功能。

## 原生 LCD 测试

启用 `Display Test Demo → Run native LCD test instead of LVGL` 后，不启动 HMI、触摸或网络，
按以下顺序循环，每个画面显示 **1 秒**：

**红 → 绿 → 蓝 → 白 → 16 级灰阶 → 图片（可选）**

灰阶横屏为竖条、左黑右白；竖屏为横条、上黑下白。

图片放在 `main/images/`，命名为 `test.png`、`test.jpg`、`test.jpeg` 或 `test.bmp`，只保留一张。
建议使用与所选屏幕相同的分辨率，尺寸不同时会拉伸到全屏；未放图片时自动跳过。
有图片时需在 ESP-IDF 的 Python 环境安装 Pillow：

```bash
python -m pip install Pillow
idf.py build flash
```

添加、更换、删除图片或切换屏幕后，需重新编译并完整烧录，以更新 `lcd_image` 图片分区。
只烧录应用 bin 不会更新图片。关闭该测试选项后恢复 HMI。

## 代码位置与修改入口

| 目录 / 文件 | 用途 |
|-------------|------|
| `main/main.c` | 应用启动和外设初始化顺序 |
| `components/bsp/boards/` | 板型引脚定义 |
| `components/panel/panels/rgb/`、`dsi/` | 屏幕分辨率、时序及 MIPI 初始化命令 |
| `components/display/`、`components/touch/` | 显示和触摸驱动 |
| `components/gui/app/` | 页面导航 |
| `components/gui/ui/` | 页面、主题和控件 |
| `components/gui/service/` | Wi-Fi、摄像头、网络、存储等服务 |
| `components/gui/lv_init.c` | LVGL 显示与触摸接入 |
| `components/wifi/`、`ethernet/`、`sdcard/` | 外设初始化 |
| `cmake/` | 组件兼容补丁和测试图片构建 |

新增屏幕时，在 `components/panel/panels/` 添加参数文件，并更新 `components/panel/panel.h`
和 `components/display/Kconfig.projbuild` 中的选择项。屏幕参数不绑定触摸芯片。

新增页面时，在 `components/gui/ui/` 实现界面，在 `service/` 封装硬件操作，
并更新 `app/`、`ui/ui_launcher.c` 和 `components/gui/CMakeLists.txt`。

LVGL 使用 Registry 的 `esp_lvgl_port`，依赖版本由 `dependencies.lock` 固定。
其他任务调用 LVGL API 时使用 `lvgl_port_lock(0)` / `lvgl_port_unlock()`。
组件兼容补丁在编译时自动应用；不要直接修改 `managed_components`，升级依赖前需核对 `cmake/` 中的版本要求。

## 联系方式

深圳市鑫洪泰电子科技有限公司 · HOTLCD

[产品官网](https://cn.display-lcd.com) · [GitHub](https://github.com/HotDisplay) · [lcd@hotlcd.com](mailto:lcd@hotlcd.com)
