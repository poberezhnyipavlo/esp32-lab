#pragma once

#include <cstdint>
#include <optional>

struct Climate {
  float temperature_c;
  float humidity_pct;
};

struct ClimateLimits {
  float min_temperature_c;
  float max_temperature_c;
  float min_humidity_pct;
  float max_humidity_pct;
};

class ClimateTracker
{
public:
  ClimateTracker(ClimateLimits limits, uint32_t stale_after_ms);

  bool update(float temperature_c, float humidity_pct, uint32_t now_ms);

  [[nodiscard]] std::optional<Climate> last() const { return last_; }
  [[nodiscard]] bool isStale(uint32_t now_ms) const;
  [[nodiscard]] uint32_t consecutiveFailures() const { return failures_; }
private:
  [[nodiscard]] bool isValid(float temperature_c, float humidity_pct) const;

  ClimateLimits limits_;
  uint32_t stale_after_ms_;
  std::optional<Climate> last_;
  uint32_t last_ok_ms_ = 0;
  uint32_t failures_ = 0;
};

