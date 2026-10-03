#include "drivers/InputPorts.hpp"

#include "includes/Config.hpp"
#include "utils/Logger.hpp"

namespace chimera {

void InputPorts::initialize(uint8_t port, PortType type) {
  port_ = port;
  type_ = type;
  if (type_ == PortType::AnalogMode) {
    pinMode(port_, INPUT);
  } else {
    pinMode(port_, INPUT_PULLUP);
  }
  initialized_ = true;
  stablePressed_ = type_ == PortType::Digital && digitalRead(port_) == LOW;
  candidatePressed_ = stablePressed_;
  candidateSinceMs_ = millis();
}

void InputPorts::setDebounceMs(uint32_t milliseconds) {
  debounceMs_ = milliseconds;
}
void InputPorts::onClick(std::function<void()> callback) {
  callback_ = std::move(callback);
}

void InputPorts::update() {
  if (!initialized_ || type_ != PortType::Digital) return;
  const bool rawPressed = digitalRead(port_) == LOW;
  if (rawPressed != candidatePressed_) {
    candidatePressed_ = rawPressed;
    candidateSinceMs_ = millis();
  }
  const uint32_t debounce = debounceMs_ ? debounceMs_ : kButtonDebounceMs;
  if (candidatePressed_ != stablePressed_ &&
      millis() - candidateSinceMs_ >= debounce) {
    stablePressed_ = candidatePressed_;
    if (stablePressed_) {
      clickPending_ = true;
      if (callback_) callback_();
    }
  }
}

bool InputPorts::wasClicked() {
  const bool value = clickPending_;
  clickPending_ = false;
  return value;
}
bool InputPorts::isPressed() const { return initialized_ && stablePressed_; }
int InputPorts::analogRead() const {
  return initialized_ && type_ == PortType::AnalogMode ? ::analogRead(port_)
                                                       : -1;
}

}  // namespace chimera
