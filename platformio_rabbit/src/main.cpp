#include <WiFi.h>         // 用于连接 WiFiWROOM#include <PubSubClient.h> // 用于 MQTT 连接和通信
#include <ArduinoJson.h>  // 用于构造 JSON 报文
#include <Pubsubclient.h>

// 定义引脚
int led[4][3] = {
  {22, 23, 13},
  {18, 19, 21},
  {12, 27, 26},
  {4, 16, 17}
}; // LED引脚

const int buttonPins[] = {39,34,36,35}; // 按钮连接的GPIO引脚

// 状态控制变量
int state = 0;                    // 云端接收值：0-空闲，2-等待按钮操作
int buttonPressed[4] = {0, 0, 0, 0};      // 记录按钮是否已上报（0:未上报/亮, 1:已上报/灭）
int prevButtonStates[4] = {LOW, LOW, LOW, LOW}; // 按钮前一次状态，用于滤除抖动/长按

// 全局变量，保存 LED 状态
int ledStates[4] = {0, 0, 0, 0};  

// WiFi 和 MQTT 连接信息
const char *ssid = "chenyu";
const char *password = "cy383245";
const char *mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char *clientId = "67b683d83f28ab3d0384f27e_leds_0_0_2025022411";
const char *mqttUser = "67b683d83f28ab3d0384f27e_leds";
const char *mqttPassword = "0b1ce8e1470edea7a7b3b1212847759bee669d6792b8de3e7efff3cf19a823a4";

String half_get_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/request_id=");
String half_response_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/response/request_id=");
String get_messages = String("$oc/devices/") + mqttUser + String("/sys/messages/down");
String post_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/report");

WiFiClient espClient;
PubSubClient client(espClient);

// 为避免重复发布，设置上次发布的时间（单位：毫秒）
unsigned long lastPublishTime = 0;

// 函数声明
void callback(char *topic, byte *message, unsigned int length);
void MQTT_Init();
void post_l(JsonDocument doc);
void bottom();
void get_l(int buttonPressed[4]);
void reconnect();

// 初始化 MQTT 连接
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
  client.setCallback(callback);
  
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
}

// MQTT 消息回调函数
void callback(char *topic, byte *message, unsigned int length) {
  Serial.print("topic: ");
  Serial.println(topic);
  String topicStr = String(topic);

  String receivedMessage = "";
  for (unsigned int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.print("message: ");
  Serial.println(receivedMessage);
  
  // 为 JSON 解析分配足够的缓冲区
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, receivedMessage);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  // 下发设备消息：收到 IoT 下发的亮灯指令
  if (topicStr == get_messages) {
    String type = doc["content"]["type"];
          Serial.println("ok0");
    if (type == "post_l") {
      post_l(doc);
      Serial.println("ok1");
    }
  }
}

// MQTT 重新连接函数（移除了超时重启逻辑，改为持续尝试）
void reconnect() {
  if (client.connected()) return; // 已连接则直接返回

  Serial.println("MQTT disconnected, attempting reconnect...");
  while (!client.connected()) {
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("Reconnected to MQTT");
    } else {
      Serial.print("Reconnect failed, state: ");
      Serial.println(client.state());
      delay(6000);
    }
  }
}

// 控制 LED 颜色函数
void color(int redPin, int greenPin, int bluePin, int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
}

