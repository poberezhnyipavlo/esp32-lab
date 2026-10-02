# 02-oled-hello

I2C scanner + "Hello World!" on a 0.96" SSD1306 OLED (128x64).

## Hardware

| OLED pin | ESP32 pin |
|---|---|
| GND | GND |
| VDD (VCC) | 3V3 |
| SCK (SCL) | GPIO22 (D22) |
| SDA | GPIO21 (D21) |

Female-female jumper wires, no breadboard. Pin order on OLED modules differs between vendors: wire by labels, not by position.

## What it does

1. Scans the I2C bus (addresses 1..126) and prints every device that ACKs.
2. Looks for the OLED at 0x3C, then 0x3D (`std::optional`).
3. If not found: prints `OLED not found` and stops (no init, no I2C error spam).
4. If found: initializes the display and prints `Hello World!`.

## Build, test, flash

```bash
pio run
wokwi-cli --timeout 5000 --expect-text 'OLED ready'   # emulator, OLED at 0x3C
pio run -t upload -t monitor                          # real board
```

## Expected result

- With OLED: `Found at address 0x3C`, `OLED ready!`, text on the screen.
- Without OLED: `No I2C devices found.`, `OLED not found ...`, no `[E]` driver errors.

## Firmware size

Flash: 321 344 bytes (24.5%), RAM: 23 812 bytes (7.3%).

## Notes

- Adafruit `SSD1306::begin()` returns `true` even with no display connected
  (it only checks buffer allocation). Probe the address before calling it.
- SSD1306 has no chip ID register: a probe proves "something ACKs at 0x3C", not "this is a display".
- `0x78` printed on the module = 8-bit form of 7-bit address `0x3C`.
- `lib_deps` pins exact versions of SSD1306, GFX and BusIO (PlatformIO has no lock file).
- Text size 1 is ~1 mm tall on this screen; size 2 is comfortable to read.
