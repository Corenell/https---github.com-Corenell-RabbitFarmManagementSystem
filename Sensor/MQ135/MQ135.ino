#define MQ135D 15       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚

float NH3Value = 0;

void setup() {
  Serial.begin(115200);//串口初始化
  Serial.println();
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
}

void loop() {
  
  Serial.println(digitalRead(MQ135D));//串口打印数字信号
  NH3Value = analogRead(MQ135A);
  Serial.println(NH3Value);//串口打印模拟信号0-4095
  delay(1000);
}

