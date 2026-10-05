#include "Relay.h"

Relay::Relay(bool active_low, uint32_t min_switch_ms, uint32_t max_on_ms)
  : active_low_(active_low), min_switch_ms_(min_switch_ms), max_on_ms_(max_on_ms) {}

void Relay::set(bool on, uint32_t now_ms) {
  on_ = on;
  switched_once_ = true;
  last_switch_ms_ = now_ms;
  ++switches_;
}

RelayResult Relay::request(bool on, uint32_t now_ms) {
  if (on == on_) {
    return RelayResult::Unchanged;
  }

  if (on && switched_once_ && (now_ms - last_switch_ms_) < min_switch_ms_) {
    return RelayResult::TooSoon;
  }

  set(on, now_ms);

  return RelayResult::Switched;
}

bool Relay::update(uint32_t now_ms) {
  if (!on_ || max_on_ms_ == 0) {
    return false;
  }

  if ((now_ms - last_switch_ms_) < max_on_ms_) {
    return false;
  }

  set(false, now_ms);

  return true;
}
