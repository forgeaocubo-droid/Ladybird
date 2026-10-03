#pragma once

#include <Arduino.h>
#include <RF24.h>

namespace chimera {

/**
 * nRF24L01 RF24 facade. CE and CSN are defined in PinMap.hpp; the IRQ pin is
 * not connected or used by this firmware.
 */
class NRF24Radio {
 public:
  /** Constructs RF24 with the assigned CE and CSN pins. */
  NRF24Radio();
  /** Initializes SPI and the radio. @return True when the radio responds. */
  bool begin();
  /** Selects an RF channel. @param channel Channel from 0 through 125. */
  void setChannel(uint8_t channel);
  /** Selects the address width in bytes. @param width Width from 3 through 5. */
  void setAddressWidth(uint8_t width);
  /** Selects output power. @param level RF24 PA level. */
  void setPALevel(rf24_pa_dbm_e level);
  /** Selects bitrate. @param rate RF24 data rate. */
  void setDataRate(rf24_datarate_e rate);
  /** Selects a transmit address. @param address Uses the configured width. */
  void openWritingPipe(const uint8_t* address);
  /** Opens a receive pipe. @param pipe Pipe number. @param address Uses the
   * configured width. */
  void openReadingPipe(uint8_t pipe, const uint8_t* address);
  /** Enables reception. */
  void startListening();
  /** Selects transmit mode. */
  void stopListening();
  /** Starts a bounded payload without waiting for acknowledgment. @param data
   * Payload bytes. @param length Payload length. @return False for empty, null,
   * or oversized payloads. */
  bool startWriteAsync(const void* data, uint8_t length);
  /** Polls completion without waiting. @param success Receives ACK success when
   * complete. @return True when TX_DS or MAX_RT occurred. */
  bool pollTxComplete(bool& success);
  /** Clears the TX FIFO. */
  void flushTx();
  /** Reports whether a payload is waiting. @return True when RX data is
   * available. */
  bool available();
  /** Returns the next dynamic payload length, rejecting invalid lengths.
   * @return Payload size or zero when invalid. */
  uint8_t dynamicPayloadSize();
  /** Reads one bounded payload. @param data Destination. @param length Buffer
   * length, at most 32 bytes. */
  void read(void* data, uint8_t length);
  /** Clears the RX FIFO. */
  void flushRx();
  /** Tests carrier detection. @return Current carrier state. */
  bool testCarrier();
  /** Tests received-power detection. @return Current RPD state. */
  bool testRPD();
  /** Checks radio connectivity. @return True when the chip is detected. */
  bool isChipConnected();
  /** Writes radio register details to Serial. */
  void printDetails();

 private:
  RF24 radio_;
};

}  // namespace chimera