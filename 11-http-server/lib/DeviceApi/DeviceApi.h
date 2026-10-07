#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace api {
  constexpr size_t k_max_host_len = 63;
  constexpr size_t k_max_info_json = 768;

  bool isValidHost(std::string_view host);

  bool constantTimeEquals(std::string_view a, std::string_view b);

  std::string_view trim(std::string_view s);

  struct PutHostResult {
    int status;
    std::string_view error;
    std::string_view host;
  };

  PutHostResult decidePutHost(
    std::string_view expectedToken,
    std::string_view givenToken,
    std::optional<std::string_view> body
  );

  struct DeviceInfo {
    std::string_view firmware;
    std::string_view version;
    std::string_view built;
    std::string_view chip_model;
    uint32_t chip_revision;
    uint32_t chip_cores;
    uint32_t cpu_mhz;
    uint32_t flash_bytes;
    std::string_view arduino_core;
    std::string_view idf;
    uint32_t heap_free;
    uint32_t heap_min_free;
    uint32_t uptime_s;
    std::string_view ip;
    std::string_view mac;
    int32_t rssi_dbm;
    std::string_view monitor_host;
    bool monitor_host_from_nvs;
  };

  size_t buildInfoJson(const DeviceInfo& info, char* buf, size_t cap);
}
