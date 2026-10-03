#include <Arduino.h>
#include <stdio.h>

#include "drivers/NRF24Radio.hpp"
#include "drivers/OutputPorts.hpp"
#include "drivers/UltrasonicSensor.hpp"
#include "drivers/WifiAPSTA.hpp"
#include "includes/Config.hpp"
#include "includes/PinMap.hpp"
#include "libs/Events.hpp"
#include "protocol/ProtocolCodec.hpp"
#include "protocol/ProtocolDispatcher.hpp"
#include "utils/Logger.hpp"

namespace {

chimera::WifiAPSTA wifi;
chimera::NRF24Radio radio;
chimera::UltrasonicSensor ultrasonic;
chimera::OutputPorts statusLed;
chimera::OutputPorts infoLed;
chimera::ProtocolCodec wifiCodec;
chimera::ProtocolCodec rfCodec;
chimera::ProtocolDispatcher dispatcher(chimera::kEventCatalog,
                                       chimera::kEventCatalogCount);
uint32_t lastTcpSessionId = 0;
uint32_t lastStatusLedToggleMs = 0;
uint32_t lastInfoLedActivityMs = 0;
bool wifiWasConnected = false;

const char* wifiChannelName() {
  return "Wi-Fi TCP";
}

rf24_pa_dbm_e nrf24PowerValue() {
  switch (chimera::kNrf24Power) {
    case chimera::Nrf24Power::Min:
      return RF24_PA_MIN;
    case chimera::Nrf24Power::Low:
      return RF24_PA_LOW;
    case chimera::Nrf24Power::High:
      return RF24_PA_HIGH;
  }
  return RF24_PA_LOW;
}

const char* nrf24PowerName() {
  switch (chimera::kNrf24Power) {
    case chimera::Nrf24Power::Min:
      return "PA_MIN";
    case chimera::Nrf24Power::Low:
      return "PA_LOW";
    case chimera::Nrf24Power::High:
      return "PA_HIGH";
  }
  return "unknown power";
}

rf24_datarate_e nrf24DataRateValue() {
  switch (chimera::kNrf24DataRate) {
    case chimera::Nrf24DataRate::Kbps250:
      return RF24_250KBPS;
    case chimera::Nrf24DataRate::Mbps1:
      return RF24_1MBPS;
    case chimera::Nrf24DataRate::Mbps2:
      return RF24_2MBPS;
  }
  return RF24_1MBPS;
}

const char* nrf24DataRateName() {
  switch (chimera::kNrf24DataRate) {
    case chimera::Nrf24DataRate::Kbps250:
      return "250 kbps";
    case chimera::Nrf24DataRate::Mbps1:
      return "1 Mbps";
    case chimera::Nrf24DataRate::Mbps2:
      return "2 Mbps";
  }
  return "unknown data rate";
}

void logFrame(const char* source, const chimera::ProtocolFrame& frame) {
  char message[80];
  size_t used = static_cast<size_t>(snprintf(
      message, sizeof(message), "%s decoded %s(0x%02X) type=0x%02X value=",
      source, chimera::eventName(frame.event),
      frame.event, static_cast<uint8_t>(frame.type)));
  for (size_t index = 0; index < frame.valueSize && used + 4 < sizeof(message);
       ++index) {
    const int written = snprintf(message + used, sizeof(message) - used,
                                 "%s%02X", index == 0 ? "" : " ",
                                 frame.value[index]);
    if (written <= 0) break;
    used += static_cast<size_t>(written);
  }
  chimera::gLogger.logDebug(message);
}

void processByte(chimera::ProtocolCodec& codec, const char* source,
                 uint8_t byte) {
  chimera::gLogger.logDebug(String(source) + " protocol decoder received byte");
  chimera::ProtocolFrame frame;
  const chimera::DecodeResult result = codec.feed(byte, &frame);
  if (result == chimera::DecodeResult::FrameReady) {
    logFrame(source, frame);
    const chimera::DispatchResult dispatchResult = dispatcher.dispatch(frame);
    if (dispatchResult == chimera::DispatchResult::Accepted) {
      chimera::gLogger.logDebug(String(source) +
                                " event handler accepted the frame");
    } else {
      chimera::gLogger.logError(String(source) +
                                " semantic validation rejected event " +
                                String(frame.event));
    }
  } else if (result == chimera::DecodeResult::CrcMismatch) {
    chimera::gLogger.logError(String(source) + " CRC mismatch");
  } else if (result == chimera::DecodeResult::UnsupportedType) {
    chimera::gLogger.logError(String(source) +
                              " unsupported or variable-length type");
  }
}

void logReceivedPayload(const char* source, const uint8_t* payload,
                        size_t payloadSize) {
  char message[128];
  size_t used = static_cast<size_t>(
      snprintf(message, sizeof(message), "%s RX (%u bytes):", source,
               static_cast<unsigned>(payloadSize)));
  for (size_t index = 0; index < payloadSize && used + 4 < sizeof(message);
       ++index) {
    const int written = snprintf(message + used, sizeof(message) - used,
                                 " %02X", payload[index]);
    if (written <= 0) break;
    used += static_cast<size_t>(written);
  }
  chimera::gLogger.logDebug(message);
  chimera::setInfoLedActivityActive(true);
  lastInfoLedActivityMs = millis();
}

void processPayload(chimera::ProtocolCodec& codec, const char* source,
                    const uint8_t* payload, size_t payloadSize) {
  logReceivedPayload(source, payload, payloadSize);
  for (size_t index = 0; index < payloadSize; ++index) {
    processByte(codec, source, payload[index]);
  }
}

void processRfPackets() {
  while (radio.available()) {
    const uint8_t payloadSize = radio.dynamicPayloadSize();
    if (payloadSize == 0 || payloadSize > chimera::kRf24MaxPayloadBytes) {
      chimera::gLogger.logError("RF received an invalid payload size");
      continue;
    }

    uint8_t payload[chimera::kRf24MaxPayloadBytes];
    radio.read(payload, payloadSize);
    processPayload(rfCodec, "RF", payload, payloadSize);
  }
}

void updateInfoLed() {
  if (chimera::infoLedActivityActive() &&
      millis() - lastInfoLedActivityMs >= chimera::kInfoLedActivityMs) {
    chimera::setInfoLedActivityActive(false);
  }
  if (chimera::infoLedLatched() || chimera::infoLedActivityActive()) {
    infoLed.on();
  } else {
    infoLed.off();
  }
}

void updateStatusLed() {
  const uint32_t now = millis();
  const bool wifiConnected = wifi.hasOperationalConnection();
  if (wifiConnected) {
    if (!wifiWasConnected) statusLed.on();
    wifiWasConnected = true;
    return;
  }
  if (wifiWasConnected) {
    statusLed.off();
    lastStatusLedToggleMs = now;
  }
  wifiWasConnected = false;
  if (now - lastStatusLedToggleMs >= chimera::kLedBlinkPeriodMs) {
    statusLed.toggle();
    lastStatusLedToggleMs = now;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  chimera::gLogger.logDebug("Serial logger started at 115200 baud");
  statusLed.initialize(chimera::kStatusLedPin, chimera::PortType::Digital,
                       chimera::kStatusLedActiveHigh);
  statusLed.off();
  infoLed.initialize(chimera::kInfoLedPin, chimera::PortType::Digital,
                     chimera::kInfoLedActiveHigh);
  infoLed.off();
  lastStatusLedToggleMs = millis();
  chimera::gLogger.logInfo("LadyBird firmware starting");
  chimera::gLogger.logDebug(
      String("Protocol event catalog entries: ") +
      String(chimera::kEventCatalogCount));
  chimera::gLogger.logDebug("Initializing ultrasonic sensor health check");
  if (!ultrasonic.begin()) {
    chimera::gLogger.logError("Ultrasonic sensor failed its startup health check");
  } else {
    chimera::gLogger.logInfo(
        String("Ultrasonic startup reading: ") +
        String(ultrasonic.distanceCm(), 1) + " cm");
  }
  chimera::gLogger.logDebug("Initializing Wi-Fi service");
  if (!wifi.initialize()) {
    chimera::gLogger.logError("Wi-Fi service failed to initialize");
  } else {
    chimera::gLogger.logInfo("Wi-Fi service initialized");
  }
  chimera::gLogger.logDebug("Initializing nRF24 radio");
  if (!radio.begin()) {
    chimera::gLogger.logError("NRF24 radio failed to initialize");
  } else {
    chimera::gLogger.logDebug("Applying configured nRF24 radio parameters");
    radio.setPALevel(nrf24PowerValue());
    radio.setDataRate(nrf24DataRateValue());
    radio.setChannel(chimera::kNrf24Channel);
    radio.setAddressWidth(chimera::kNrf24AddressWidth);
    radio.openReadingPipe(1, chimera::kNrf24Address);
    radio.startListening();
    char message[128];
    size_t used = static_cast<size_t>(snprintf(
        message, sizeof(message), "NRF24 listening: %s, %s, channel %u, "
                                  "address (%u bytes) ",
        nrf24PowerName(), nrf24DataRateName(), chimera::kNrf24Channel,
        chimera::kNrf24AddressWidth));
    for (uint8_t index = 0;
         index < chimera::kNrf24AddressWidth && used + 4 < sizeof(message);
         ++index) {
      const int written = snprintf(
          message + used, sizeof(message) - used, "%s%02X",
          index == 0 ? "" : " ", chimera::kNrf24Address[index]);
      if (written <= 0) break;
      used += static_cast<size_t>(written);
    }
    chimera::gLogger.logInfo(message);
  }
}

void loop() {
  wifi.update();
  processRfPackets();
  if (ultrasonic.update()) {
    const float distanceCm = ultrasonic.distanceCm();
    if (distanceCm > 0.0f) {
      chimera::gLogger.logInfo(String("Ultrasonic reading: ") +
                               String(distanceCm, 1) + " cm");
    } else {
      chimera::gLogger.logInfo("Ultrasonic reading: 0 cm (no echo)");
    }
  }
  const int available = wifi.tcpAvailable();
  updateStatusLed();
  updateInfoLed();
  const uint32_t sessionId = wifi.tcpSessionId();
  if (sessionId != lastTcpSessionId) {
    chimera::gLogger.logDebug("Wi-Fi TCP session changed; resetting decoder");
    wifiCodec.reset();
    lastTcpSessionId = sessionId;
  }
  if (available <= 0) return;
  uint8_t input[chimera::kProtocolMaxFrameSize];
  const size_t received = wifi.tcpRead(input, sizeof(input));
  if (received > 0) {
    processPayload(wifiCodec, wifiChannelName(), input, received);
  }
}
