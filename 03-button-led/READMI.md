# 03 — Button → LED (debounced)

Press the button to toggle an LED. The button is debounced in software
by a small `Debouncer` class that has no Arduino dependency, so it is
unit-tested on the host with GoogleTest.

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

- `Debouncer::update(raw, nowMs)` accepts a new state only after it has
  been stable for 50 ms, and returns `true` exactly once per press.
- Time is passed in as a parameter instead of calling `millis()` inside,
  which keeps the class pure and testable.
- Elapsed time is computed as `now - lastChange` on `uint32_t`, which
  stays correct across the `millis()` overflow (~49 days).
- `loop()` never calls `delay()`.

## Run

```bash
pio run -e esp32dev -t upload -t monitor     # real board
pio test -e native                           # unit tests (6)
pio run && wokwi-cli --timeout 10000 --scenario test.scenario.yaml
```

## Expected output
