#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文

// 引脚定义
const int PUL_PIN = 5;  // 脉冲信号引脚（PUL）
const int DIR_PIN = 6;  // 方向信号引脚（DIR）

// 参数定义
const long PULSE_COUNT = 110000;  // 脉冲数量（控制电机旋转角度）
const int PULSE_DELAY = 26;       // 脉冲周期（微秒，控制电机转速）
const int DELAY_BETWEEN_MOVES = 1000;  // 正反转之间的延时（毫秒#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文

// 引脚定义
const int PUL_PIN = 5;  // 脉冲信号引脚（PUL）
const int DIR_PIN = 6;  // 方向信号引脚（DIR）

// 参数定义
const long PULSE_COUNT = 110000;  // 脉冲数量（控制电机旋转角度）
const int PULSE_DELAY = 26;       // 脉冲周期（微秒，控制电机转速）
const int DELAY_BETWEEN_MOVES = 1000;  // 正反转之间的延时（毫秒）


// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_motor_0_0_2025022306";
const char* mqttUser = "67b683d83f28ab3d0384f27e_motor";
const char* mqttPassword = "17a85498dc8339943237186c61e7aa2861be33405971bd0eab52d080f762ae92";
//mqtt连接信息
String half_get_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/properties/get/request_id=";
String half_response_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/properties/get/response/request_id=";
String half_get_command = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/commands/request_id=";
String half_response_command = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/commands/response/request_id={request_id}";
String get_messages = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/messages/down";
String post_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/properties/report";



WiFiClient espClient;
PubSubClient client(espClient);

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
  
  if (topicStr == get_messages) {
    MQTT_response1(receivedMessage);
  }
  else if (topicStr.startsWith(half_get_command)) {
    String requestId = topicStr.substring(half_get_command.length());
    MQTT_response3(requestId);
    // 小车继续运行



    
  }
}


//接受开始投喂指令
void MQTT_response1(String receivedMessage) {
 JsonDocument doc;
  DeserializationError error = deserializeJson(doc, receivedMessage);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }
  // 解析json数据
  int feed = doc["feed"];
  Serial.println(feed);


  //当json数据满足一定条件时，舵机开始转动
  // 转动到指定位置时，
  // 运行发送停止信号函数
  // MQTT_response2() 
}





// // 发送信息给nano，
// void MQTT_response2() {

//   // 构造 JSON 响应
//   JsonDocument responseDoc;
//   responseDoc["a"] = b;

//   // 序列化响应消息
//   String responseMessage;
//   serializeJson(responseDoc, responseMessage);



//   String responseTopic = post_properties
//   if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
//     Serial.println("Response sent success");
//   } else {
//     Serial.println("Error sending response");
//   }
// }


// 回应
void MQTT_response3(String requestId) {

  // 构造 JSON 响应
  JsonDocument responseDoc;
  responseDoc["state"] = 1;

  // 序列化响应消息
  String responseMessage;
  serializeJson(responseDoc, responseMessage);
  // 更新响应的主题，包含 request_id


  String responseTopic = half_response_command + requestId;
  if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent success");
  } else {
    Serial.println("Error sending response");
  }
}




// MQTT 重新连接函数
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
    // 添加超时机制
    if (attempt > 30) {
      Serial.println("Reconnect timeout, restarting...");
      ESP.restart();
    }
  }
}


void setup() {
  Serial.begin(115200);//初始化串口
  MQTT_Init();
  // 初始化引脚
  // pinMode(PUL_PIN, OUTPUT);
  // pinMode(DIR_PIN, OUTPUT);
}

void loop() {
  if (!client.connected()) {
  reconnect();
  }
  client.loop();
}
  // 正转
//   digitalWrite(DIR_PIN, HIGH);  // 设置方向为正转
//   generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
//   delay(DELAY_BETWEEN_MOVES);  // 延时

//   // 反转
//   digitalWrite(DIR_PIN, LOW);  // 设置方向为反转
//   generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
//   delay(DELAY_BETWEEN_MOVES);  // 延时
// }

