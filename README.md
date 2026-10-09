# esp32-lab

A step-by-step learning log in embedded C++ on the ESP32: from blinking an LED to a node that samples data in one
FreeRTOS task and streams it over TCP from another, with a bounded queue in between.

Every folder is a **small, self-contained PlatformIO project** that solves one problem and can be copied as a
reference for "how to do X". The projects do not depend on each other; later ones reuse ideas and classes from
earlier ones by copying and evolving them.

Background: I have been a PHP/Laravel developer for 6+ years and I am moving into C++ and embedded development. This
repository shows how I learn: small steps, tests, measurements on real hardware, and honest notes about what went wrong.

## How each project is done

1. **One goal, one folder.** A short scope agreed up front; anything bigger becomes the next project.
2. **Logic without Arduino.** Classes in `lib/` take time and inputs as parameters and do not call `digitalRead`,
   `millis` or `WiFi` directly, so they are unit-tested on the host with GoogleTest (`pio test -e native`).
3. **Simulation.** [Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started) smoke tests and scenarios (button
   presses, sensor values, serial input, pin levels) run without hardware. Each check is verified to **fail** when the
   code is broken.
4. **Real hardware is the last and mandatory step**, including the negative cases: a disconnected sensor, a stopped
   receiver, Wi-Fi gone.
5. **README with numbers.** What was measured, what broke, firmware size, and bugs found along the way.

## Projects

| # | Project | What it covers | Unit tests | Wokwi | Hardware |
|---|---|---|:---:|:---:|:---:|
| 01 | [blink](01-blink) | toolchain, `setup()`/`loop()`, GPIO output, Serial | | ✅ | ✅ |
| 02 | [oled-hello](02-oled-hello) | I2C, bus scanner, probing a device before trusting a library's `begin()` | | ✅ | ✅ |
| 03 | [button-led](03-button-led) | `INPUT_PULLUP`, debouncing, short/long press as events, first unit tests | ✅ | ✅ | ✅ |
| 04 | [pot-pwm](04-pot-pwm) | ADC, PWM (LEDC), integer exponential smoothing, `constexpr` + `static_assert` | ✅ | ✅ | ✅ |
| 05 | [dht11-oled](05-dht11-oled) | single-wire sensor, data validation and stale readings, multiple build environments | ✅ | ✅ | ✅ |
| 06 | [pir-alarm](06-pir-alarm) | interrupts, ISR rules, `volatile`, critical sections | ✅ | ✅ | ✅ |
| 07 | [light-sensor](07-light-sensor) | thresholds with hysteresis vs. a comparator without it (10 vs. 22 switches) | ✅ | ✅ | ✅ |
| 08 | [relay-basics](08-relay-basics) | active-LOW outputs, safety rules for actuators, parsing serial commands without heap | ✅ | ✅ | ✅ |
| 09 | [wifi-connect](09-wifi-connect) | non-blocking Wi-Fi state machine with backoff, secrets kept out of git | ✅ | ✅ | ✅ |
| 10 | [udp-sender](10-udp-sender) | metrics over UDP, a versioned text protocol, "sent" ≠ "delivered" | ✅ | ✅ | ✅ |
| 11 | [http-server](11-http-server) | small HTTP API, token check in constant time, settings in NVS | ✅ | ✅ | ✅ |
| 12 | [tcp-mode](12-tcp-mode) | persistent TCP with reconnect, measuring how long network calls block, half-open connections | ✅ | ✅ | ✅ |
| 13 | [freertos-tasks](13-freertos-tasks) | sampling and networking in separate FreeRTOS tasks, ring buffer under a mutex, ThreadSanitizer | ✅ | ✅ | ✅ |

## Related repositories

- [metric-protocol](https://github.com/poberezhnyipavlo/metric-protocol): the wire format used from project 10 on,
  extracted into a versioned package. The ESP32 projects pull it with PlatformIO `lib_deps` (`#v2.0.0`), the
  desktop receiver with CMake `FetchContent`. Version 2 adds device id, boot id and sample age.
- **esp32-monitoring-node** (next, in progress): the final device built on these projects. It reads temperature,
  humidity and motion, drives an alert LED and an OLED through an event bus, and has CI with unit tests and linters.

## A few things learned the hard way

- **A library saying "ready" proves little.** The OLED driver's `begin()` returned `true` with no display attached
  (02), so the projects probe the device first.
- **"Sent" is not "delivered".** UDP reports success with the receiver stopped (10). A successful TCP `write()` only
  means the bytes are in a buffer; 52 samples disappeared into an unread socket on a half-open connection (13), and
  the `seq` gap on the receiver is what revealed it.
- **A passing test does not prove there is no race.** With the queue's lock removed, the multi-threaded test still
  passed every run; ThreadSanitizer reported the race on the first one (13).
- **Mac-only builds hide bugs.** A header relied on includes that libc++ pulls in transitively and libstdc++ does not,
  so a tag of the protocol package did not build on Linux. Building on a second platform is now part of releasing.

## Hardware

ESP32-WROOM-32 DevKit (ESP32-D0WD-V3, CH340C, USB-C), MB-102 breadboards, SSD1306 0.96" OLED (I2C), DHT11, HC-SR501
PIR, LM393 + LDR light module, 2-channel 5 V relay module (no mains voltage connected), buttons, LEDs, a potentiometer
and buzzers. Wiring for each project is in its README.

## Toolchain

- [PlatformIO](https://platformio.org/) with the [pioarduino](https://github.com/pioarduino/platform-espressif32)
  platform: Arduino core for ESP32 **3.3.12** (ESP-IDF 5.5)
- C++17 for host-tested code; GoogleTest 1.17 for unit tests
- Wokwi CLI for simulation
- CLion as the IDE

```bash
cd 13-freertos-tasks
pio test -e native               # unit tests on the host
pio run                          # build the firmware
pio run -t upload -t monitor     # flash and open the serial monitor
```

Projects that use Wi-Fi need `include/secrets.h`. Copy `include/secrets.example.h` and fill it in. The real file is
git-ignored.

## Conventions

- Library and platform versions are pinned in `platformio.ini`: PlatformIO has no lock file.
- No secrets in the repository, logs or screenshots.
- One commit per finished project, with its README.
