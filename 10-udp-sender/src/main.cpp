#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <array>
#include <optional>

#include "MetricProtocol.h"
#include "SignalQuality.h"
#include "WifiKeeper.h"

#if defined(WOKWI_WIFI)
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
#ifndef WOKWI_MONITOR_HOST
#define WOKWI_MONITOR_HOST "host.wokwi.internal"
#endif
constexpr char MONITOR_HOST[] = WOKWI_MONITOR_HOST;
#elif __has_include("secrets.h")
#include "secrets.h"
constexpr int32_t WIFI_CHANNEL = 0;
#else
#error "No include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill it in"
#endif

namespace {
  constexpr uint16_t MONITOR_PORT = 9000;
  constexpr uint32_t SEND_PERIOD_MS = 2000;
  constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
  constexpr uint32_t BACKOFF_BASE_MS = 1000;
  constexpr uint32_t BACKOFF_MAX_MS = 30000;

  WifiKeeper keeper(CONNECT_TIMEOUT_MS, BACKOFF_BASE_MS, BACKOFF_MAX_MS);
  bool beganOnce = false;

  WiFiUDP upd;
  std::optional<IPAddress> monitorIp;
  uint32_t seq = 0;
  uint32_t lastSendMs = 0;
  uint32_t sentOk = 0;
  uint32_t sendFailed = 0;

  void resolveMonitor() {
    IPAddress ip;
    if (WiFi.hostByName(MONITOR_HOST, ip) != 1) {
      Serial.printf("Cannot resolve %s, will retry\n", MONITOR_HOST);
      monitorIp.reset();
      return;
    }
    monitorIp = ip;
    Serial.printf("Monitor at %s:%u (%s)\n", ip.toString().c_str(), static_cast<unsigned>(MONITOR_PORT), MONITOR_HOST);
  }

  bool sendMetric(const metric::Metric& m) {
    std::array<char, metric::k_max_message_len> buf{};
    const uint32_t mySeq = seq;
    seq = seq + 1;

    const size_t len = metric::serialize(m, mySeq, buf.data(), buf.size());
    if (len == 0) {
      Serial.printf(
        "Bad metric %.*s, seq=%lu skipped\n",
        static_cast<int>(m.name.size()),
        m.name.data(),
        static_cast<unsigned long>(mySeq)
      );
      return false;
    }
    if (upd.beginPacket(*monitorIp, MONITOR_PORT) != 1) {
      return false;
    }
    upd.write(reinterpret_cast<const uint8_t *>(buf.data()), len);

    return upd.endPacket() == 1;
  }

  void sendBatch(uint32_t now) {
    const std::array<metric::Metric, 3> batch = {{
        {.name = "uptime_s", .value = static_cast<double>(now / 1000)},
      {.name = "rssi_dbm", .value = static_cast<double>(WiFi.RSSI())},
      {.name = "free_heap_bytes", .value = static_cast<double>(ESP.getFreeHeap())}
      }};

    const uint32_t first = seq;
    uint32_t failedNow = 0;
    for (const auto&m : batch) {
      if (sendMetric(m)) {
        ++sentOk;
      } else {
        ++failedNow;
      }
    }
    sendFailed += failedNow;
    Serial.printf(
      "Sent seq=%lu..%lu, failed=%lu, total ok=%lu failed=%lu\n",
      static_cast<unsigned long>(first),
      static_cast<unsigned long>(seq - 1),
      static_cast<unsigned long>(failedNow),
      static_cast<unsigned long>(sentOk),
      static_cast<unsigned long>(sendFailed));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("10-udp-sender");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  Serial.printf(
    "SSID: %s, monitor %s:%u, every %lu ms\n",
    WIFI_SSID,
    MONITOR_HOST,
    static_cast<unsigned>(MONITOR_PORT),
    static_cast<unsigned long>(SEND_PERIOD_MS)
  );
  Serial.println("Ready");
}

void loop() {
  const uint32_t now = millis();

  switch (keeper.update(WiFi.status() == WL_CONNECTED, now)) {
    case WifiAction::Connect:
      Serial.printf(
        "Connecting to %s (attempt %lu)...\n",
        WIFI_SSID,
        static_cast<unsigned long>(keeper.attempt())
      );
      if (beganOnce) {
        WiFi.disconnect();
      }
      beganOnce = true;
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
      break;
    case WifiAction::Connected:
      Serial.printf(
        "Connected, IP %s, RSSI %d dBm (%s)\n",
        WiFi.localIP().toString().c_str(),
        WiFi.RSSI(),
        signalQuality(WiFi.RSSI())
      );
      resolveMonitor();
      break;
    case WifiAction::Timeout:
      Serial.printf(
        "Connect timeout, retry in %lu ms\n",
        static_cast<unsigned long>(keeper.backoffMs())
      );
      break;
    case WifiAction::Lost:
      Serial.printf(
        "Connection lost, retry in %lu ms\n",
        static_cast<unsigned long>(keeper.backoffMs())
      );
      break;
    case WifiAction::None:
      break;
  }

  if (!keeper.isConnected()) {
    return;
  }

  if (now - lastSendMs < SEND_PERIOD_MS) {
    return;
  }
  lastSendMs = now;

  if (!monitorIp) {
    resolveMonitor();
    return;
  }
  sendBatch(now);
}
