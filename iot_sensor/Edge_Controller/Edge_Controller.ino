#include <WiFi.h>         // 用于连接 WiFi
#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文

// 定义LED和按钮的引脚
const int ledPins[] = {2, 3, 4, 5};        // LED连接的GPIO引脚
const int buttonPins[] = {6, 7, 8, 9};     // 按钮连接的GPIO引脚

// 状态控制变量
int state = 0;  //云端接收值：不执行
int buttonPressed[4] = {0}; // 记录按钮按压状态
int prevButtonStates[4] = {LOW}; // 按钮前一次状态记录，滤除抖动/长按
int response = 0;  //返回云端值：未执行完成

/*
void MQTT(){
  //需要时，让state = 1
}
*/

// WiFi 和 MQTT 连接信息
const char* ssid = "SCU_Makers";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_leds_0_0_2025022321";
const char* mqttUser = "67b683d83f28ab3d0384f27e_leds";
const char* mqttPassword = "17a85498dc8339943237186c61e7aa2861be33405971bd0eab52d080f762ae92";

String half_get_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/request_id=");
String half_response_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/response/request_id=");
String get_messages = String("$oc/devices/") + mqttUser + String("/sys/messages/down");
String post_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/report");

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
      // get_tha(requestId);
    }
  }

  // 下发设备消息
  else if (topicStr == get_messages) {
    String type = doc["content"]["type"];
    if(type == "post_l"){
       post_l(doc);
    }
  }
}


void post_l(JsonDocument doc) {
   int led = doc["content"]["led"];
   if(led == 1){
    state = 1;
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

void illume(){
  //云端指令为1时
  if (state == 1) {
    // 点亮所有LED
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], HIGH);
    }
    
    // 重置所有按钮状态
    memset(buttonPressed, 0, sizeof(buttonPressed));
    state = 2;  // 清除指令标记
  }
}

void bottom(){
  if(state == 2){
    
    int allPressed = 1; // 假设按钮全部按下

    for (int i = 0; i < 4; i++) {
    int currentState = digitalRead(buttonPins[i]);  //读取currentState

    if (currentState == HIGH && prevButtonStates[i] == LOW) {  //检测到按下动作 且 上一次按钮状态为未按下
      if (buttonPressed[i] == 0) {  //如果 按钮是否按下 为否
        digitalWrite(ledPins[i], LOW);  // 熄灭对应LED
        buttonPressed[i] = 1;        // 标记按钮已按下
      }
    }
    prevButtonStates[i] = currentState; // 保存当前状态
    
    // 检查是否全部按钮已操作
    if (buttonPressed[i] == 0) {  //如果 按钮是否按下 为否
      allPressed = 0;  //则按钮未全部按下
    }
  }

  
  if (allPressed) {  // 全部按钮操按下后
   state = 0;  //state置0，等待下一次云端任务
   response = 1;  //返回云端值：执行完成
  }
  }
}

void get_l() {
  JsonDocument responseDoc;
  JsonArray services = responseDoc.createNestedArray("services");
  JsonObject service = services.createNestedObject();
  service["service_id"] = "led";
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

void setup() {
  // 初始化LED引脚为输出模式
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);  // 初始状态熄灭
  }
  
  // 初始化按钮引脚为输入模式
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT);  // 按下高电平，释放低电平
  }
}


void loop() {
  illume();
  bottom();
  if(response==1){
    get_l();
    response = 0;
  }
  
  delay(20); // 消抖延时兼功耗控制
}