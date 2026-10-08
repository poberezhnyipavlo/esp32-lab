#include <gtest/gtest.h>

#include "PeakTracker.h"

TEST(PeakTracker, KeepsMaximum) {
  PeakTracker p;
  p.record(3);
  p.record(3002);
  p.record(5);
  EXPECT_EQ(p.take(), 3002U);
}

TEST(PeakTracker, TakeStartsNewPeriod) {
  PeakTracker p;
  p.record(3002);
  EXPECT_EQ(p.take(), 3002U);
  EXPECT_EQ(p.take(), 0U);
  p.record(4);
  EXPECT_EQ(p.take(), 4U);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
