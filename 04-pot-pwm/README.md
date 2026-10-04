# 04 — Potentiometer → LED brightness (ADC + PWM)

Turning a potentiometer smoothly changes LED brightness.
ADC reading is smoothed with an integer EMA and mapped to an 8-bit PWM duty.
Logic lives in two Arduino-free units, unit-tested on the host with GoogleTest.

## Hardware

| Part | ESP32 pin |
|---|---|
| Potentiometer B10K, outer leg | 3V3 |
| Potentiometer, **middle leg (wiper)** | D34 (ADC1_CH6) |
| Potentiometer, other outer leg | GND |
| 220 Ω resistor → LED anode, LED cathode → GND | D18 |

- D34 is on **ADC1**: ADC2 pins stop working when Wi-Fi is on.
- The wiper must never go to 3V3/GND directly: at an end position it would short them.
- Never power the potentiometer from VIN (5 V): ADC pins tolerate 3.3 V max.

## How it works

- Every 20 ms (timer on `millis()`, no `delay()`): `analogRead` → `AdcSmoother` → `adcToDuty` → `ledcWrite`.
- `AdcSmoother` — integer exponential moving average, alpha = 1/8 (`acc += raw - acc >> 3`).
  Converges exactly to the input, no floats.
- `adcToDuty` — `constexpr` linear map with clamping, also checked at compile time via `static_assert`.
- PWM uses the core 3.x LEDC API: `ledcAttach(pin, freq, bits)` / `ledcWrite(pin, duty)`
  (`ledcSetup` / `ledcAttachPin` from older tutorials no longer exist). 5 kHz, 8 bits.
- Only changes of duty are printed to Serial.

## Run

```bash
pio run -e esp32dev -t upload -t monitor
pio test -e native                                                   # 11 tests
pio run && wokwi-cli --timeout 10000 --scenario test.scenario.yaml   # pot 0 → 1 → 0
```

## Expected output

```
04-pot-pwm
Ready
duty=255, raw=4095, smooth=4095
duty=254, raw=4064, smooth=4091
...
```

## Notes from real hardware

- Full range reached: `raw` 0…4095, `duty` 0…255.
- ADC noise on a still knob was only about ±5 LSB on this board (less than typically quoted for ESP32).
- `analogReadMilliVolts` reports ~142 mV at `raw=0`: the ADC cannot resolve the lowest ~0.14 V.
- Smoothing adds visible lag (~1.3 s for a full 0 → 255 swing) — a trade-off between noise and responsiveness.

## Verified

- [x] Unit tests: 11/11 (DutyMap 5, AdcSmoother 6)
- [x] Wokwi scenario passes
- [x] Real board: ESP32 DevKit V1 (CH340C), full 0…255 range, smooth brightness change
- Size: Flash 295 491 B (22.5%), RAM 22 544 B (6.9%)
