#include "libs/Events.hpp"

#include <math.h>
#include <stddef.h>

#include "includes/Config.hpp"
#include "includes/ProtocolTypes.hpp"

namespace chimera {
namespace {

bool gInfoLedLatched = false;
bool gInfoLedActivityActive = false;

bool isFiniteFloat32(const ProtocolFrame& frame) {
  if (frame.type != ValueType::Float32 || frame.valueSize != 4) return false;
  union {
    uint32_t raw;
    float value;
  } payload{};
  payload.raw = static_cast<uint32_t>(frame.value[0]) |
                (static_cast<uint32_t>(frame.value[1]) << 8U) |
                (static_cast<uint32_t>(frame.value[2]) << 16U) |
                (static_cast<uint32_t>(frame.value[3]) << 24U);
  return isfinite(payload.value);
}

bool validateBoolEvent(const ProtocolFrame& frame, uint8_t expectedValue) {
  return frame.type == ValueType::Bool && frame.valueSize == 1 &&
         frame.value[0] == expectedValue;
}

bool validateUInt8Event(const ProtocolFrame& frame, uint8_t minValue,
                        uint8_t maxValue) {
  if (frame.type != ValueType::UInt8 || frame.valueSize != 1) return false;
  const uint8_t value = frame.value[0];
  return value >= minValue && value <= maxValue;
}

bool validateUInt16Event(const ProtocolFrame& frame, uint16_t minValue,
                         uint16_t maxValue) {
  if (frame.type != ValueType::UInt16 || frame.valueSize != 2) return false;
  const uint16_t value = static_cast<uint16_t>(frame.value[0]) |
                         (static_cast<uint16_t>(frame.value[1]) << 8U);
  return value >= minValue && value <= maxValue;
}

bool validateFloat32Event(const ProtocolFrame& frame, float minValue,
                          float maxValue) {
  if (!isFiniteFloat32(frame)) return false;
  union {
    uint32_t raw;
    float value;
  } payload{};
  payload.raw = static_cast<uint32_t>(frame.value[0]) |
                (static_cast<uint32_t>(frame.value[1]) << 8U) |
                (static_cast<uint32_t>(frame.value[2]) << 16U) |
                (static_cast<uint32_t>(frame.value[3]) << 24U);
  return payload.value >= minValue && payload.value <= maxValue;
}

bool handleNoAction(const ProtocolFrame&) { return true; }

bool handleInfoLedLatchOn(const ProtocolFrame&) {
  gInfoLedLatched = true;
  return true;
}

bool handleInfoLedLatchOff(const ProtocolFrame&) {
  gInfoLedLatched = false;
  gInfoLedActivityActive = false;
  return true;
}

bool validateButtonEventPressed(const ProtocolFrame& frame) {
  return validateBoolEvent(frame, 1);
}

bool validateButtonEventReleased(const ProtocolFrame& frame) {
  return validateBoolEvent(frame, 0);
}

bool validateRequestEvent(const ProtocolFrame& frame, uint8_t eventId,
                         ValueType expectedType, uint16_t minValue,
                         uint16_t maxValue) {
  if (frame.event != eventId) return false;
  if (frame.type != expectedType) return false;
  const uint16_t value = static_cast<uint16_t>(frame.value[0]);
  return value >= minValue && value <= maxValue;
}

}  // namespace

const EventDefinition kEventCatalog[] = {
    {0x01, ValueType::Bool, validateButtonEventPressed, handleInfoLedLatchOn},
    {0x02, ValueType::Bool, validateButtonEventReleased, handleInfoLedLatchOff},
    {0x03, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x04, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x05, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x06, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x07, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x08, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x09, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x0A, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x0B, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x0C, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x0D, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x0E, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x0F, ValueType::UInt16,
     [](const ProtocolFrame& frame) { return validateUInt16Event(frame, 0, 4095); },
     handleNoAction},
    {0x10, ValueType::UInt16,
     [](const ProtocolFrame& frame) { return validateUInt16Event(frame, 0, 4095); },
     handleNoAction},
    {0x11, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x12, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x13, ValueType::UInt16,
     [](const ProtocolFrame& frame) { return validateUInt16Event(frame, 0, 4095); },
     handleNoAction},
    {0x14, ValueType::UInt16,
     [](const ProtocolFrame& frame) { return validateUInt16Event(frame, 0, 4095); },
     handleNoAction},
    {0x15, ValueType::Float32,
     [](const ProtocolFrame& frame) { return validateFloat32Event(frame, -20.0f, 20.0f); },
     handleNoAction},
    {0x16, ValueType::Float32,
     [](const ProtocolFrame& frame) { return validateFloat32Event(frame, -20.0f, 20.0f); },
     handleNoAction},
    {0x17, ValueType::Float32,
     [](const ProtocolFrame& frame) { return validateFloat32Event(frame, -20.0f, 20.0f); },
     handleNoAction},
    {0x18, ValueType::UInt8,
     [](const ProtocolFrame& frame) { return validateUInt8Event(frame, 0, 255); },
     handleNoAction},
    {0x19, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x1A, ValueType::Bool, validateButtonEventReleased, handleNoAction},
    {0x1B, ValueType::Bool, validateButtonEventPressed, handleNoAction},
    {0x1C, ValueType::Bool, validateButtonEventReleased, handleNoAction},
};

const size_t kEventCatalogCount = sizeof(kEventCatalog) / sizeof(kEventCatalog[0]);

const EventDefinition* eventCatalog(size_t* count) {
  if (count != nullptr) *count = kEventCatalogCount;
  return kEventCatalog;
}

const char* eventName(uint8_t eventId) {
  switch (eventId) {
    case 0x01: return "BUTTON_UP_PRESSED";
    case 0x02: return "BUTTON_UP_RELEASED";
    case 0x03: return "BUTTON_DOWN_PRESSED";
    case 0x04: return "BUTTON_DOWN_RELEASED";
    case 0x05: return "BUTTON_LEFT_PRESSED";
    case 0x06: return "BUTTON_LEFT_RELEASED";
    case 0x07: return "BUTTON_RIGHT_PRESSED";
    case 0x08: return "BUTTON_RIGHT_RELEASED";
    case 0x09: return "BUTTON_CONFIRM_PRESSED";
    case 0x0A: return "BUTTON_BACK_PRESSED";
    case 0x0B: return "BUTTON_CONFIRM_RELEASED";
    case 0x0C: return "BUTTON_BACK_RELEASED";
    case 0x0D: return "LEFT_JOY_BUTTON_PRESSED";
    case 0x0E: return "LEFT_JOY_BUTTON_RELEASED";
    case 0x0F: return "LEFT_JOY_X";
    case 0x10: return "LEFT_JOY_Y";
    case 0x11: return "RIGHT_JOY_BUTTON_PRESSED";
    case 0x12: return "RIGHT_JOY_BUTTON_RELEASED";
    case 0x13: return "RIGHT_JOY_X";
    case 0x14: return "RIGHT_JOY_Y";
    case 0x15: return "SENSOR_X";
    case 0x16: return "SENSOR_Y";
    case 0x17: return "SENSOR_Z";
    case 0x18: return "DEVICE_STATUS";
    case 0x19: return "LEFT_JOY_X_THRESHOLD_PRESSED";
    case 0x1A: return "LEFT_JOY_X_THRESHOLD_RELEASED";
    case 0x1B: return "BUTTON_UP_DOWN_PRESSED";
    case 0x1C: return "BUTTON_UP_DOWN_RELEASED";
    case 0xF0: return "ACK";
    case 0xF1: return "INFO";
    case 0xF2: return "ERR";
    case 0xF3: return "REPT";
    default: return "UNKNOWN";
  }
}

uint16_t eventThreshold(uint8_t eventId) {
  switch (eventId) {
    case 0x0F:
    case 0x10:
    case 0x13:
    case 0x14:
      return 64;
    case 0x15:
    case 0x16:
    case 0x17:
      return 2;
    default:
      return 0;
  }
}

bool isEventActionConfigured(uint8_t eventId) {
  return eventId == 0x01 || eventId == 0x02;
}

void setInfoLedLatched(bool latched) { gInfoLedLatched = latched; }
bool infoLedLatched() { return gInfoLedLatched; }
void setInfoLedActivityActive(bool active) { gInfoLedActivityActive = active; }
bool infoLedActivityActive() { return gInfoLedActivityActive; }

}  // namespace chimera
