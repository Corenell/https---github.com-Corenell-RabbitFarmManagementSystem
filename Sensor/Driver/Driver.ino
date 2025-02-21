// 引脚定义
const int PUL_PIN = 5;  // 脉冲信号引脚（PUL）
const int DIR_PIN = 6;  // 方向信号引脚（DIR）

// 参数定义
const long PULSE_COUNT = 110000;  // 脉冲数量（控制电机旋转角度）
const int PULSE_DELAY = 26;       // 脉冲周期（微秒，控制电机转速）
const int DELAY_BETWEEN_MOVES = 1000;  // 正反转之间的延时（毫秒）

void setup() {
  // 初始化引脚
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
}

void loop() {
  // 正转
  digitalWrite(DIR_PIN, HIGH);  // 设置方向为正转
  generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
  delay(DELAY_BETWEEN_MOVES);  // 延时

  // 反转
  digitalWrite(DIR_PIN, LOW);  // 设置方向为反转
  generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
  delay(DELAY_BETWEEN_MOVES);  // 延时
}

// 生成指定数量的脉冲
void generatePulses(long pulseCount, int pulseDelay) {
  for (long i = 0; i < pulseCount; i++) {
    digitalWrite(PUL_PIN, HIGH);  // 发送 HIGH 脉冲
    delayMicroseconds(pulseDelay);  // 延时
    digitalWrite(PUL_PIN, LOW);  // 发送 LOW 脉冲
    delayMicroseconds(pulseDelay);  // 延时
  }
}