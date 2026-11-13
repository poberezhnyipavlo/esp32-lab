# 15-power-notify

ESP32 on a plain USB wall charger (no UPS, no power bank) sends **one Telegram message when mains power comes back** after a scheduled outage. My smart bulb is set not to turn on after a power cut, so without this I don't notice the power is back.

## How it works

```
power on → reset reason → Wi-Fi (retry with backoff) → HTTPS POST sendMessage (retry with backoff) → Wi-Fi off, idle
```

- Message text depends on `esp_reset_reason()`:
  - `POWERON` / `BROWNOUT` → `💡 Світло є` (power flicker can show up as `BROWNOUT`);
  - anything else (`PANIC`, `TASK_WDT`, …) → `⚠️ Плата перезапустилась: <reason>`.
  - On ESP32 the **EN button also gives `POWERON`** (`rst:0x1`), so EN is a handy way to simulate power coming back.
- Wi-Fi: `WifiKeeper` copied unchanged from [09-wifi-connect](../09-wifi-connect) (timeout 15 s, backoff 1…30 s).
- Telegram Bot API without libraries: `HTTPClient` + `NetworkClientSecure`, JSON body.
- No NTP: Telegram shows the time itself.

## TLS: why `useBuiltinCACertBundle()`

| | `setInsecure()` | `setCACert(root)` | `useBuiltinCACertBundle()` |
|---|---|---|---|
| Verifies server | no (MITM sees the bot token) | yes, one pinned root | yes, Mozilla root set from ESP-IDF |
| Telegram changes CA | works | **breaks until reflash** | works if the new CA is common |
| Flash (measured) | 1 036 832 B | ~+1–2 KB | 1 107 536 B (**+~70 KB**) |

A device that must work at a rare moment months from now should not silently break on a CA change, and 70 KB of flash is available, so the bundle wins. Unlike PHP/curl, the roots are **compiled into the firmware** and never update by themselves.

Honest limit: without NTP the clock starts in 1970 and the default mbedTLS config does not check certificate dates, so an expired certificate would be accepted (signatures are still fully verified).

## Retry policy (`lib/SendPolicy`)

| `POST` result | Action |
|---|---|
| `2xx` | done → Wi-Fi off |
| `< 0` (DNS / TCP / TLS / timeout) | retry |
| `429`, `5xx` | retry |
| other `4xx` (bad token `401`, `400 chat not found`) | give up, log Telegram's `description` |

- Backoff: 5 s → 10 → 20 → … → 120 s cap, no attempt limit. First attempt immediately.
- `429`: `retry_after` is ignored (simplification; one message per outage).
- Timeouts: TLS handshake 15 s (core default is **120 s**), HTTP connect / read 10 s.
- Unit tests: `pio test -e native` → 8/8.

## Secrets

`include/secrets.h` (in `.gitignore`) holds Wi-Fi and `TELEGRAM_BOT_TOKEN` / `TELEGRAM_CHAT_ID`; the repo has `include/secrets.example.h`. The token is compiled into `firmware.bin`, so the `.bin` must not be published either. The URL with the token is never printed.

Getting the chat id: create a bot with @BotFather, press Start in the bot, then

```bash
read -rs TG_TOKEN && export TG_TOKEN     # token not stored in shell history
curl -s "https://api.telegram.org/bot${TG_TOKEN}/getUpdates" | python3 -m json.tool | grep -A4 '"chat"'
```

## Checks

- `pio run`: Flash 84.5 % (1 107 536 B), RAM 15.0 %, no warnings.
- `pio test -e native`: 8/8.
- Wokwi (`env:wokwi`): real TLS handshake with `api.telegram.org` through `Wokwi-GUEST` with a **fake** token → expects `Telegram rejected the request: HTTP 4xx`; the real token never goes through the public Wokwi gateway.

## Results on hardware (spare board, ESP32-D0WD-V3 rev 3.1)

| Case | Result |
|---|---|
| Normal start | `Connected` (RSSI −50…−59 dBm) → `Sent`, message on the phone |
| EN button | `rst:0x1 (POWERON_RESET)` → `💡 Світло є` |
| Wrong token | `401` → give up |
| Wrong chat id | `400 Bad Request: chat not found` → give up |
| Broken JSON body | `400 Bad Request: message text is empty` (misleading: Telegram reports what it parsed, not what was sent) |
| Wi-Fi up, no internet (WAN cable out) | `DNS Failed … -54` → HTTPClient `-1 (connection refused)`, ~10 s per attempt, retries 5 s / 10 s, `Sent` once internet is back |
| Router off | `Wi-Fi timeout`, retries 1/2/4/8 s → `Connected` → `Sent` |
| Wall charger unplugged and plugged back | message on the phone |

Findings:
- **HTTPClient reports a DNS failure as `-1 connection refused`.** After the DNS failure the client still waits the full connect timeout, hence ~10 s per attempt.
- **Backoff must start after the attempt, not before it.** The first version passed the `millis()` captured at the start of `loop()`; a 10 s attempt "ate" the pause, and the 10 s retry started after 17 ms. Fixed with `onFailure(millis())`. Unit tests could not catch it: in tests time does not move during an attempt.
- `E wifi:timeout when WiFi un-init` after `WIFI_OFF`: harmless, the message is already sent.

## Ideas for version 2

- Heartbeat to [healthchecks.io](https://healthchecks.io) every minute → "power lost" notification when nobody is home.
- Show "trying / sent / gave up" with the on-board LED (GPIO2) or the spare OLED, so a bad token is visible without Serial.
- Honour `retry_after` for `429`.
