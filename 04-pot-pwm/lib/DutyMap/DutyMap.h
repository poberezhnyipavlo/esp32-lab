#pragma once

#include <cstdint>

constexpr uint32_t adcToDuty(uint32_t raw, uint32_t adc_max, uint32_t duty_max) {
  if (raw >= adc_max) {
    return duty_max;
  }

  return raw * duty_max /adc_max;
}
