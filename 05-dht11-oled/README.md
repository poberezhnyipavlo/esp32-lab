# 05 — DHT11 temperature & humidity on OLED

Reads a DHT11 every 2 s and shows temperature and humidity on a 128×64 SSD1306 OLED
(large font) and in Serial. Sensor glitches are filtered, and stale data is flagged.

<img src="docs/wokwi-oled.png" alt="OLED in Wokwi" width="384">

## Hardware

| Part | Pin | Notes |
|---|---|---|
| ESP32 3V3 / GND | breadboard + / − rails | the board has a single 3V3 pin, so modules share the rails |
| DHT11 module (KY-015): `S` | D4 | middle pin = VCC → +, `−` → GND |
| OLED SSD1306: VDD / GND | + / − rails | |
| OLED SCK / SDA | D22 / D21 | I2C address 0x3C |

DHT11 is powered from **3.3 V only**: the module's pull-up would put 5 V on the GPIO otherwise.

## How it works

- Every 2 s (`millis()` timer): `readTemperature()` + `readHumidity()` — one sensor transaction,
  the second call returns the cached result.
- `ClimateTracker` (Arduino-free, unit-tested) rejects `NaN` and implausible values,
  keeps the last good reading, counts consecutive failures and reports staleness (> 10 s).
- OLED is optional: probed on I2C before `begin()`; without it the firmware runs on Serial only.
- Wokwi has no DHT11, only DHT22: a separate `env:wokwi` builds the same code with
  `-D DHT_SENSOR_TYPE=DHT22`; `#error` stops the build if the type is not set.

## Run

```bash
pio run -e esp32dev -t upload -t monitor
pio test -e native                                                          # 9 tests
pio run -e wokwi && wokwi-cli --timeout 10000 --expect-text 'T=24.0 H=40.0'
```

## Notes from real hardware

- Works: e.g. `25.3 °C / 48 %`, updates every 2 s; this DHT11 also reports tenths of a degree.
- Versus a household hygrometer: +1.6 °C / −11 % RH next to the ESP32. Most of the RH gap is
  the temperature difference (warmer air → lower relative humidity); the rest is within
  DHT11 spec (±2 °C, ±5 % RH).
- Data wire disconnected → `read error (N in a row)`, `read errors: N` on OLED, `STALE` after 10 s;
  reconnected → `OK` again.

## Verified

- [x] Unit tests: 9/9 (`ClimateTracker`)
- [x] Wokwi: DHT22 24 °C / 40 % → `T=24.0 H=40.0`; 85 °C → `read error`
- [x] Real board: DHT11 + OLED, readings on screen, disconnect / reconnect handled
- Size: Flash 323 104 B (24.7%), RAM 23 876 B (7.3%) — wokwi env; esp32dev is the same code
