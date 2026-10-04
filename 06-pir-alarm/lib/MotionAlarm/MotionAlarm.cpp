#include "MotionAlarm.h"

MotionAlarm::MotionAlarm(uint32_t start_ms, uint32_t warmup_ms, uint32_t alarm_ms, uint32_t beep_ms)
  : start_ms_(start_ms), warmup_ms_(warmup_ms), alarm_ms_(alarm_ms), beep_ms_(beep_ms) {}

bool MotionAlarm::isArmed(uint32_t now_ms) const {
  return (now_ms - start_ms_) >= warmup_ms_;
}

bool MotionAlarm::onMotion(uint32_t now_ms) {
  if (!isArmed(now_ms)) {
    return false;
  }

  alarm_started_ = true;
  alarm_start_ms_ = now_ms;
  ++events_;

  return true;
}

bool MotionAlarm::isAlarmActive(uint32_t now_ms) const {
  return alarm_started_ && (now_ms - alarm_start_ms_) < alarm_ms_;
}

bool MotionAlarm::buzzerOn(uint32_t now_ms) const {
  if (!isAlarmActive(now_ms)) {
    return false;
  }

  const uint32_t elapsed = now_ms - alarm_start_ms_;

  return (elapsed / beep_ms_) % 2 == 0;
}
