#include <Arduino.h>
#include <WiFi.h>

#include <atomic>

#include "SignalQuality.h"
#include "WifiKeeper.h"

#if defined(WOKWI_WIFI)
#ifndef  WOKWI_SSID
#define WOKWI_SSID "Wokwi-GUEST"
#endif
constexpr char WIFI_SSID[] = WOKWI_SSID;
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
#elif __has_include("secrets.h")
#include "secrets.h"
constexpr int32_t WIFI_CHANNEL = 0;
#else
#error "No include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill in your Wi-Fi"
#endif

namespace {
  constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
  constexpr uint32_t BACKOFF_BASE_MS = 1000;
  constexpr uint32_t BACKOFF_MAX_MS = 30000;
  constexpr uint32_t STATUS_PERIOD_MS = 5000;

  WifiKeeper keeper(CONNECT_TIMEOUT_MS, BACKOFF_BASE_MS, BACKOFF_MAX_MS);
  uint32_t lastStatusMs = 0;
  bool beganOnce = false;

  std::atomic<uint32_t> disconnectCount{0};
  std::atomic<uint16_t> lastDisconnectReason{0};
  uint32_t reportedDisconnects = 0;

  void onStaDisconnected(arduino_event_id_t /*event*/, arduino_event_info_t info) {
    lastDisconnectReason.store(info.wifi_sta_disconnected.reason);
    disconnectCount.fetch_add(1);
  }

  void scanNetworks() {
    Serial.println("Scanning...");
    const int16_t found = WiFi.scanNetworks();
    if (found < 0) {
      Serial.printf("Scan failed: %d\n", found);
      return;
    }
    Serial.printf("Scan: %d networks\n", found);
    for (int i = 0; i < found; i++) {
      const auto item = static_cast<uint8_t>(i);
      const String ssid = WiFi.SSID(item);
      const int32_t rssi = WiFi.RSSI(item);

      Serial.printf(
        "  %2d) %-32s ch=%2ld rssi=%4ld (%s) %s\n",
        i + 1,
        ssid.isEmpty() ? "(hidden)" : ssid.c_str(),
        static_cast<long>(WiFi.channel(item)),
        static_cast<long>(rssi), signalQuality(rssi),
        WiFi.encryptionType(item) == WIFI_AUTH_OPEN ? "open" : "secured"
      );
    }
    WiFi.scanDelete();
  }

  void printConnected() {
    const int32_t rssi = WiFi.RSSI();
    Serial.printf("Connected to %s\n", WIFI_SSID);
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("  RSSI: %ld dBm (%s)\n", static_cast<long>(rssi), signalQuality(rssi));
    Serial.printf("  channel: %ld\n", static_cast<long>(WiFi.channel()));
    Serial.printf("  MAC: %s\n", WiFi.macAddress().c_str());
  }

  void reportDisconnectReason() {
    const uint32_t count = disconnectCount.load();
    if (count == reportedDisconnects) {
      return;
    }

    reportedDisconnects = count;
    uint16_t reason = lastDisconnectReason.load();
    if (reason == 0) {
      reason = WIFI_REASON_UNSPECIFIED;
    }
    Serial.printf(
      "Disconnect reason %u: %s (total %lu)\n",
      static_cast<unsigned>(reason),
      WiFi.STA.disconnectReasonName(static_cast<wifi_err_reason_t>(reason)),
      static_cast<unsigned long>(count)
    );
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("09-wifi-connect");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.onEvent(onStaDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  Serial.printf(
    "SSID: %s, timeout %lu ms, backoff %lu..%lu ms\n",
    WIFI_SSID,
    static_cast<unsigned long>(CONNECT_TIMEOUT_MS),
    static_cast<unsigned long>(BACKOFF_BASE_MS),
    static_cast<unsigned long>(BACKOFF_MAX_MS)
  );
  scanNetworks();
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
      printConnected();
      break;
    case WifiAction::Timeout:
      Serial.printf(
        "Connect timeout after %lu ms, retry in %lu ms\n",
        static_cast<unsigned long>(CONNECT_TIMEOUT_MS),
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

  reportDisconnectReason();

  if (now - lastStatusMs < STATUS_PERIOD_MS) {
    return;
  }
  lastStatusMs = now;
  if (keeper.isConnected()) {
    const int32_t rssi = WiFi.RSSI();
    Serial.printf(
      "Status: connected, RSSI=%ld dBm (%s), uptime=%lu s\n",
      static_cast<long>(rssi),
      signalQuality(rssi), static_cast<unsigned long>(now / 1000)
    );
  } else {
    Serial.printf(
      "Status: not connected (attempt %lu)\n",
      static_cast<unsigned long>(keeper.attempt())
    );
  }
}
