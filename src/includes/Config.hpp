#define PWM_FREQUENCY 20000
#define PWM_RESOLUTION 8
#define LOGGING_ENABLED true

constexpr uint32_t kProtocolAckTimeoutMs = 100;
constexpr uint8_t kWifiMaximumConnectionAttempts = 3;
constexpr uint32_t kWifiReconnectIntervalMs = 5000;
constexpr uint8_t kMpu6050Address = 0x68;
constexpr uint16_t kWifiPort = 3333;
constexpr uint32_t kSensorIntervalMs = 1000;