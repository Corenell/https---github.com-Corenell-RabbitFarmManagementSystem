#include <Arduino.h>
#include <ESP32Servo.h>
#include "HX711.h"
#include "soc/rtc.h"

// 硬件引脚定义
#define LOADCELL_DOUT_PIN 17
#define LOADCELL_SCK_PIN  16

#define CALIBRATION_FACTOR 719 //校准因子

// 系统参数
float TARGET_WEIGHT = 1;  // 目标投喂量（克）
float TARGET_NUM = 1;  //云端发送的饲料参数
int State = 0;  //喂料初始状态为0（不执行）
const int FEEDING_SPEED = 2000;     // 输料舵机全速运转角度 1500-2000 顺
const int STOP_ANGLE = 1500;         // 输料舵机停止角度
const int DUMP_ANGLE = 1;        // 翻斗倾倒角度
const int DUMP_ANGLE2 = 30;       //翻斗倾倒角度2
const int RETURN_ANGLE = 120;        // 翻斗复位角度
const int SETTLE_TIME = 10000;      // 料斗稳定时间(ms)


// 系统状态
enum SystemState { IDLE, FEEDING, DUMPING };
SystemState currentState = IDLE;  //初始状态为闲置
unsigned long actionStartTime = 0;

Servo servo360;  // 输料舵机（360°连续旋转）
Servo servo180;  // 翻斗舵机（180°标准）
HX711 scale;  //HX711

// 移动平均滤波（称重稳定性较好，暂时关闭滤波）
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
  setCpuFrequencyMhz(80);  //HX711需要esp32工作在80MHz
  
  // 初始化舵机
  servo360.attach(25);  // 输料螺杆舵机（连续旋转）
  servo180.attach(26);  // 翻斗舵机
  servo360.write(STOP_ANGLE);  // 初始停止状态1500
  servo180.write(RETURN_ANGLE);  //初始水平状态120
  Serial.println("Servo Ready!");

  // 初始化称重模块
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);  //设置校准因子
  scale.tare();  //称 清零
  Serial.println("HX711 Ready!");

}


void loop() {
  float currentWeight = filteredWeight();  //获取当前料斗上的饲料重量

  //从IOT获取State值
  if(State == 1){  //State为1，开始投喂流程

  switch(currentState) {
    case IDLE:  //闲置
      TARGET_WEIGHT = TARGET_NUM;  //获取新的目标投喂量
      if(currentWeight <= TARGET_WEIGHT) {  //是否达到目标量
        currentState = FEEDING;  //切换为FEEDING状态
        servo360.writeMicroseconds(FEEDING_SPEED);  // 启动输料
        Serial.println("开始输料...");
      }
      break;

    case FEEDING:  //喂料
      if(currentWeight >= TARGET_WEIGHT) {  //是否达到目标量
        servo360.writeMicroseconds(STOP_ANGLE);  // 停止输料
        currentState = DUMPING;  //切换为倒料状态
        actionStartTime = millis();     // 记录饲料称重完成的时刻
        Serial.println("达到目标重量，准备倾倒");
      }
      break;

    case DUMPING: {  //倾倒
      static uint8_t phase = 0;  //初始状态0
      unsigned long elapsed = millis() - actionStartTime;  //计算动作时刻到目前的时间差

      switch(phase) {
        case 0: //执行倾倒
          if(elapsed > SETTLE_TIME) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE);  //倾倒
            actionStartTime = millis();  //更新倾倒时刻
            phase = 1;  
          }
          break;
          
        case 1: // 抖动倾倒
          if(elapsed > 1000) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE2);  //90度位
            actionStartTime = millis();  //更新抖动时刻
            phase = 2;
          }
          break;
          
        case 2: // 再次倾倒
          if(elapsed > 1000) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE);  //再次倾倒
            actionStartTime = millis();  //更新倾倒时刻
            phase = 3;
          }
          break;

        case 3: // 保持倾倒
          if(elapsed > 3000) {  //时间差大于稳定间隔
            servo180.write(RETURN_ANGLE); //料斗复位
            actionStartTime = millis();  //更新动作时间
            phase = 4;
          }
          break;
          
        case 4: // 复位完成
          if(elapsed > 2000) {  //时间差大于稳定间隔
            scale.tare();  //称 清零
            currentState = IDLE;  //切换为闲置状态
            phase = 0;  //切换状态0
            Serial.println("完成投喂循环");
            State = 0;  //完成投喂
          }
          break;
      }
      break;
    }
  }
  }

  //向IOT更新当前State
  
  // 调试输出
  static unsigned long lastPrint = 0;  //上次输出时刻
  if(millis() - lastPrint > 1000) {  //间隔1s
    Serial.print("当前状态: ");
    Serial.print(State);
    Serial.print(" | 流程: ");
    Serial.print(currentState);
    Serial.print(" | 重量: ");
    Serial.print(currentWeight);
    Serial.println("g");
    Serial.print("目标重量: ");
    Serial.print(TARGET_WEIGHT);
    Serial.println("g");
    lastPrint = millis();  //更新输出时刻
  }
  
  //delay(50);  // 主循环延迟
}
















