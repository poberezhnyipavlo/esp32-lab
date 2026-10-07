#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

#include "DeviceApi.h"

using api::constantTimeEquals;
using api::decidePutHost;
using api::isValidHost;
using api::trim;

TEST(Host, AcceptsIpv4AndNames) {
  EXPECT_TRUE(isValidHost("192.168.0.247"));
  EXPECT_TRUE(isValidHost("10.13.37.1"));
  EXPECT_TRUE(isValidHost("0.0.0.0"));
  EXPECT_TRUE(isValidHost("255.255.255.255"));
  EXPECT_TRUE(isValidHost("pavlo-mac.local"));
  EXPECT_TRUE(isValidHost("Pavlo-MacBook.local"));
  EXPECT_TRUE(isValidHost("monitor"));
  EXPECT_TRUE(isValidHost("host.wokwi.internal"));
  EXPECT_TRUE(isValidHost("1password.com"));
}

TEST(Host, RejectsBadIpv4) {
  EXPECT_FALSE(isValidHost("256.1.1.1"));
  EXPECT_FALSE(isValidHost("999.1.1.1"));
  EXPECT_FALSE(isValidHost("192.168.0"));
  EXPECT_FALSE(isValidHost("192.168.0.1.5"));
  EXPECT_FALSE(isValidHost("192.168.00.1"));
  EXPECT_FALSE(isValidHost("192.168..1"));
  EXPECT_FALSE(isValidHost("192.168.0.1."));
  EXPECT_FALSE(isValidHost("1234.1.1.1"));
}

TEST(Host, RejectsBadNames) {
  EXPECT_FALSE(isValidHost(""));
  EXPECT_FALSE(isValidHost("my host"));
  EXPECT_FALSE(isValidHost("host:9000"));
  EXPECT_FALSE(isValidHost("http://192.168.0.1"));
  EXPECT_FALSE(isValidHost("-mac.local"));
  EXPECT_FALSE(isValidHost("mac-.local"));
  EXPECT_FALSE(isValidHost(".local"));
  EXPECT_FALSE(isValidHost("mac..local"));
  EXPECT_FALSE(isValidHost("mac.local."));
  EXPECT_FALSE(isValidHost("ім'я.local"));
  EXPECT_FALSE(isValidHost("a\"b"));
  EXPECT_TRUE(isValidHost(std::string(63, 'a')));
  EXPECT_FALSE(isValidHost(std::string(64, 'a')));
}

TEST(ConstantTime, ComparesWholeStrings) {
  EXPECT_TRUE(constantTimeEquals("secret-token", "secret-token"));
  EXPECT_TRUE(constantTimeEquals("", ""));
  EXPECT_FALSE(constantTimeEquals("secret-token", "secret-tokeN"));
  EXPECT_FALSE(constantTimeEquals("secret-token", "Secret-token"));
  EXPECT_FALSE(constantTimeEquals("secret-token", "secret-toke"));
  EXPECT_FALSE(constantTimeEquals("secret-token", ""));
}

TEST(Trim, StripsSpacesAndLineEnds) {
  EXPECT_EQ(trim("192.168.0.1\n"), "192.168.0.1");
  EXPECT_EQ(trim("  192.168.0.1\r\n"), "192.168.0.1");
  EXPECT_EQ(trim("\t\n "), "");
  EXPECT_EQ(trim(""), "");
  EXPECT_EQ(trim("a b"), "a b");
}

TEST(PutHost, HappyPath) {
  const auto r = decidePutHost("tok", "tok", std::string_view("192.168.0.247\n"));
  EXPECT_EQ(r.status, 200);
  EXPECT_EQ(r.host, "192.168.0.247");
  EXPECT_TRUE(r.error.empty());
}

TEST(PutHost, TokenIsCheckedFirst) {
  EXPECT_EQ(decidePutHost("tok", "", std::string_view("192.168.0.1")).status, 401);
  EXPECT_EQ(decidePutHost("tok", "bad", std::string_view("192.168.0.1")).status, 401);
  EXPECT_EQ(decidePutHost("tok", "", std::nullopt).status, 401);
  EXPECT_EQ(decidePutHost("tok", "bad", std::string_view("not a host")).status, 401);
}

