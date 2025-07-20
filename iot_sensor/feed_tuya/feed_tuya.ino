#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <SHA256.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>  // 用于构造 JSON 报文
#include <ESP32Servo.h>
#include "HX711.h"
#include "soc/rtc.h"
#include <HTTPClient.h>

#define SHA256HMAC_SIZE 32

unsigned long lastReport = 0;
const unsigned long reportInterval = 5000;//上报延时5s

// 硬件引脚定义
#define LOADCELL_DOUT_PIN 17
#define LOADCELL_SCK_PIN  16
#define CALIBRATION_FACTOR 719 //校准因子

// 系统参数
int State = 0;
float TARGET_WEIGHT = 0;  // 目标投喂量（克）
const int FEEDING_SPEED = 2000;     // 输料舵机全速运转角度 1500-2000 顺
const int STOP_ANGLE = 1500;         // 输料舵机停止角度
const int DUMP_ANGLE = 140;        // 翻斗倾倒角度
const int DUMP_ANGLE2 = 120;       //翻斗倾倒角度2
const int RETURN_ANGLE = 180;        // 翻斗复位角度
const int SETTLE_TIME = 2000;      // 料斗稳定时间(ms)

// 初始化参数
enum SystemState { IDLE, FEEDING, DUMPING };
SystemState currentState = IDLE;  //初始状态为闲置
unsigned long actionStartTime = 0;
Servo servo360;  // 输料舵机（360°连续旋转）
Servo servo180;  // 翻斗舵机（180°标准）
HX711 scale;  //HX711

// WiFi credentials
const char *wifi_ssid = "Creator_Space";             // Replace with your WiFi name
const char *wifi_password = "iloveSCU";   // Replace with your WiFi password

// MQTT Broker settings
const int mqtt_port = 8883;  // MQTT port (TLS)
const char *mqtt_broker = "m1.tuyacn.com";  // EMQX broker endpoint
const char *mqtt_topic = "tylink/%s/thing/model/get";     // MQTT topic

//需要使用到NTP时间服务获取unix时间戳用于连接MQTT使用
// NTP Server settings
const char *ntp_server = "pool.ntp.org";     // Default NTP server
// const char* ntp_server = "cn.pool.ntp.org"; // Recommended NTP server for users in China
const long gmt_offset_sec = 0;            // GMT offset in seconds (adjust for your time zone)
const int daylight_offset_sec = 0;        // Daylight saving time offset in seconds

//productId、deviceId、deviceSecret是涂鸦平台获取到授权码信息
const char productId[] = "g5yjslqwdzfb81pw";
const char deviceId[] = "26c4ef8f3b04520df9vuiw";
const char deviceSecret[] = "jL5SEEBRjk6wyhA4";

char clientID[50] ;
char username[100];
char password[96] ;

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);


// SSL certificate for MQTT broker
static const char ca_cert[]
{
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"
  "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"
  "EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"
  "ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"
  "NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"
  "EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"
  "AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"
  "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"
  "E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"
  "/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"
  "DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"
  "GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"
  "tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"
  "AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"
  "FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"
  "WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"
  "9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"
  "gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"
  "2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"
  "LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"
  "4uJEvlz36hz1\n"
  "-----END CERTIFICATE-----\n"
};

const char iot_dns_cert_der[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIICGDCCAb2gAwIBAgIRAI4kVSI/DR6TlRqvv0C7A4EwCgYIKoZIzj0EAwIwNTEdMBsGA1UECgwU\n"
    "U2luYmF5IEdyb3VwIExpbWl0ZWQxFDASBgNVBAMMC0Nsb3VkIFJDQSAyMCAXDTIyMDUzMTE2MDAw\n"
    "MFoYDzIwNzIwNjMwMTU1OTU5WjA1MR0wGwYDVQQKDBRTaW5iYXkgR3JvdXAgTGltaXRlZDEUMBIG\n"
    "A1UEAwwLQ2xvdWQgUkNBIDIwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATqjfuzyXh8P0MuuWrH\n"
    "PUSoOp9OqsSHnCvDL18EK/Wfo1MOaQoIAy82zaC+ggjQph0AwCICTfzauMr0AUKw28Vko4GrMIGo\n"
    "MA4GA1UdDwEB/wQEAwIBBjBFBgNVHSUEPjA8BggrBgEFBQcDAQYIKwYBBQUHAwIGCCsGAQUFBwMD\n"
    "BggrBgEFBQcDCAYIKwYBBQUHAwQGCCsGAQUFBwMJMA8GA1UdEwQIMAYBAf8CAQEwHwYDVR0jBBgw\n"
    "FoAUjW5pdbOF5Bmvn+MrD+yG6tcJ7yowHQYDVR0OBBYEFI1uaXWzheQZr5/jKw/shurXCe8qMAoG\n"
    "CCqGSM49BAMCA0kAMEYCIQDaNnFTr66LnhYY+55C234I7MWBveU3RLg5pcVzb5EYUAIhAJN4+4go\n"
    "F3rrb03/o2AsmPMLLZ+UjTjeCXrTXUyxBt2N\n"
    "-----END CERTIFICATE-----\n"};


