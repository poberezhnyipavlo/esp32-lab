# 06 — PIR motion alarm (interrupts)

HC-SR501 detects motion → a GPIO **interrupt** fires → the buzzer beeps
(200 ms on / 200 ms off) for 3 s and the on-board LED blinks with it.
Motion during the sensor's 30 s warm-up is ignored.

## Hardware

| Part | ESP32 pin | Notes |
|---|---|---|
| HC-SR501 VCC | VIN (5 V) | direct wire, **not** the breadboard 3V3 rail |
| HC-SR501 GND | GND | |
| HC-SR501 OUT | D4 | output is 3.3 V (on-board regulator) — safe for the GPIO |
| Active buzzer + / − | D18 / GND | direct drive (~15–30 mA); a transistor is the proper way |
| Indicator | D2 | on-board LED |

PIR set to minimum Time (~3 s) and minimum Sensitivity for desk testing.

## Interrupt design

- `attachInterrupt(..., RISING)` — one event per start of motion.
- The ISR is minimal: sets a flag and a counter, nothing else
  (no `Serial`, no `delay`, no library calls). All logic runs in `loop()`.
- `IRAM_ATTR` — the ISR lives in RAM, so it works while flash cache is disabled.
- Shared data is `volatile` **and** accessed inside `portENTER_CRITICAL` / `portMUX`:
  `volatile` stops the compiler from caching the value, but does not make
  "read flag + clear flag" atomic. The mux is also a spinlock between the two cores.
- `x = x + 1` instead of `++x`: increment of a `volatile` is deprecated in C++20
  (the ESP32 core builds with `gnu++2b`).

## Logic (`MotionAlarm`, Arduino-free, 8 unit tests)

Warm-up, 3 s alarm restarted by new motion, beep pattern computed from time
(`(elapsed / 200) % 2`), event counter, `millis()` overflow handled.

## Testing in Wokwi

Wokwi's PIR can only be triggered by a mouse click, not from a scenario,
so the PIR output is emulated by a button between 3V3 and D4 (`bounce: 0`).
The firmware sees the same signal: a rising edge on D4. A separate `env:wokwi`
shortens warm-up to 2 s via `-D PIR_WARMUP_MS=2000`.

```bash
pio test -e native                                                    # 8 tests
pio run -e wokwi && wokwi-cli --timeout 15000 --scenario test.scenario.yaml
pio run -e esp32dev -t upload -t monitor
```

## Notes from real hardware

- `interrupts` == `events` every time: HC-SR501 produces clean edges, no bounce.
- Repeated motion within 3 s restarts the alarm (events without `Alarm off` in between).
- The buzzer's polarity was not obvious from leg length; verified by powering it from 3V3 directly.
- The PIR also worked from 3V3 (out of spec, ≥ 4.5 V) — VIN is used as specified.

## Verified

- [x] Unit tests: 8/8 (`MotionAlarm`)
- [x] Wokwi scenario: motion ignored during warm-up, alarm after, `Alarm off`
- [x] Real board: motion → interrupt → 3 s beeping, LED in sync
- Size: Flash 281 204 B (21.5%), RAM 22 772 B (6.9%)
