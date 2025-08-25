#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <SHA256.h>
#include <WiFiClientSecure.h>
#include <Adafruit_AHTX0.h>// 引用库
#include <BMP280.h>

#define SHA256HMAC_SIZE 32

// WiFi credentials
const char *wifi_ssid = "zeyun";             // Replace with your WiFi name
const char *wifi_password = "11235813";   // Replace with your WiFi password

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

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
}

void reportProperty(float temperature, float humidity) {
  long timestamp = time(nullptr);  // 推荐用UNIX时间戳
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
        "\"humidity\":{\"value\":%.2f,\"time\":%ld}"
      "}"
    "}",
    timestamp,
    temperature, timestamp,
    humidity, timestamp);

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
  // 读取传感器数据并上报
  sensors_event_t humidity, temp;  //AHT20填充温湿度
  aht.getEvent(&humidity, &temp);
  float tempread = temp.temperature;
  float humiread = humidity.relative_humidity;

  Serial.print("Temperature: ");
  Serial.print(tempread);
  Serial.print(" °C, Humidity: ");
  Serial.print(humiread);
  Serial.println(" %");

  reportProperty(tempread, humiread);

  delay(1000);  // 每分钟上报一次

  
}



