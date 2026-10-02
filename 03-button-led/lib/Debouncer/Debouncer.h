#pragma once

#include <cstdint>

class Debouncer {
public:
  explicit Debouncer(uint32_t stable_ms);

  void update(bool raw, uint32_t now_ms);

  [[nodiscard]] bool isPressed() const { return  stable_; };

private:
  uint32_t stable_ms_;
  bool stable_ = false;
  bool last_raw_ = false;
  uint32_t last_change_ms_ = 0;
};
