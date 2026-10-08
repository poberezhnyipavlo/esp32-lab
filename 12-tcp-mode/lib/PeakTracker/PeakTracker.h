#pragma once

#include <cstdint>

class PeakTracker {
public:
  void record(uint32_t value) {
    if (value > peak_) {
      peak_ = value;
    }
  }

  [[nodiscard]] uint32_t take() {
    const uint32_t result = peak_;
    peak_ = 0;
    return result;
  }

private:
  uint32_t peak_ = 0;
};
