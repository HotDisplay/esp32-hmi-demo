#!/usr/bin/env python3
"""Exercise the local C6 reset hook and both patched SDIO reset strategies."""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def extract_function(source, name):
    # Ignore braces in comments and strings while preserving source offsets.
    lexical = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                     lambda match: " " * len(match[0]), source)
    start = re.search(rf"\bint\s+{name}\s*\([^)]*\)\s*\{{", lexical)
    if start is None:
        raise RuntimeError(f"Missing function: {name}")
    depth = 1
    for end in range(start.end(), len(lexical)):
        depth += (lexical[end] == "{") - (lexical[end] == "}")
        if depth == 0:
            return source[start.start():end + 1]
    raise RuntimeError(f"Unbalanced braces: {name}")


HEADERS = {
    "sdkconfig.h": "#define CONFIG_ESP_HOSTED_SDIO_RESET_DELAY_MS 1500\n",
    "freertos/FreeRTOS.h": "#define pdMS_TO_TICKS(ms) (ms)\n",
    "freertos/task.h": "void vTaskDelay(unsigned ticks);\n",
    "esp_check.h": """#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_RETURN_ON_ERROR(expr, tag, ...) do { \\
    esp_err_t err = (expr); if (err != ESP_OK) return err; \\
} while (0)
""",
    "esp_log.h": """#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGE(tag, ...) ((void)(tag))
""",
    "bsp.h": '#include "esp_check.h"\nesp_err_t bsp_wifi_set_reset(int level);\n',
}


HARNESS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"

#define CARD_INIT_TIMEOUT_MS 1500
#define TRANSPORT_RX_ACTIVE 1
#define RESET_ERROR 0x105
#define CARD_ERROR 0x109
static const char *TAG = "RESET_TEST";
static int events[32], event_count, writes, fail_write;
static int card_results[2], card_count, transport_state, cases;
static struct { int read_index, write_index, read_data_len; } double_buf;
static void *sem_double_buf_xfer_data;

static void record(int event) {
    assert(event_count < (int)(sizeof(events) / sizeof(events[0])));
    events[event_count++] = event;
}
esp_err_t bsp_wifi_set_reset(int level) {
    record(100 + level);
    return (++writes == fail_write) ? RESET_ERROR : ESP_OK;
}
void vTaskDelay(unsigned ticks) { record(2000 + (int)ticks); }
static void sleep_ms(unsigned ms) { (void)ms; }
static int get_semaphore(void *sem, unsigned timeout) {
    (void)sem; (void)timeout;
    return -1;
}
static struct {
    void (*_h_msleep)(unsigned);
    int (*_h_get_semaphore)(void *, unsigned);
} functions = { sleep_ms, get_semaphore };
static struct { __typeof__(functions) *funcs; } g_h = { &functions };
static bool esp_hosted_woke_from_power_save(void) { return false; }
static void stop_host_power_save(void) { assert(!"Power save is disabled"); }
static void set_transport_state(int state) { transport_state = state; }
static int transport_card_init(void *handle, unsigned timeout) {
    assert(handle == (void *)1);
    assert(timeout == CARD_INIT_TIMEOUT_MS);
    assert(card_count < 2);
    record(4000);
    return card_results[card_count++];
}
extern esp_err_t board_wifi_reset(void);

/* INSERT_ENSURE_FUNCTION */

static void begin_case(void) {
    memset(events, 0, sizeof(events));
    event_count = writes = fail_write = card_count = transport_state = 0;
    card_results[0] = card_results[1] = ESP_OK;
    ++cases;
}
static void check_reset_sequence(int offset) {
    const int expected[] = { 101, 2010, 100, 2010, 101, 3500 };
    assert(event_count >= offset + 6);
    assert(memcmp(events + offset, expected, sizeof(expected)) == 0);
}

