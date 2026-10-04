#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#include <ClimateTracker.h>

#ifndef DHT_SENSOR_TYPE
#error "DHT_SENSOR_TYPE is not set: add -D DHT_SENSOR_TYPE=DHT11 (or DHT22) to build_flags"
#endif

namespace {
  constexpr uint8_t DHT_PIN = 4;
  constexpr uint8_t OLED_ADDR = 0x3C;
  constexpr int16_t SCREEN_WIDTH = 128;
  constexpr int16_t SCREEN_HEIGHT = 64;
  constexpr uint32_t READ_PERIOD_MS = 2000;
  constexpr uint32_t STALE_AFTER_MS = 10000;
  constexpr ClimateLimits LIMITS{
    .min_temperature_c = -20.0F,
    .max_temperature_c = 60.0F,
    .min_humidity_pct = 0.0F,
    .max_humidity_pct = 100.0F
  };

  DHT dht(DHT_PIN, DHT_SENSOR_TYPE);
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  ClimateTracker climate(LIMITS, STALE_AFTER_MS);
  bool displayReady = false;
  uint32_t lasrReadMs = 0;

  bool i2cDevicePresent(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
  }

  void drawClimate(uint32_t now) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    const auto last = climate.last();
    if (last) {
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.printf("%.1f C", last->temperature_c);
      display.setCursor(0, 24);
      display.printf("%.0f %%", last->humidity_pct);
    } else {
      display.setTextColor(1);
      display.setCursor(0, 0);
      display.print("Waiting for sensor....");
    }

    display.setTextSize(1);
    display.setCursor(0, 56);

    if (last && climate.isStale(now)) {
      display.print("STALE");
    } else if (climate.consecutiveFailures()) {
      display.printf("read errors: %lu", static_cast<unsigned long>(climate.consecutiveFailures()));
    } else if (last) {
      display.print("OK");
    }
    display.display();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("05-dht11-oled");

  Wire.begin();
  dht.begin();

  if (!i2cDevicePresent(OLED_ADDR)) {
    Serial.println("OLED not found, Serial only");
  } else if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    displayReady = true;
    drawClimate(millis());
  } else {
    displayReady = true;
    drawClimate(millis());
  }

  Serial.println("Ready");
}

void loop() {
  const uint32_t now = millis();

  if (now - lasrReadMs < READ_PERIOD_MS) {
    return;
  }

  lasrReadMs = now;

  const float t = dht.readTemperature();
  const float h = dht.readHumidity();

  if (climate.update(t, h, now)) {
    Serial.printf("T=%.1f H=%.1f\n", t, h);
  } else {
    Serial.printf("read error (%lu in a row)\n", static_cast<unsigned long>(climate.consecutiveFailures()));
  }

  if (displayReady) {
    drawClimate(now);
  }
}
