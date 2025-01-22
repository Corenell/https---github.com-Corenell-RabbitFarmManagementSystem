#include <Arduino.h>
#include "HX711.h"
#include "soc/rtc.h"

#define CALIBRATION_FACTOR 695 //校准因子

// HX711接线
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;

void setup() {
  Serial.begin(115200);
  Serial.println("正在初始化");
  setCpuFrequencyMhz(80); 
  Serial.println("HX711");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
/*
  Serial.println("无校准：");
  Serial.print("读数: \t\t");
  Serial.println(scale.read());      // 打印ADC的原始读数

  Serial.print("读数平均值: \t\t");
  Serial.println(scale.read_average(20));   // 打印ADC 20个读数的平均值

  Serial.print("获取值: \t\t");
  Serial.println(scale.get_value(5));   // 打印ADC的5个读数的平均值减去皮重（尚未设置）

  Serial.print("获取单位: \t\t");
  Serial.println(scale.get_units(5), 1);  // 打印ADC的5个读数的平均值减去皮重（未设置）
  // 通过传感器参数（尚未设置）
*/     

  scale.set_scale(CALIBRATION_FACTOR); //该值是通过用已知重量校准天平而获得的
  scale.tare();               //将刻度重置为0
/*
  Serial.println("设置磅秤后:");

  Serial.print("读数: \t\t");
  Serial.println(scale.read());                 // 设置磅秤后

  Serial.print("read ave: \t\t");
  Serial.println(scale.read_average(20));       // 打印ADC 20个读数的平均值

  Serial.print("获取值: \t\t");
  Serial.println(scale.get_value(5));   // 打印ADC的5个读数的平均值减去皮重，用皮重（）设置

  Serial.print("获取单位: \t\t");
  Serial.println(scale.get_units(5), 1);        // 打印ADC的5个读数的平均值减去皮重，除以
//通过使用set_scale设置的S传感器参数
*/
  Serial.println("初始化完成");
}

void loop() {
  Serial.print("读数：\t");
  Serial.print(scale.get_units(), 1);
  Serial.println("g");
  //Serial.print("\t| average:\t");
  //Serial.println(scale.get_units(10), 5);

  scale.power_down();             // 将ADC置于睡眠模式
  delay(100);
  scale.power_up();
}