int main(void) {
    begin_case();
    assert(board_wifi_reset() == ESP_OK);
    assert(writes == 3 && event_count == 6);
    check_reset_sequence(0);
    for (int failure = 1; failure <= 3; ++failure) {
        begin_case(); fail_write = failure;
        assert(board_wifi_reset() == RESET_ERROR);
        assert(writes == failure && event_count == failure * 2 - 1);
        assert(events[event_count - 1] == 100 + (failure != 2));
    }

#if H_SLAVE_RESET_ONLY_IF_NECESSARY
    begin_case();
    assert(ensure_slave_bus_ready((void *)1) == ESP_OK);
    assert(card_count == 1 && writes == 0 && event_count == 1);
    assert(transport_state == TRANSPORT_RX_ACTIVE);

    begin_case(); card_results[0] = CARD_ERROR;
    assert(ensure_slave_bus_ready((void *)1) == ESP_OK);
    assert(card_count == 2 && writes == 3 && event_count == 8);
    check_reset_sequence(1);
    assert(events[7] == 4000 && transport_state == TRANSPORT_RX_ACTIVE);

    begin_case(); card_results[0] = card_results[1] = CARD_ERROR;
    assert(ensure_slave_bus_ready((void *)1) == CARD_ERROR);
    assert(card_count == 2 && writes == 3 && transport_state == 0);

    for (int failure = 1; failure <= 3; ++failure) {
        begin_case(); fail_write = failure; card_results[0] = CARD_ERROR;
        assert(ensure_slave_bus_ready((void *)1) == RESET_ERROR);
        assert(card_count == 1 && writes == failure && transport_state == 0);
        assert(event_count == failure * 2);
    }
#else
    begin_case();
    assert(ensure_slave_bus_ready((void *)1) == ESP_OK);
    assert(card_count == 1 && writes == 3 && event_count == 7);
    check_reset_sequence(0);
    assert(events[6] == 4000 && transport_state == TRANSPORT_RX_ACTIVE);

    begin_case(); card_results[0] = CARD_ERROR;
    assert(ensure_slave_bus_ready((void *)1) == CARD_ERROR);
    assert(card_count == 1 && writes == 3 && transport_state == 0);

    for (int failure = 1; failure <= 3; ++failure) {
        begin_case(); fail_write = failure;
        assert(ensure_slave_bus_ready((void *)1) == RESET_ERROR);
        assert(card_count == 0 && writes == failure && transport_state == 0);
        assert(event_count == failure * 2 - 1);
    }
#endif
    printf("%s: %d reset scenarios passed\n",
           H_SLAVE_RESET_ONLY_IF_NECESSARY ? "reset if necessary" : "reset every boot", cases);
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=shutil.which("gcc"), help="Host GCC executable")
    parser.add_argument("--source", type=Path,
                        default=ROOT / "build/esp_hosted_reset_compat/sdio_drv.c")
    args = parser.parse_args()
    if not args.cc:
        parser.error("Set --cc to a host GCC executable")
    if not args.source.is_file():
        parser.error("Configure the P4 project first to generate the patched SDIO source")

    board = ROOT / "components/wifi/board_esp32p4_demo_a02_wifi.c"
    board_code = board.read_text(encoding="utf-8")
    if re.search(r"esp_hosted|host_config|\bg_h\b|constructor", board_code):
        raise RuntimeError("Board reset source still depends on Hosted private APIs")
    ensure = extract_function(args.source.read_text(encoding="utf-8"), "ensure_slave_bus_ready")
    if re.search(r"reset_pin|transport_gpio_reset|release_slave_reset_gpio_post_wakeup", ensure):
        raise RuntimeError("Patched SDIO initialization still resets native GPIO")
    if ensure.count("board_wifi_reset()") != 2:
        raise RuntimeError("Both SDIO reset strategies must call the board hook")

    output = ROOT / "build/wifi_reset_test"
    for name, contents in HEADERS.items():
        header = output / "include" / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text(contents, encoding="utf-8")
    harness = output / "reset_test.c"
    harness.write_text(HARNESS.replace("/* INSERT_ENSURE_FUNCTION */", ensure), encoding="utf-8")
    for strategy in (0, 1):
        executable = output / f"reset_test_{strategy}.exe"
        subprocess.run([args.cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-function", "-Wno-unused-variable",
                        f"-DH_SLAVE_RESET_ONLY_IF_NECESSARY={strategy}",
                        "-I", str(output / "include"), str(board), str(harness),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
