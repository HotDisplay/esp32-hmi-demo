#!/usr/bin/env python3
"""Build the matching ESP-Hosted SDIO coprocessor firmware with ESP-IDF 6.0."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys


VERSION = "2.12.3"
ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "build" / "c6_hosted_2_12_3"


def prepare():
    component = ROOT / "managed_components" / "espressif__esp_hosted"
    manifest = (component / "idf_component.yml").read_text(encoding="utf-8")
    if not re.search(rf"^version: ['\"]?{re.escape(VERSION)}['\"]?$", manifest, re.M):
        raise RuntimeError(f"Resolve the P4 project's esp_hosted {VERSION} dependency first")
    files = {}
    for source, prefix in ((component / "slave", Path()), (component / "common", Path("main/common"))):
        for directory, dirs, names in os.walk(source):
            dirs[:] = [d for d in dirs if d not in ("build", "managed_components", ".git", "__pycache__")]
            for name in names:
                if name in ("sdkconfig", "sdkconfig.old", "dependencies.lock"):
                    continue
                path = Path(directory) / name
                files[(prefix / path.relative_to(source)).as_posix()] = path.read_bytes()
    files["sdkconfig.defaults"] += (
        "\n# Match the verified DEMO-A02 C6 transport. EN is reset by P4.\n"
        "CONFIG_ESP_SDIO_HOST_INTERFACE=y\n"
        "CONFIG_ESP_SDIO_STREAMING_MODE=y\n"
        "CONFIG_ESP_SDIO_HIGH_SPEED=y\n"
        "CONFIG_ESP_SDIO_PSEND_PSAMPLE=y\n"
        "CONFIG_ESP_SDIO_GPIO_RESET=-1\n"
    ).encode("utf-8")
    for name, content in files.items():
        target = PROJECT / name
        if target.exists() and target.read_bytes() != content:
            raise RuntimeError(f"Preserving modified file: {target}")
    for name, content in files.items():
        target = PROJECT / name
        if not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    prepare()
    print(f"Prepared ESP-Hosted {VERSION}: {PROJECT}", flush=True)
    if args.prepare_only:
        return
    idf_path = os.environ.get("IDF_PATH")
    if not idf_path:
        raise RuntimeError("Run this script in an activated ESP-IDF 6.0 environment")
    idf_py = Path(idf_path) / "tools" / "idf.py"
    result = subprocess.run(
        [sys.executable, str(idf_py), "--version"], check=True,
        capture_output=True, text=True,
    )
    if not re.search(r"ESP-IDF v6\.0(?:\s|$|[-.])", result.stdout):
        raise RuntimeError(f"Expected ESP-IDF 6.0, got: {result.stdout.strip()}")
    env = os.environ.copy()
    env["ESP_IDF_VERSION"] = "6.0"
    env["IDF_COMPONENT_CACHE_PATH"] = str(ROOT / "build" / "c6cache")
    subprocess.run(
        [sys.executable, str(idf_py), "-G", "Ninja", "-B", "out", "-D", "IDF_TARGET=esp32c6",
         "-D", f"PROJECT_VER=hosted-{VERSION}", "reconfigure"],
        cwd=PROJECT, env=env, check=True,
    )
    config = (PROJECT / "sdkconfig").read_text(encoding="utf-8")
    for required in (
        "CONFIG_ESP_SDIO_HOST_INTERFACE=y",
        "CONFIG_ESP_SDIO_STREAMING_MODE=y",
        "CONFIG_ESP_SDIO_HIGH_SPEED=y",
        "CONFIG_ESP_SDIO_PSEND_PSAMPLE=y",
        "CONFIG_ESP_WIFI_ENABLE_WPA3_SAE=y",
        'CONFIG_ESPTOOLPY_FLASHSIZE="4MB"',
    ):
        if required not in config.splitlines():
            raise RuntimeError(f"C6 sdkconfig requires {required}")
    subprocess.run(
        ["cmake", "--build", "out", "--parallel", "4"],
        cwd=PROJECT, env=env, check=True,
    )
    image = f"esp_hosted_{VERSION}_esp32c6_sdio_stream.bin"
    subprocess.run(
        [sys.executable, "-m", "esptool", "--chip", "esp32c6", "merge-bin",
         "-o", image, "@flash_args"],
        cwd=PROJECT / "out", env=env, check=True,
    )
    print(f"Merged image (flash at 0x0): {PROJECT / 'out' / image}", flush=True)
    print(f"Flash from {PROJECT}: idf.py -B out -p <C6_PORT> flash", flush=True)


if __name__ == "__main__":
    main()
