# 07-light-sensor

A night light: when it gets dark, the ESP32 turns on an LED; when it gets light again, it turns it off.
The switching is clean, with no flicker at the boundary, thanks to **hysteresis** (a software Schmitt trigger).

The module's own hardware comparator (LM393, `DO` pin) is read in parallel, so the log shows
both approaches side by side on the same light changes.

## Hardware

| Part | Pin | ESP32 |
|---|---|---|
| LDR module (LM393, 4 pins) `VCC` | power | **3V3** (breadboard + rail) |
| `GND` | ground | GND (breadboard − rail) |
| `AO` (analog) | ADC1 | **D34** |
| `DO` (comparator output) | digital input | **D35** |
| Night light | built-in blue LED | **D2** |

⚠️ The module has no voltage regulator: `AO` and `DO` swing up to `VCC`.
Power it from **3V3 only**; at 5 V the outputs would exceed the ESP32's 3.3 V input limit.

GPIO 34–39 are input-only and have **no internal pull-ups/pull-downs**.

## How it works

The module is a voltage divider (LDR + 10 kΩ). Darker → higher LDR resistance → **higher `raw`**
(verified on hardware).

A single threshold (`lamp = raw > 2250`) flickers when the signal is noisy and sits near the threshold.
Two thresholds fix this:

```
         raw ▲
             │        ON ──────────────┐
     2500 ───┼──── ▲ (turn on here)    │
             │     │   band: state     │
     2000 ───┼─────┼── does not change ▼ (turn off here)
             │  OFF┘                  OFF
             └───────────────────────────────▶
```

- turn **on** when `raw > 2500` (really dark);
- turn **off** when `raw < 2000` (really light);
- inside the band, keep the current state.

The same `raw = 2250` means ON if we came from darkness and OFF if we came from light:
the output depends on history, so it is an object with state, not a pure function.

### Hysteresis vs smoothing vs debounce

| | Filters | Cost | Used in |
|---|---|---|---|
| Smoothing (EMA) | noise in the **value** | latency (~1.3 s in 04); a smoothed signal near the threshold can still cross it | [04-pot-pwm](../04-pot-pwm) |
| Debounce | changes shorter than N ms (**time**) | N ms latency | [03-button-led](../03-button-led) |
| **Hysteresis** | small changes in the **value** | no latency; thresholds are shifted apart | this project |

## Code structure

```
lib/HysteresisSwitch/   # Schmitt trigger, no Arduino dependencies
src/main.cpp            # reads AO/DO, drives the LED, prints status every 500 ms
test/                   # GoogleTest unit tests (run on the host)
diagram.json            # Wokwi: ESP32 + wokwi-photoresistor-sensor
test.scenario.yaml      # Wokwi scenario: sets lux, checks the lamp state
```

`HysteresisSwitch::update(value)` returns a `SwitchEvent` (`None` / `TurnedOn` / `TurnedOff`) once, at the
moment of change; `main` only reacts via `switch`. The thresholds are checked at compile time:
`static_assert(LAMP_OFF_BELOW <= LAMP_ON_ABOVE, ...)`.

## Serial output

```
07-light-sensor
Thresholds: ON above 2500, OFF below 2000
Ready
raw=1637, do=0, lamp=OFF, lamp_changes=0, do_changes=0
Lamp ON (raw=2503)
raw=3236, do=1, lamp=ON, lamp_changes=1, do_changes=7
Lamp OFF (raw=1995)
```

## Checks

```bash
pio test -e native                                                   # unit tests (8)
pio run && wokwi-cli --timeout 15000 --scenario test.scenario.yaml   # Scenario completed successfully
```

- **Unit tests (8):** thresholds with strict `>` / `<`, one event per change, same value depends on history,
  noise inside the band does not toggle, and a test that documents the problem: with zero band,
  5 noisy values cause 4 toggles.
- **Wokwi scenario:** the photoresistor's `lux` is set from the scenario (500 → 20 → 70 → 300 → 70).
  The same 70 lux (`raw≈2299`, inside the band) must give `ON` after darkness and `OFF` after light.
- **The scenario can fail in the right place:** with both thresholds set to 2250 (no hysteresis),
  it passes every step except the last one and times out (exit code 42).

## Results on real hardware

Calibration (`raw`, 0…4095):

| Condition | `raw` |
|---|---|
| room light | ~1600…1700 (noise about ±40) |
| half covered | ~2150 (inside the band, no change) |
| covered with a T-shirt | ~3150…3260 |
| fully covered | 4095 (ADC ceiling) |

The thresholds 2000 / 2500 were confirmed by measurement: the band is ~6× wider than the noise,
and room light is ~300 below the OFF threshold.

**Software vs hardware threshold, on the same 10 light transitions:**

| | Changes |
|---|---|
| `HysteresisSwitch` (our lamp) | **10** (exactly one per transition, at `raw=2501…2505` / `1992…1996`) |
| LM393 comparator (`DO`) | **22** |

On slow transitions the comparator toggled several times (e.g. 1 → 6 in one transition), while the lamp
switched once. When the light stayed inside the band (`raw=2265`), `DO` toggled 3 times and the lamp
did not change at all. The comparator on this module has no hysteresis.

`do_changes` is counted by polling in `loop()`, so it is a lower bound: very short glitches can be missed.

### Negative case: sensor disconnected

With `AO` unplugged, `raw` reads **0**, which the firmware treats as "very bright": the night light silently
never turns on, and nothing appears in the log. A floating ADC input is not guaranteed to read 0,
it can drift with noise or touch.

A reliable fix is a high-value resistor (~1 MΩ) from D34 to GND or 3V3: the module's 10 kΩ divider overrides it
when connected, and a broken wire gives a predictable `0` or `4095`. Pulling up makes a broken sensor visible
(lamp on); pulling down hides it (lamp off). Telling "wire fell off" from "direct sunlight" by `raw` alone is
still impossible: both read near 0.

## Firmware size

Flash 285 336 bytes (21.8%), RAM 22 356 bytes (6.8%).

## Notes and pitfalls

- `[E][esp32-hal-gpio.c] IO 2 is not set as GPIO` → a missing `pinMode(LAMP_PIN, OUTPUT)`.
  The Wokwi scenario still passed, because it only checks Serial: read the whole log, not just the last line.
- A scenario timeout at exactly 5 / 10 / 15 s revealed `STATUS_PERIOD_MS = 5000` instead of `500`.
  Fix the typo instead of increasing `--timeout`.
- Use a separator after each value (`lamp=ON,`) so `wait-serial` does not match a neighbouring line.
- `Lamp ON` (event) and `lamp=ON` (state) are different strings: Wokwi matching is case-sensitive.
