#include <gtest/gtest.h>

#include "RelayCommand.h"

namespace {
  constexpr uint8_t k_channels = 2;
}

TEST(RelayCommand, ParsesStatusAndAllOff) {
  const auto status = parseCommand("status", k_channels);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(status->type, CommandType::Status);

  const auto off = parseCommand("off", k_channels);
  ASSERT_TRUE(off.has_value());
  EXPECT_EQ(off->type, CommandType::AllOff);
}

TEST(RelayCommand, ParsesChannelOnOff) {
  const auto on = parseCommand("1 on", k_channels);
  ASSERT_TRUE(on.has_value());
  EXPECT_EQ(on->type, CommandType::Set);
  EXPECT_EQ(on->channel, 1);
  EXPECT_TRUE(on->on);

  const auto off = parseCommand("2 off", k_channels);
  ASSERT_TRUE(off.has_value());
  EXPECT_EQ(off->type, CommandType::Set);
  EXPECT_EQ(off->channel, 2);
  EXPECT_FALSE(off->on);
}

TEST(RelayCommand, IgnoresSurroundingSpacesAndCarriageReturn) {
  const auto cmd = parseCommand("  1   on \r", k_channels);
  ASSERT_TRUE(cmd.has_value());
  EXPECT_EQ(cmd->channel, 1);
  EXPECT_TRUE(cmd->on);
}

TEST(RelayCommand, RejectsChannelOutOfRange) {
  EXPECT_FALSE(parseCommand("0 on", k_channels).has_value());
  EXPECT_FALSE(parseCommand("3 on", k_channels).has_value());
  EXPECT_FALSE(parseCommand("12 on", k_channels).has_value());
}

TEST(RelayCommand, RejectsMalformedInput) {
  EXPECT_FALSE(parseCommand("", k_channels).has_value());
  EXPECT_FALSE(parseCommand("   ", k_channels).has_value());
  EXPECT_FALSE(parseCommand("1on", k_channels).has_value());
  EXPECT_FALSE(parseCommand("1 maybe", k_channels).has_value());
  EXPECT_FALSE(parseCommand("on 1", k_channels).has_value());
  EXPECT_FALSE(parseCommand("1 ON", k_channels).has_value());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