// 接收亮灯指令后执行亮灯函数
void post_l(JsonDocument doc) {
  JsonObject content = doc["content"];

  // 如果 JSON 中有对应的键，才更新，否则保持之前的值
  if (content.containsKey("led1")) {
    ledStates[0] = content["led1"].as<int>();
  }
  if (content.containsKey("led2")) {
    ledStates[1] = content["led2"].as<int>();
  }
  if (content.containsKey("led3")) {
    ledStates[2] = content["led3"].as<int>();
  }
  if (content.containsKey("led4")) {
    ledStates[3] = content["led4"].as<int>();
  }


for (int i = 0; i < 4; i++) {
  // 根据不同状态控制 LED 颜色
  if (ledStates[i] == 1) {
    color(led[i][0], led[i][1], led[i][2], 255, 0, 0); // 红色
  } else if (ledStates[i] == 2) {
    color(led[i][0], led[i][1], led[i][2], 0, 255, 0); // 绿色
  } else if (ledStates[i] == 3) {
    color(led[i][0], led[i][1], led[i][2], 0, 0, 255); // 蓝色
  } else if (ledStates[i] == 4) {
    color(led[i][0], led[i][1], led[i][2], 255, 255, 0); // 黄色
  } else if (ledStates[i] == 5) {
    color(led[i][0], led[i][1], led[i][2], 0, 255, 255); // 青色
  } else if (ledStates[i] == 6) {
    color(led[i][0], led[i][1], led[i][2], 255, 0, 255); // 紫色
  } else if (ledStates[i] == 7) {
    color(led[i][0], led[i][1], led[i][2], 255, 255, 255); // 白色
  } else if (ledStates[i] == 8) {
    color(led[i][0], led[i][1], led[i][2], 255, 165, 0); // 橙色
  } else if (ledStates[i] == 0) {
    color(led[i][0], led[i][1], led[i][2], 0, 0, 0); // 关灯
  }

  Serial.printf("%d ok", ledStates[i]);
}
  // 重置按钮状态，进入按钮上报阶段
  memset(buttonPressed, 0, sizeof(buttonPressed));
  state = 2;
}

// 检测按钮状态并上报 LED 熄灭信息
// 修改思路：当有按钮按下时，设置按钮状态；每个循环周期内，如果检测到新的按下且距离上次上报超过阈值，则统一上报一次
void bottom() {
  int allPressed = 1; // 1代表所有灯都已熄灭，0代表还有灯亮
  bool publishNeeded = false;

  for (int i = 0; i < 4; i++) {
    int currentState = digitalRead(buttonPins[i]); // 读取当前按钮状态
    if (currentState == HIGH && prevButtonStates[i] == LOW) { // 检测到新按下动作
      if (buttonPressed[i] == 0) {
        // 关闭对应 LED（全关）
        color(led[i][0], led[i][1], led[i][2], 0, 0, 0);
        buttonPressed[i] = 1;
        publishNeeded = true;
      }
    }
    prevButtonStates[i] = currentState;
    if (buttonPressed[i] == 0) { // 只要有任一灯未熄灭，则allPressed为0
      allPressed = 0;
    }
  }

  // 如果有新的按下事件且距离上次上报超过500ms，则上报按钮状态
  if (publishNeeded && (millis() - lastPublishTime > 500)) {
    get_l(buttonPressed);
    lastPublishTime = millis();
  }
  
  // 如果全部按钮均已上报熄灭，则退出按钮监测阶段
  if (allPressed) {
    state = 0;
  }
}

// 上报按钮状态给云端
void get_l(int buttonPressed[4]) {
  DynamicJsonDocument responseDoc(256);
  JsonArray services = responseDoc.createNestedArray("services");
  JsonObject service = services.createNestedObject();
  service["service_id"] = "get_l";
  JsonObject properties = service.createNestedObject("properties");

  for (int i = 0; i < 4; i++) {
    String key = "led" + String(i + 1);
    properties[key] = buttonPressed[i]; // 0代表亮, 1代表灭
  }

  String responseMessage;
  serializeJson(responseDoc, responseMessage);
  Serial.println(responseMessage);
  if (client.publish(post_properties.c_str(), responseMessage.c_str())) {
    Serial.println("Response sent success");
  } else {
    Serial.println("Error sending response");
  }
}

void setup() {
  Serial.begin(115200); // 初始化串口
  MQTT_Init();

  // 设置 RGB LED 的引脚为输出模式
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
     pinMode(led[i][j], OUTPUT);
      Serial.printf("%d ", led[i][j]);  // 使用 Serial.printf 输出 LED 引脚号
      }
  }
for (int i = 0; i < 4; i++) {
  pinMode(buttonPins[i], INPUT);
}
Serial.println("response");
}

void loop() {
  // 保证 MQTT 连接稳定，不断检查并重连
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 当处于按钮操作阶段时检测按钮状态
  if (state == 2) {
    bottom();
  }
  delay(20); // 消抖延时及降低功耗
}
