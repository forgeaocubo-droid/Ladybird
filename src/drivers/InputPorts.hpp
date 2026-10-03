#pragma once

#include <Arduino.h>

#include <functional>

#include "drivers/OutputPorts.hpp"

namespace chimera {

/** Debounced, active-low digital input and raw ADC input wrapper. */
class InputPorts {
 public:
  /** Configures an input; digital inputs use INPUT_PULLUP where supported.
   * @param port GPIO number. @param type Digital or ADC mode. */
  void initialize(uint8_t port, PortType type);
  /** Sets debounce time before initialization. @param milliseconds Required
   * stable time. */
  void setDebounceMs(uint32_t milliseconds);
  /** Registers a lightweight callback invoked after a debounced press. @param
   * callback Press callback. */
  void onClick(std::function<void()> callback);
  /** Samples and debounces the input; call regularly from the main loop. */
  void update();
  /** Returns and clears the pending press event. @return True once per press.
   */
  bool wasClicked();
  /** Returns active-low logical state. @return True when pressed. */
  bool isPressed() const;
  /** Reads the ADC input. @return Raw ADC value or -1 for a digital input. */
  int analogRead() const;

 private:
  uint8_t port_ = 0;
  PortType type_ = PortType::Digital;
  bool initialized_ = false;
  bool stablePressed_ = false;
  bool candidatePressed_ = false;
  bool clickPending_ = false;
  uint32_t debounceMs_ = 0;
  uint32_t candidateSinceMs_ = 0;
  std::function<void()> callback_;
};

}  // namespace chimera