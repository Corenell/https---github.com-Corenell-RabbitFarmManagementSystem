package com.iot.mqtt.example.demo;

import static com.iot.mqtt.example.demo.MqttConstants.ACCESS_CODE;
import static com.iot.mqtt.example.demo.MqttConstants.ACCESS_KEY;
import static com.iot.mqtt.example.demo.MqttConstants.HOST;
import static com.iot.mqtt.example.demo.MqttConstants.INSTANCE_ID;
import static com.iot.mqtt.example.demo.MqttConstants.PORT;
import static com.iot.mqtt.example.demo.MqttConstants.SUBSCRIBE_TOPIC;

import com.iot.mqtt.example.client.IMqttClient;
import com.iot.mqtt.example.client.MqttClient;
import com.iot.mqtt.example.client.MqttClientOptions;

import lombok.extern.slf4j.Slf4j;

@Slf4j
public class MqttTlsDemo {
    public static void main(String[] args) {
        IMqttClient mqttClient = new MqttClient(MqttClientOptions.builder()
            .host(HOST)
            .port(PORT)
            .accessKey(ACCESS_KEY)
            .accessCode(ACCESS_CODE)
            .instanceId(INSTANCE_ID)
            .trustAll(false)
            // 可替换成对应实例的证书，默认证书下载地址（https://support.huaweicloud.com/devg-iothub/iot_02_1004.html#section3）
            // 可在linux机器上按照下列方法将pem证书格式转成jks格式
            // pem -> der  openssl x509 -outform der -in cert.pem -out cert.der
            // der -> jks  keytool -import -keystore cert.jks -file cert.der
            .jksSourceRootPath("DigiCertGlobalRootCA.jks")
            .jksPassword(null)
            .build());
        mqttClient.setRawMessageListener(message -> {
            // handler subscribe msg
            log.info("begin to handler msg. topic = {}, payload = {}", message.getTopic(),
                new String(message.getPayload()));
        });
        mqttClient.connect();
        mqttClient.subscribeTopic(SUBSCRIBE_TOPIC, null);
    }
}
