# 01-blink

First firmware: blink the onboard LED and print a counter to Serial.

## Hardware

| Part | Pin |
|---|---|
| Onboard blue LED | GPIO2 |

Board: ESP32 DevKit V1 (ESP32-WROOM-32, 30 pins, USB-C, CH340C USB-UART bridge).
No external wiring.

## Build, test, flash

```bash
pio run                                                   # build
wokwi-cli --timeout 10000 --expect-text 'Hello from ESP32!'  # smoke test in emulator (exit code 0)
pio run -t upload -t monitor                              # flash + Serial monitor (115200)
```

## Expected result

- Blue LED blinks once per second.
- Serial: `Hello from ESP32!`, then `tick N: LED ON` / `tick N: LED OFF`.
- After pressing EN the counter restarts from 0.

## Firmware size

Flash: 278 400 bytes (21.2%), RAM: 22 252 bytes (6.8%).

## Notes

- `upload_speed = 921600` fails on this board (CH340C on macOS): `No more data to read from the serial port`. 460800 is stable.
- In a `.cpp` file (unlike `.ino`) `#include <Arduino.h>` is required.
- File-level globals live in an anonymous namespace (internal
