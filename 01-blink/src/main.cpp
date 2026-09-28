#include <Arduino.h>

namespace {
  constexpr int LED_PIN = 2;
  constexpr unsigned long BLINK_MS = 500;

  unsigned long counter = 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Hello from ESP32!");

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.printf("tick %lu: LED ON\n", counter);
  delay(BLINK_MS);

  digitalWrite(LED_PIN, LOW);
  Serial.printf("tick %lu: LED OFF\n", counter);
  delay(BLINK_MS);

  ++counter;
}
