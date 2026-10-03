#include <gtest/gtest.h>
#include "DutyMap.h"

namespace {
  constexpr uint32_t k_adc_max = 4095;
  constexpr uint32_t k_duty_max = 255;
}

static_assert(adcToDuty(k_adc_max, k_adc_max, k_duty_max) == k_duty_max);

TEST(DutyMap, ZeroGivesZero) {
  EXPECT_EQ(adcToDuty(0, k_adc_max, k_duty_max), 0U);
}

TEST(DutyMap, MaxGivesMax) {
  EXPECT_EQ(adcToDuty(k_adc_max, k_adc_max, k_duty_max), k_duty_max);
}

TEST(DutyMap, AboveMaxIsClamped) {
  EXPECT_EQ(adcToDuty(5000, k_adc_max, k_duty_max), k_duty_max);
}

TEST(DutyMap, MiddleGivesHalf) {
  EXPECT_EQ(adcToDuty(2048, k_adc_max, k_duty_max), 127U);
}

TEST(DutyMap, IsMonotonic) {
  uint32_t prev = 0;
  for (uint32_t raw = 0; raw <= k_adc_max; ++raw) {
    const uint32_t duty = adcToDuty(raw, k_adc_max, k_duty_max);
    ASSERT_GE(duty, prev) << "raw=" << raw;
    prev = duty;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
