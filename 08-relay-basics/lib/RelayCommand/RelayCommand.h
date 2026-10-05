#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class CommandType {
  Set,
  AllOff,
  Status
};

struct Command {
  CommandType type;
  uint8_t channel = 0;
  bool on = false;
};

std::optional<Command> parseCommand(std::string_view line, uint8_t channels);
