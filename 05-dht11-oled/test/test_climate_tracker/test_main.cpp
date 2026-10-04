#include <gtest/gtest.h>

#include <limits>

#include "ClimateTracker.h"

namespace {
constexpr ClimateLimits k_limits{-20.0F, 60.0F, 0.0F, 100.0F};
constexpr uint32_t k_stale_ms = 10000;
constexpr float k_nan = std::numeric_limits<float>::quiet_NaN();
}

TEST(ClimateTracker, NoReadingInitially) {
  const ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_FALSE(t.last().has_value());
  EXPECT_TRUE(t.isStale(0));
  EXPECT_EQ(t.consecutiveFailures(), 0U);
}

TEST(ClimateTracker, ValidReadingIsAccepted) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_TRUE(t.update(23.5F, 45.0F, 1000));
  ASSERT_TRUE(t.last().has_value());
  EXPECT_FLOAT_EQ(t.last()->temperature_c, 23.5F);
  EXPECT_FLOAT_EQ(t.last()->humidity_pct, 45.0F);
  EXPECT_FALSE(t.isStale(1000));
}

TEST(ClimateTracker, NanIsRejected) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_FALSE(t.update(k_nan, 45.0F, 0));
  EXPECT_FALSE(t.update(23.0F, k_nan, 0));
  EXPECT_FALSE(t.last().has_value());
  EXPECT_EQ(t.consecutiveFailures(), 2U);
}

TEST(ClimateTracker, OutOfRangeIsRejected) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_FALSE(t.update(85.0F, 45.0F, 0));   // занадто гаряче
  EXPECT_FALSE(t.update(23.0F, 120.0F, 0));  // вологість > 100 %
  EXPECT_EQ(t.consecutiveFailures(), 2U);
}

TEST(ClimateTracker, LimitsAreInclusive) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_TRUE(t.update(-20.0F, 0.0F, 0));
  EXPECT_TRUE(t.update(60.0F, 100.0F, 0));
}

TEST(ClimateTracker, FailureKeepsLastGoodValue) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_TRUE(t.update(22.0F, 50.0F, 1000));
  EXPECT_FALSE(t.update(k_nan, k_nan, 3000));
  ASSERT_TRUE(t.last().has_value());
  EXPECT_FLOAT_EQ(t.last()->temperature_c, 22.0F);
  EXPECT_EQ(t.consecutiveFailures(), 1U);
}

TEST(ClimateTracker, SuccessResetsFailures) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_FALSE(t.update(k_nan, k_nan, 0));
  EXPECT_FALSE(t.update(k_nan, k_nan, 2000));
  EXPECT_TRUE(t.update(21.0F, 40.0F, 4000));
  EXPECT_EQ(t.consecutiveFailures(), 0U);
}

TEST(ClimateTracker, BecomesStaleAfterTimeout) {
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_TRUE(t.update(22.0F, 50.0F, 1000));
  EXPECT_FALSE(t.isStale(1000 + k_stale_ms));
  EXPECT_TRUE(t.isStale(1000 + k_stale_ms + 1));
}

TEST(ClimateTracker, StaleWorksAcrossMillisOverflow) {
  constexpr uint32_t nearMax = 0xFFFFFFFF - 1000;
  ClimateTracker t(k_limits, k_stale_ms);
  EXPECT_TRUE(t.update(22.0F, 50.0F, nearMax));
  EXPECT_FALSE(t.isStale(nearMax + k_stale_ms));
  EXPECT_TRUE(t.isStale(nearMax + k_stale_ms + 1));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
