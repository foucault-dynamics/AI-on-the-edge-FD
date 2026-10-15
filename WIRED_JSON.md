# Wired JSON (Foucault Dynamics)

A small addition to AI-on-the-edge-device for the Foucault Dynamics CV node: the
cam answers `GET /json` on its UART with the same JSON its `/json` web page
returns, so a board wired to the cam can read it without Wi-Fi.

The node side, the full protocol and the wiring live in the IoT-Firmware repo,
`docs/CV_WIRED_LINK.md`. This page covers the cam side.

**Status: prototype, runs on a cam.** On 2026-10-15 an ESP32-CAM flashed from
this branch answered every `GET /json` on UART0 with a 189 byte frame in about
50 ms, the values matching its `/json` web page. The same build still serves
`/json` over Wi-Fi: a CV node on `cam_link 0` read it as before. Not yet wired
to a node.

## What it does

| | |
|---|---|
| UART | UART0, the console: U0TXD GPIO 1, U0RXD GPIO 3, 115200 8N1 |
| Request | One line, `GET /json`, ended by `\n` (`\r` is ignored) |
| Reply | `0x02`, the JSON from `flowctrl.getJSON()` (exactly what `handler_json` sends), `0x03` |
| Before the flow starts, or another path | `0x02 0x03`, an empty reply |
| Anything that is not a `GET` line | Ignored |

The console keeps printing its log as before. The node only reads what sits
between `0x02` and `0x03`, and log text never contains either byte.

## Files

| File | Change |
|---|---|
| `code/components/jomjol_flowcontroll/WiredJson.cpp`, `WiredJson.h` | New. Installs the UART0 driver, reads request lines in a task, writes the framed reply |
| `code/main/main.cpp` | Calls `StartWiredJsonTask()` after the web handlers are registered |
| `code/platformio.ini` | `-D ENABLE_WIRED_JSON` in `[env:esp32cam]` |
| `code/components/jomjol_flowcontroll/CMakeLists.txt` | `driver` added to `REQUIRES`, for `driver/uart.h` |

Turn it off by deleting the `-D ENABLE_WIRED_JSON` line and rebuilding. Every
line of the feature is inside `#ifdef ENABLE_WIRED_JSON`.

## Build and flash

The same as the stock firmware, from `code/`:

```sh
pio run -e esp32cam                 # build
pio run -e esp32cam -t upload       # flash, with the cam on its ESP32-CAM-MB board
```

At boot the log shows `WIREDJSON: Answering GET /json on UART0 (GPIO 1 TX, GPIO 3 RX)`.

If `-t upload` cannot connect (`Failed to connect to ESP32`), the MB board is
not resetting the cam by itself, as on the board this was tested with: hold
IO0, tap RST on the cam, release IO0, upload, then tap RST. Close any serial
monitor first, or open it with DTR released, or the cam boots back into
download mode.

## Test it on its own

With the cam still on its MB board, the board's USB serial chip already reaches
UART0. From the IoT-Firmware repo:

```sh
python3 CamSim/cam_uart_poll.py /dev/ttyUSB0
```

```text
#1: 374 byte frame in 66 ms: main='531.82235' (no error)
```

A reply of `no flows` means the flow has not started yet. Wait for the first
round to finish.

## Things to know

- **GPIO 1 and 3 belong to the link.** Do not enable `IO1` or `IO3` in the
  `[GPIO]` section of `config.ini`.
- **Take the cam off the MB board when wiring it to the node.** The MB board's
  USB serial chip also drives GPIO 3.
- **Log lines can still land inside a reply** if one is printed by another task
  while the reply is being written. The node then fails to parse that reply and
  asks again on its next poll. A lower console log level makes it rarer.
