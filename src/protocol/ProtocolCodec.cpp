#include "protocol/ProtocolCodec.hpp"

namespace chimera {
namespace {

uint16_t updateCrc(uint16_t crc, uint8_t byte) {
  crc ^= byte;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    crc = (crc & 1U) ? static_cast<uint16_t>((crc >> 1U) ^ 0xA001U)
                     : static_cast<uint16_t>(crc >> 1U);
  }
  return crc;
}

}  // namespace

bool protocolValueSize(ValueType type, size_t* size) {
  if (size == nullptr) return false;
  switch (type) {
    case ValueType::Bool:
    case ValueType::UInt8:
    case ValueType::Int8:
      *size = 1;
      return true;
    case ValueType::UInt16:
    case ValueType::Int16:
      *size = 2;
      return true;
    case ValueType::UInt32:
    case ValueType::Int32:
    case ValueType::Float32:
      *size = 4;
      return true;
    case ValueType::Float64:
      *size = 8;
      return true;
    case ValueType::Bytes:
      return false;
  }
  return false;
}

ProtocolCodec::ProtocolCodec() { reset(); }

DecodeResult ProtocolCodec::feed(uint8_t byte, ProtocolFrame* frame) {
  if (frame == nullptr) return DecodeResult::None;
  switch (state_) {
    case State::Idle:
      if (byte == kProtocolSof) startFrame();
      return DecodeResult::None;
    case State::Event:
      workingFrame_.event = byte;
      crc_ = updateCrc(crc_, byte);
      state_ = State::Type;
      return DecodeResult::None;
    case State::Type: {
      workingFrame_.type = static_cast<ValueType>(byte);
      crc_ = updateCrc(crc_, byte);
      if (!protocolValueSize(workingFrame_.type, &valueSize_)) {
        reset();
        return DecodeResult::UnsupportedType;
      }
      workingFrame_.valueSize = static_cast<uint8_t>(valueSize_);
      valueIndex_ = 0;
      state_ = State::Value;
      return DecodeResult::None;
    }
    case State::Value:
      workingFrame_.value[valueIndex_++] = byte;
      crc_ = updateCrc(crc_, byte);
      if (valueIndex_ == valueSize_) state_ = State::CrcLow;
      return DecodeResult::None;
    case State::CrcLow:
      receivedCrcLow_ = byte;
      state_ = State::CrcHigh;
      return DecodeResult::None;
    case State::CrcHigh: {
      const uint16_t receivedCrc =
          static_cast<uint16_t>(receivedCrcLow_) |
          (static_cast<uint16_t>(byte) << 8U);
      const bool valid = receivedCrc == crc_;
      if (valid) *frame = workingFrame_;
      reset();
      return valid ? DecodeResult::FrameReady : DecodeResult::CrcMismatch;
    }
  }
  reset();
  return DecodeResult::None;
}

bool ProtocolCodec::encode(const ProtocolFrame& frame, uint8_t* output,
                           size_t capacity, size_t* encodedSize) const {
  size_t expectedValueSize = 0;
  if (output == nullptr || encodedSize == nullptr ||
      !protocolValueSize(frame.type, &expectedValueSize) ||
      frame.valueSize != expectedValueSize) {
    return false;
  }
  const size_t frameSize = expectedValueSize + 5;
  if (capacity < frameSize) return false;
  output[0] = kProtocolSof;
  output[1] = frame.event;
  output[2] = static_cast<uint8_t>(frame.type);
  for (size_t index = 0; index < expectedValueSize; ++index)
    output[index + 3] = frame.value[index];
  const uint16_t crc = crc16Modbus(output + 1, expectedValueSize + 2);
  output[frameSize - 2] = static_cast<uint8_t>(crc & 0xFFU);
  output[frameSize - 1] = static_cast<uint8_t>(crc >> 8U);
  *encodedSize = frameSize;
  return true;
}

void ProtocolCodec::reset() {
  state_ = State::Idle;
  workingFrame_ = ProtocolFrame{};
  valueSize_ = 0;
  valueIndex_ = 0;
  crc_ = 0xFFFF;
  receivedCrcLow_ = 0;
}

uint16_t ProtocolCodec::crc16Modbus(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  if (data == nullptr) return crc;
  for (size_t index = 0; index < length; ++index)
    crc = updateCrc(crc, data[index]);
  return crc;
}

void ProtocolCodec::startFrame() {
  reset();
  state_ = State::Event;
}

}  // namespace chimera