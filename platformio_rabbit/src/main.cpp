#include <Arduino.h>
// 宏定义 GPIO 输出引脚
#define LED_PIN 12

void setup() {
    // 配置 GPIO 输出引脚为输出模式
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    // 实现渐亮效果
    for (int i = 0; i < 256; i++) {
        // 设置亮度模拟值
        analogWrite(LED_PIN, i);
        // 延时 10ms
        delay(10);
    }

    // 实现渐灭效果
    for (int i = 255; i >= 0; i--) {
        // 设置亮度模拟值
        analogWrite(LED_PIN, i);
        // 延时 10ms
        delay(10);
    }
}
