#pragma once

#include <stddef.h>
#include <stdint.h>

namespace chimera {

constexpr uint8_t kProtocolSof = 0xAA;
constexpr size_t kProtocolMaxValueSize = 8;
constexpr size_t kProtocolMaxFrameSize = 1 + 1 + 1 + kProtocolMaxValueSize + 2;

enum class ValueType : uint8_t {
  Bool = 0x01,
  UInt8 = 0x02,
  Int8 = 0x03,
  UInt16 = 0x04,
  Int16 = 0x05,
  UInt32 = 0x06,
  Int32 = 0x07,
  Float32 = 0x08,
  Float64 = 0x09,
  Bytes = 0x0A,
};

enum class ProtocolEvent : uint8_t {
  Ack = 0xF0,
  Info = 0xF1,
  Error = 0xF2,
  Repeat = 0xF3,
};

/** Decoded fixed-size frame; BYTES values are deliberately unsupported. */
struct ProtocolFrame {
  uint8_t event = 0;
  ValueType type = ValueType::UInt8;
  uint8_t value[kProtocolMaxValueSize]{};
  uint8_t valueSize = 0;
};

/** Returns a fixed type's payload size. @param type Protocol value type.
 * @param size Receives the size on success. @return False for BYTES or unknown
 * types. */
bool protocolValueSize(ValueType type, size_t* size);

}  // namespace chimera