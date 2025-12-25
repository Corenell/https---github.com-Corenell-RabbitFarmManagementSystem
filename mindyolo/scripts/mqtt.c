#include <ESP32Servo.h>
#include "HX711.h"
#include "soc/rtc.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 硬件引脚定义
#define LOADCELL_DOUT_PIN 17
#define LOADCELL_SCK_PIN  16
#define CALIBRATION_FACTOR 719.862 //校准因子


// WiFi 和 MQTT 连接信息
// WiFi和MQTT配置
const char* ssid = "LJXY"; // WiFi SSID
const char* password = "15181918836"; // WiFi密码
const char* mqttServer = "0e7519d568.iot-mqtts.cn-north-4.myhuaweicloud.com"; // MQTT服务器地址
const int mqttPort = 1883; // MQTT服务器端口
const char* clientId = "694a6484cfb5ee5b35742750_feed_0_0_2025122404"; // MQTT客户端ID
const char* mqttUser = "694a6484cfb5ee5b35742750_feed"; // MQTT用户名
const char* mqttPassword = "dd14b309f706b4a4c11582e31b7868ff7912d7558ae1e9f5776ff0899316610c"; // MQTT密码
const char* topic_properties_report = "$oc/devices/694a6484cfb5ee5b35742750_feed/sys/properties/report"; // 属性报告主题
const char* topic_command = "$oc/devices/694a6484cfb5ee5b35742750_feed/sys/commands/#"; // 命令接收主题
char* topic_Commands_Response = "$oc/devices/694a6484cfb5ee5b35742750_feed/sys/commands/response/request_id="; // 命令响应主题

// 系统参数
int State = 0,Control=-1;
float weight=0;
int pwm360=0,pwm180=0;
Servo servo360;  // 输料舵机（360°连续旋转）
Servo servo180;  // 翻斗舵机（180°标准）
HX711 scale;  //HX711
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


// 解析华为云平台下发的命令，并为相关数据赋值
void callback(char *topic, byte *payload, unsigned int length) {
  char *pstr = topic; // 指向topic字符串，提取request_id用

  // 串口打印出收到的平台消息或者命令
  Serial.println();
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); // 将收到消息的topic展示出来
  Serial.print("] ");
  Serial.println();

  payload[length] = '\0'; // 在收到的内容后面加上字符串结束符
  char strPayload[255] = {0};
  strcpy(strPayload, (const char*)payload);
  Serial.println((char *)payload); // 打印出收到的内容
  Serial.println(strPayload);

  // request_id解析部分
  char arr[100]; // 存放request_id
  int flag = 0;
  char *p = arr;
  while(*pstr) { // 以'='为标志，提取出request_id
    if(flag) *p ++ = *pstr;
    if(*pstr == '=') flag = 1;
    pstr++;
  }
  *p = '\0';
  Serial.println(arr);

  // 将命令响应topic与resquest_id结合起来
  char topicRes[200] = {0};
  strcat(topicRes, topic_Commands_Response);
  strcat(topicRes, arr);
  Serial.println(topicRes);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* service_id = doc["service_id"]; // 获取服务ID
  const char* command_name = doc["command_name"]; // 获取命令名称

  switch(command_name[0]) {
    case 'f':
      Control = 1;
      weight = doc["paras"]["weight"];
      break;
    case 'c':
      Control = 2;
      pwm360 = doc["paras"]["pwm360"];
      pwm180 = doc["paras"]["pwm180"];
      break;
  }  
  MQTT_response(topicRes); // 发送响应参数
  machine_control(Control); // 根据参数执行对应的动作
  Control = -1;
}

// 发送命令响应
void MQTT_response(char *topic) {
  String responsed;
  StaticJsonDocument<128> doc;
  JsonObject response = doc.createNestedObject("response");
  serializeJson(doc, responsed);
  client.publish(topic, responsed.c_str());
  Serial.println(responsed);
}

// 投喂饲料完成上报
void Done_inform() {
  String JSONmessageBuffer; // 定义字符串接收序列化好的JSON数据
  StaticJsonDocument<128> doc; // 增大文档大小以适应更大的数组

  JsonObject services_0 = doc["services"].createNestedObject(); // 创建嵌套对象
  services_0["service_id"] = "feed"; // 定义服务ID

  JsonObject services_0_properties = services_0.createNestedObject("properties");
  services_0_properties["state"] = 1; // 称重传感器采集时间长度

  serializeJson(doc, JSONmessageBuffer);
  Serial.println("Sending message to MQTT topic..");

  if (client.publish(topic_properties_report, JSONmessageBuffer.c_str()) == true) { // 使用c_str函数将string转换为char
    Serial.println(JSONmessageBuffer.c_str());
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
    MQTT_Init();
  }
}
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
void  machine_control(int Control)
{
  if(Control==1)
  {
    feed_control();
  }
  if(Control==2)
  {
    serve_control();
  }
} // 根据参数执行对应的动作

//舵机调试
void serve_control()
{
  servo360.write(pwm360);
  servo180.write(pwm180);
}
//喂料函数
void feed_control()
{
  Serial.print("需要投喂的饲料量是：");
  Serial.println(weight);
  servo360.write(180);
  int time=0;
  float W=0;
  scale.power_up();
  while(W<weight)
  {
    W=scale.get_units();
    Serial.println(W);
    time++;
    if(time>1000)
    {
      servo360.write(90);
      scale.power_down();
      return;
    }
    delay(100);
  }
  scale.power_down();
  servo360.write(90);
  delay(2000);
  Feed_openandclose(1);
  delay(5000);
  Feed_openandclose(0);
  Serial.println("正常投喂结束。");
  Done_inform();
}
// 饲料碗倒和关
void Feed_openandclose(bool choose)
{
  if(choose)
    servo180.write(180);//打开
  else
    servo180.write(0);//关闭
}

void setup() {
  Serial.begin(115200);
  MQTT_Init();
  Serial.println("正在初始化");
  setCpuFrequencyMhz(80);  //HX711需要esp32工作在80MHz
  
  // 初始化舵机
  servo360.attach(25);  // 输料螺杆舵机（连续旋转）
  servo180.attach(26);  // 翻斗舵机
  servo360.write(90);  // 初始停止状态1500
  servo180.write(0);  //初始水平状态120
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
}