TEST(PutHost, EmptyServerTokenRejectsEverything) {
  EXPECT_EQ(decidePutHost("", "", std::string_view("192.168.0.1")).status, 500);
  EXPECT_EQ(decidePutHost("", "anything", std::string_view("192.168.0.1")).status, 500);
}

TEST(PutHost, BodyTypeThenValue) {
  EXPECT_EQ(decidePutHost("tok", "tok", std::nullopt).status, 415);
  EXPECT_EQ(decidePutHost("tok", "tok", std::string_view("")).status, 400);
  EXPECT_EQ(decidePutHost("tok", "tok", std::string_view("my host")).status, 400);
  EXPECT_EQ(decidePutHost("tok", "tok", std::string_view("256.0.0.1")).status, 400);
}

namespace {
  api::DeviceInfo sampleInfo() {
    return api::DeviceInfo{
      "11-http-server", "1.0.0", "Oct  7 2026 14:00:00", "ESP32-D0WD-V3",
      301, 2, 240, 4194304, "3.3.12", "v5.5.5", 230340, 221000, 123,
      "192.168.0.220", "20:9B:A9:61:15:6C", -60, "192.168.0.247", true
    };
  }
}

TEST(InfoJson, ExactFormat) {
  std::array<char, api::k_max_info_json> buf{};
  const size_t n = api::buildInfoJson(sampleInfo(), buf.data(), buf.size());
  ASSERT_GT(n, 0U);
  EXPECT_EQ(std::string_view(buf.data(), n),
            "{\"firmware\":\"11-http-server\",\"version\":\"1.0.0\","
            "\"built\":\"Oct  7 2026 14:00:00\","
            "\"chip\":{\"model\":\"ESP32-D0WD-V3\",\"revision\":301,\"cores\":2,\"cpu_mhz\":240,"
            "\"flash_bytes\":4194304},"
            "\"sdk\":{\"arduino_core\":\"3.3.12\",\"idf\":\"v5.5.5\"},"
            "\"heap\":{\"free_bytes\":230340,\"min_free_bytes\":221000},"
            "\"uptime_s\":123,"
            "\"wifi\":{\"ip\":\"192.168.0.220\",\"mac\":\"20:9B:A9:61:15:6C\",\"rssi_dbm\":-60},"
            "\"monitor_host\":{\"value\":\"192.168.0.247\",\"source\":\"nvs\"}}\n");
}

TEST(InfoJson, SourceFirmware) {
  auto info = sampleInfo();
  info.monitor_host_from_nvs = false;
  std::array<char, api::k_max_info_json> buf{};
  const size_t n = api::buildInfoJson(info, buf.data(), buf.size());
  ASSERT_GT(n, 0U);
  EXPECT_NE(std::string_view(buf.data(), n).find("\"source\":\"firmware\""), std::string_view::npos);
}

TEST(InfoJson, RejectsUnsafeStringsAndSmallBuffer) {
  std::array<char, api::k_max_info_json> buf{};
  auto info = sampleInfo();
  info.chip_model = "ESP32\"injected";
  EXPECT_EQ(api::buildInfoJson(info, buf.data(), buf.size()), 0U);
  info = sampleInfo();
  info.built = "line\nbreak";
  EXPECT_EQ(api::buildInfoJson(info, buf.data(), buf.size()), 0U);

  std::array<char, 64> small{};
  EXPECT_EQ(api::buildInfoJson(sampleInfo(), small.data(), small.size()), 0U);
}

TEST(InfoJson, LongestHostFits) {
  auto info = sampleInfo();
  const std::string host(api::k_max_host_len, 'a');
  info.monitor_host = host;
  std::array<char, api::k_max_info_json> buf{};
  EXPECT_GT(api::buildInfoJson(info, buf.data(), buf.size()), 0U);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
