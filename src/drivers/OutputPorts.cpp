#include "drivers/OutputPorts.hpp"

#include "utils/Logger.hpp"

namespace chimera {
namespace {
uint8_t nextPwmChannel = 0;
}

void OutputPorts::initialize(uint8_t port, PortType type, bool activeHigh) {
  port_ = port;
  type_ = type;
  activeHigh_ = activeHigh;
  if (type_ == PortType::AnalogMode) {
    channel_ = nextPwmChannel++;
    ledcSetup(channel_, 5000, 8);
    ledcAttachPin(port_, channel_);
    ledcWrite(channel_, activeHigh_ ? 0 : 255);
  } else {
    pinMode(port_, OUTPUT);
    digitalWrite(port_, activeHigh_ ? LOW : HIGH);
  }
  initialized_ = true;
  dutyPercent_ = 0;
}

void OutputPorts::on() {
  if (!initialized_) return;
  if (type_ == PortType::AnalogMode)
    setDutyCycle(100);
  else {
    digitalWrite(port_, activeHigh_ ? HIGH : LOW);
    dutyPercent_ = 100;
  }
}
void OutputPorts::off() {
  if (!initialized_) return;
  if (type_ == PortType::AnalogMode)
    setDutyCycle(0);
  else {
    digitalWrite(port_, activeHigh_ ? LOW : HIGH);
    dutyPercent_ = 0;
  }
}
void OutputPorts::toggle() {
  if (isOn())
    off();
  else
    on();
}
bool OutputPorts::isOn() const { return initialized_ && dutyPercent_ > 0; }

void OutputPorts::setDutyCycle(uint8_t percent) {
  if (!initialized_) return;
  if (type_ != PortType::AnalogMode) {
    gLogger.logError("PWM duty request ignored for digital GPIO " +
                     String(port_));
    return;
  }
  dutyPercent_ = constrain(percent, 0, 100);
  const uint8_t physicalDuty =
      activeHigh_ ? dutyPercent_ : static_cast<uint8_t>(100 - dutyPercent_);
  ledcWrite(channel_, static_cast<uint32_t>(physicalDuty) * 255U / 100U);
}

}  // namespace chimera
