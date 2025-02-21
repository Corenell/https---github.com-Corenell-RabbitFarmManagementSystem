// 引脚定义
const int FAN_PIN = 13;  // 风扇连接的 PWM 引脚

// PWM 参数
const int PWM_FREQ = 1000;  // PWM 频率（Hz）
const int PWM_RESOLUTION = 8;  // PWM 分辨率（8 位，范围为 0-255）
const int PWM_CHANNEL = 0;  // PWM 通道（ESP32 有 16 个通道，0-15）

void setup() {
  // 配置 PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);  // 设置 PWM 通道
  ledcAttachPin(FAN_PIN, PWM_CHANNEL);  // 将 PWM 通道连接到引脚
}

void loop() {
  // 从最低速到最高速逐步增加
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle += 5) {
    ledcWrite(PWM_CHANNEL, dutyCycle);  // 设置 PWM 占空比
    delay(100);  // 延时 100ms
  }

  // 从最高速到最低速逐步降低
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle -= 5) {
    ledcWrite(PWM_CHANNEL, dutyCycle);  // 设置 PWM 占空比
    delay(100);  // 延时 100ms
  }
}
