// 校准称重传感器
#include <Arduino.h>
#include "soc/rtc.h"
#include "HX711.h"

// HX711电路接线
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 17;

HX711 scale;

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80); 
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {

  if (scale.is_ready()) {
    scale.set_scale();    
    Serial.println("等待... 移除托盘重物");
    delay(5000);
    scale.tare();
    Serial.println("皮重测量完成...");
    Serial.print("放已知重量物体...");
    delay(5000);
    long reading = scale.get_units(10);
    Serial.print("结果: ");
    Serial.println(reading);
  } 
  else {
    Serial.println("HX711离线.");
  }
  delay(1000);
}

//695