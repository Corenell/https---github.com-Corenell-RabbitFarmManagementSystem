#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文

// 引脚定义
const int PUL_PIN = 16;  // 脉冲信号引脚（PUL）
const int DIR_PIN = 17;  // 方向信号引脚（DIR）
const int EN_PIN = 4;    //使能引脚

// 参数定义
const long PULSE_COUNT = 5000;  // 脉冲数量（控制电机旋转角度）
const int PULSE_DELAY = 80;       // 脉冲周期（微秒，控制电机转速）
//const int DELAY_BETWEEN_MOVES = 1000;  // 正反转之间的延时（毫秒#include <WiFi.h>         // 用于连接 WiFi


// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_motor_0_0_2025022306";
const char* mqttUser = "67b683d83f28ab3d0384f27e_motor";
const char* mqttPassword = "17a85498dc8339943237186c61e7aa2861be33405971bd0eab52d080f762ae92";


// ssss
String half_get_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/request_id=");
String half_response_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/response/request_id=");
String half_get_command = String("$oc/devices/") + mqttUser + String("/sys/commands/request_id=");
String half_response_command = String("$oc/devices/") + mqttUser + String("/sys/commands/response/request_id=");
String get_messages = String("$oc/devices/") + mqttUser + String("/sys/messages/down");
String post_properties = String("$oc/devices/") + mqttUser + String("/properties/report");



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


void get_s() {
  JsonDocument responseDoc;
  JsonArray services = responseDoc.createNestedArray("services");
  JsonObject service = services.createNestedObject();
  service["service_id"] = "stop";
  JsonObject properties = service.createNestedObject("properties");
  properties["state"] = 1;
  String responseMessage;
  serializeJson(responseDoc, responseMessage);
  String responseTopic = post_properties;
  if (client.publish(responseTopic.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent success");
  } else {
    Serial.println("Error sending response");
  }
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
    }
  }

  // 下发设备命令
  else if (topicStr.startsWith(half_get_command)) {
    String requestId = topicStr.substring(half_get_command.length());
    String type = doc["command_name"];
    if(type == "state"){
      response_sf(doc,requestId);
    }
  
  }
  //下发设备消息
  else if (topicStr == get_messages) {
    String type = doc["content"]["type"];
    if(type == "post_f"){
      post_f(doc);
    }
  
  }
}

//接收开始投喂指令
void post_f(JsonDocument doc) {

  int feed = doc["content"]["feed"];
  if(feed == 1){
    Moving();
  }
}

void Moving(){
  // 正转（远离电机）
  digitalWrite(DIR_PIN, HIGH);  // 设置方向为正转
  generatePulses(PULSE_COUNT, PULSE_DELAY);  // 生成脉冲
  Serial.println("move");
  get_s();
  // 上云传函数
}
void generatePulses(long pulseCount, int pulseDelay) {
  for (long i = 0; i < pulseCount; i++) {
    digitalWrite(PUL_PIN, HIGH);  // 发送 HIGH 脉冲
    delayMicroseconds(pulseDelay);  // 延时
    digitalWrite(PUL_PIN, LOW);  // 发送 LOW 脉冲
    delayMicroseconds(pulseDelay);  // 延时

  }
}





void response_sf(JsonDocument doc,String requestId) {
  int feed = doc["paras"]["feed"];
  if(feed == 1){
    Moving();
  }




  JsonDocument responseDoc;
  responseDoc["state"] = 1;
  String responseMessage;
  serializeJson(responseDoc, responseMessage);
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
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN,LOW);
}

void loop() {
  if (!client.connected()) {
  reconnect();
  }
  client.loop();
}

