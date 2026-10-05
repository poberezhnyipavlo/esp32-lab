#include <gtest/gtest.h>

#include "Relay.h"

namespace {
constexpr uint32_t k_min_switch_ms = 500;
constexpr uint32_t k_max_on_ms = 10000;

Relay makeRelay(uint32_t max_on_ms = k_max_on_ms) {
  return Relay(true, k_min_switch_ms, max_on_ms);
}
}

TEST(Relay, StartsOffWithInactiveLevel) {
  const Relay r = makeRelay();
  EXPECT_FALSE(r.isOn());
  EXPECT_TRUE(r.pinLevel());
  EXPECT_EQ(r.switchCount(), 0U);
}

TEST(Relay, OnDrivesPinLowWhenActiveLow) {
  Relay r = makeRelay();
  EXPECT_EQ(r.request(true, 0), RelayResult::Switched);
  EXPECT_TRUE(r.isOn());
  EXPECT_FALSE(r.pinLevel());
}

TEST(Relay, ActiveHighMapsDirectly) {
  Relay r(false, k_min_switch_ms, k_max_on_ms);
  EXPECT_FALSE(r.pinLevel());
  EXPECT_EQ(r.request(true, 0), RelayResult::Switched);
  EXPECT_TRUE(r.pinLevel());
}

TEST(Relay, FirstSwitchIsNeverTooSoon) {
  Relay r = makeRelay();
  EXPECT_EQ(r.request(true, 0), RelayResult::Switched);  // now - 0 < 500, але це перше перемикання
}

TEST(Relay, RepeatedRequestIsUnchanged) {
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, 0), RelayResult::Switched);
  EXPECT_EQ(r.request(true, 1), RelayResult::Unchanged);
  EXPECT_EQ(r.request(true, 5000), RelayResult::Unchanged);
  EXPECT_EQ(r.switchCount(), 1U);
}

TEST(Relay, OffIsAlwaysAllowed) {
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, 1000), RelayResult::Switched);
  EXPECT_EQ(r.request(false, 1001), RelayResult::Switched);
  EXPECT_FALSE(r.isOn());
}

TEST(Relay, OnTooSoonAfterSwitchIsRefused) {
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, 1000), RelayResult::Switched);
  ASSERT_EQ(r.request(false, 1010), RelayResult::Switched);
  EXPECT_EQ(r.request(true, 1010 + k_min_switch_ms - 1), RelayResult::TooSoon);
  EXPECT_FALSE(r.isOn());
  EXPECT_EQ(r.request(true, 1010 + k_min_switch_ms), RelayResult::Switched);
}

TEST(Relay, AutoOffAfterMaxOn) {
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, 1000), RelayResult::Switched);
  EXPECT_FALSE(r.update(1000 + k_max_on_ms - 1));
  EXPECT_TRUE(r.isOn());
  EXPECT_TRUE(r.update(1000 + k_max_on_ms));
  EXPECT_FALSE(r.isOn());
  EXPECT_TRUE(r.pinLevel());
  EXPECT_FALSE(r.update(1000 + k_max_on_ms + 1));
}

TEST(Relay, AutoOffCountsAsSwitch) {
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, 0), RelayResult::Switched);
  ASSERT_TRUE(r.update(k_max_on_ms));
  EXPECT_EQ(r.switchCount(), 2U);
  EXPECT_EQ(r.request(true, k_max_on_ms + 1), RelayResult::TooSoon);
}

TEST(Relay, ZeroMaxOnDisablesAutoOff) {
  Relay r = makeRelay(0);
  ASSERT_EQ(r.request(true, 0), RelayResult::Switched);
  EXPECT_FALSE(r.update(0xFFFFFFFF));
  EXPECT_TRUE(r.isOn());
}

TEST(Relay, AutoOffWorksAcrossMillisOverflow) {
  constexpr uint32_t start = 0xFFFFFFFF - 1000;
  Relay r = makeRelay();
  ASSERT_EQ(r.request(true, start), RelayResult::Switched);
  EXPECT_FALSE(r.update(start + k_max_on_ms - 1));
  EXPECT_TRUE(r.update(start + k_max_on_ms));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
