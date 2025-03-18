#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>
BMP280 bmp280;
Adafruit_AHTX0 aht;

// 引脚定义
const int FAN_PIN = 13;  // 风扇连接的 PWM 引脚
const int PUMP_PIN = 12; 

#define MQ135D 15       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚

// PWM 参数
const int PWM_FREQ = 500;  // PWM 频率（Hz）
const int PWM_RESOLUTION = 8;  // PWM 分辨率（8 位，范围为 0-255）
const int PWM_CHANNEL = 0;  // PWM 通道（ESP32 有 16 个通道，0-15）


float NH3Value = 0;

void setup() {
  Serial.begin(115200);
  // 配置 PWM
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION);  // 设置 PWM 通道
  ledcAttach(PUMP_PIN, PWM_FREQ, PWM_RESOLUTION);  // 设置 PWM 通道
  //ledcAttachPin(FAN_PIN, PWM_CHANNEL);  // 将 PWM 通道连接到引脚
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
    Serial.println("AHT20+BMP280 demo!");
  Wire.begin(); 
  bmp280.begin();//初始化BMP280
  if (! aht.begin()) {
    Serial.println("Could not find AHT20+BMP280? Check wiring");
    while (1) delay(10);
  }//初始化ATH20
  Serial.println("AHT10 or AHT20 found");
}

void loop() {
  int power1 = 250;
  int power2 = 255;
  ledcWrite(FAN_PIN, power1);  // 设置 PWM 占空比
  ledcWrite(PUMP_PIN, power2);

  Serial.print("氨气: ");
  Serial.print(digitalRead(MQ135D));//串口打印数字信号
  NH3Value = analogRead(MQ135A);
  Serial.println(NH3Value);//串口打印模拟信号0-4095

  uint32_t pressure = bmp280.getPressure();//BMP280填充气压
  sensors_event_t humidity, temp;//AHT20填充温湿度
  aht.getEvent(&humidity, &temp);
  Serial.print("温度: "); Serial.print(temp.temperature); Serial.println("℃");
  Serial.print("湿度: "); Serial.print(humidity.relative_humidity); Serial.println("％");
  Serial.print("气压: ");
  Serial.print(pressure/1000);
  Serial.println("KPa");
  delay(5000);
}
