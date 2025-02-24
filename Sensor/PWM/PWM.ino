
// 引脚定义
const int FAN_PIN = 13;  // 风扇连接的 PWM 引脚

// PWM 参数
const int PWM_FREQ = 200;  // PWM 频率（Hz）
const int PWM_RESOLUTION = 8;  // PWM 分辨率（8 位，范围为 0-255）
const int PWM_CHANNEL = 0;  // PWM 通道（ESP32 有 16 个通道，0-15）

int power = 0;

void setup() {
  Serial.begin(115200);
  // 配置 PWM
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION);  // 设置 PWM 通道
  //ledcAttachPin(FAN_PIN, PWM_CHANNEL);  // 将 PWM 通道连接到引脚
}

void loop() {
  power = 255;
  ledcWrite(FAN_PIN, power);  // 设置 PWM 占空比
  delay(1000);
}
