#include <Arduino.h>
#include <ESP32Servo.h>
#include "HX711.h"
#include "soc/rtc.h"

// 硬件引脚定义
#define LOADCELL_DOUT_PIN 17
#define LOADCELL_SCK_PIN  16

#define CALIBRATION_FACTOR 719 //校准因子

// 系统参数
const float TARGET_WEIGHT = 15.0;  // 目标投喂量（克）

const int FEEDING_SPEED = 2000;     // 输料舵机全速运转角度 1500-2000 顺
const int STOP_ANGLE = 1500;         // 输料舵机停止角度
const int DUMP_ANGLE = 1;        // 翻斗倾倒角度
const int RETURN_ANGLE = 120;        // 翻斗复位角度
const int SETTLE_TIME = 10000;      // 料斗稳定时间(ms)

Servo servo360;  // 输料舵机（360°连续旋转）
Servo servo180;  // 翻斗舵机（180°标准）
HX711 scale;

// 系统状态
enum SystemState { IDLE, FEEDING, DUMPING };
SystemState currentState = IDLE;
unsigned long actionStartTime = 0;

// 移动平均滤波（稳定性较好，暂时关闭滤波）
float filteredWeight() {
  /*
  static float buffer[5];
  static byte index = 0;
  
  buffer[index] = scale.get_units();
  index = (index + 1) % 5;
  
  float sum = 0;
  for(byte i=0; i<5; i++) sum += buffer[i];
  return sum / 5;
  */
  float num = scale.get_units();
  return num;
}

void setup() {
  Serial.begin(115200);
  Serial.println("正在初始化");
  setCpuFrequencyMhz(80); 
  
  // 初始化舵机
  servo360.attach(25);  // 输料螺杆舵机（连续旋转）
  servo180.attach(26);  // 翻斗舵机
  servo360.write(STOP_ANGLE);  // 初始停止状态1500
  servo180.write(RETURN_ANGLE);
  Serial.println("Servo Ready!");

  // 初始化称重模块
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();
  Serial.println("HX711 Ready!");

}

void loop() {
  float currentWeight = filteredWeight();
  
  switch(currentState) {
    case IDLE:
      if(currentWeight <= TARGET_WEIGHT) {  // 检测到开始投喂
        currentState = FEEDING;
        servo360.writeMicroseconds(FEEDING_SPEED);  // 启动输料
        Serial.println("开始输料...");
      }
      break;

    case FEEDING:
      if(currentWeight >= TARGET_WEIGHT) {
        servo360.writeMicroseconds(STOP_ANGLE);     // 停止输料
        currentState = DUMPING;
        actionStartTime = millis();     // 记录动作开始时间
        Serial.println("达到目标重量，准备倾倒");
      }
      break;

    case DUMPING: {
      static uint8_t phase = 0;
      unsigned long elapsed = millis() - actionStartTime;

      switch(phase) {
        case 0: // 等待稳定
          if(elapsed > SETTLE_TIME) {
            servo180.write(DUMP_ANGLE);
            actionStartTime = millis();
            phase = 1;
          }
          break;
          
        case 1: // 保持倾倒
          if(elapsed > 2000) {
            servo180.write(RETURN_ANGLE);
            actionStartTime = millis();
            phase = 2;
          }
          break;
          
        case 2: // 复位完成
          if(elapsed > 2000) {
            scale.tare();
            currentState = IDLE;
            phase = 0;
            Serial.println("完成投喂循环");
          }
          break;
      }
      break;
    }
  }

  // 调试输出
  static unsigned long lastPrint = 0;
  if(millis() - lastPrint > 500) {
    Serial.print("当前状态: ");
    Serial.print(currentState);
    Serial.print(" | 重量: ");
    Serial.print(currentWeight);
    Serial.println("g");
    lastPrint = millis();
  }
  
  //delay(50);  // 主循环延迟
}
