#include <gtest/gtest.h>
#include "PressDetector.h"

namespace {
  constexpr uint32_t k_long_ms = 1000;
}

TEST(PressDetector, ShortPressReportedOnRelease) {
  PressDetector d(k_long_ms);

  EXPECT_EQ(d.update(true, 0), PressEvent::None);
  EXPECT_EQ(d.update(true, 200), PressEvent::None);
  EXPECT_EQ(d.update(false, 200), PressEvent::Short);
}

TEST(PressDetector, LongPressReportedAtThreshold) {
  PressDetector d(k_long_ms);
  EXPECT_EQ(d.update(true, 0), PressEvent::None);
  EXPECT_EQ(d.update(true, 999), PressEvent::None);
  EXPECT_EQ(d.update(true, 1000), PressEvent::Long);
}

TEST(PressDetector, LongPressReportedOnlyOnce) {
  PressDetector d(k_long_ms);
  EXPECT_EQ(d.update(true, 0), PressEvent::None);
  EXPECT_EQ(d.update(true, 1000), PressEvent::Long);
  EXPECT_EQ(d.update(true, 3000), PressEvent::None);
}

TEST(PressDetector, ReleaseAfterLongIsNotShort) {
  PressDetector d(k_long_ms);
  EXPECT_EQ(d.update(true, 0), PressEvent::None);
  EXPECT_EQ(d.update(true, 1000), PressEvent::Long);
  EXPECT_EQ(d.update(false, 1500), PressEvent::None);
}

TEST(PressDetector, WorksAcrossMillisOverflow) {
  constexpr uint32_t nearMax = 0xFFFFFFFF - 500;
  PressDetector d(k_long_ms);
  EXPECT_EQ(d.update(true, nearMax), PressEvent::None);
  EXPECT_EQ(d.update(true, nearMax + 999), PressEvent::None);
  EXPECT_EQ(d.update(true, nearMax + 1000), PressEvent::Long);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}


