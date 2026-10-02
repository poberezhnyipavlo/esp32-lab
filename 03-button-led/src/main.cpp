#include <Arduino.h>

#include "Debouncer.h"

namespace {
  constexpr uint8_t BUTTON_PIN = 4;
  constexpr uint8_t LED_PIN = 18;
  constexpr uint32_t DEBOUNCE_MS = 50;

  Debouncer button(DEBOUNCE_MS);
  bool led_on = false;
  unsigned long press_count = 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("03-button-led");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Ready");
}

void loop() {

  const bool raw = digitalRead(BUTTON_PIN) == LOW;

  if (button.update(raw, millis())) {
    led_on = !led_on;
    digitalWrite(LED_PIN, led_on ? HIGH : LOW);
    ++press_count;
    Serial.printf("Press %lu: LED %s\n", press_count, led_on ? "ON" : "OFF");
  }
}
