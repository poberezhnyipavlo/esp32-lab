# 08-relay-basics

Control a 2-channel relay module from Serial commands (`1 on`, `2 off`, `off`, `status`) so that a relay
**never switches on by itself**: not at boot, not during flashing, not from a burst of commands.

No mains voltage in this project. See [Safety](#safety-no-mains-here).

## Hardware

Module: **HL-52S V1.0**, two Songle `SRD-05VDC-SL-C` relays (5 V coil), optocoupler inputs, `JD-VCC` jumper.

| Module pin | ESP32 | Why |
|---|---|---|
| `JD-VCC` (3-pin header) | **VIN** (5 V), direct wire, not on the breadboard rail | relay coils need 5 V |
| `VCC`, `GND` (3-pin header) | not connected | `VCC` is the same net as on the input header |
| `GND` (input header) | GND (breadboard − rail) | |
| `IN1` | **D26** | channel 1 |
| `IN2` | **D27** | channel 2 |
| `VCC` (input header) | **3V3** (breadboard + rail) | optocoupler side at 3.3 V |

The `JD-VCC`–`VCC` **jumper is removed**.

D26 and D27 are not strapping pins and do not toggle during boot, so the relays stay silent while the ESP32 starts.
All relay wires are on the same (left) header row of the board as VIN.

## Active-LOW and logic levels

On the module, the optocoupler LED sits between `VCC` and `IN`. Current flows only when `IN` is pulled **low**, so
**LOW = relay on, HIGH = relay off** ("active-LOW").

The inversion lives in exactly one place, `Relay::pinLevel()`. The rest of the code (and the tests) think in
"on / off".

With the jumper in place, the whole module runs from 5 V. Then a 3.3 V HIGH from the ESP32 leaves
5 − 3.3 = 1.7 V across the optocoupler LED, which may or may not be enough to keep it off, depending on the module.
With the jumper removed, the input side runs from 3V3 (HIGH = exactly `VCC`, zero current, guaranteed off) and the
coils run from 5 V.

Honest note: the jumper exists for **galvanic isolation** (a separate supply and ground for the coils). Here both
sides share one USB ground, so there is no real isolation. The jumper is removed for **correct logic levels**, not
for isolation.

## Safety rules (in code)

`Relay` enforces three rules per channel:

1. **Turning off is always allowed.** It is the safe action and is never delayed.
2. **Turning on at most once per 500 ms.** A mechanical relay is rated for roughly 100 000 operations; a buggy client
   spamming commands would wear it out in a day.
3. **Auto-off after 10 s.** If whoever turned the relay on disappears (network down, client hung), it does not stay on
   forever.

Limit of rule 3: the timeout runs in the same `loop()`. It protects against a **lost client**, not against a **hung
firmware**. That needs a hardware watchdog or protection in the load itself.

### Fail-safe states

| Moment | ESP32 pin | Relay |
|---|---|---|
| boot, flashing, holding EN | input without pull (high-Z), no current through the optocoupler | off |
| first microseconds after `pinMode(OUTPUT)` | usually LOW | the armature needs milliseconds, it does not react |
| `setup()` | immediately HIGH | off |
| reset while running | high-Z again | releases; stays off after boot (state is not restored on purpose) |
| power loss | — | releases |

So: a load that must switch **off** on failure (heater, pump) goes to **NO**; a load that must switch **on** on failure
(a "power lost" alarm) goes to **NC**.

## Serial commands

```
1 on      2 on
1 off     2 off
off       all channels off
status    state, pin level and switch count per channel
```

The line is read into a fixed `std::array<char, 32>` (no `String`, no heap). A line longer than the buffer is
dropped entirely with `Line too long` instead of being truncated: a truncated command could turn into a different,
valid one.

`parseCommand()` is a pure function (`std::string_view` in, `std::optional<Command>` out) with no Serial dependency,
so the same parser can later take a line from an HTTP request.

## Code structure

```
lib/Relay/          # one channel: active-LOW, min switch interval, auto-off (no Arduino)
lib/RelayCommand/   # parseCommand(): "1 on" -> Command
src/main.cpp        # pins, Serial line reader, prints
test/test_relay/            # GoogleTest, 11 tests
test/test_relay_command/    # GoogleTest, 5 tests
diagram.json        # Wokwi: two LEDs from 3V3 emulate the module's optocoupler inputs
test.scenario.yaml  # Wokwi scenario: write-serial + expect-pin
```

## Checks

```bash
pio test -e native                                                   # 16 tests (11 + 5)
pio run && wokwi-cli --timeout 20000 --scenario test.scenario.yaml   # Scenario completed successfully
```

- **Unit tests:** inactive level at start, active-LOW and active-HIGH mapping, first switch is never "too soon",
  repeated request is `Unchanged`, off is always allowed, on is refused within 500 ms, auto-off exactly at 10 s and
  only once, auto-off counts as a switch, auto-off across `millis()` overflow, command parsing and rejection.
- **The tests can fail:** removing `on &&` from the "too soon" check breaks `OffIsAlwaysAllowed`;
  removing `switched_once_` breaks 6 tests.
- **Wokwi scenario checks pin levels, not only text.** `write-serial` sends commands, `expect-pin` checks GPIO 26/27
  after each one (`{ part-id: esp, name: '26', value: 0 }`). Wokwi's relay part does not model the optocoupler or
  `JD-VCC`, so the module input is emulated with **3V3 → 1 kΩ → LED → GPIO**: the LED is lit exactly when the real relay
  would be on.

## Results on real hardware

- **No click during flashing** or boot.
- `1 on` → relay clicks, `DS1` lights up.
- `Relay 1 auto-off after 10000 ms` → clicks back exactly after 10 s.
- `2 on` → `DS2`.

### Pitfall: terminal keys are bytes

The PlatformIO monitor (`monitor_echo = yes`, `send_on_enter`) has no line editing or history. Pressing ↑ sent
`ESC [ A` (shown as `␛[A`), and the firmware correctly answered `Unknown command`. Arrow keys and Backspace can leave
invisible bytes in a line that *looks* correct. Printing the line as hex (`bytes: 32 20 6F 6E`) shows what was
actually received.

## Firmware size

Flash 279 864 bytes (21.4%), RAM 22 332 bytes (6.8%).

## Safety: no mains here

The relay is rated `10A 250VAC`, but that is the rating of the relay itself, not permission to wire a socket to a
module on a breadboard: the back of the module has exposed solder joints that would carry mains voltage, cheap modules
often have too little clearance between the mains and low-voltage tracks, and there is no enclosure, fuse or earth.
The load in this project is LEDs from 3.3 V.
