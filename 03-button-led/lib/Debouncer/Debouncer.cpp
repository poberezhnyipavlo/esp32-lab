#include "Debouncer.h"

Debouncer::Debouncer(uint32_t stable_ms) : stable_ms_(stable_ms) {}

bool Debouncer::update(bool raw, uint32_t now_ms) {
  if (raw != last_raw_) {
    last_raw_ = raw;
    last_change_ms_ = now_ms;

    return false;
  }

  if (raw != stable_ && (now_ms - last_change_ms_) >= stable_ms_) {
    stable_ = raw;

    return stable_;
  }

  return false;
}
