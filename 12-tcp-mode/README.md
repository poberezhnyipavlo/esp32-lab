# 12-tcp-mode

The same three metrics as in [10-udp-sender](../10-udp-sender) (uptime, Wi-Fi RSSI, free heap, every 2 s), but over
**one persistent TCP connection** to the Mac, with reconnect and exponential backoff when it breaks. The receiver is
plain `nc`. The wire format comes unchanged from the
[metric-protocol](https://github.com/poberezhnyipavlo/metric-protocol) package, `v1.0.0`.

The firmware deliberately keeps blocking network calls in `loop()` and **measures** how long a single `loop()` pass
takes (`loop max N ms`). The next step (13) moves the network into a FreeRTOS task and compares the numbers.

## Hardware

Nothing to wire: bare ESP32 on USB. The Mac and the ESP32 are on the same LAN.

## Run

```bash
nc -lk 9002                      # on the Mac: -l listen, -k keep listening after a client disconnects
pio run -t upload -t monitor     # include/secrets.h: Wi-Fi and the Mac's IP
```

Port 9002, not 9001: on this Mac `lsof -nP -iTCP:9001 -sTCP:LISTEN` showed the MinIO console already listening on
all interfaces. Without that check the ESP32 would have connected "successfully" to MinIO.

## TCP vs UDP, observed

| | 10 (UDP) | 12 (TCP) |
|---|---|---|
| receiver not running | `failed=0`, the sender never knows | `connect()` fails in ~20–30 ms, the sender knows |
| message boundaries | one datagram per metric | none: a byte stream, lines are split by `\n` |
| order, duplicates | not guaranteed | guaranteed while the connection lives |
| "sent successfully" means | the stack accepted the datagram | the bytes are in the send buffer |
| blocking | none observed | `connect()` up to 3 s, `write()` up to 10 s |

## Experiments

Each row is from the Serial log of the board and, where relevant, `nc` / `netstat` on the Mac.

| # | What I did | What happened |
|---|---|---|
| 1 | `nc` not running | `connect failed after 21–33 ms`, core logs `errno: 104 "Connection reset by peer"` (the Mac answers the SYN with RST). Retries after 1, 2, 4, 8 s |
| 2 | started `nc` | connected on the next attempt, three lines per batch, `seq` consecutive, `loop max` 6–8 ms (≈240 ms on the batch right after `connect()`) |
| 3 | Ctrl+C in `nc` while connected | the board noticed it on the next `loop()` pass as `connection lost` |
| 4 | `nc` again | reconnected on attempt 5 (after the 1-2-4-8 s backoff), `seq` continued |
| 5 | `MONITOR_HOST` set to an IP nobody has | `connect failed after 3003–3006 ms` on every attempt, **no log line from the core** (the timeout is logged at info level). `loop()` stands still for 3 s each time |
| 6 | turned off Wi-Fi on the **Mac** for ~30 s | one `write()` blocked for **~10 s** (`loop max 10028 ms`) and then failed; reconnect attempts timed out after 3 s each while the Mac was away |
| 7 | turned off 2.4 GHz on the router | `write()` failed with `errno: 113 "Software caused connection abort"`, Wi-Fi came back in ~7 s, TCP reconnected in 180 ms, no manual action needed |

### Half-open connection: data acknowledged, never read

After experiment 6 and reflashing the board, the board got stuck in a loop: `connected` → `connection lost` →
`connected`… with occasional `Connection reset by peer` on `write()`. The board was fine; the receiver was not:

```
$ netstat -an -p tcp | grep 9002
tcp4  1619  0  <mac>.9002  <board>.58826  CLOSE_WAIT
tcp4     0  0  <mac>.9002  <board>.64922  ESTABLISHED
tcp4     0  0  *.9002      *.*            LISTEN
```

- `ESTABLISHED` to a board that had been **reflashed**: the new firmware knows nothing about that connection and will
  never send FIN or RST for it. `nc` only reads, has no timeout, and waits forever.
- `nc -lk` serves one client at a time, so the next connection sat in the kernel queue in `CLOSE_WAIT` with
  **1619 bytes (~49 metric lines) acknowledged by the Mac's TCP stack and never read by `nc`**.
- New connections from the board were accepted by the kernel and then reset. Restarting `nc` fixed it immediately.

Two lessons:

1. A TCP ACK means "the receiver's kernel has the bytes", not "the application processed them". If that matters, the
   protocol needs its own application-level acknowledgement.
2. A server that only reads cannot notice a client that vanished without closing (power loss, reset, reflash). A real
   receiver needs a read timeout (the board sends every 2 s, so 10 s of silence = dead connection), should serve
   clients independently, and may use TCP keepalive.

### Where the board blocks

| Cause | Worst `loop()` pass seen |
|---|---|
| `connect()` to an address nobody has | ~3000 ms (the explicit `connect()` timeout) |
| `write()` while the peer silently disappeared | ~10000 ms (the core retries `select()` up to 10 × 1 s) |
| normal operation | 6–8 ms |

During those seconds the board does nothing else. That is the motivation for moving the network into its own
FreeRTOS task in the next project.

## Code structure

```
lib/RetryTimer/     # when to try connect() again: 1, 2, 4 … 30 s; no Arduino, unit-tested
lib/PeakTracker/    # longest loop() pass since the last report
lib/WifiKeeper/     # from 09: Wi-Fi state machine with backoff
lib/SignalQuality/  # from 09: RSSI -> words
src/main.cpp        # one WiFiClient, reconnect, send a batch every 2 s, measure loop()
test/               # GoogleTest: RetryTimer (6), PeakTracker (2), WifiKeeper (9), SignalQuality (2)
```

`RetryTimer` is simpler than `WifiKeeper`: `WiFi.begin()` is asynchronous and needs a "connecting" state with its own
timeout, while `client.connect()` blocks until it has an answer, so only "when is the next attempt" is needed.

Notes from the ESP32 Arduino core 3.3.12 sources (`NetworkClient.cpp`):

- the `connect()` timeout also becomes the socket send/receive timeout;
- `write()` closes the socket itself on an error other than `EAGAIN`;
- `write()` returns the **number of bytes written**, unlike `connect()` / `endPacket()`, which return `1` for success.

## Checks

```bash
pio test -e native                                                          # 19 tests
pio run -e wokwi && wokwi-cli --timeout 30000 --scenario test.scenario.yaml # startup and the first connect attempt
```

## Bug found on hardware

`client.write(buf, len) == 1` instead of `== len`. The first metric of every batch reached `nc`, but the board
counted it as failed, dropped a healthy connection and reconnected every 2 s. Visible only by comparing the sender's
log with what the receiver actually got.

## Firmware size

Flash 911 336 bytes (69.5 %), RAM 47 208 (14.4 %). 10-udp-sender: 905 372 (69.1 %); the TCP client adds about 6 KB.
