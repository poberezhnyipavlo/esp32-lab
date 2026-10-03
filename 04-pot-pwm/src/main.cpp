#include <Arduino.h>

#include "AdcSmoother.h"
#include "DutyMap.h"

namespace {
  constexpr uint8_t POT_PIN = 34;
  constexpr uint8_t LED_PIN = 18;
  constexpr uint8_t ADC_BITS = 12;
  constexpr uint32_t ADC_MAX = (1UL << ADC_BITS) - 1;
  constexpr uint32_t PWM_FREQ_HZ = 5000;
  constexpr uint8_t PWM_BITS = 8;
  constexpr uint32_t DUTY_MAX = (1U << PWM_BITS) - 1;
  constexpr uint8_t SMOOTH_SHIFT = 3;
  constexpr uint32_t SAMPLE_PERIOD_MS = 20;

  AdcSmoother smoother(SMOOTH_SHIFT);
  bool pwm_ready = false;
  uint32_t last_sample_ms = 0;
  uint32_t last_duty = UINT32_MAX;
}


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("04-pot-pwm");

  analogReadResolution(ADC_BITS);
  pwm_ready = ledcAttach(LED_PIN, PWM_FREQ_HZ, PWM_BITS);

  if (!pwm_ready) {
    Serial.println("PWM attach failed");
    return;
  }

  Serial.println("Ready");
}

void loop() {
  if (!pwm_ready) {
    return;
  }
  const uint32_t now = millis();

  if (now - last_sample_ms < SAMPLE_PERIOD_MS) {
    return;
  }

  last_sample_ms = now;

  const uint16_t raw = analogRead(POT_PIN);
  const uint16_t smooth = smoother.update(raw);
  const uint32_t duty = adcToDuty(smooth, ADC_MAX, DUTY_MAX);
  ledcWrite(LED_PIN, duty);

  if (duty != last_duty) {
    last_duty = duty;
    Serial.printf(
      "duty=%lu, raw=%u, smooth=%u\n",
      static_cast<unsigned long>(duty),
      static_cast<unsigned>(raw),
      static_cast<unsigned>(smooth)
    );
  }
}
