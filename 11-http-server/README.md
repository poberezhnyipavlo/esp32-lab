# 11-http-server

A small HTTP API on the ESP32: read device info and change one setting over Wi-Fi, without reflashing.

- `GET /info` returns JSON with firmware, chip, memory, Wi-Fi and the current `monitor_host`.
- `PUT /config/monitor-host` sets a new monitor host (token required). The value is stored in NVS (flash) and survives
  a reboot.

Tested from a Mac with `curl`.

## Hardware

Nothing to wire: bare ESP32 on USB, Wi-Fi is built in. The Mac and the ESP32 are on the same LAN.

## API

| Method and path | Success | Errors |
|---|---|---|
| `GET /info` | `200` JSON | `500` if the response cannot be built |
| `PUT /config/monitor-host` | `200` JSON with `changed` | `401` missing/wrong token, `415` body is not `text/plain`, `400` invalid host, `500` NVS write failed |
| any other method on these paths | | `405` with an `Allow` header |
| any other path | | `404` |

Checks in `PUT` run in a fixed order: token → content type → value. A client without the token learns nothing about
what else is wrong with its request.

```bash
ESP=<board ip>          # printed in Serial: "HTTP server on http://…/info"
read -s TOKEN           # API token from include/secrets.h; read -s keeps it out of shell history

curl -s http://$ESP/info | python3 -m json.tool

curl -i -X PUT -H "X-Api-Token: $TOKEN" -H 'Content-Type: text/plain' \
     --data '<new host>' http://$ESP/config/monitor-host
```

Example `GET /info` (addresses replaced):

```json
{
  "firmware": "11-http-server", "version": "1.0.0", "built": "Oct  7 2026 20:24:26",
  "chip": {"model": "ESP32-D0WD-V3", "revision": 301, "cores": 2, "cpu_mhz": 240, "flash_bytes": 4194304},
  "sdk": {"arduino_core": "3.3.12", "idf": "v5.5.5"},
  "heap": {"free_bytes": 228788, "min_free_bytes": 227868},
  "uptime_s": 6,
  "wifi": {"ip": "<board ip>", "mac": "<board mac>", "rssi_dbm": -50},
  "monitor_host": {"value": "<host>", "source": "firmware"}
}
```

`monitor_host.source` is `firmware` until a `PUT` saves a value, then `nvs`.

### Why `PUT`

`PUT` means "set this resource to this value", so it is idempotent: sending the same request twice changes nothing
the second time. The response says so (`"changed": false`), and the firmware does not rewrite flash in that case.
That makes a retry after a lost response safe.

### `curl --data` and `Content-Type`

Without `-H 'Content-Type: text/plain'`, `curl --data` sends `application/x-www-form-urlencoded`. The ESP32
`WebServer` then parses the body as a form and does not expose it as `plain`, so the API answers `415`. This is
deliberate: the endpoint accepts exactly one body format.

## Security notes

- The token is compared in constant time (same idea as PHP `hash_equals`): the loop always walks the whole string and
  does not exit at the first mismatch. Length is not hidden.
- An empty server token is treated as "not configured" (`500`), so an empty `API_TOKEN` cannot match a request
  without the header.
- This is **not** protection against sniffing. Over plain HTTP the token travels in clear text in every request;
  anyone who can see LAN traffic can read it. Real protection needs TLS.
- The token lives only in `include/secrets.h` (git-ignored) and is never printed to Serial.

## Code structure

```
lib/DeviceApi/      # no Arduino: host validation, constant-time compare, PUT decision, /info JSON
lib/WifiKeeper/     # from 09: connection state machine with backoff
lib/SignalQuality/  # from 09: RSSI -> words
src/main.cpp        # WebServer routes, NVS (Preferences), wiring
test/               # GoogleTest: DeviceApi (13), WifiKeeper (9), SignalQuality (2)
```

All API decisions are in `DeviceApi` and run as unit tests on the Mac. `main.cpp` only reads the request, calls
`DeviceApi` and sends the answer.

## Checks

```bash
pio test -e native                                                          # 24 tests
pio run -e wokwi && wokwi-cli --timeout 20000 --scenario test.scenario.yaml # startup only
```

The Wokwi scenario checks startup, the firmware default host in a clean NVS, Wi-Fi and the server start. HTTP
requests into the simulator are not tested: the API is covered by unit tests (logic) and `curl` on hardware (wiring).

## Results on real hardware

| Request | Result |
|---|---|
| `GET /info` | `200`, 418 bytes of JSON, `source: firmware` |
| `PUT` without token | `401` |
| `PUT` with token, no `Content-Type` | `415` |
| `PUT` with token, host `my host` | `400` |
| `PUT` valid host | `200`, `"changed": true` |
| same `PUT` again | `200`, `"changed": false` |
| `GET /info` after `PUT` | new host, `source: nvs` |
| `POST /info`, `GET /nope` | `405` with `Allow: GET`, `404` |
| EN (reboot), then `GET /info` | host still from `nvs` |

Bugs found while building it, and what caught them:

- **Response buffer sized with the wrong constant** (`k_max_host_len`, 63 bytes, instead of `k_max_info_json`):
  `/info` returned `500`. `buildInfoJson` refuses to return truncated output (`n >= cap`), so the bug showed up as an
  explicit error instead of broken JSON. Unit tests could not see it: the buffer size lives in `main.cpp`.
- **`micros()` instead of `millis()`** for uptime: correct type and format, wrong value (×1000, and it wraps after
  ~71 minutes). Only visible in the data.
- **`sprintf` / `putShort` instead of `snprintf` / `putString`** (autocomplete). The IDE offered
  `reinterpret_cast` to make them compile; that would have turned a buffer size into a pointer and stored a
  truncated address in flash instead of the string.
- **Missing `#include <algorithm>`**: built on the Mac (libc++ pulled it in indirectly), failed with the ESP32
  toolchain (libstdc++). Include what you use.

## Firmware size

Flash ~945 KB (72.1 %), RAM ~47.6 KB (14.5 %). For comparison, 10-udp-sender: 905 372 bytes (69.1 %);
`WebServer` + `Preferences` add about 40 KB.
