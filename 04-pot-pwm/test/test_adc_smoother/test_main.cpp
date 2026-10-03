#include <gtest/gtest.h>
#include "AdcSmoother.h"

namespace {
  constexpr uint8_t k_shift = 3;
  constexpr uint16_t k_adc_max = 4095;
}

TEST(AdcSmoother, FirstSampleIsTakenAsIs) {
  AdcSmoother s(k_shift);
  EXPECT_EQ(s.update(1000), 1000);
}

TEST(AdcSmoother, ConstantInputStaysConstant) {
  AdcSmoother s(k_shift);
  for (int i = 0; i < 50; ++i) {
    ASSERT_EQ(s.update(1234), 1234);
  }
}

TEST(AdcSmoother, StepMovesOneEighthOfTheWay) {
  AdcSmoother s(k_shift);
  EXPECT_EQ(s.update(0), 0);
  EXPECT_EQ(s.update(k_adc_max), 511);
}

TEST(AdcSmoother, StepUpReachesTargetExactly) {
  AdcSmoother s(k_shift);
  EXPECT_EQ(s.update(0), 0);
  uint16_t prev = 0;
  for (int i = 0; i < 200; ++i) {
    const uint16_t v = s.update(k_adc_max);
    ASSERT_GE(v, prev);
    prev = v;
  }
  EXPECT_EQ(s.value(), k_adc_max);
}

TEST(AdcSmoother, StepDownReachesTargetExactly) {
  AdcSmoother s(k_shift);
  EXPECT_EQ(s.update(k_adc_max), k_adc_max);
  uint16_t prev = k_adc_max;
  for (int i = 0; i < 200; ++i) {
    const uint16_t v = s.update(0);
    ASSERT_LE(v, prev);
    prev = v;
  }
  EXPECT_EQ(s.value(), 0);
}

TEST(AdcSmoother, SingleSpikeIsDamped) {
  AdcSmoother s(k_shift);
  EXPECT_EQ(s.update(2000), 2000);
  EXPECT_LT(s.update(4000), 2300);
  EXPECT_EQ(s.update(2000), 2218);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