// // 生成指定数量的脉冲
// void generatePulses(long pulseCount, int pulseDelay) {
//   for (long i = 0; i < pulseCount; i++) {
//     digitalWrite(PUL_PIN, HIGH);  // 发送 HIGH 脉冲
//     delayMicroseconds(pulseDelay);  // 延时
//     digitalWrite(PUL_PIN, LOW);  // 发送 LOW 脉冲
//     delayMicroseconds(pulseDelay);  // 延时
//   }
）


// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_motor_0_0_2025022306";
const char* mqttUser = "67b683d83f28ab3d0384f27e_motor";
const char* mqttPassword = "17a85498dc8339943237186c61e7aa2861be33405971bd0eab52d080f762ae92";
//mqtt连接信息
String half_get_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/properties/get/request_id=";
String half_response_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/properties/get/response/request_id=";
String half_get_command = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/commands/request_id=";
String half_response_command = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/commands/response/request_id={request_id}";
String get_messages = "$oc/devices/67b683d83f28ab3d0384f27e_motor/sys/messages/down";
String post_properties = "$oc/devices/67b683d83f28ab3d0384f27e_motor/properties/report";



WiFiClient espClient;
PubSubClient client(espClient);

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
  
  if (topicStr == get_messages) {
    MQTT_response1(receivedMessage);
  }
  else if (topicStr.startsWith(half_get_command)) {
    String requestId = topicStr.substring(half_get_command.length());
    MQTT_response3(requestId);
    // 小车继续运行



    
  }
}


//接受开始投喂指令
void MQTT_response1(String receivedMessage) {
 JsonDocument doc;
  DeserializationError error = deserializeJson(doc, receivedMessage);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }
  // 解析json数据
  int feed = doc["feed"];
  Serial.println(feed);


  //当json数据满足一定条件时，舵机开始转动
  // 转动到指定位置时，
  // 运行发送停止信号函数
  // MQTT_response2() 
}





// // 发送信息给nano，
// void MQTT_response2() {

//   // 构造 JSON 响应
//   JsonDocument responseDoc;
//   responseDoc["a"] = b;

//   // 序列化响应消息
//   String responseMessage;
//   serializeJson(responseDoc, responseMessage);



//   String responseTopic = post_properties
//   if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
//     Serial.println("Response sent success");
//   } else {
//     Serial.println("Error sending response");
//   }
// }


// 回应
void MQTT_response3(String requestId) {

  // 构造 JSON 响应
  JsonDocument responseDoc;
  responseDoc["state"] = 1;

  // 序列化响应消息
  String responseMessage;
  serializeJson(responseDoc, responseMessage);
  // 更新响应的主题，包含 request_id


  String responseTopic = half_response_command + requestId;
  if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent success");
  } else {
    Serial.println("Error sending response");
  }
}




// MQTT 重新连接函数
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
    // 添加超时机制
    if (attempt > 30) {
      Serial.println("Reconnect timeout, restarting...");
      ESP.restart();
    }
  }
}


void setup() {
  Serial.begin(115200);//初始化串口
  MQTT_Init();
  // 初始化引脚
  // pinMode(PUL_PIN, OUTPUT);
  // pinMode(DIR_PIN, OUTPUT);
}

void loop() {
  if (!client.connected()) {
  reconnect();
  }
  client.loop();
}
  // 正转
//   digitalWrite(DIR_PIN, HIGH);  // 设置方向为正转
//   generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
//   delay(DELAY_BETWEEN_MOVES);  // 延时

//   // 反转
//   digitalWrite(DIR_PIN, LOW);  // 设置方向为反转
//   generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
//   delay(DELAY_BETWEEN_MOVES);  // 延时
// }

// // 生成指定数量的脉冲
// void generatePulses(long pulseCount, int pulseDelay) {
//   for (long i = 0; i < pulseCount; i++) {
//     digitalWrite(PUL_PIN, HIGH);  // 发送 HIGH 脉冲
//     delayMicroseconds(pulseDelay);  // 延时
//     digitalWrite(PUL_PIN, LOW);  // 发送 LOW 脉冲
//     delayMicroseconds(pulseDelay);  // 延时
//   }
