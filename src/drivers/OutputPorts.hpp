#pragma once

#include <Arduino.h>

namespace chimera {

enum class PortType : uint8_t { Digital, AnalogMode };

/** Digital and PWM output abstraction. */
class OutputPorts {
 public:
  /** Configures an output GPIO. @param port GPIO number. @param type Digital or
   * PWM output mode. */
  void initialize(uint8_t port, PortType type, bool activeHigh = true);
  /** Drives HIGH or applies 100 percent duty. @return Nothing. */
  void on();
  /** Drives LOW or applies zero duty. @return Nothing. */
  void off();
  /** Toggles digital state or PWM between zero and full duty. @return Nothing.
   */
  void toggle();
  /** Reports whether the output is active. @return True for HIGH or nonzero
   * PWM. */
  bool isOn() const;
  /** Sets PWM duty; ignored for digital ports. @param percent Duty from 0
   * through 100. */
  void setDutyCycle(uint8_t percent);

 private:
  uint8_t port_ = 0;
  uint8_t channel_ = 0;
  PortType type_ = PortType::Digital;
  bool initialized_ = false;
  bool activeHigh_ = true;
  uint8_t dutyPercent_ = 0;
};

}  // namespace chimera