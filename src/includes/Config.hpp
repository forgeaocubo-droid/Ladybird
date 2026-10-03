#pragma once

#include <stdint.h>

namespace chimera {

enum class LogLevel : uint8_t { Debug, Info, Error };
enum class WifiMode : uint8_t { AccessPoint, Station, AccessPointStation };
enum class Nrf24Power : uint8_t { Min, Low, High };
enum class Nrf24DataRate : uint8_t { Kbps250, Mbps1, Mbps2 };

constexpr WifiMode kWifiMode = WifiMode::Station;

constexpr char kApSsid[] = "LadyBird";
constexpr char kApPassword[] = "LadyBird123";
constexpr uint16_t kApTcpPort = 3333;
constexpr char kStaSsid[] = "VivoPA";
constexpr char kStaPassword[] = "pa314159180267";
constexpr uint16_t kStaTcpPort = 8080;
constexpr uint16_t kWifiTcpPort = kApTcpPort;
constexpr uint8_t kRf24MaxPayloadBytes = 32;
constexpr uint8_t kNrf24Channel = 76;
constexpr uint8_t kNrf24AddressWidth = 5;
constexpr uint8_t kNrf24Address[kNrf24AddressWidth] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
constexpr Nrf24Power kNrf24Power = Nrf24Power::High;
constexpr Nrf24DataRate kNrf24DataRate = Nrf24DataRate::Mbps1;

constexpr bool kLoggingEnabled = true;
constexpr LogLevel kDefaultLogLevel = LogLevel::Debug;
constexpr uint32_t kLedBlinkPeriodMs = 500;
constexpr bool kStatusLedActiveHigh = true;
constexpr bool kInfoLedActiveHigh = true;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kWifiNetworkTtlMs = 30000;
constexpr uint32_t kApClientTtlMs = 30000;
constexpr uint32_t kInfoLedActivityMs = 120;
constexpr uint32_t kUltrasonicMeasurementIntervalMs = 1000;
constexpr uint32_t kUltrasonicTimeoutUs = 30000;

}  // namespace chimera