#include <ESP32Servo.h>
#include "HX711.h"
#include "soc/rtc.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 硬件引脚定义
#define LOADCELL_DOUT_PIN 17
#define LOADCELL_SCK_PIN  16
#define CALIBRATION_FACTOR 719 //校准因子


// WiFi 和 MQTT 连接信息
const char* ssid = "Creator_Space";
const char* password = "iloveSCU";
const char* mqttServer = "ef861ca468.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
const char* clientId = "67b683d83f28ab3d0384f27e_feedcar_0_0_2025022308";
const char* mqttUser = "67b683d83f28ab3d0384f27e_feedcar";
const char* mqttPassword = "98c142b691bea4b4265fc308fcea02d847d0d2d8b667b7ba1d18caadc834a825";

String half_get_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/request_id=");
String half_response_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/get/response/request_id=");
String get_messages = String("$oc/devices/") + mqttUser + String("/sys/messages/down");
String post_properties = String("$oc/devices/") + mqttUser + String("/sys/properties/report");

// 系统参数
int State = 0;
float TARGET_WEIGHT = 0;  // 目标投喂量（克）
const int FEEDING_SPEED = 2000;     // 输料舵机全速运转角度 1500-2000 顺
const int STOP_ANGLE = 1500;         // 输料舵机停止角度
const int DUMP_ANGLE = 1;        // 翻斗倾倒角度
const int DUMP_ANGLE2 = 30;       //翻斗倾倒角度2
const int RETURN_ANGLE = 120;        // 翻斗复位角度
const int SETTLE_TIME = 10000;      // 料斗稳定时间(ms)

enum SystemState { IDLE, FEEDING, DUMPING };
SystemState currentState = IDLE;  //初始状态为闲置
unsigned long actionStartTime = 0;
Servo servo360;  // 输料舵机（360°连续旋转）
Servo servo180;  // 翻斗舵机（180°标准）
HX711 scale;  //HX711
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


// 根据华为云传来的数据调用不同函数

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

  // 下发设备消息
  else if (topicStr == get_messages) {
    String type = doc["content"]["type"];
    if(type == "post_w"){
       post_w(doc);
    }
  
  }
}
// 接收华为云传来的预期投喂量，修改state为1
void post_w(JsonDocument  doc) {
  TARGET_WEIGHT = doc["content"]["weight"];
  if(TARGET_WEIGHT>0){
  State = 1;
  }
}

// 投喂完成函数，给esp32小车发送开始运动指令
void get_w() {
  JsonDocument responseDoc;
  JsonArray services = responseDoc.createNestedArray("services");
  JsonObject service = services.createNestedObject();
  service["service_id"] = "feed";
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

// 移动平均滤波（称重稳定性较好，暂时关闭滤波）
float filteredWeight() {
  /*
  static float buffer[5];
  static byte index = 0;
  
  buffer[index] = scale.get_units();
  index = (index + 1) % 5;
  
  float sum = 0;
  for(byte i=0; i<5; i++) sum += buffer[i];
  return sum / 5;
  */
  float num = scale.get_units();
  return num;
}

// 如果state为1，则执行一次饲料投喂指令，修改state=o，调用MQTT_response2函数
void feed(){

  float currentWeight = filteredWeight();  //获取当前料斗上的饲料重量


  switch(currentState) {
    case IDLE:  //闲置
      if(currentWeight <= TARGET_WEIGHT) {  //是否达到目标量
        currentState = FEEDING;  //切换为FEEDING状态
        servo360.writeMicroseconds(FEEDING_SPEED);  // 启动输料
        Serial.println("开始输料...");
      }
      break;

    case FEEDING:  //喂料
      if(currentWeight >= TARGET_WEIGHT) {  //是否达到目标量
        servo360.writeMicroseconds(STOP_ANGLE);  // 停止输料
        currentState = DUMPING;  //切换为倒料状态
        actionStartTime = millis();     // 记录饲料称重完成的时刻
        Serial.println("达到目标重量，准备倾倒");
      }
      break;

    case DUMPING: {  //倾倒
      static uint8_t phase = 0;  //初始状态0
      unsigned long elapsed = millis() - actionStartTime;  //计算动作时刻到目前的时间差

      switch(phase) {
        case 0: //执行倾倒
          if(elapsed > SETTLE_TIME) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE);  //倾倒
            actionStartTime = millis();  //更新倾倒时刻
            phase = 1;  
          }
          break;
          
        case 1: // 抖动倾倒
          if(elapsed > 1000) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE2);  //90度位
            actionStartTime = millis();  //更新抖动时刻
            phase = 2;
          }
          break;
          
        case 2: // 再次倾倒
          if(elapsed > 1000) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE);  //再次倾倒
            actionStartTime = millis();  //更新倾倒时刻
            phase = 3;
          }
          break;

        case 3: // 保持倾倒
          if(elapsed > 3000) {  //时间差大于稳定间隔
            servo180.write(RETURN_ANGLE); //料斗复位
            actionStartTime = millis();  //更新动作时间
            phase = 4;
          }
          break;
          
        case 4: // 复位完成
          if(elapsed > 2000) {  //时间差大于稳定间隔
            scale.tare();  //称 清零
            currentState = IDLE;  //切换为闲置状态
            phase = 0;  //切换状态0
            Serial.println("完成投喂循环");
            State = 0;  //完成投喂
            get_w();
          }
          break;
      }
      break;
    }
  }
  
  // 调试输出
  static unsigned long lastPrint = 0;  //上次输出时刻
  if(millis() - lastPrint > 1000) {  //间隔1s
    Serial.print("当前状态: ");
    Serial.print(State);
    Serial.print(" | 流程: ");
    Serial.print(currentState);
    Serial.print(" | 重量: ");
    Serial.print(currentWeight);
    Serial.println("g");
    Serial.print("目标重量: ");
    Serial.print(TARGET_WEIGHT);
    Serial.println("g");
    lastPrint = millis();  //更新输出时刻
  }
  

}
  
void setup() {
  Serial.begin(115200);
  MQTT_Init();
  Serial.println("正在初始化");
  setCpuFrequencyMhz(80);  //HX711需要esp32工作在80MHz
  
  // 初始化舵机
  servo360.attach(25);  // 输料螺杆舵机（连续旋转）
  servo180.attach(26);  // 翻斗舵机
  servo360.write(STOP_ANGLE);  // 初始停止状态1500
  servo180.write(RETURN_ANGLE);  //初始水平状态120
  Serial.println("Servo Ready!");

  // 初始化称重模块
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);  //设置校准因子
  scale.tare();  //称 清零
  Serial.println("HX711 Ready!");

}


void loop() {
  if (!client.connected()) {
  reconnect();
  }
  client.loop();
  if(State == 1){
    feed();
  }

}


















