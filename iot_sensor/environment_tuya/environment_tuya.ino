#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <SHA256.h>
#include <WiFiClientSecure.h>
#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>
#include <ArduinoJson.h>  // 用于构造 JSON 报文

#define SHA256HMAC_SIZE 32

// 气敏传感器引脚
#define MQ135D 25       //定义光敏传感器数字输出引脚
#define MQ135A 34       //定义光敏传感器模拟输出引脚

unsigned long lastReport = 0;
const unsigned long reportInterval = 5000;//上报延时5s

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

int currentFanPower = 0;              // 记录当前风机功率
int currentWaterCurtainPower = 0;    // 记录当前水帘功率

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
const char productId[] = "aic9x82esujsqpoy";
const char deviceId[] = "26ef7df505aeb2487fapyv";
const char deviceSecret[] = "UlANftUAXW1kmPOp";

char clientID[50] ;
char username[100];
char password[96] ;

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);
Adafruit_AHTX0 aht;
BMP280 bmp280;

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
/*
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // 将接收到的主题转换成字符串方便打印和判断
  String topicStr(topic);
  Serial.print("收到MQTT消息,主题: ");
  Serial.println(topicStr);

  // payload 是原始字节数组，不带 \0 结束符，需要拼接成 String
  String msg;
  for (unsigned int i = 0; i < length; ++i) msg += (char)payload[i];
  Serial.print("消息内容: ");
  Serial.println(msg);

  // 使用 ArduinoJson 解析 JSON 格式消息
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.print("JSON解析失败: ");
    Serial.println(err.c_str());
    return;  // JSON 格式不对，退出
  }

  if (doc.containsKey("data")) {
    int fanPower = doc["data"]["fan_power"] | -1;           // 提取风机功率
    int waterCurtainPower = doc["data"]["water_curtain_power"] | -1;  // 提取水帘功率

    if (fanPower >= 0) {
      Serial.printf("设置风机功率: %d%%\n", fanPower);
      setPower(FAN_CHANNEL, fanPower);
    }
    if (waterCurtainPower >= 0) {
      Serial.printf("设置水帘功率: %d%%\n", waterCurtainPower);
      setPower(WATER_CURTAIN_CHANNEL, waterCurtainPower);
    }
  }

}
*/
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

    if (data.containsKey("fan_power")) {
      currentFanPower = constrain((int)data["fan_power"], 0, 100);
      Serial.printf("设置风机功率: %d%%\n", currentFanPower);
      updated = true;
    }

    if (data.containsKey("water_curtain_power")) {
      currentWaterCurtainPower = constrain((int)data["water_curtain_power"], 0, 100);
      Serial.printf("设置水帘功率: %d%%\n", currentWaterCurtainPower);
      updated = true;
    }

    if (updated) {
      control(currentFanPower, currentWaterCurtainPower);
    }
  }
}

void control(int currentFanPower, int currentWaterCurtainPower) {
  currentFanPower = constrain(currentFanPower, 0, 100);
  currentWaterCurtainPower = constrain(currentWaterCurtainPower, 0, 100);
  int power = round(currentFanPower * 255.0 / 100.0);
  int power2 = round(currentWaterCurtainPower * 255.0 / 100.0);

  ledcWrite(FAN_PIN, power);  // 设置 PWM 占空比
  ledcWrite(PUMP_PIN, power2);  // 设置 PWM 占空比

  // 输出结果
  Serial.print("Fan Power: ");
  Serial.println(power);
  Serial.print("Water Curtain Power: ");
  Serial.println(power2);
}

/*
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
}
*/
void reportProperty(float temperature, float humidity, float NH3Value) {
  long timestamp = time(nullptr);  // UNIX时间戳
  char topic[64];
  snprintf(topic, sizeof(topic), "tylink/%s/thing/property/report", deviceId);

  // 生成消息体
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{"
      "\"msgId\":\"45lkj3551234002\","
      "\"time\":%ld,"
      "\"data\":{"
        "\"temperature\":{\"value\":%.2f,\"time\":%ld},"
        "\"humidity\":{\"value\":%.2f,\"time\":%ld},"
        "\"ammonia\":{\"value\":%.2f,\"time\":%ld}"
      "}"
    "}",
    timestamp,
    temperature, timestamp,
    humidity, timestamp,
    NH3Value, timestamp
    );

  mqtt_client.publish(topic, payload);
  Serial.print("Published topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);//初始化串口
  Serial.println("AHT20");
  connectToWiFi();
    syncTime();  // X.509 validation requires synchronization time
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTT();
  Wire.begin(); 
  bmp280.begin();  //初始化BMP280
  pinMode(MQ135D, INPUT);//定义GPIO15为输入模式
  pinMode(MQ135A, INPUT);//定义GPIO34为输入模式
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
  if (!mqtt_client.connected()) {
    connectToMQTT();
    }
  mqtt_client.loop();

  unsigned long now = millis();
  if (now - lastReport >= reportInterval) {
    lastReport = now;

    // 读取传感器数据并上报
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    float tempread = temp.temperature;
    float humiread = humidity.relative_humidity;
    float NH3Value = analogRead(MQ135A);

    Serial.print("Temperature: ");
    Serial.print(tempread);
    Serial.print(" °C, Humidity: ");
    Serial.print(humiread);
    Serial.print(" %, ammonia: ");
    Serial.println(NH3Value);

    reportProperty(tempread, humiread, NH3Value);
  }

  delay(500);  // 每0.5s轮询一次
  
}



