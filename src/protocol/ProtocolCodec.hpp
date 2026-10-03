#pragma once

#include <stddef.h>
#include <stdint.h>

#include "includes/ProtocolTypes.hpp"

namespace chimera {

enum class DecodeResult : uint8_t {
  None,
  FrameReady,
  UnsupportedType,
  CrcMismatch,
};

/** Incremental fixed-size frame codec with independent state per instance. */
class ProtocolCodec {
 public:
  /** Creates an idle decoder. */
  ProtocolCodec();
  /** Consumes one byte and reports a completed frame or framing error.
   * @param byte Incoming stream byte. @param frame Receives a valid frame.
   * @return Result for this byte. */
  DecodeResult feed(uint8_t byte, ProtocolFrame* frame);
  /** Encodes one fixed-size frame with a little-endian CRC16 trailer.
   * @param frame Frame to encode. @param output Destination buffer.
   * @param capacity Destination capacity. @param encodedSize Receives bytes
   * written. @return False for invalid value size or insufficient capacity. */
  bool encode(const ProtocolFrame& frame, uint8_t* output, size_t capacity,
              size_t* encodedSize) const;
  /** Clears partial decoder state. */
  void reset();
  /** Calculates CRC16-MODBUS. @param data Bytes to checksum.
   * @param length Number of bytes. @return CRC initialized with 0xFFFF. */
  static uint16_t crc16Modbus(const uint8_t* data, size_t length);

 private:
  enum class State : uint8_t { Idle, Event, Type, Value, CrcLow, CrcHigh };
  void startFrame();

  State state_;
  ProtocolFrame workingFrame_;
  size_t valueSize_;
  size_t valueIndex_;
  uint16_t crc_;
  uint8_t receivedCrcLow_;
};

}  // namespace chimera