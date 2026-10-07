#include "DeviceApi.h"

#include <cstdio>
#include <algorithm>

namespace api {
  namespace {
    bool isDigit(char c) { return c >= '0' && c <= '9'; }
    bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

    bool isValidIpv4(std::string_view s) {
      int parts = 0;
      while (true) {
        const size_t dot = s.find('.');
        const std::string_view part = s.substr(0, dot);
        if (part.empty() || part.size() > 3) {
          return false;
        }
        if (part.size() > 1 && part[0] == '0') {
          return false;
        }
        int value = 0;
        for (const char c : part) {
          if (!isDigit(c)) {
            return false;
          }
          value = value * 10 + (c - '0');
        }
        if (value > 255) {
          return false;
        }
        ++parts;
        if (dot == std::string_view::npos) {
          break;
        }
        s.remove_prefix(dot + 1);
      }
      return parts == 4;
    }

    bool isValidLabel(std::string_view label) {
      if (label.empty() || label.length() > 63) {
        return false;
      }
      if (label.front() == '-' || label.back() == '-') {
        return false;
      }

      return std::all_of(label.begin(), label.end(),
        [](char c) { return isLetter(c) || isDigit(c) || c == '-'; }
      );
    }

    bool onlyDigitsAndDots(std::string_view s) {
      return std::all_of(s.begin(), s.end(), [](char c) { return isDigit(c) || c=='.'; });
    }

    bool isJsonSafe(std::string_view s) {
      return std::none_of(s.begin(), s.end(), [](char c) {
         return c == '"' || c == '\\' || static_cast<unsigned char>(c) < 0x20;
       });
    }

    int len(std::string_view s) { return static_cast<int>(s.size()); };
  }

  bool isValidHost(std::string_view host) {
    if (host.empty() || host.length() > k_max_host_len) {
      return false;
    }

    if (onlyDigitsAndDots(host)) {
      return isValidIpv4(host);
    }

    while (true) {
      const size_t dot = host.find('.');
      if (!isValidLabel(host.substr(0, dot))) {
        return false;
      }
      if (dot == std::string_view::npos) {
        return true;
      }
      host.remove_prefix(dot + 1);
    }
  }

  bool constantTimeEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
      return false;
    }

    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
      diff = static_cast<unsigned char>(
        diff | (static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]))
      );
    }

    return diff == 0;
  }

  std::string_view trim(std::string_view s) {
    constexpr std::string_view spaces = " \t\r\n";
    const size_t first = s.find_first_not_of(spaces);
    if (first == std::string_view::npos) {
      return {};
    }
    const size_t last = s.find_last_not_of(spaces);
    return s.substr(first, last - first + 1);
  }

  PutHostResult decidePutHost(
    std::string_view expectedToken,
    std::string_view givenToken,
    std::optional<std::string_view> body
  ) {
    if (expectedToken.empty()) {
      return {.status = 500, .error = "API token is not configured", .host = {}};
    }
    if (!constantTimeEquals(expectedToken, givenToken)) {
      return {.status = 401, .error = "missing or wrong X-Api-Token", .host = {}};
    }
    if (!body) {
      return {.status = 415, .error = "send the host as Content-Type: text/plain", .host = {}};
    }
    const std::string_view host = trim(*body);
    if (!isValidHost(host)) {
      return {.status = 400, .error = "invalid host", .host = {}};
    }
    return {.status = 200, .error = {}, .host = host};
  }

  size_t buildInfoJson(const DeviceInfo &info, char *buf, size_t cap) {
    for (const std::string_view s : {info.firmware, info.version, info.built, info.chip_model, info.arduino_core, info.idf, info.ip, info.mac, info.monitor_host}) {
      if (!isJsonSafe(s)) {
        return 0;
      }
    }

    const int n = std::snprintf(
      buf,
      cap,
      "{\"firmware\":\"%.*s\",\"version\":\"%.*s\",\"built\":\"%.*s\","
      "\"chip\":{\"model\":\"%.*s\",\"revision\":%lu,\"cores\":%lu,\"cpu_mhz\":%lu,"
      "\"flash_bytes\":%lu},"
      "\"sdk\":{\"arduino_core\":\"%.*s\",\"idf\":\"%.*s\"},"
      "\"heap\":{\"free_bytes\":%lu,\"min_free_bytes\":%lu},"
      "\"uptime_s\":%lu,"
      "\"wifi\":{\"ip\":\"%.*s\",\"mac\":\"%.*s\",\"rssi_dbm\":%ld},"
      "\"monitor_host\":{\"value\":\"%.*s\",\"source\":\"%s\"}}\n",
      len(info.firmware),
      info.firmware.data(),
      len(info.version),
      info.version.data(),
      len(info.built),
      info.built.data(),
      len(info.chip_model),
      info.chip_model.data(),
      static_cast<unsigned long>(info.chip_revision),
      static_cast<unsigned long>(info.chip_cores),
      static_cast<unsigned long>(info.cpu_mhz),
      static_cast<unsigned long>(info.flash_bytes),
      len(info.arduino_core),
      info.arduino_core.data(),
      len(info.idf),
      info.idf.data(),
      static_cast<unsigned long>(info.heap_free),
      static_cast<unsigned long>(info.heap_min_free),
      static_cast<unsigned long>(info.uptime_s),
      len(info.ip),
      info.ip.data(),
      len(info.mac),
      info.mac.data(),
      static_cast<long>(info.rssi_dbm),
      len(info.monitor_host),
      info.monitor_host.data(),
      info.monitor_host_from_nvs ? "nvs" : "firmware"
    );

    if (n < 0 || static_cast<size_t>(n) >= cap) {
      return 0;
    }

    return static_cast<size_t>(n);
  }
}
