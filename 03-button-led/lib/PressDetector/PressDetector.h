#pragma once
#include <cstdint>

enum class PressEvent : uint8_t {
  None, Short, Long
};

class PressDetector {
public:
  explicit PressDetector(uint32_t long_ms);
  [[nodiscard]] PressEvent update(bool pressed, uint32_t now_ms);

private:
  uint32_t long_ms_;
  bool was_pressed_ = false;
  bool long_fired_ = false;
  uint32_t press_start_ms_ = 0;
};

