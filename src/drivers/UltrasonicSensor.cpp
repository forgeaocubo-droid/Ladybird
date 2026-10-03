#include "drivers/UltrasonicSensor.hpp"

#include "includes/Config.hpp"
#include "includes/PinMap.hpp"
#include "utils/Logger.hpp"

namespace chimera {
namespace {

constexpr float kUltrasonicMicrosecondsPerCm = 58.0f;

float durationToCentimeters(uint32_t durationUs) {
  return static_cast<float>(durationUs) / kUltrasonicMicrosecondsPerCm;
}

}  // namespace

bool UltrasonicSensor::begin() {
  gLogger.logDebug("Ultrasonic sensor: configuring trigger and echo GPIOs");
  pinMode(kUltrasonicTriggerPin, OUTPUT);
  pinMode(kUltrasonicEchoPin, INPUT);
  digitalWrite(kUltrasonicTriggerPin, LOW);

  trigger();
  const uint32_t durationUs =
      pulseIn(kUltrasonicEchoPin, HIGH, kUltrasonicTimeoutUs);
  distanceCm_ = durationToCentimeters(durationUs);
  if (durationUs == 0 || distanceCm_ <= 0.0f) {
    gLogger.logError("Ultrasonic sensor initialization failed: no valid echo");
    return false;
  }

  initialized_ = true;
  lastMeasurementStartedMs_ = millis();
  gLogger.logDebug("Ultrasonic sensor startup measurement completed");
  return true;
}

bool UltrasonicSensor::update() {
  if (!initialized_) return false;

  if (state_ == MeasurementState::Idle) {
    if (millis() - lastMeasurementStartedMs_ <
        kUltrasonicMeasurementIntervalMs) {
      return false;
    }
    trigger();
    measurementStartedUs_ = micros();
    lastMeasurementStartedMs_ = millis();
    state_ = MeasurementState::WaitingForRise;
    gLogger.logDebug("Ultrasonic sensor: measurement started");
    return false;
  }

  const uint32_t nowUs = micros();
  if (nowUs - measurementStartedUs_ >= kUltrasonicTimeoutUs) {
    finishMeasurement(0.0f);
    gLogger.logDebug("Ultrasonic sensor: echo timed out");
    return true;
  }

  if (state_ == MeasurementState::WaitingForRise) {
    if (digitalRead(kUltrasonicEchoPin) == HIGH) {
      echoStartedUs_ = nowUs;
      state_ = MeasurementState::WaitingForFall;
    }
    return false;
  }

  if (digitalRead(kUltrasonicEchoPin) == LOW) {
    finishMeasurement(durationToCentimeters(nowUs - echoStartedUs_));
    gLogger.logDebug("Ultrasonic sensor: measurement completed");
    return true;
  }
  return false;
}

float UltrasonicSensor::distanceCm() const { return distanceCm_; }

void UltrasonicSensor::trigger() {
  digitalWrite(kUltrasonicTriggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(kUltrasonicTriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(kUltrasonicTriggerPin, LOW);
}

void UltrasonicSensor::finishMeasurement(float distanceCm) {
  distanceCm_ = distanceCm;
  state_ = MeasurementState::Idle;
}

}  // namespace chimera
