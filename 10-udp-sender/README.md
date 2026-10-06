# 10-udp-sender

Every 2 seconds the ESP32 sends three metrics to the System Monitor on a Mac over UDP (port 9000): uptime, Wi-Fi
signal strength and free heap. Each metric is one datagram in a small versioned text format. The receiver parses it
strictly and accepts only the metric names on its whitelist.

## Hardware

Nothing to wire: bare ESP32 on USB (Wi-Fi is built in). The relay module from 08 is disconnected.
The Mac and the ESP32 must be on the same LAN (not a guest network: those usually isolate clients).

## Wire format

```
ver=1 seq=42 name=uptime_s value=123
```

| Field | Meaning |
|---|---|
| `ver` | wire format version; the receiver rejects versions it does not know |
| `seq` | datagram number, +1 for every send attempt (including failed ones) |
| `name` | `[a-z][a-z0-9_]*`, 1–31 characters |
| `value` | finite number, `%.10g` (any `uint32` without loss, `23.5`, `-58`) |

- One metric per datagram. UDP keeps message boundaries, so the trailing `\n` is only for readability in `nc`;
  `parse()` ignores it.
- Strict parsing: exactly four fields in this order, known version, valid name and numbers; anything else is rejected.
- `serialize()` / `parse()` live in `lib/MetricProtocol` with no Arduino dependencies, so the same code is compiled on
  the ESP32 and in the System Monitor.

### Contract vs policy

| | What | Where |
|---|---|---|
| Format (`ver= seq= name= value=`, valid name, finite number) | contract between both sides | `MetricProtocol`, on **both** sides |
| Which names are accepted (`uptime_s`, `rssi_dbm`, `free_heap_bytes`) | receiver policy | whitelist, **System Monitor only** |

The ESP32 only guarantees that what it sends is well-formed (`isValidName()` keeps spaces or `=` from breaking the
line). Whether a metric is welcome is decided by the receiver. A new metric on the device does not touch any
"permission" in the firmware.

The receiver never logs raw input: a rejected datagram is logged by size only.

## UDP: sent ≠ received

`endPacket() == 1` means the network stack accepted the datagram, not that anyone received it. Seen on real hardware:

| Situation | ESP32 log | What the receiver saw |
|---|---|---|
| System Monitor running | `failed=0` | every `seq` in order |
| System Monitor stopped (nobody listening on 9000) | still `failed=0` | nothing; the sender cannot tell |
| ESP32 started before the monitor | `failed=0` from `seq=0` | first message was `seq 9`; 0–8 silently lost |
| Mac got a new IP from DHCP, ESP32 kept sending to the old one | `could not send data: 12` (`ENOMEM`), `failed=3` in most batches | nothing |
| Normal operation | `failed=0` | once `seq 33` was missing between 32 and 34: a real UDP loss |

`ENOMEM` (errno 12) from `sendto()` here does not mean the heap is full (`free_heap_bytes` stayed ~230 KB). Before
sending on the LAN, the stack must resolve the destination MAC address (ARP). Nobody answered for the old IP, the
pending-packet queue filled up, and further datagrams were refused. This is the one case where the sender does learn
about a problem.

Lesson: reserve the Mac's IP in the router (DHCP reservation), or resolve the receiver by name (mDNS) instead of
a hard-coded address.

### Why `seq`

Only `seq` lets the receiver notice loss: a gap means lost datagrams, a jump back to 0 means the device rebooted
(observed after pressing EN). The ESP32 spends a number even when a send fails, so the receiver sees an honest gap.
While Wi-Fi is down nothing is sent and no numbers are spent: after reconnecting, `seq` continued from 351 right after
350.

## Code structure

```
lib/MetricProtocol/   # serialize() / parse() / isValidName(), no Arduino
lib/WifiKeeper/       # from 09: connection state machine with backoff
lib/SignalQuality/    # from 09: RSSI -> words
src/main.cpp          # resolve the receiver once per connection, send 3 datagrams every 2 s
test/                 # GoogleTest: protocol (12), WifiKeeper (9), SignalQuality (2)
```

The receiver address (`MONITOR_HOST`) is kept in `include/secrets.h` (not in git) next to the Wi-Fi credentials.

## Checks

```bash
pio test -e native                                                          # 23 tests
pio run -e wokwi && wokwi-cli --timeout 20000 --scenario test.scenario.yaml # Scenario completed successfully
```

- Protocol tests: exact wire format, large integers without loss, NaN/inf and invalid names rejected, too-small
  buffer, round trip, `\n` / `\r\n`, wrong structure / order / extra fields, unknown version, `seq` overflow,
  non-numeric values.
- Wokwi: the scenario checks connection, receiver resolution and `Sent seq=0..2, failed=0`. It sends to the
  simulator gateway `10.13.37.1`, where nobody listens: delivery is checked on hardware only.
- Wokwi finding: sending to `host.wokwi.internal` (the Mac) resolved to `10.13.37.254`, but the `wokwi-cli` session
  closed right after the first datagram (`API Error: Connection to transport closed unexpectedly: code 1006`, an
  abnormal WebSocket close between the CLI and the Wokwi server). With the gateway address as the target the scenario
  passes every time, which narrows the cause to forwarding UDP to the host.
- Negative check: `'-D WOKWI_MONITOR_HOST="nope.invalid"'` → `Cannot resolve`, the scenario fails (exit 42).

## Results on real hardware

- System Monitor log: `Metric uptime_s = …`, `Metric rssi_dbm = …`, `Metric free_heap_bytes = …` every 2 s, `seq`
  consecutive over several minutes (one loss seen in another run, see above).
- The whitelist caught a real typo on the device: `Rejected: unknown metric 'free_head_bytes'`. The name was
  well-formed, so the sender accepted it; only the receiver's policy noticed.
- A value bug that no test caught: `uptime_s` grew by 20 every 2 s (`now / 100` instead of `/ 1000`). Correct type,
  correct format: only visible in real data. Fixed with a named constant `MS_PER_SECOND`.
- `free_heap_bytes` stable at ~230 KB over several minutes (no leak seen).
- Wi-Fi 2.4 GHz switched off on the router: `Connection lost`, reconnect on attempt 2, `seq` continued without a gap.

## Firmware size

Flash 905 372 bytes (69.1 %), RAM 47 544 bytes (14.5 %). For comparison, 09 (Wi-Fi only): 894 244 (68.2 %); UDP and
the protocol add ~11 KB on top of the Wi-Fi stack.
