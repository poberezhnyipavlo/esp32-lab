#include "PressDetector.h"

PressDetector::PressDetector(uint32_t long_ms) : long_ms_(long_ms) {}

PressEvent PressDetector::update(bool pressed, uint32_t now_ms) {
  if (pressed && !was_pressed_) {
    was_pressed_ = true;
    long_fired_ = false;
    press_start_ms_ = now_ms;

    return PressEvent::None;
  }

  if (pressed) {
    if (!long_fired_ && (now_ms - press_start_ms_) >= long_ms_) {
      long_fired_ = true;
      return PressEvent::Long;
    }

    return PressEvent::None;
  }

  if (was_pressed_) {
    was_pressed_ = false;
    return long_fired_ ? PressEvent::None : PressEvent::Short;
  }

  return PressEvent::None;
}
