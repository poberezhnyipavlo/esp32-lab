#include <Arduino.h>
#include <WiFi.h>

#include <array>
#include <optional>

#include <metric_protocol/MetricProtocol.h>

#include "PeakTracker.h"
#include "RetryTimer.h"
#include "SignalQuality.h"
#include "WifiKeeper.h"


#if defined(WOKWI_WIFI)
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
constexpr char MONITOR_HOST[] = "10.13.37.1";
#elif __has_include("secrets.h")
#include "secrets.h"  // WIFI_SSID, WIFI_PASSWORD, MONITOR_HOST
constexpr int32_t WIFI_CHANNEL = 0;
#else
#error "No include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill it in"
#endif


namespace {
  constexpr uint16_t MONITOR_PORT = 9002;
  constexpr uint32_t SEND_PERIOD_MS = 2000;
  constexpr int32_t TCP_CONNECT_TIMEOUT_MS = 3000;
  constexpr uint32_t TCP_RETRY_BASE_MS = 1000;
  constexpr uint32_t TCP_RETRY_MAX_MS = 30000;
  constexpr uint32_t MS_PER_SECOND = 1000;

  constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
  constexpr uint32_t WIFI_BACKOFF_BASE_MS = 1000;
  constexpr uint32_t WIFI_BACKOFF_MAX_MS = 30000;

  WifiKeeper keeper(WIFI_CONNECT_TIMEOUT_MS, WIFI_BACKOFF_BASE_MS, WIFI_BACKOFF_MAX_MS);
  bool beganOnce = false;

  WiFiClient client;
  std::optional<IPAddress> monitorIp;
  RetryTimer tcpRetry(TCP_RETRY_BASE_MS, TCP_RETRY_MAX_MS);
  bool tcpUp = false;

  uint32_t seq = 0;
  uint32_t lastSendMs = 0;
  uint32_t sentOk = 0;
  uint32_t sendFailed = 0;

  PeakTracker loopPeak;

  void resolveMonitor() {
    IPAddress ip;
    if (WiFi.hostByName(MONITOR_HOST, ip) != 1) {
      Serial.printf("Cannot resolve %s, will retry\n", MONITOR_HOST);
      monitorIp.reset();
      return;
    }
    monitorIp = ip;
    Serial.printf("Monitor at %s:%u\n", ip.toString().c_str(), static_cast<unsigned>(MONITOR_PORT));
  }

  void dropConnection(const char* reason, uint32_t now) {
    client.stop();
    tcpRetry.failed(now);
    tcpUp = false;
    Serial.printf("TCP: %s, retry in %lu ms\n", reason, static_cast<unsigned long>(tcpRetry.delayMs()));
  }

  void tryConnect() {
    Serial.printf(
      "TCP: connecting to %s:%u (attempt %lu)...\n",
      monitorIp->toString().c_str(),
      static_cast<unsigned>(MONITOR_PORT),
      static_cast<unsigned long>(tcpRetry.failures() + 1)
    );
    const uint32_t started = millis();
    const bool ok = client.connect(*monitorIp, MONITOR_PORT, TCP_CONNECT_TIMEOUT_MS) == 1;
    const uint32_t took = millis() - started;

    if (!ok) {
      tcpRetry.failed(millis());
      Serial.printf(
        "TCP: connect failed after %lu ms, retry in %lu ms\n",
        static_cast<unsigned long>(took),
        static_cast<unsigned long>(tcpRetry.delayMs())
      );
      return;
    }
    client.setNoDelay(true);
    tcpRetry.succeeded();
    tcpUp = true;
    Serial.printf("TCP: connected in %lu ms\n", static_cast<unsigned long>(took));
  }

  bool sendMetric(const metric::Metric& m) {
    std::array<char, metric::k_max_message_len> buf{};
    const uint32_t mySeq = seq;
    seq = seq + 1;

    const size_t len = metric::serialize(m, mySeq, buf.data(), buf.size());
    if (len == 0) {
      return false;
    }

    return client.write(reinterpret_cast<const uint8_t*>(buf.data()), len) == len;
  }

  void sendBatch(uint32_t now) {
    const std::array<metric::Metric, 3> batch = {{
      {.name = "uptime_s", .value = static_cast<double>(now / MS_PER_SECOND)},
      {.name = "rssi_dbm", .value = static_cast<double>(WiFi.RSSI())},
      {.name = "free_heap_bytes", .value = static_cast<double>(ESP.getFreeHeap())},
    }};

    const uint32_t first = seq;
    uint32_t failedNow = 0;
    for (const auto& m : batch) {
      if (sendMetric(m)) {
        ++sentOk;
      } else {
        ++failedNow;
        break;
      }
    }
    sendFailed += failedNow;
    Serial.printf(
      "Sent seq=%lu..%lu, failed=%lu, total ok=%lu failed=%lu, loop max %lu ms\n",
      static_cast<unsigned long>(first),
      static_cast<unsigned long>(seq - 1),
      static_cast<unsigned long>(failedNow),
      static_cast<unsigned long>(sentOk),
      static_cast<unsigned long>(sendFailed),
      static_cast<unsigned long>(loopPeak.take())
    );

    if (failedNow > 0) {
      dropConnection("write failed", millis());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("12-tcp-mode");

  WiFi.persistent(false);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(false);


  Serial.printf(
    "SSID: %s, monitor %s:%u (TCP), every %lu ms\n",
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
      Serial.printf("Connecting to %s (attempt %lu)...\n", WIFI_SSID,
                    static_cast<unsigned long>(keeper.attempt()));
      if (beganOnce) {
        WiFi.disconnect();
      }
      beganOnce = true;
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
      break;
    case WifiAction::Connected:
      Serial.printf("Connected, IP %s, RSSI %d dBm (%s)\n", WiFi.localIP().toString().c_str(),
                    WiFi.RSSI(), signalQuality(WiFi.RSSI()));
      resolveMonitor();
      break;
    case WifiAction::Timeout:
      Serial.printf("Connect timeout, retry in %lu ms\n",
                    static_cast<unsigned long>(keeper.backoffMs()));
      break;
    case WifiAction::Lost:
      Serial.printf("Connection lost, retry in %lu ms\n",
                    static_cast<unsigned long>(keeper.backoffMs()));
      if (tcpUp) {
        client.stop();  // сокет без Wi-Fi уже нічого не варт
        tcpUp = false;
      }
      break;
    case WifiAction::None:
      break;
  }

  if (keeper.isConnected()) {
    if (!monitorIp) {
      resolveMonitor();
    }

    if (tcpUp && !client.connected()) {
      dropConnection("connection lost", now);
    }

    if (!tcpUp && monitorIp && tcpRetry.due(now)) {
      tryConnect();
    }

    if (tcpUp && now - lastSendMs >= SEND_PERIOD_MS) {
      lastSendMs = now;
      sendBatch(now);
    }

    loopPeak.record(millis() - now);
  }
}
