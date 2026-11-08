#include "Includes/Init.hpp"
#include "Includes/Config.hpp"
#include "Includes/PinMap.hpp"
#include "Drivers/WifiSTA/Wifi_sta.hpp"

using esp32_component::Accelerometer;
using esp32_component::ColorDisplay;
using esp32_component::HBridgeMotor;
using esp32_component::Logger;
using esp32_component::LogLevel;
using esp32_component::MotorDirection;
using esp32_component::RGBLed;
using esp32_component::ServoMotor;
using esp32_component::Ultrasonic;
using esp32_component::WifiSta;

namespace {

constexpr char kWifiSsid[] = "VivoPA";
constexpr char kWifiPassword[] = "pa314159180267";
constexpr uint32_t kTelemetryIntervalMs = 20;

Ultrasonic ultrasonic{ULTRASONIC_TRIGGER_PIN, ULTRASONIC_ECHO_PIN};
Accelerometer accelerometer{kMpu6050Address, ACCELEROMETER_SDA_PIN,
                            ACCELEROMETER_SCL_PIN};
ServoMotor servo{SERVO_PIN};
RGBLed rgb_led{RGB_LED_PIN};
ColorDisplay display{DISPLAY_TFT_CS, DISPLAY_TFT_DC, DISPLAY_TFT_MOSI,
                     DISPLAY_TFT_SCLK, DISPLAY_TFT_RST, DISPLAY_TFT_LEDBL};
HBridgeMotor right_motor{MOTOR_RIGHT_FORWARD_PIN, MOTOR_RIGHT_BACKWARD_PIN,
                         MOTOR_RIGHT_PWM_PIN};
HBridgeMotor left_motor{MOTOR_LEFT_FORWARD_PIN, MOTOR_LEFT_BACKWARD_PIN,
                        MOTOR_LEFT_PWM_PIN};
WifiSta wifi_sta{kWifiPort, kWifiReconnectIntervalMs};
uint32_t last_telemetry_ms = 0;
bool wifi_was_connected = false;

void LogInitializationFailure(const String& component) {
  Logger::Instance().LogError(component + " initialization failed");
}

String BuildTelemetryJson() {
  const uint32_t distance_mm = ultrasonic.MeasureNow();
  float acceleration_x{0.0F};
  float acceleration_y{0.0F};
  float acceleration_z{0.0F};
  float gyro_x{0.0F};
  float gyro_y{0.0F};
  float gyro_z{0.0F};
  accelerometer.SampleNow();
  accelerometer.ReadAxes(acceleration_x, acceleration_y, acceleration_z);
  accelerometer.ReadGyroAxes(gyro_x, gyro_y, gyro_z);

  return String("{\"acceleration_x\":") + String(acceleration_x, 3) +
         ",\"acceleration_y\":" + String(acceleration_y, 3) +
         ",\"acceleration_z\":" + String(acceleration_z, 3) +
         ",\"gyroscope_x\":" + String(gyro_x, 3) +
         ",\"gyroscope_y\":" + String(gyro_y, 3) +
         ",\"gyroscope_z\":" + String(gyro_z, 3) +
         ",\"temperature_c\":" +
         String(accelerometer.ReadTemperature(), 2) +
         ",\"distance_cm\":" + String(distance_mm / 10.0F, 1) + "}";
}

void LogConnection() {
  Logger::Instance().LogInfo(String("Wi-Fi connected: IP=") +
                             wifi_sta.LocalIp().toString() +
                             " port=" + String(wifi_sta.Port()));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Logger::Instance().Configure(LOGGING_ENABLED, LogLevel::Info);
  Logger::Instance().LogInfo("Starting Ladybird");

  if (!ultrasonic.Initialize()) LogInitializationFailure("Ultrasonic");
  if (!accelerometer.Initialize()) LogInitializationFailure("Accelerometer");
  if (!servo.Initialize()) LogInitializationFailure("Servo");
  if (!servo.SetAngle(90)) LogInitializationFailure("Servo angle");
  servo.Update();
  if (!rgb_led.Initialize()) LogInitializationFailure("RGB LED");
  if (!rgb_led.SetColor(10, 10, 10)) LogInitializationFailure("RGB LED color");
  if (!display.Initialize()) LogInitializationFailure("Color display");
  if (!display.ShowText("Forge Ao cubo", 15, 50, ST77XX_WHITE, 1)) {
    LogInitializationFailure("Color display text");
  }
  if (!right_motor.Initialize()) LogInitializationFailure("Right motor");
  if (!left_motor.Initialize()) LogInitializationFailure("Left motor");
  right_motor.SetDirection(MotorDirection::Stopped);
  right_motor.SetIntensity(0);
  right_motor.Update();

  left_motor.SetDirection(MotorDirection::Stopped);
  left_motor.SetIntensity(0);
  left_motor.Update();
  if (!wifi_sta.Initialize(kWifiSsid, kWifiPassword)) {
    LogInitializationFailure("Wi-Fi STA");
  }
  Logger::Instance().LogInfo(String("Wi-Fi telemetry TCP port: ") +
                             String(wifi_sta.Port()));
}

void loop() {
  wifi_sta.Update();
  const uint32_t now = millis();
  if (wifi_sta.IsConnected() && now - last_telemetry_ms >= kTelemetryIntervalMs) {
    last_telemetry_ms = now;
    wifi_sta.Broadcast(BuildTelemetryJson());
  }
  const bool wifi_connected = wifi_sta.IsConnected();
  if (wifi_connected && !wifi_was_connected) {
    LogConnection();
  }
  wifi_was_connected = wifi_connected;
}
