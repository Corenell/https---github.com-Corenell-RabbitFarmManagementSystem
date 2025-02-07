#include <Arduino.h>
#include "HX711.h"
#include "soc/rtc.h"
#include <ESP32Servo.h>

Servo servo1;

#define CALIBRATION_FACTOR 695 //校准因子
#define WEIGHT 50 //定量重量

// HX711接线
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;

void setup() {
  Serial.begin(115200);
  Serial.println("正在初始化");
  setCpuFrequencyMhz(80); 
  Serial.println("HX711");
  servo1.attach(14);
  Serial.println("Servo_OUT");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR); //该值是通过用已知重量校准天平而获得的
  scale.tare();               //将刻度重置为0

  Serial.println("初始化完成");
}

void loop() {
  float weight = scale.get_units();

  if(weight > 5){
    if(weight >= WEIGHT){
      Serial.print("达到目标值:");
      Serial.println(weight);
      servo1.write(1);
      delay(2000);
      servo1.write(120);
    }
  }
  
  Serial.println(weight);
  delay(50);
  
}
