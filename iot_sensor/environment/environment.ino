#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>
#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文

// 光敏传感器引脚
#define MQ135D 25       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚


//I2C接线// 引脚定义
const int FAN_PIN = 13;  // 风扇连接的 PWM 引脚
const int PUMP_PIN = 12;  // 风扇连接的 PWM 引脚

// PWM 参数
const int PWM_FREQ = 500;  // PWM 频率（Hz）
const int PWM_RESOLUTION = 8;  // PWM 分辨率（8 位，范围为 0-255）
//const int PWM_CHANNEL = 0;  // PWM 通道（ESP32 有 16 个通道，0-15）

const int PWM_FREQ2 = 500;  // PWM 频率（Hz）
const int PWM_RESOLUTION2 = 8;  // PWM 分辨率（8 位，范围为 0-255）
//const int PWM_CHANNEL2 = 1;  // PWM 通道（ESP32 有 16 个通道，0-15）
//SDA 21  SCL 22

// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_environment_0_0_2025022306";
const char* mqttUser = "67b683d83f28ab3d0384f27e_environment";
const char* mqttPassword = "17a85498dc8339943237186c61e7aa2861be33405971bd0eab52d080f762ae92";
String half_get_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/request_id=");
String half_response_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/response/request_id=");
String get_messages = String("$oc/devices/") + mqttUser + String("/sys/messages/down");

// 初始化参数
BMP280 bmp280;
Adafruit_AHTX0 aht;
WiFiClient espClient;
PubSubClient client(espClient);

// 连接wifi和mqtt
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
// 接收mqtt信息并且调用相应的函数
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("topic: ");
  Serial.println(topic);
  String topicStr = String(topic);

  String receivedMessage = ""; 
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.print("message: ");
  Serial.println(receivedMessage);
  JsonDocument doc;
    DeserializationError error = deserializeJson(doc, receivedMessage);
    if (error) {
     Serial.print("Failed to parse JSON: ");
     Serial.println(error.f_str());
      return;
    }
  // 查询设备属性
  if (topicStr.startsWith(half_get_properties)) {
    String requestId = topicStr.substring(half_get_properties.length());
    String type = doc["service_id"];
    if(type == "get_tha"){
      get_tha(requestId);
    }
  }

  // 下发设备消息
  else if (topicStr == get_messages) {
    String type = doc["content"]["type"];
    if(type == "post_fw"){
       post_fw(doc);
    }
  }
}

// 获取温湿度氨气浓度并且发送至云平台
void get_tha(String requestId) {
  //uint32_t pressure = bmp280.getPressure();  //BMP280填充气压
  sensors_event_t humidity, temp;  //AHT20填充温湿度
  aht.getEvent(&humidity, &temp);
  float tempread = temp.temperature;
  float humiread = humidity.relative_humidity;
  float NH3Value = analogRead(MQ135A);
  //调试输出
  Serial.print("温度: "); Serial.print(tempread); Serial.println("℃");
  Serial.print("湿度: "); Serial.print(humiread); Serial.println("%");
  //Serial.print("气压: "); Serial.print(pressure/1000); Serial.println("KPa");
  Serial.print("氨气浓度: ");Serial.println(NH3Value);//串口打印模拟信号0-4095

  // 构造 JSON 响应
  JsonDocument responseDoc;
  responseDoc["temperature"] = tempread;
  responseDoc["humidity"] = humiread;
  responseDoc["ammonia"] = NH3Value;
  // 序列化响应消息
  String responseMessage;
  serializeJson(responseDoc, responseMessage);

  String responseTopic = half_response_properties + requestId;
  if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent success");
  } else {
    Serial.println("Error sending response");
  }
}

// 接收风机水帘参数调控
void post_fw(JsonDocument doc) {
  int fanPower = doc["content"]["fan_power"];
  int waterCurtainPower = doc["content"]["water_curtain_power"];
  Serial.println(fanPower);
  Serial.println(waterCurtainPower);

  control(fanPower, waterCurtainPower);
}

void control(int fanPower, int waterCurtainPower) {
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

// MQTT 断连重新连接
// MQTT 重新连接函数
// 重新连接mqtt
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
  MQTT_Init();
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
  Wire.begin(); 
  bmp280.begin();  //初始化BMP280
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(PUMP_PIN, PWM_FREQ2, PWM_RESOLUTION2); 
  //Aht20初始化
  while(!aht.begin()){
    Serial.println("Aht20 initial error!");
    delay(500);
  }
  Serial.println("AHT20 found");
}

void loop() {
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(1000);
}





