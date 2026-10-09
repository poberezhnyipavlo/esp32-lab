# 13-freertos-tasks

The firmware from [12-tcp-mode](../12-tcp-mode), split into **two FreeRTOS tasks** with a bounded queue between them:

- `loop()` samples three metrics (uptime, free heap, Wi-Fi RSSI) **exactly every 2 s** and puts them into the queue;
- the network task owns Wi-Fi and the TCP connection and sends from the queue, oldest first.

When the link is down, samples wait in the queue (up to 1000 entries, ~11 minutes) and later arrive at the receiver
as one batch with a large `age_ms`. Sampling no longer depends on the network: the 3 s `connect()` and 10 s `write()`
blocking measured in 12 now stall only the network task. The wire format is
[metric-protocol](https://github.com/poberezhnyipavlo/metric-protocol) `v2.0.0`:

```
ver=2 dev=20:9b:a9:61:15:6c boot=9d208525 seq=50 age_ms=7043 name=rssi_dbm value=-51
```

## Hardware

Nothing to wire: bare ESP32 on USB, the Mac and the ESP32 on the same LAN.

## Run

```bash
nc -lk 9002                      # on the Mac
pio run -t upload -t monitor     # include/secrets.h: Wi-Fi and the Mac's address
```

Restart `nc` before every upload: a reflashed board leaves a half-open connection behind (see 12), and `nc -lk`
serves one client at a time.

## Architecture

```
loop() (core 1) ── every 2 s ──▶ [ queue: oldest … newest ] ──▶ net task (core 0) ──▶ TCP ──▶ receiver
                                          ▲                              │
                                          └── write failed: the entry ───┘
                                              stays first in the queue
```

| Who | Owns | Shares |
|---|---|---|
| `loop()` (`loopTask`, priority 1, core 1) | the sampling clock (`vTaskDelayUntil`), `seq` | pushes into `SampleQueue` |
| net task (8 KB stack, priority 1, core 0) | `WiFi`, `WiFiClient`, reconnect timers, `dev` / `boot` | reads `SampleQueue`, writes RSSI into a `std::atomic<int32_t>` |

Rules the code follows:

- **One owner for network objects.** Nothing outside the net task touches `WiFi` or `WiFiClient`.
- **No lock held during I/O.** `SampleQueue` locks a `std::mutex` only for the buffer operation; `write()` runs
  without it, so a 10 s blocking `write()` cannot stall sampling.
- **Peek, send, then pop.** The sender takes the oldest entry with `front()`, removes it with `popFront(id)` only after
  `write()` returned the full length, and stops at the first failure. No separate retry list: a failed entry is still
  first, so order is preserved by construction.
- **Queue full → drop the oldest.** Fresh data is worth more. The rule lives in one place (`RingBuffer::push`) and
  every eviction is counted (`dropped=`).
- **Entry ids against "removed the wrong one".** Between `front()` and `popFront()` the producer may overflow the
  buffer and evict the head. `popFront(id)` removes only if the head still has that id, so a stale call is a no-op
  instead of silently deleting a newer sample.
- **`seq` is assigned when sampling**, not when sending. A retried sample keeps its number, so a gap on the receiver
  always means a lost sample, never a renumbering.

## Before / after

| Situation | 12 (single loop) | 13.2 (two tasks + queue) |
|---|---|---|
| Sampling interval, normal | ~2000 ms | 2000 ms |
| Receiver stopped | board stuck in `connect()` | interval stays 2000 ms; connect refused in ~20 ms, backoff 1 → 2 → 4 → 8 → 16 s; queue grows |
| Mac Wi-Fi off | `write()` blocked the board for ~10 s | net task blocked ~12 s, then a 3 s connect timeout; sampling unaffected |
| After recovery | samples from the outage lost | backlog delivered in order, `age_ms` decreasing in 2 s steps |
| Queue full | — | oldest dropped and counted; `dropped` equals the `seq` gap on the receiver |
| Net task stack | — | minimum free 5884 of 8192 bytes |

## Experiments

| # | What I did | What happened |
|---|---|---|
| 1 | normal run | `[app] sampled … interval=2000 ms`, `[net] sent 3, max age_ms=15–33`, `seq` consecutive in `nc` |
| 2 | Ctrl+C in `nc`, started it again | `connect failed after 21 ms` (RST, `errno 104`), backoff 1–16 s, `interval=2000` throughout; on reconnect the whole queue arrived as one batch |
| 3 | Mac Wi-Fi off for a few seconds | `write()` blocked the net task for ~12 s, then `connect failed after 3007 ms` (timeout: nobody answers, unlike the RST in #2). Not a single sampling interval changed |
| 4 | `QUEUE_CAPACITY = 10`, receiver stopped for ~30 s | 45 samples taken (`seq` 15…59): 10 delivered (`seq` 50…59, first `age_ms=7043`), `dropped=35`, and the receiver saw exactly the gap `seq` 15…49 |
| 5 | reflash | `boot` changed (`992ff077` → `9d208525`), `seq` restarted at 0 |
| 6 | `net_stack_free` over all runs | minimum 5884 of 8192 bytes, so ~2.3 KB used. `MONITOR_HOST` was an IP literal, so no DNS lookup was exercised |

Reading a batch after an outage (from experiment 2):

```
seq=111 age_ms=29035 name=uptime_s value=76
seq=114 age_ms=27054 name=uptime_s value=78
seq=117 age_ms=25071 name=uptime_s value=80
…
seq=156 age_ms=10    name=uptime_s value=106
```

The receiver computes the sample time as `receive_time − age_ms`, so the board needs no clock.

### What a queue does not fix

The queue protects a sample **until `write()`**. After that it is TCP's problem, and a successful `write()` only means
"the bytes are in the send buffer". Observed during experiment 3:

1. The Mac dropped off Wi-Fi; the board timed out, closed the socket, and reconnected after the Mac came back.
2. The FIN/RST for the old connection never reached the Mac, so `nc` was still sitting on it (half-open, as in 12).
3. The Mac's kernel accepted the new connection and buffered the data, but `nc -lk` serves one client at a time and
   never called `accept()` on it.
4. The board saw successful writes and popped **52 samples (`seq` 206…257)** that were never read. They disappeared
   when `nc` was restarted.

The `seq` gap 205 → 258 within one `boot` made the loss visible. Making it impossible needs an application-level
acknowledgement (`ack seq=N` from the receiver, pop only after it) and a receiver with read timeouts that serves
clients independently. That belongs to the receiver, not to this project. The queue also lives in RAM and is lost on
reset.

## Code structure

```
lib/RingBuffer/     # fixed-size FIFO, drop-oldest on overflow, entry ids; not thread-safe, no Arduino
lib/SampleQueue/    # Sample + RingBuffer behind a std::mutex; the only object both tasks touch
lib/RetryTimer/     # from 12: next connect() attempt after 1, 2, 4 … 30 s
lib/WifiKeeper/     # from 09: Wi-Fi state machine with backoff
lib/SignalQuality/  # from 09: RSSI -> words
src/main.cpp        # loop() samples; class Uplink in the net task owns Wi-Fi, TCP and sending
test/               # GoogleTest: RingBuffer (6), SampleQueue (2), RetryTimer (6), WifiKeeper (9), SignalQuality (2)
```

`RingBuffer` index math: the next free slot is `(head + count) % N`, popping moves `head = (head + 1) % N`. The id of
the head (`first_id`) only grows and is unrelated to the slot index.

## Checks

```bash
pio test -e native                                                          # 25 tests
pio run -e wokwi && wokwi-cli --timeout 30000 --scenario test.scenario.yaml # sampling continues while TCP fails
```

`SampleQueue` has a producer/consumer test with two `std::thread`s and 200 000 entries: order is preserved and
`received + dropped == pushed`. That test **still passes with the lock removed**: the race exists but rarely shows.
ThreadSanitizer catches it on the first run:

```bash
GT=.pio/libdeps/native/googletest/googletest
clang++ -std=c++17 -g -O1 -fsanitize=thread -I$GT/include -I$GT -Ilib/RingBuffer -Ilib/SampleQueue \
  test/test_sample_queue/test_main.cpp $GT/src/gtest-all.cc -o /tmp/tsan_queue && /tmp/tsan_queue
```

With the lock: no warnings. Without it: `WARNING: ThreadSanitizer: data race`.

## Bugs found on the way

- **`RingBuffer::push` wrote to `(head + first_id) % N`** instead of `(head + count) % N`. Until the first pop
  `first_id` is 0, so every push overwrote the same slot. Caught by the unit tests: ids and sizes were right, only the
  values were wrong.
- **`age_ms` computed with `micros()`**: `age_ms=3764682` at 3.7 s uptime. Visible only on hardware, as a value
  1000× too large.
- **Two `nc` listeners on port 9002.** macOS `nc` sets `SO_REUSEPORT`, so a second `nc -lk 9002` starts without
  "address in use" and the kernel splits connections between them. A leftover listener from an earlier session
  accepted connections and reset them on write. `lsof -nP -iTCP:9002` showed two PIDs.

## Firmware size

Flash 916 156 bytes (69.9 %), RAM 71 616 (21.9 %). 12-tcp-mode: RAM 47 208. The difference (~24 KB) is the queue:
1000 entries × 24 bytes in `.bss`, visible at build time. With `QUEUE_CAPACITY = 10` RAM was 47 856.
