//驱动180读舵机 
//接线 棕色 负极   红色 5V    橘黄色 信号线 D4
#include <ESP32Servo.h>
 
#define servoPin 14

Servo servo1;
 
void setup() {
    Serial.begin(115200);
    servo1.attach(servoPin);
}
 
void loop() {
 if (Serial.available() > 0) {
    int angle = Serial.parseInt();
    
    // 如果 angle 不等于 0，说明成功读取到了有效的角度值
    if (angle != 0) {
      // 将角度限制在0到180度之间
      angle = constrain(angle, 0, 180);
      
      // 控制舵机转动到指定角度
      servo1.write(angle);
      
      // 显示当前舵机角度
      Serial.print("舵机角度：");
      Serial.println(angle);
    }
  }
}