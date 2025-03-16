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
public class MqttDemo {
    public static void main(String[] args) {
        IMqttClient mqttClient = new MqttClient(MqttClientOptions.builder()
            .host(HOST)
            .port(PORT)
            .accessKey(ACCESS_KEY)
            .accessCode(ACCESS_CODE)
            .instanceId(INSTANCE_ID)
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
