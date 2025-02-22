#include <HardwareSerial.h>
//GPIO16 is RXD2
//GPIO17 is TXD2
HardwareSerial SerialPort(2); // use UART2

float temperature = -1;
float humidity = -1;
int nh3 = -1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  //初始化串口
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
  // put your main code here, to run repeatedly:
   if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n'); // 读取一行数据
    parseData(data);

    Serial.print("温度: "); Serial.println(temperature);
    Serial.print("湿度："); Serial.println(humidity);
    Serial.print("氨气浓度："); Serial.println(nh3);
  }
}

void parseData(String input) {
  int tIndex = input.indexOf("T:"); // 查找温度标识
  int hIndex = input.indexOf("H:"); // 查找湿度标识
  int nIndex = input.indexOf("N:"); // 查找氨气标识
  if (tIndex != -1 && hIndex != -1 && nIndex != -1) {
    // 提取温度值
    String tempStr = input.substring(tIndex + 2, input.indexOf(',', tIndex));
    temperature = tempStr.toFloat();
    
    // 提取湿度值
    String humiStr = input.substring(hIndex + 2, input.indexOf(',', hIndex));
    humidity = humiStr.toFloat();

    // 提取氨气浓度
    String nh3Str = input.substring(nIndex + 2);
    nh3 = nh3Str.toInt();
    
  }
}
