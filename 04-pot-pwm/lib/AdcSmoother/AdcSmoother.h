#pragma once

#include <cstdint>

class AdcSmoother {
public:
  explicit AdcSmoother(uint8_t shift);

  [[nodiscard]] uint16_t update(uint16_t raw);
  [[nodiscard]] uint16_t value() const { return static_cast<uint16_t>(acc_ >> shift_); }
private:
  uint8_t shift_;
  uint32_t acc_ = 0;
  bool primed_ = false;
};
