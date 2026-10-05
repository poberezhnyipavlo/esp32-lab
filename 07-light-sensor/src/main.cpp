#include <Arduino.h>

#include "HysteresisSwitch.h"

namespace {
  constexpr uint8_t AO_PIN = 34;
  constexpr uint8_t DO_PIN = 35;
  constexpr uint8_t LAMP_PIN = 2;

  constexpr uint16_t LAMP_ON_ABOVE = 2500;
  constexpr uint16_t LAMP_OFF_BELOW = 2000;
  static_assert(LAMP_OFF_BELOW <= LAMP_ON_ABOVE, "OFF threshold must not exceed ON threshold");

  constexpr uint32_t STATUS_PERIOD_MS = 500;

  HysteresisSwitch lamp(LAMP_OFF_BELOW, LAMP_ON_ABOVE);
  uint32_t lastStatusMS = 0;
  int lastDo = -1;
  uint16_t doChanges = 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("07-light-sensor");

  pinMode(DO_PIN, INPUT);
  pinMode(LAMP_PIN, OUTPUT);
  digitalWrite(LAMP_PIN, LOW);


  Serial.printf(
    "Thresholds: ON above %u, OFF below %u\n",
    static_cast<unsigned>(LAMP_ON_ABOVE), static_cast<unsigned>(LAMP_OFF_BELOW)
  );
  Serial.println("Ready");
}

void loop() {
  const uint32_t now = millis();
  const auto raw = static_cast<uint16_t>(analogRead(AO_PIN));

  switch (lamp.update(raw)) {
    case SwitchEvent::TurnedOn:
      digitalWrite(LAMP_PIN, HIGH);
      Serial.printf("Lamp ON (raw=%u)\n", static_cast<unsigned>(raw));
      break;
    case SwitchEvent::TurnedOff:
      digitalWrite(LAMP_PIN, LOW);
      Serial.printf("Lamp OFF (raw=%u)\n", static_cast<unsigned>(raw));
      break;
    case SwitchEvent::None:
      break;
  }

  const int doLevel = digitalRead(DO_PIN);
  if (lastDo != -1 && doLevel != lastDo) {
    ++doChanges;
  }
  lastDo = doLevel;

  if (now - lastStatusMS < STATUS_PERIOD_MS) {
    return;
  }

  lastStatusMS = now;
  Serial.printf(
    "raw=%u, do=%d, lamp=%s, lamp_changes=%lu, do_changes=%lu\n",
      static_cast<unsigned>(raw), doLevel, lamp.isOn() ? "ON" : "OFF",
      static_cast<unsigned long>(lamp.changeCount()),
      static_cast<unsigned long>(doChanges)
  );
}
