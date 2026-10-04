#include "ClimateTracker.h"

#include <cmath>

ClimateTracker::ClimateTracker(ClimateLimits limits, uint32_t stale_after_ms)
  : limits_(limits), stale_after_ms_(stale_after_ms) {}

bool ClimateTracker::isValid(float temperature_c, float humidity_pct) const {
  if (std::isnan(temperature_c) || std::isnan(humidity_pct)) {
    return false;
  }

  return temperature_c >= limits_.min_temperature_c
    && temperature_c <= limits_.max_temperature_c
    && humidity_pct >= limits_.min_humidity_pct
    && humidity_pct <= limits_.max_humidity_pct;
}

bool ClimateTracker::update(float temperature_c, float humidity_pct, uint32_t now_ms) {
  if (!isValid(temperature_c, humidity_pct)) {
    ++failures_;
    return false;
  }

  last_ = Climate{.temperature_c = temperature_c, .humidity_pct = humidity_pct};
  last_ok_ms_ = now_ms;
  failures_ = 0;

  return true;
}

bool ClimateTracker::isStale(uint32_t now_ms) const {
  if (!last_) {
    return true;
  }

  return (now_ms - last_ok_ms_) > stale_after_ms_;
}
