package com.iot.mqtt.example.client;

import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class MqttClientOptions {
    private String host;
    @Builder.Default
    private int port = 8883;
    private String accessKey;
    private String accessCode;
    private String instanceId;
    /**
     * 是否忽略证书校验
     */
    @Builder.Default
    private boolean trustAll = true;
    /**
     * jks证书存放的相对路径
     */
    private String jksSourceRootPath;
    /**
     * jks证书密码
     */
    private char[] jksPassword;
}
