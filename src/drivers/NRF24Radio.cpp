#include "drivers/NRF24Radio.hpp"

#include "includes/Config.hpp"
#include "includes/PinMap.hpp"
#include "utils/Logger.hpp"

namespace chimera {

NRF24Radio::NRF24Radio()
  : radio_(kNrf24ChipEnablePin, kNrf24ChipSelectPin) {}

bool NRF24Radio::begin() {
  gLogger.logDebug("NRF24: initializing SPI bus");
  SPI.begin(kSpiClockPin, kSpiMisoPin, kSpiMosiPin, kNrf24ChipSelectPin);
  const bool ready = radio_.begin();
  if (ready) {
    radio_.enableDynamicPayloads();
    gLogger.logDebug("NRF24: radio detected; dynamic payloads enabled");
  } else {
    gLogger.logError("NRF24: radio initialization failed");
  }
  return ready;
}

void NRF24Radio::setChannel(uint8_t channel) {
  radio_.setChannel(channel);
  gLogger.logDebug("NRF24: channel configured");
}
void NRF24Radio::setAddressWidth(uint8_t width) {
  radio_.setAddressWidth(width);
  gLogger.logDebug("NRF24: address width configured");
}
void NRF24Radio::setPALevel(rf24_pa_dbm_e level) {
  radio_.setPALevel(level);
  gLogger.logDebug("NRF24: output power configured");
}
void NRF24Radio::setDataRate(rf24_datarate_e rate) {
  radio_.setDataRate(rate);
  gLogger.logDebug("NRF24: data rate configured");
}
void NRF24Radio::openWritingPipe(const uint8_t* address) {
  radio_.openWritingPipe(address);
  gLogger.logDebug("NRF24: writing pipe opened");
}
void NRF24Radio::openReadingPipe(uint8_t pipe, const uint8_t* address) {
  radio_.openReadingPipe(pipe, address);
  gLogger.logDebug("NRF24: reading pipe opened");
}
void NRF24Radio::startListening() {
  radio_.startListening();
  gLogger.logDebug("NRF24: listening started");
}
void NRF24Radio::stopListening() {
  radio_.stopListening();
  gLogger.logDebug("NRF24: listening stopped");
}
bool NRF24Radio::startWriteAsync(const void* data, uint8_t length) {
  if (data == nullptr || length == 0 || length > kRf24MaxPayloadBytes)
    return false;
  radio_.startFastWrite(data, length, false, true);
  return true;
}
bool NRF24Radio::pollTxComplete(bool& success) {
  bool transmitOk = false;
  bool transmitFailed = false;
  bool receiveReady = false;
  radio_.whatHappened(transmitOk, transmitFailed, receiveReady);
  if (!transmitOk && !transmitFailed) return false;
  success = transmitOk;
  if (transmitFailed) radio_.flush_tx();
  return true;
}
void NRF24Radio::flushTx() { radio_.flush_tx(); }
bool NRF24Radio::available() { return radio_.available(); }
uint8_t NRF24Radio::dynamicPayloadSize() {
  const uint8_t size = radio_.getDynamicPayloadSize();
  if (size > kRf24MaxPayloadBytes) {
    radio_.flush_rx();
    return 0;
  }
  return size;
}
void NRF24Radio::read(void* data, uint8_t length) {
  if (data != nullptr && length <= kRf24MaxPayloadBytes) {
    radio_.read(data, length);
    gLogger.logDebug("NRF24: payload read from RX FIFO");
  }
}
void NRF24Radio::flushRx() { radio_.flush_rx(); }
bool NRF24Radio::isChipConnected() { return radio_.isChipConnected(); }
bool NRF24Radio::testCarrier() { return radio_.testCarrier(); }
bool NRF24Radio::testRPD() { return radio_.testRPD(); }
void NRF24Radio::printDetails() { radio_.printDetails(); }

}  // namespace chimera