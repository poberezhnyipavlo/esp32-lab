#pragma once

#include <cstdint>

enum class RelayResult {
  Switched,
  Unchanged,
  TooSoon
};

class Relay {
public:
  Relay(bool active_low, uint32_t min_switch_ms, uint32_t max_on_ms);

  [[nodiscard]] RelayResult request(bool on, uint32_t now_ms);

  [[nodiscard]] bool update(uint32_t now_ms);

  [[nodiscard]] bool isOn() const { return on_; }
  [[nodiscard]] bool pinLevel() const { return active_low_ ? !on_ : on_; }
  [[nodiscard]] uint32_t switchCount() const { return switches_; }

private:
  void set(bool on, uint32_t now_ms);

  bool active_low_;
  uint32_t min_switch_ms_;
  uint32_t max_on_ms_;
  bool on_ = false;
  bool switched_once_ = false;
  uint32_t last_switch_ms_ = 0;
  uint32_t switches_ = 0;
};