static String hmac256(const String& signcontent, const String& ds) {
  byte hashCode[SHA256HMAC_SIZE];
  SHA256 sha256;

  const char* key = ds.c_str();
  size_t keySize = ds.length();

  sha256.resetHMAC(key, keySize);
  sha256.update((const byte*)signcontent.c_str(), signcontent.length());
  sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

  String sign = "";
  for (byte i = 0; i < SHA256HMAC_SIZE; ++i) {
    sprintf(password + 2 * i, "%02x", hashCode[i]);
    //下面两行功能和sprintf功能一样，只不过是string类型存储的
    sign += "0123456789ABCDEF"[hashCode[i] >> 4];
    sign += "0123456789ABCDEF"[hashCode[i] & 0xf];
  }

  return sign;
}

static int tuya_mqtt_auth_signature_calculate(const char* deviceId, const char* deviceSecret) {
  if (NULL == deviceId || NULL == deviceSecret) {
    return -1;
  }

  uint32_t timestamp = time(nullptr);

  /* client ID */
  sprintf(username, "%s|signMethod=hmacSha256,timestamp=%d,secureMode=1,accessType=1", deviceId, timestamp);
  Serial.print("username:");
  Serial.println(username);

  /* username */
  sprintf(clientID, "tuyalink_%s", deviceId);
  Serial.print("clientID:");
  Serial.println(clientID);

  /* password */
  int i = 0;
  char passward_stuff[255];
  size_t slen = sprintf(passward_stuff, "deviceId=%s,timestamp=%d,secureMode=1,accessType=1", deviceId, timestamp);
  hmac256(passward_stuff, deviceSecret);

  Serial.print("password:");
  Serial.println(password);

  return 0;
}


void connectToWiFi() {
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.println("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi Success !");
}

void syncTime() {
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    Serial.print("Waiting for NTP time sync: ");
    while (time(nullptr) < 8 * 3600 * 2) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Time synchronized");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Current time: ");
        Serial.println(asctime(&timeinfo));
    } else {
        Serial.println("Failed to obtain local time");
    }
}
void connectToMQTT() {

   
   espClient.setCACert(ca_cert);
 
    // espClient.setFingerprint(fingerprint);
    while (!mqtt_client.connected()) {
        tuya_mqtt_auth_signature_calculate(deviceId, deviceSecret);
        Serial.printf("Connecting to MQTT Broker as %s.....\n", clientID);
        if (mqtt_client.connect(clientID,  username, password)) {
            Serial.println("Connected to MQTT broker");
            char auto_subscribe_topic[64];
            sprintf(auto_subscribe_topic, "tylink/%s/channel/downlink/auto_subscribe", deviceId);            
            mqtt_client.subscribe(auto_subscribe_topic);
            sprintf(auto_subscribe_topic,mqtt_topic,deviceId);
            // Publish message upon successful connection
            mqtt_client.publish(auto_subscribe_topic, "{\"data\":{\"format\":\"simple\"}}");
        } else {
            char err_buf[128];
            //espClient.getLastSSLError(err_buf, sizeof(err_buf));
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.println(mqtt_client.state());
            Serial.print("SSL error: ");
            Serial.println(err_buf);
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String topicStr = String(topic);

  // 判断是否是属性下发主题
  if (topicStr.endsWith("/thing/property/set")) {
    Serial.println("收到属性下发消息");

    String message;
    for (unsigned int i = 0; i < length; ++i) message += (char)payload[i];
    Serial.print("Payload: ");
    Serial.println(message);

    StaticJsonDocument<512> doc;
    auto err = deserializeJson(doc, message);
    if (err) {
      Serial.print("JSON解析失败: ");
      Serial.println(err.c_str());
      return;
    }
    JsonObject data = doc["data"];
// 只更新收到的字段
    bool updated = false;

    if (data.containsKey("weight")) {
      TARGET_WEIGHT = data["weight"];
      Serial.printf("收到饲料量: %d\n", TARGET_WEIGHT);
      updated = true;
    }

    if (updated) {
      State = 1;
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

// 如果state为1，则执行一次饲料投喂指令，然后修改state=0，调用get_w函数让小车继续运动
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
          if(elapsed > 500) {  //时间差大于稳定间隔
            servo180.write(DUMP_ANGLE2);  //90度位
            actionStartTime = millis();  //更新抖动时刻
            phase = 2;
          }
          break;
          
        case 2: // 再次倾倒
          if(elapsed > 500) {  //时间差大于稳定间隔
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
            report(); //上报后端
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

// 上报后端
void report() {
    // 构造 JSON
    DynamicJsonDocument responseDoc(256);
    JsonArray services = responseDoc.createNestedArray("services");
    JsonObject service = services.createNestedObject();
    service["service_id"] = "feed";
    JsonObject properties = service.createNestedObject("properties");
    properties["state"] = 1;

    // 序列化 JSON
    String responseMessage;
    serializeJson(responseDoc, responseMessage);

    // HTTP 发送
    HTTPClient http;
    String url = "http://116.63.168.84:9090/rabbit/touwei_done_tuya"; // 接口
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(responseMessage); // 发送 JSON
    if (httpCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        String payload = http.getString();
        Serial.print("HTTP Response payload: ");
        Serial.println(payload);
    } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpCode);
    }

    http.end(); // 释放资源
}

void setup() {
  Serial.begin(115200);//初始化串口
  connectToWiFi();
    syncTime();  // X.509 validation requires synchronization time
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTT();

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
  if (!mqtt_client.connected()) {
    connectToMQTT();
    }
  mqtt_client.loop();

  if(State == 1){
    feed();
  }  
  //delay(500);  // 每0.5s轮询一次
  
}



