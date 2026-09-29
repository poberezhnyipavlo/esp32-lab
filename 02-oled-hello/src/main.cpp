#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("I2C scaner");

  Wire.begin();

  uint8_t found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    const uint8_t status = Wire.endTransmission();
    if (status == 0) {
      found++;
      Serial.printf("Found at address 0x%02X\n", addr);
    }
  }

  Wire.end();

  if (found == 0) {
    Serial.println("No I2C devices found.");
  } else {
    Serial.printf("Found count: %d\n", found);
  }

  Serial.println("Scan done");
}

void loop() {}
