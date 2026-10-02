#include <gtest/gtest.h>

#include "Debouncer.h"

namespace {
  constexpr uint32_t k_stable_ms = 50;
}

TEST(Debouncer, StartReleased) {
  Debouncer d(k_stable_ms);
  EXPECT_FALSE(d.isPressed());
}

TEST(Debouncer, CleanPressIsReportedOnceAfterStableTime) {
  Debouncer d(k_stable_ms);
  d.update(true, 0);
  d.update(true, 49);
  EXPECT_FALSE(d.isPressed());
  d.update(true, 50);
  EXPECT_TRUE(d.isPressed());
}

TEST(Debouncer, BounceIsIgnored) {
  Debouncer d(k_stable_ms);
  d.update(true, 0);
  d.update(false, 10);
  d.update(true, 20);
  d.update(false, 30);
  d.update(true, 40);
  d.update(true, 89);
  EXPECT_FALSE(d.isPressed());
  d.update(true, 90);
  EXPECT_TRUE(d.isPressed());
}

TEST(Debouncer, ReleaseDoesNotProduceEvent) {
  Debouncer d(k_stable_ms);
  d.update(true, 0);
  d.update(true, 50);
  ASSERT_TRUE(d.isPressed());
  d.update(false, 100);
  d.update(false, 149);
  EXPECT_TRUE(d.isPressed());
  d.update(false, 150);
  EXPECT_FALSE(d.isPressed());
}

TEST(Debouncer, ShortGlitchIsIgnored) {
  Debouncer d(k_stable_ms);
  d.update(true, 0);
  d.update(false, 10);
  d.update(false, 200);
  EXPECT_FALSE(d.isPressed());
}

TEST(Debouncer, WorksAcrossMillisOverflow) {
  constexpr uint32_t nearMax = 0xFFFFFFFF - 20;
  Debouncer d(k_stable_ms);
  d.update(true, nearMax);
  d.update(true, nearMax + 49);
  EXPECT_FALSE(d.isPressed());
  d.update(true, nearMax + 50);
  EXPECT_TRUE(d.isPressed());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
