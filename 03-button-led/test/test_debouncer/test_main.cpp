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

  EXPECT_FALSE(d.update(true, 0));
  EXPECT_FALSE(d.update(true, 49));
  EXPECT_TRUE(d.update(true, 50));
  EXPECT_TRUE(d.isPressed());
  EXPECT_FALSE(d.update(true, 100));
}

TEST(Debouncer, BounceIsIgnored) {
  Debouncer d(k_stable_ms);

  EXPECT_FALSE(d.update(true, 0));
  EXPECT_FALSE(d.update(false, 2));
  EXPECT_FALSE(d.update(true, 4));
  EXPECT_FALSE(d.update(false, 7));
  EXPECT_FALSE(d.update(true, 10));
  EXPECT_FALSE(d.update(true, 59));
  EXPECT_TRUE(d.update(true, 60));
}

TEST(Debouncer, ReleaseDoesNotProduceEvent) {
  Debouncer d(k_stable_ms);
  d.update(true, 0);

  ASSERT_TRUE(d.update(true, 50));
  EXPECT_FALSE(d.update(false, 100));
  EXPECT_FALSE(d.update(false, 150));
  EXPECT_FALSE(d.isPressed());
}

TEST(Debouncer, ShortGlitchIsIgnored) {
  Debouncer d(k_stable_ms);

  EXPECT_FALSE(d.update(true, 0));
  EXPECT_FALSE(d.update(false, 20));
  EXPECT_FALSE(d.update(false, 200));
  EXPECT_FALSE(d.isPressed());
}

TEST(Debouncer, WorksAcrossMillisOverflow) {
  Debouncer d(k_stable_ms);

  const uint32_t nearMax = 0xFFFFFFFF - 20;
  EXPECT_FALSE(d.update(true, nearMax));
  EXPECT_FALSE(d.update(true, nearMax + 40));
  EXPECT_TRUE(d.update(true, nearMax + 50));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
