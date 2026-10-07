#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_arduino_version.h>

#include <array>
#include <cstdio>
#include <optional>
#include <string_view>

#include "DeviceApi.h"
#include "SignalQuality.h"
#include "WifiKeeper.h"

#if defined(WOKWI_WIFI)
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
constexpr char API_TOKEN[] = "wokwi-test-token";
constexpr char MONITOR_HOST[] = "10.13.37.1";
#elif __has_include("secrets.h")
#include "secrets.h"
constexpr int32_t WIFI_CHANNEL = 0;
#else
#error "No include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill it in"
#endif

namespace {
  constexpr char FW_NAME[] = "11-http-server";
  constexpr char FW_VERSION[] = "1.0.0";
  constexpr char FW_BUILT[] = __DATE__ " " __TIME__;

  constexpr uint16_t HTTP_PORT = 80;
  constexpr uint32_t MS_PER_SECOND = 1000;
  constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
  constexpr uint32_t BACKOFF_BASE_MS = 1000;
  constexpr uint32_t BACKOFF_MAX_MS = 30000;

  constexpr char NVS_NAMESPACE[] = "config";
  constexpr char NVS_KEY_HOST[] = "monitor_host";
  constexpr char TOKEN_HEADER[] = "X-Api-Token";

  WifiKeeper keeper(CONNECT_TIMEOUT_MS, BACKOFF_BASE_MS, BACKOFF_MAX_MS);
  bool beganOnce = false;

  WebServer server(HTTP_PORT);
  bool serverStarted = false;

  std::array<char, api::k_max_host_len + 1> monitorHost{};
  bool monitorHostFromNvs = false;

  std::string_view toView(const String& s) { return {s.c_str(), s.length()}; }

  void useFirmwareHost() {
    std::snprintf(monitorHost.data(), monitorHost.size(), "%s", MONITOR_HOST);
    monitorHostFromNvs = false;
  }

  void loadMonitorHost() {
    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, false)) {
      Serial.println("NVS: cannot open, using firmware default");
      useFirmwareHost();
      return;
    }

    if (prefs.isKey(NVS_KEY_HOST)) {
      std::array<char, api::k_max_host_len + 1> stored{};
      prefs.getString(NVS_KEY_HOST, stored.data(), stored.size());
      if (api::isValidHost(stored.data())) {
        monitorHost = stored;
        monitorHostFromNvs = true;
      } else {
        Serial.println("NVS: stored host is invalid, ignoring it");
        useFirmwareHost();
      }
    } else {
      useFirmwareHost();
    }
    prefs.end();
  }

  bool saveMonitorHost(std::string_view host) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
      return false;
    }
    std::array<char, api::k_max_host_len + 1> value{};
    std::snprintf(value.data(), value.size(), "%.*s", static_cast<int>(host.size()), host.data());
    const bool ok = prefs.putString(NVS_KEY_HOST, value.data()) == host.size();
    prefs.end();
    if (ok) {
      monitorHost = value;
      monitorHostFromNvs = true;
    }

    return ok;
  }

  const char* methodName(HTTPMethod m) {
    switch (m) {
      case HTTP_GET: return "GET";
      case HTTP_POST: return "POST";
      case HTTP_PUT: return "PUT";
      case HTTP_PATCH: return "PATCH";
      case HTTP_DELETE: return "DELETE";
      default: return "OTHER";
    }
  }

  void reply(int status, const char* contentType, const char* body) {
    server.send(status, contentType, body);
    Serial.printf("%s %s -> %d\n", methodName(server.method()), server.uri().c_str(), status);
  }

  void replyError(int status, std::string_view message) {
    std::array<char, 96> body{};
    std::snprintf(
      body.data(),
      body.size(),
      "{\"error\":\"%.*s\"}\n",
      static_cast<int>(message.size()),
      message.data()
    );
    reply(status, "application/json", body.data());
  }

  void handleInfo() {
    const String ip = WiFi.localIP().toString();
    const String mac = WiFi.macAddress();

    const api::DeviceInfo info{
      FW_NAME,
      FW_VERSION,
      FW_BUILT,
      ESP.getChipModel(),
      ESP.getChipRevision(),
      ESP.getChipCores(),
      ESP.getCpuFreqMHz(),
      ESP.getFlashChipSize(),
      ESP_ARDUINO_VERSION_STR,
      ESP.getSdkVersion(),
      ESP.getFreeHeap(),
      ESP.getMinFreeHeap(),
      millis() / MS_PER_SECOND,
      toView(ip),
      toView(mac),
      WiFi.RSSI(),
      monitorHost.data(),
      monitorHostFromNvs
    };

    std::array<char, api::k_max_info_json> json{};
    if (api::buildInfoJson(info, json.data(), json.size()) == 0) {
      replyError(500, "cannot build info");
      return;
    }
    reply(200, "application/json", json.data());
  }

  void handlePutMonitorHost() {
    std::optional<std::string_view> body;
    String plain;
    if (server.hasArg("plain")) {
      plain = server.arg("plain");
      body = toView(plain);
    }
    const String token = server.header(TOKEN_HEADER);

    const api::PutHostResult r = api::decidePutHost(API_TOKEN, toView(token), body);
    if (r.status != 200) {
      replyError(r.status, r.error);
      return;
    }

    const bool changed = r.host != std::string_view(monitorHost.data());
    if (changed && !saveMonitorHost(r.host)) {
      replyError(500, "cannot save monitor host");
      return;
    }

    std::array<char, 160> json{};
    std::snprintf(
      json.data(),
      json.size(),
      "{\"monitor_host\":{\"value\":\"%s\",\"source\":\"nvs\"},\"changed\":%s}\n",
      monitorHost.data(),
      changed ? "true" : "false"
    );
    reply(200, "application/json", json.data());
  }

  void methodNotAllowed(const char* allow) {
    server.sendHeader("Allow", allow);
    replyError(405, "method not allowed");
  }

  void setupRoutes() {
    server.on("/info", HTTP_GET, handleInfo);
    server.on("/info", HTTP_ANY, [] { methodNotAllowed("GET"); });
    server.on("/config/monitor-host", HTTP_PUT, handlePutMonitorHost);
    server.on("/config/monitor-host", HTTP_ANY, [] { methodNotAllowed("PUT"); });
    server.onNotFound([] { replyError(404, "not found"); });


    static const char* headers[] = {TOKEN_HEADER};
    server.collectHeaders(headers, 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(FW_NAME);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  loadMonitorHost();
  setupRoutes();

  Serial.printf("SSID: %s\n", WIFI_SSID);
  Serial.printf("Monitor host: %s (%s)\n", monitorHost.data(), monitorHostFromNvs ? "nvs" : "firmware");
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
      if (!serverStarted) {
        server.begin();
        serverStarted = true;
      }
      Serial.printf("HTTP server on http://%s/info\n", WiFi.localIP().toString().c_str());
      break;
    case WifiAction::Timeout:
      Serial.printf("Connect timeout, retry in %lu ms\n",
                    static_cast<unsigned long>(keeper.backoffMs()));
      break;
    case WifiAction::Lost:
      Serial.printf("Connection lost, retry in %lu ms\n",
                    static_cast<unsigned long>(keeper.backoffMs()));
      break;
    case WifiAction::None:
      break;
  }

  if (keeper.isConnected()) {
    server.handleClient();
  }
}
