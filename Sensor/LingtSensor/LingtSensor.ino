
#define LightSensorD 15       //定义光敏传感器数字输出引脚
#define LightSensorA 34       //定义光敏传感器模拟输出引脚

int lightValue = 0;

void setup() {
  Serial.begin(115200);//串口初始化
  Serial.println();
  pinMode(LightSensorD, INPUT);//定义GPIO15为输入模式
  pinMode(LightSensorA, INPUT);//定义GPIO34为输入模式
}

void loop() {
  
  Serial.println(digitalRead(LightSensorD));//串口打印数字信号
  lightValue = analogRead(LightSensorA);
  Serial.println(lightValue);//串口打印模拟信号0-4095
  delay(1000);
}

//光强越强，模拟量越小，数字信号为0