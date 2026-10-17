"""
@file convert_gui_logos.py
@brief 将公司 PNG 标识转换为首页使用的 LVGL 9 ARGB8888 静态图片资源。

@copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
@author Lee
@date 2026-10-17

原图保存在 components/gui/assets，转换仅调整显示尺寸和像素格式。
在工程根目录运行 python tools/convert_gui_logos.py，需要 Pillow。
"""

from pathlib import Path
from datetime import date

from PIL import Image


ASSETS = Path(__file__).resolve().parents[1] / "components/gui/assets"


def convert(name: str, source: str, width: int) -> None:
    """按原始比例缩小标识，并输出带透明度的 LVGL 原生图片描述符。"""
    with Image.open(ASSETS / source) as original:
        height = round(original.height * width / original.width)
        image = original.convert("RGBA").resize((width, height), Image.Resampling.LANCZOS)
    pixels = image.tobytes("raw", "BGRA")
    lines = ["    " + ", ".join(f"0x{byte:02x}" for byte in pixels[i:i + 24]) + ","
             for i in range(0, len(pixels), 24)]
    header = f"""/**
 * @file {name}.c
 * @brief 首页品牌图片资源，由 tools/convert_gui_logos.py 根据 {source} 生成。
 *
 * @copyright Copyright (c) 2026 Hot Display Technology Co.,Ltd
 * @author Lee
 * @date {date.today().isoformat()}
 */
#include "ui_brand_assets.h"

LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST static const uint8_t {name}_pixels[] = {{
"""
    descriptor = f"""
}};

const lv_image_dsc_t {name} = {{
    .header = {{
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_ARGB8888,
        .w = {width},
        .h = {height},
        .stride = {width * 4},
    }},
    .data_size = sizeof({name}_pixels),
    .data = {name}_pixels,
}};
"""
    (ASSETS / f"{name}.c").write_text(header + "\n".join(lines) + descriptor, encoding="utf-8")
    print(f"{name}: {width}x{height}, {len(pixels)} bytes")


if __name__ == "__main__":
    convert("ui_logo_wide", "logo_wide.png", 360)
    convert("ui_logo_compact", "logo_compact.png", 168)
