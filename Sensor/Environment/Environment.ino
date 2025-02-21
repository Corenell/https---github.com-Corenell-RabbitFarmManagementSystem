#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>

#define MQ135D 25       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚
//I2C接线
//SDA 21  SCL 22

BMP280 bmp280;
Adafruit_AHTX0 aht;

int NH3Value = -10;

void setup() {
  Serial.begin(115200);//初始化串口
  Serial.println("AHT20+MQ135");
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
  Wire.begin(); 
  bmp280.begin();  //初始化BMP280
  
  while(!aht.begin()){
    Serial.println("Aht20 initial error!");
    delay(500);
  }  //Aht20初始化

  Serial.println("AHT20 found");
}

void loop() {
  //uint32_t pressure = bmp280.getPressure();  //BMP280填充气压
  sensors_event_t humidity, temp;  //AHT20填充温湿度
  aht.getEvent(&humidity, &temp);
  float tempread = temp.temperature;
  float humiread = humidity.relative_humidity;
  NH3Value = analogRead(MQ135A);

  //调试输出
  Serial.print("温度: "); Serial.print(tempread); Serial.println("℃");
  Serial.print("湿度: "); Serial.print(humiread); Serial.println("%");
  //Serial.print("气压: "); Serial.print(pressure/1000); Serial.println("KPa");
  Serial.println(digitalRead(MQ135D));//串口打印数字信号
  Serial.println(NH3Value);//串口打印模拟信号0-4095
  delay(1000);
}