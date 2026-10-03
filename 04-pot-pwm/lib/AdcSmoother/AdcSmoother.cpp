#include "AdcSmoother.h"

AdcSmoother::AdcSmoother(uint8_t shift) : shift_(shift) {}

uint16_t AdcSmoother::update(uint16_t raw) {
  if (!primed_) {
    acc_ = static_cast<uint32_t>(raw) << shift_;
    primed_ = true;
    return raw;
  }

  acc_ = acc_ - (acc_ >> shift_) + raw;

  return value();
}
