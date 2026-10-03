#pragma once

#include <stdint.h>

namespace chimera {

constexpr uint8_t kNrf24ChipSelectPin = 10;
constexpr uint8_t kSpiMosiPin = 7;
constexpr uint8_t kSpiClockPin = 6;
constexpr uint8_t kSpiMisoPin = 2;
constexpr uint8_t kNrf24ChipEnablePin = 5;
constexpr uint8_t kStatusLedPin = 0;
constexpr uint8_t kInfoLedPin = 1;
constexpr uint8_t kUltrasonicTriggerPin = 4;
constexpr uint8_t kUltrasonicEchoPin = 3;

}  // namespace chimera