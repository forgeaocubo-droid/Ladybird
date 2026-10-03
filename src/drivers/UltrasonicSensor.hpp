#pragma once

#include <Arduino.h>

namespace chimera {

/** HC-SR04-compatible ultrasonic range sensor with bounded measurements. */
class UltrasonicSensor {
 public:
  /** Configures the GPIOs and validates the initial distance reading. */
  bool begin();
  /** Starts or advances a periodic measurement without waiting for an echo.
   * @return True when a new measurement has completed. */
  bool update();
  /** Returns the latest distance in centimeters; zero means no echo. */
  float distanceCm() const;

 private:
  enum class MeasurementState : uint8_t { Idle, WaitingForRise, WaitingForFall };

  void trigger();
  void finishMeasurement(float distanceCm);

  MeasurementState state_ = MeasurementState::Idle;
  uint32_t lastMeasurementStartedMs_ = 0;
  uint32_t measurementStartedUs_ = 0;
  uint32_t echoStartedUs_ = 0;
  float distanceCm_ = 0.0f;
  bool initialized_ = false;
};

}  // namespace chimera
