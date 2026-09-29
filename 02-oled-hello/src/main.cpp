#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

namespace {
  void scanI2C() {
    Serial.println();
    Serial.println("I2C scaner");

    uint8_t found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      const uint8_t status = Wire.endTransmission();
      if (status == 0) {
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

  Adafruit_SSD1306 display(128, 64, &Wire, -1);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin();

  scanI2C();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
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
