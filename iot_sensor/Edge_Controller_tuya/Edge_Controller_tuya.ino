#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <SHA256.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>  // 用于构造 JSON 报文
#include <HTTPClient.h>

#define SHA256HMAC_SIZE 32

unsigned long lastPublishTime = 0;

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
int ledStates[4] = {0, 0, 0, 0};

// WiFi credentials
const char *wifi_ssid = "Money";             // Replace with your WiFi name
const char *wifi_password = "050506qdd";   // Replace with your WiFi password

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
const char productId[] = "aqq8eliw0xqdrooa";
const char deviceId[] = "26ef769663dce48dd5dzj2";
const char deviceSecret[] = "SWggF1z4IbBUhqoY";

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

// 控制 LED 颜色函数
void color(int redPin, int greenPin, int bluePin, int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
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

    for (int i = 0; i < 4; ++i) {
      String key = "led" + String(i + 1);
      if (data.containsKey(key)) {
        ledStates[i] = data[key].as<int>();
        updated = true;
        Serial.printf("更新 led%d -> %d\n", i + 1, ledStates[i]);
      }
    }

    if (updated) {
      control();
    }
  }
}

void control() {
  for (int i = 0; i < 4; i++) {
    switch (ledStates[i]) {
      case 1:
        color(led[i][0], led[i][1], led[i][2], 255, 0, 0); // 蓝色
        Serial.printf("LED%d: 蓝色\n", i + 1);
        break;
      case 2:
        color(led[i][0], led[i][1], led[i][2], 0, 255, 0); // 绿色
        Serial.printf("LED%d: 绿色\n", i + 1);
        break;
      case 3:
        color(led[i][0], led[i][1], led[i][2], 0, 0, 255); // 红色
        Serial.printf("LED%d: 红色\n", i + 1);
        break;
      case 4:
        color(led[i][0], led[i][1], led[i][2], 255, 255, 0); // 青色
        Serial.printf("LED%d: 青色\n", i + 1);
        break;
      default:
        color(led[i][0], led[i][1], led[i][2], 0, 0, 0); // 熄灭
        Serial.printf("LED%d: 熄灭\n", i + 1);
        break;
    }
  }

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
        ledStates[i] = 0;  // 同步逻辑状态
        //color(led[i][0], led[i][1], led[i][2], 0, 0, 0);
        buttonPressed[i] = 1;
        publishNeeded = true;
        control();
      }
    }
    prevButtonStates[i] = currentState;
    if (buttonPressed[i] == 0) { // 只要有任一灯未熄灭，则allPressed为0
      allPressed = 0;
    }
  }

  // 如果有新的按下事件且距离上次上报超过500ms，则上报按钮状态
  if (publishNeeded && (millis() - lastPublishTime > 500)) {
    report(buttonPressed);
    lastPublishTime = millis();
  }
 
  // 如果全部按钮均已上报熄灭，则退出按钮监测阶段
  if (allPressed) {
    state = 0;
    Serial.println("所有按钮均按下，退出按钮检测");
  }
}

// 上报按钮状态给云端
void report(int buttonPressed[4]) {
    // 构造 JSON
    DynamicJsonDocument responseDoc(256);
    JsonArray services = responseDoc.createNestedArray("services");
    JsonObject service = services.createNestedObject();
    service["service_id"] = "led";
    JsonObject properties = service.createNestedObject("properties");

    for (int i = 0; i < 4; i++) {
        String key = "led" + String(i + 1);
        properties[key] = buttonPressed[i]; // 0=亮, 1=灭
    }

    // 序列化 JSON
    String responseMessage;
    serializeJson(responseDoc, responseMessage);

    Serial.print("上报数据: ");
    Serial.println(responseMessage);

    // HTTP 发送
    HTTPClient http;
    String url = "http://<服务器地址>/api/report"; //填写实际 HTTP 接口
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(responseMessage); // 发送 JSON
    if (httpCode > 0) {
        Serial.print("HTTP 响应码: ");
        Serial.println(httpCode);
        String payload = http.getString();
        Serial.print("HTTP 响应内容: ");
        Serial.println(payload);
    } else {
        Serial.print("HTTP 错误码: ");
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
  if (!mqtt_client.connected()) {
    connectToMQTT();
    }
  mqtt_client.loop();

  // 当处于按钮操作阶段时检测按钮状态
  if (state == 2) {
    bottom();
  }

  delay(20); // 消抖延时及降低功耗
  
}



