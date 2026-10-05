#include <Arduino.h>

#include <array>
#include <string_view>

#include "Relay.h"
#include "RelayCommand.h"

namespace {
  constexpr uint8_t CHANNELS = 2;
  constexpr std::array<uint8_t, CHANNELS> RELAY_PINS = {26, 27};
  constexpr bool ACTIVE_LOW = true;
  constexpr uint32_t MIN_SWITCH_MS = 500;
  constexpr uint32_t MAX_ON_MS = 10000;

  std::array<Relay, CHANNELS> relays = {
    Relay(ACTIVE_LOW, MIN_SWITCH_MS, MAX_ON_MS),
    Relay(ACTIVE_LOW, MIN_SWITCH_MS, MAX_ON_MS),
  };

  std::array<char, 32> lineBuf{};
  size_t lineLen = 0;
  bool lineOverflow = false;

  void applyPin(size_t i) {
    digitalWrite(RELAY_PINS[i], relays[i].pinLevel() ? HIGH : LOW);
  }

  unsigned channelNumber(size_t i) { return static_cast<unsigned>(i + 1); }

  void printStatus() {
    for (size_t i = 0; i < CHANNELS; i++) {
      Serial.printf(
        "Relay %u: %s, pin=%s, switches=%lu,\n",
        channelNumber(i),
        relays[i].isOn() ? "ON" : "OFF",
        relays[i].pinLevel() ? "HIGH" : "LOW",
        static_cast<unsigned long>(relays[i].switchCount())
      );
    }
  }

  void setRelay(size_t i, bool on, uint32_t now) {
    switch (relays[i].request(on, now)) {
      case RelayResult::Switched:
        applyPin(i);
        Serial.printf("Relay %u %s\n", channelNumber(i), on ? "ON" : "OFF");
        break;
      case RelayResult::Unchanged:
        Serial.printf("Relay %u already %s\n", channelNumber(i), on ? "ON" : "OFF");
        break;
      case RelayResult::TooSoon:
        Serial.printf(
          "Relay %u: too soon (min %lu ms between switches)\n",
          channelNumber(i),
          static_cast<unsigned long>(MIN_SWITCH_MS)
        );
        break;
    }
  }

  void handle(const Command& cmd, uint32_t now) {
    switch (cmd.type) {
      case CommandType::Status:
        printStatus();
        break;
      case CommandType::AllOff:
        for (size_t i = 0; i < CHANNELS; i++) {
          setRelay(i, false, now);
        }
        break;
      case CommandType::Set:
        setRelay(static_cast<size_t>(cmd.channel - 1), cmd.on, now);
        break;
    }
  }

  bool readLine() {
    while (Serial.available() > 0) {
      const int c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (lineOverflow) {
          lineOverflow = false;
          lineLen = 0;
          Serial.println("Line too long");
          continue;
        }
        if (lineLen == 0) {
          continue;
        }
        return true;
      }
      if (lineLen < lineBuf.size()) {
        lineBuf[lineLen] = static_cast<char>(c);
        ++lineLen;
      } else {
        lineOverflow = true;
      }
    }

    return false;
  }
}

void setup() {
  for (size_t i = 0; i < CHANNELS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    applyPin(i);
  }

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("08-relay-basics");
  Serial.printf(
    "Active-%s, min switch %lu ms, auto-off %lu ms\n",
    ACTIVE_LOW ? "LOW" : "HIGH",
    static_cast<unsigned long>(MIN_SWITCH_MS),
    static_cast<unsigned long>(MAX_ON_MS)
  );
  Serial.println("Commands: '1 on', '2 off', 'off', 'status'");
  Serial.println("Ready");
}

void loop() {
  const uint32_t now = millis();

  for (size_t i = 0; i < CHANNELS; i++) {
    if (relays[i].update(now)) {
      applyPin(i);
      Serial.printf(
        "Relay %u auto-off after %lu ms\n",
        channelNumber(i),
        static_cast<unsigned long>(MAX_ON_MS)
      );
    }
  }

  if (!readLine()) {
    return;
  }
  const std::string_view line(lineBuf.data(), lineLen);
  lineLen = 0;

  const auto cmd = parseCommand(line, CHANNELS);
  if (!cmd) {
    Serial.printf("Unknown command: %.*s\n", static_cast<int>(line.size()), line.data());
    Serial.print("  bytes:");
    for (const char ch : line) {
      Serial.printf(" %02X", static_cast<unsigned char>(ch));
    }
    Serial.println();
    return;
  }
  handle(*cmd, now);
}
