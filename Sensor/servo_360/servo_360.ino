#include <ESP32Servo.h>
Servo servo;

void setup() {
  servo.attach(14);
}

void loop() {
  // rotate counter-clockwise full-speed
  int time = Serial.parseInt();
  servo.writeMicroseconds(1000);
  delay(2000);//顺时针120度

  // rotation stopped
  servo.writeMicroseconds(1500);
  delay(1000);//停1s

  // rotate clockwise full-speed
  servo.writeMicroseconds(2000);
  delay(1798);//逆时针120度
  //360舵机不好精确控制角度有一个误差，暂时这么用把，等180度舵机来了换

  // rotation stopped
  servo.writeMicroseconds(1500);
  delay(1000);
}