#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <esp_system.h>

#include <cstdio>

#include "SendPolicy.h"
#include "WifiKeeper.h"

// --- Звідки беремо мережу і бота ----------------------------------------------
#if defined(WOKWI_WIFI)
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
constexpr char TELEGRAM_BOT_TOKEN[] = "123456789:wokwi-fake-token";  // Telegram має відхилити
constexpr char TELEGRAM_CHAT_ID[] = "1";
#elif __has_include("secrets.h")
#include "secrets.h"
constexpr int32_t WIFI_CHANNEL = 0;
#else
#error "No include/secrets.h: copy include/secrets.example.h to include/secrets.h and fill it in"
#endif

namespace {
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_BACKOFF_BASE_MS = 1000;
constexpr uint32_t WIFI_BACKOFF_MAX_MS = 30000;
constexpr uint32_t SEND_BACKOFF_BASE_MS = 5000;
constexpr uint32_t SEND_BACKOFF_MAX_MS = 120000;
constexpr uint32_t TLS_HANDSHAKE_TIMEOUT_S = 15;  // за замовчуванням у ядрі 120 с
constexpr int32_t HTTP_CONNECT_TIMEOUT_MS = 10000;
constexpr uint16_t HTTP_READ_TIMEOUT_MS = 10000;

WifiKeeper keeper{WIFI_CONNECT_TIMEOUT_MS, WIFI_BACKOFF_BASE_MS, WIFI_BACKOFF_MAX_MS};
Backoff sendBackoff(SEND_BACKOFF_BASE_MS, SEND_BACKOFF_MAX_MS);
bool beganOnce = false;
bool finished = false;
char message[96];

const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_POWERON:
        return "POWERON";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_EXT:
        return "EXT";
    case ESP_RST_SW:
        return "SW";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INT_WDT";
    case ESP_RST_TASK_WDT:
        return "TASK_WDT";
    case ESP_RST_WDT:
        return "WDT";
    default:
        return "OTHER";
    }
}

// POWERON і BROWNOUT — живлення з'явилось (при «блиманні» світла буває BROWNOUT).
void buildMessage(esp_reset_reason_t reason) {
    if (reason == ESP_RST_POWERON || reason == ESP_RST_BROWNOUT) {
        snprintf(message, sizeof(message), "💡 Світло є");
        return;
    }
    snprintf(message, sizeof(message), "⚠️ Плата перезапустилась: %s", resetReasonName(reason));
}

// Повертає код HTTPClient: < 0 — до HTTP не дійшло, інакше HTTP-статус.
int postToTelegram() {
    char url[128];
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage", TELEGRAM_BOT_TOKEN);
    char body[160];
    snprintf(body, sizeof(body), R"({"chat_id":"%s","text":"%s"})", TELEGRAM_CHAT_ID, message);

    NetworkClientSecure client;
    client.useBuiltinCACertBundle();
    client.setHandshakeTimeout(TLS_HANDSHAKE_TIMEOUT_S);

    HTTPClient http;
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(HTTP_READ_TIMEOUT_MS);
    if (!http.begin(client, url)) {
        return HTTPC_ERROR_CONNECTION_REFUSED;
    }
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(body);
    if (code > 0 && classifySend(code) != SendResult::Sent) {
        Serial.printf("Telegram says: %s\n", http.getString().c_str());
    }
    http.end();
    return code;
}

void stopWifi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

void trySend() {
    Serial.println("Sending to Telegram...");
    const int code = postToTelegram();
    switch (classifySend(code)) {
        case SendResult::Sent:
            Serial.println("Sent. Done, Wifi off.");
            finished = true;
            stopWifi();
            return;
        case SendResult::GiveUp:
            Serial.printf("Telegram rejected the request: HTTP %d. Giving up.\n", code);
            finished = true;
            stopWifi();
            return;
        case SendResult::Retry:
            sendBackoff.onFailure(millis());
            Serial.printf(
                    "Send failed: %d (%s), retry in %lu ms\n",
                    code,
                    code < 0 ? HTTPClient::errorToString(code).c_str() : "HTTP",
                    static_cast<unsigned long>(sendBackoff.delayMs())
                );
            return;
    }
}

void handleWifi(uint32_t now) {
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
                "Connected, IP %s, RSSI %ld dBm\n",
                WiFi.localIP().toString().c_str(),
                static_cast<long>(WiFi.RSSI())
            );
            break;
        case  WifiAction::Timeout:
            Serial.printf(
                 "Wi-Fi timeout, retry in %lu ms\n",
                 static_cast<unsigned long>(keeper.backoffMs())
            );
            break;
        case WifiAction::Lost:
            Serial.println("Wi-Fi lost");
            break;
        case WifiAction::None:
            break;
    }
}
} // namespace

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("15-power-notify");

    const esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("Reset reason: %s\n", resetReasonName(reason));
    buildMessage(reason);

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
}

void loop() {
    if (finished) {
        delay(1000);
        return;
    }

    const uint32_t now = millis();
    handleWifi(now);
    if (keeper.isConnected() && sendBackoff.isDue(now)) {
        trySend();
    }
}
