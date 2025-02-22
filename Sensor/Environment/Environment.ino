#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>
#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文


#define MQ135D 25       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚


//I2C接线// 引脚定义
const int FAN_PIN = 13;  // 风扇连接的 PWM 引脚
const int PUMP_PIN = 12;  // 风扇连接的 PWM 引脚

// PWM 参数
const int PWM_FREQ = 1000;  // PWM 频率（Hz）
const int PWM_RESOLUTION = 8;  // PWM 分辨率（8 位，范围为 0-255）
//const int PWM_CHANNEL = 0;  // PWM 通道（ESP32 有 16 个通道，0-15）

const int PWM_FREQ2 = 1000;  // PWM 频率（Hz）
const int PWM_RESOLUTION2 = 8;  // PWM 分辨率（8 位，范围为 0-255）
//const int PWM_CHANNEL2 = 1;  // PWM 通道（ESP32 有 16 个通道，0-15）
//SDA 21  SCL 22

// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_rabbit_0_0_2025022101";
const char* mqttUser = "67b683d83f28ab3d0384f27e_rabbit";
const char* mqttPassword = "7be5c6ed9fe56b50c0dcb0bc443b577c373d2e15ac94345746b0646cfdd3b4fb";
String half_get_properties = "$oc/devices/67b683d83f28ab3d0384f27e_rabbit/sys/properties/get/request_id=";
String half_response_properties = "$oc/devices/67b683d83f28ab3d0384f27e_rabbit/sys/properties/get/response/request_id=";
String get_messages = "$oc/devices/67b683d83f28ab3d0384f27e_rabbit/sys/messages/down";


BMP280 bmp280;
Adafruit_AHTX0 aht;
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  String topicStr = String(topic);
  String receivedMessage = ""; 
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.print("Message: ");
  Serial.println(receivedMessage);
  
  if (topicStr.startsWith(half_get_properties)) {
    String requestId = topicStr.substring(half_get_properties.length());
    Serial.print("Extracted requestId: ");
    Serial.println(requestId);
    MQTT_response1(requestId);  // 调用 MQTT_response1
}
  else if (topicStr == get_messages) {
    MQTT_response2(receivedMessage);  // 调用 MQTT_response2
  }
  else {
    // 如果 topic 不是指定的 topic
    Serial.println("Unknown topic");
  }
}


void MQTT_Init() {
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setKeepAlive(60);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed to connect, state: ");
      Serial.println(client.state());
      delay(6000);
    }
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setCallback(callback);


}


void MQTT_response1(String requestId) {
  Serial.print("Extracted requestId: ");
  Serial.println(requestId);

  //uint32_t pressure = bmp280.getPressure();  //BMP280填充气压
  sensors_event_t humidity, temp;  //AHT20填充温湿度
  aht.getEvent(&humidity, &temp);
  float tempread = temp.temperature;
  float humiread = humidity.relative_humidity;

  int NH3Value = analogRead(MQ135A);
  NH3Value = 3;
  //调试输出
  Serial.print("温度: "); Serial.print(tempread); Serial.println("℃");
  Serial.print("湿度: "); Serial.print(humiread); Serial.println("%");
  //Serial.print("气压: "); Serial.print(pressure/1000); Serial.println("KPa");
  Serial.println(digitalRead(MQ135D));//串口打印数字信号
  Serial.println(NH3Value);//串口打印模拟信号0-4095


  String JSONmessageBuffer;
  StaticJsonDocument<256> responseDoc;
  responseDoc["code"] = 200;
  responseDoc["temperature"] = tempread;
  responseDoc["humidity"] = humiread;
  responseDoc["ammonia"] = NH3Value;
  // 序列化响应消息
  String responseMessage;
  serializeJson(responseDoc, responseMessage);

  // 更新响应的主题，包含 request_id
  String responseTopic = half_get_properties + requestId;
  Serial.println(responseTopic);
  if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent successfully");
  } else {
    Serial.println("Error sending response");
  }
}

void MQTT_response2(String receivedMessage) {
  // 创建一个 JSON 文档
  DynamicJsonDocument doc(256);

  // 解析 JSON 数据
  DeserializationError error = deserializeJson(doc, receivedMessage);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  // 提取 fan_power 和 water_curtain_power
  int fanPower = doc["content"]["fan_power"];
  int waterCurtainPower = doc["content"]["water_curtain_power"];

  fanPower = constrain(fanPower, 0, 100);
  waterCurtainPower = constrain(waterCurtainPower, 0, 100);

  int power = round(fanPower * 255.0 / 100.0);
  int power2 = round(waterCurtainPower * 255.0 / 100.0);
  
  ledcWrite(FAN_PIN, power);  // 设置 PWM 占空比
  ledcWrite(PUMP_PIN, power2);  // 设置 PWM 占空比

  // 输出结果
  Serial.print("Fan Power: ");
  Serial.println(fanPower);
  Serial.print("Water Curtain Power: ");
  Serial.println(waterCurtainPower);
}

void reconnect() {
  int attempt = 0;
  while (!client.connected()) {
    Serial.printf("Attempt %d: Reconnecting to MQTT...\n", ++attempt);
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("Reconnected to MQTT");
    } else {
      Serial.printf("Failed to reconnect, state: %d. Retrying...\n", client.state());
      delay(6000);
    }
  }
}



void setup() {
  Serial.begin(115200);//初始化串口
  Serial.println("AHT20+MQ135");
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
  Wire.begin(); 
  bmp280.begin();  //初始化BMP280
  // 配置 PWM
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(PUMP_PIN, PWM_FREQ2, PWM_RESOLUTION2);  
  MQTT_Init();
  
  while(!aht.begin()){
    Serial.println("Aht20 initial error!");
    delay(500);
  }  //Aht20初始化

  Serial.println("AHT20 found");
}

void loop() {

    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(1000);
}