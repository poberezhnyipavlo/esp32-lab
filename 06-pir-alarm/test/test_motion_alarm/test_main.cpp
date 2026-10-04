#include <gtest/gtest.h>

#include "MotionAlarm.h"

namespace {
constexpr uint32_t k_warmup_ms = 30000;
constexpr uint32_t k_alarm_ms = 3000;
constexpr uint32_t k_beep_ms = 200;

MotionAlarm makeAlarm(uint32_t start_ms = 0) {
  return MotionAlarm(start_ms, k_warmup_ms, k_alarm_ms, k_beep_ms);
}
}

TEST(MotionAlarm, NotArmedDuringWarmup) {
  const MotionAlarm a = makeAlarm();
  EXPECT_FALSE(a.isArmed(0));
  EXPECT_FALSE(a.isArmed(k_warmup_ms - 1));
  EXPECT_TRUE(a.isArmed(k_warmup_ms));
}

TEST(MotionAlarm, MotionDuringWarmupIsIgnored) {
  MotionAlarm a = makeAlarm();
  EXPECT_FALSE(a.onMotion(1000));
  EXPECT_EQ(a.eventCount(), 0U);
  EXPECT_FALSE(a.isAlarmActive(1000));
  EXPECT_FALSE(a.buzzerOn(1000));
}

TEST(MotionAlarm, MotionAfterWarmupStartsAlarm) {
  MotionAlarm a = makeAlarm();
  EXPECT_TRUE(a.onMotion(k_warmup_ms));
  EXPECT_EQ(a.eventCount(), 1U);
  EXPECT_TRUE(a.isAlarmActive(k_warmup_ms));
}

TEST(MotionAlarm, AlarmLastsExactlyAlarmMs) {
  MotionAlarm a = makeAlarm();
  const uint32_t t = k_warmup_ms + 500;
  EXPECT_TRUE(a.onMotion(t));
  EXPECT_TRUE(a.isAlarmActive(t + k_alarm_ms - 1));
  EXPECT_FALSE(a.isAlarmActive(t + k_alarm_ms));
  EXPECT_FALSE(a.buzzerOn(t + k_alarm_ms));
}

TEST(MotionAlarm, BuzzerBeepsOnAndOff) {
  MotionAlarm a = makeAlarm();
  const uint32_t t = k_warmup_ms;
  EXPECT_TRUE(a.onMotion(t));
  EXPECT_TRUE(a.buzzerOn(t));
  EXPECT_TRUE(a.buzzerOn(t + k_beep_ms - 1));
  EXPECT_FALSE(a.buzzerOn(t + k_beep_ms));
  EXPECT_FALSE(a.buzzerOn(t + 2 * k_beep_ms - 1));
  EXPECT_TRUE(a.buzzerOn(t + 2 * k_beep_ms));
}

TEST(MotionAlarm, RetriggerRestartsAlarm) {
  MotionAlarm a = makeAlarm();
  const uint32_t t = k_warmup_ms;
  EXPECT_TRUE(a.onMotion(t));
  EXPECT_TRUE(a.onMotion(t + 2000));
  EXPECT_EQ(a.eventCount(), 2U);
  EXPECT_TRUE(a.isAlarmActive(t + 2000 + k_alarm_ms - 1));
  EXPECT_FALSE(a.isAlarmActive(t + 2000 + k_alarm_ms));
}

TEST(MotionAlarm, NoBuzzerWithoutMotion) {
  const MotionAlarm a = makeAlarm();
  EXPECT_FALSE(a.isAlarmActive(k_warmup_ms + 10000));
  EXPECT_FALSE(a.buzzerOn(k_warmup_ms + 10000));
}

TEST(MotionAlarm, WorksAcrossMillisOverflow) {
  constexpr uint32_t start = 0xFFFFFFFF - 10000;
  MotionAlarm a = makeAlarm(start);
  EXPECT_FALSE(a.isArmed(start + k_warmup_ms - 1));
  EXPECT_TRUE(a.isArmed(start + k_warmup_ms));
  const uint32_t t = 0xFFFFFFFF - 100;
  MotionAlarm b = makeAlarm(t - k_warmup_ms);
  EXPECT_TRUE(b.onMotion(t));
  EXPECT_TRUE(b.isAlarmActive(t + k_alarm_ms - 1));
  EXPECT_FALSE(b.isAlarmActive(t + k_alarm_ms));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
