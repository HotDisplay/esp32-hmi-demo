"""Convert an optional test image to the selected panel's native pixel format."""

import argparse
from pathlib import Path
import re
import struct


def convert(panel_header, image_path, pixel_format, output):
    text = panel_header.read_text(encoding="utf-8")
    dimensions = []
    for name in ("LCD_H_RES", "LCD_V_RES"):
        match = re.search(r"^\s*#define\s+" + name + r"\s+(\d+)\b", text, re.MULTILINE)
        if not match:
            raise ValueError(f"Cannot read {name} from {panel_header}")
        dimensions.append(int(match[1]))
    width, height = dimensions
    pixel_bytes = 2 if pixel_format == "rgb565" else 3
    payload = b""
    if image_path:
        if width * height * pixel_bytes + 24 > 4 * 1024 * 1024:
            raise ValueError("Native image exceeds the 4 MiB lcd_image partition")
        try:
            from PIL import Image, ImageOps
        except ImportError as exc:
            raise ValueError("Install Pillow in the ESP-IDF Python environment: python -m pip install Pillow") from exc
        with Image.open(image_path) as source:
            image = ImageOps.exif_transpose(source).convert("RGB")
            if image.size != (width, height):
                print(f"Resize {image.size} to {(width, height)} (full-screen stretch)")
                image = image.resize((width, height), Image.Resampling.LANCZOS)
            rgb = image.tobytes()
        if pixel_bytes == 2:
            payload = bytearray(width * height * 2)
            for offset in range(0, len(rgb), 3):
                r, g, b = rgb[offset:offset + 3]
                pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                struct.pack_into("<H", payload, offset // 3 * 2, pixel)
        else:
            payload = bytearray(rgb)
            payload[0::3], payload[2::3] = rgb[2::3], rgb[0::3]

    header = struct.pack("<8sIIII", b"LCDTEST1", width, height, pixel_bytes, len(payload))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(header + payload)
    print(f"LCD test image: {width}x{height}, {pixel_format}, {len(payload)} bytes"
          + (" (missing image: skip)" if not image_path else ""))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--panel-header", type=Path, required=True)
    parser.add_argument("--image", default="", help="Optional input image; empty means skip")
    parser.add_argument("--format", choices=("rgb565", "bgr888"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        convert(args.panel_header, Path(args.image) if args.image else None, args.format, args.output)
    except (ValueError, OSError) as exc:
        parser.exit(1, f"LCD test image error: {exc}\n")
