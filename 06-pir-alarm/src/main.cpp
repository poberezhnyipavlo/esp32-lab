#include <Arduino.h>

#include "MotionAlarm.h"

#ifndef PIR_WARMUP_MS
#define PIR_WARMUP_MS 30000
#endif

namespace {
  constexpr uint8_t PIR_PIN = 4;
  constexpr uint8_t BUZZER_PIN = 18;
  constexpr uint8_t LED_PIN = 2;
  constexpr uint32_t ALARM_MS = 3000;
  constexpr uint32_t BEEP_MS = 200;

  volatile bool motionStarted = false;
  volatile uint32_t isrCount = 0;
  portMUX_TYPE iseMux = portMUX_INITIALIZER_UNLOCKED;

  void IRAM_ATTR onPirRising() {
    portENTER_CRITICAL_ISR(&iseMux);
    motionStarted = true;
    isrCount = isrCount + 1;
    portEXIT_CRITICAL_ISR(&iseMux);
  }

  bool takeMotionFlag(uint32_t& count) {
    portENTER_CRITICAL(&iseMux);
    const bool started = motionStarted;
    motionStarted = false;
    count = isrCount;
    portEXIT_CRITICAL(&iseMux);

    return started;
  }

  MotionAlarm motionAlarm(0, PIR_WARMUP_MS, ALARM_MS, BEEP_MS);
  bool armedAnnounced = false;
  bool lastAlarmActive = false;
  bool lastBuzzer = false;
}


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("06-pir-alarm");

  pinMode(PIR_PIN, INPUT_PULLDOWN);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(PIR_PIN), onPirRising, RISING);

  Serial.printf("Warming up %lu ms\n", static_cast<unsigned long>(PIR_WARMUP_MS));
  Serial.println("Ready");
}

void loop() {
  const uint32_t now = millis();

  if (!armedAnnounced && motionAlarm.isArmed(now)) {
    armedAnnounced = true;
    Serial.println("Armed");
  }

  uint32_t count = 0;
  if (takeMotionFlag(count)) {
    if (motionAlarm.onMotion(now)) {
      Serial.printf(
        "Motion! event %lu (interrupts: %lu)\n",
        static_cast<unsigned long>(motionAlarm.eventCount()),
        static_cast<unsigned long>(count)
      );
    } else {
      Serial.println("Motion ignored (warming up)");
    }
  }

  const bool active = motionAlarm.isAlarmActive(now);
  if (lastAlarmActive && !active) {
    Serial.println("Alarm off");
  }
  lastAlarmActive = active;

  const bool buzzer = motionAlarm.buzzerOn(now);
  if (buzzer != lastBuzzer) {
    lastBuzzer = buzzer;
    digitalWrite(BUZZER_PIN, buzzer ? HIGH : LOW);
    digitalWrite(LED_PIN, buzzer ? HIGH : LOW);
  }
}
