#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <optional>

namespace {
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
  constexpr uint8_t OLED_ADDRS[] = {0x3C, 0x3D};

  bool i2cDevicePresent(uint8_t addr) {
    Wire.beginTransmission(addr);

    return Wire.endTransmission() == 0;
  }

  std::optional<uint8_t> findOledAddr() {
    for (uint8_t addr : OLED_ADDRS) {
      if (i2cDevicePresent(addr)) {
        return addr;
      }
    }

    return std::nullopt;
  }

  void scanI2C() {
    Serial.println();
    Serial.println("I2C scaner");

    uint8_t found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {

      if (i2cDevicePresent(addr)) {
        found++;
        Serial.printf("Found at address 0x%02X\n", addr);
      }
    }

    if (found == 0) {
      Serial.println("No I2C devices found.");
    } else {
      Serial.printf("Found count: %d\n", found);
    }

    Serial.println("Scan done");
  }

}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin();

  scanI2C();

  const auto addr = findOledAddr();

  if (!addr) {
    Serial.println("No I2C device found.");
    return;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, *addr)) {
    Serial.println("OLED not found (tried 0x3C, 0x3D)");
    return;
  }

  Serial.println("OLED ready!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello World!");
  display.display();
}

void loop() {}
