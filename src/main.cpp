#include "Includes/init.hpp"
#include "Includes/config.hpp"
#include "Includes/pinMap.hpp"

void setup() {
  Serial.begin(115200);
  g_logger = Logger(LOGGING_ENABLED, LogLevel::Debug);

  Serial.println("Initializing System");
}

void loop() {

}





/*int retry_counter = 0;
long duracao;
float distancia;

Servo servo;
Adafruit_MPU6050 mpu;
Adafruit_NeoPixel led(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_ST7735 tft = Adafruit_ST7735(DISPLAY_TFT_CS, DISPLAY_TFT_DC, DISPLAY_TFT_MOSI, DISPLAY_TFT_SCLK, DISPLAY_TFT_RST);

void setup() {

  tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(15, 50);
  tft.println("FORGE AO CUBO");
  tft.drawRect(10, 40, 145, 40, ST77XX_YELLOW);

  pinMode(DISPLAY_TFT_LEDBL, OUTPUT);
  digitalWrite(DISPLAY_TFT_LEDBL, LOW);  
  
  Serial.begin(115200);

  Wire.begin(ACCELEROMETER_SDA_PIN, ACCELEROMETER_SCL_PIN);

  mpu.begin();
  Serial.println("MPU6050 iniciado");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(MOTOR_RIGHT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_FORWARD_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_BACKWARD_PIN, OUTPUT);

  pinMode(MOTOR_LEFT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_LEFT_FORWARD_PIN, OUTPUT);
  pinMode(MOTOR_LEFT_BACKWARD_PIN, OUTPUT);

  pinMode(ULTRASONIC_TRIGGER_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  
  servo.attach(SERVO_PIN); // Servo conectado ao pino 9
  led.begin();
  led.clear();

  // Vermelho (R, G, B)
  led.setPixelColor(0, led.Color(10, 10, 10));
  led.show();
  servo.write(90);   // Vai para 90°

  digitalWrite(MOTOR_RIGHT_FORWARD_PIN, LOW);
  digitalWrite(MOTOR_RIGHT_BACKWARD_PIN, LOW);
  analogWrite(MOTOR_RIGHT_PWM_PIN, 0); // PWM 0-255

  digitalWrite(MOTOR_LEFT_FORWARD_PIN, LOW);
  digitalWrite(MOTOR_LEFT_BACKWARD_PIN, LOW);
  analogWrite(MOTOR_LEFT_PWM_PIN, 0); // PWM 0-255


}

void loop() {
  sensors_event_t a, g, temp;

  digitalWrite(DISPLAY_TFT_LEDBL, HIGH);

  digitalWrite(ULTRASONIC_TRIGGER_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(ULTRASONIC_TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIGGER_PIN, LOW);

  // Lê o tempo de retorno
  duracao = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);

  // Calcula distância em cm
  distancia = duracao * 0.034 / 2;

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  mpu.getEvent(&a, &g, &temp);

  Serial.println("=== Acelerometro ===");
  Serial.print("X: ");
  Serial.print(a.acceleration.x);
  Serial.print("  Y: ");
  Serial.print(a.acceleration.y);
  Serial.print("  Z: ");
  Serial.println(a.acceleration.z);

  Serial.println("=== Giroscopio ===");
  Serial.print("X: ");
  Serial.print(g.gyro.x);
  Serial.print("  Y: ");
  Serial.print(g.gyro.y);
  Serial.print("  Z: ");
  Serial.println(g.gyro.z);

  Serial.print("Temperatura: ");
  Serial.print((temp.temperature/340.8)+36.53);
  Serial.println(" °C");

  delay(500);
}*/