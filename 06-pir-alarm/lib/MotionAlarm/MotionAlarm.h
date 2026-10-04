#pragma once

#include  <cstdint>

class MotionAlarm {
public:
  MotionAlarm(uint32_t start_ms, uint32_t warmup_ms, uint32_t alarm_ms, uint32_t beep_ms);

  bool onMotion(uint32_t now_ms);

  [[nodiscard]] bool isArmed(uint32_t now_ms) const;
  [[nodiscard]] bool isAlarmActive(uint32_t now_ms) const;
  [[nodiscard]] bool buzzerOn(uint32_t now_ms) const;
  [[nodiscard]] uint32_t eventCount() const { return events_; };
private:
  uint32_t start_ms_;
  uint32_t warmup_ms_;
  uint32_t alarm_ms_;
  uint32_t beep_ms_;
  bool alarm_started_ = false;
  uint32_t alarm_start_ms_ = 0;
  uint32_t events_ = 0;
};
