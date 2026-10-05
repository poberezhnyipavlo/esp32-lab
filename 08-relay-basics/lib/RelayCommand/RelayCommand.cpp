#include "RelayCommand.h"

namespace {
  constexpr std::string_view k_spaces = " \t\r\n";

  std::string_view trim(std::string_view s) {
    const auto first = s.find_first_not_of(k_spaces);
    if (first == std::string_view::npos) {
      return {};
    }
    const auto last = s.find_last_not_of(k_spaces);
    return s.substr(first, last - first + 1);
  }
}

std::optional<Command> parseCommand(std::string_view line, uint8_t channels) {
  const std::string_view s = trim(line);

  if (s == "status") {
    return Command{CommandType::Status};
  }
  if (s == "off") {
    return Command{CommandType::AllOff};
  }

  if (s.size() < 4 || s[0] < '1' || s[0] > '9' || s[1] != ' ') {
    return std::nullopt;
  }
  const auto channel = static_cast<uint8_t>(s[0] - '0');
  if (channel > channels) {
    return std::nullopt;
  }

  const std::string_view word = trim(s.substr(1));
  if (word == "on") {
    return Command{CommandType::Set, channel, true};
  }
  if (word == "off") {
    return Command{CommandType::Set, channel, false};
  }
  return std::nullopt;
}
