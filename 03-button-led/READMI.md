# 03 — Button → LED (debounced, short / long press)

Short press toggles an LED, long press (≥ 1 s) prints `Long press`
and does not toggle. Logic is split into two pure classes with no
Arduino dependency, both unit-tested on the host with GoogleTest:

- `Debouncer` — filters contact bounce, exposes the stable state via `isPressed()`.
- `PressDetector` — turns the stable state + time into events:
  `None`, `Short` (on release, before 1 s), `Long` (once, at 1 s while held).

## Hardware

| Part | ESP32 pin | Breadboard |
|---|---|---|
| Push button (one leg) | D4 | A9 |
| Push button (diagonal leg) | GND | J11 |
| 220 Ω resistor → LED anode (long leg) | D18 | A20 (resistor B20–B24, LED C24) |
| LED cathode (short leg) | GND | A25 (LED C25) |

The button uses `INPUT_PULLUP`, so the pin reads **LOW when pressed**.
No external pull-up resistor is needed.

## How it works

- Each `loop()` iteration: read pin → `Debouncer::update()` →
  `PressDetector::update()` → `switch` on the event.
- Both classes take time as a parameter instead of calling `millis()`,
  which keeps them pure and testable.
- Short press is reported on **release**: at the moment of pressing
  it is not yet known whether the press will be long.
- Elapsed time uses `uint32_t` subtraction, correct across the
  `millis()` overflow (~49 days).
- `PressDetector::update()` is `[[nodiscard]]`: ignoring the event is a bug.
- `loop()` never calls `delay()`.

## Verified

- [x] Unit tests: 11/11 pass (Debouncer 6, PressDetector 5)
- [x] Wokwi scenario passes
- [x] Real board: ESP32 DevKit V1 (CH340C), 6 presses → 6 toggles, no double events

## Run

```bash
pio run -e esp32dev -t upload -t monitor     # real board
pio test -e native                           # unit tests (6)
pio run && wokwi-cli --timeout 10000 --scenario test.scenario.yaml
```

## Expected output
