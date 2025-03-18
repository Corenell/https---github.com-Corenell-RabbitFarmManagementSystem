package com.iot.mqtt.example.demo;

public interface MqttConstants {
    /**
     * MQTT接入域名
     *
     * <a href="https://support.huaweicloud.com/usermanual-iothub/iot_01_00113.html">中国站配置说明</a>
     * <a href="https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00113.html">国际站配置说明</a>
     */
    String HOST = "${HOST}";

    /**
     * MQTT接入端口
     *
     * <a href="https://support.huaweicloud.com/usermanual-iothub/iot_01_00113.html">中国站配置说明</a>
     * <a href="https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00113.html">国际站配置说明</a>
     */
    int PORT = 8883;

    /**
     * 接入凭证键值
     *
     * <a href="https://support.huaweicloud.com/usermanual-iothub/iot_01_00113.html">中国站配置说明</a>
     * <a href="https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00113.html">国际站配置说明</a>
     */
    String ACCESS_KEY = "${ACCESS_KEY}";

    /**
     * 接入凭证密钥
     *
     * <a href="https://support.huaweicloud.com/usermanual-iothub/iot_01_00113.html">中国站配置说明</a>
     * <a href="https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00113.html">国际站配置说明</a>
     */
    String ACCESS_CODE = "${ACCESS_CODE}";

    /**
     * 非必填参数，当同一region购买多个标准版实例该参数必填
     *
     * <a href="https://support.huaweicloud.com/usermanual-iothub/iot_01_00113.html">中国站配置说明</a>
     * <a href="https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00113.html">国际站配置说明</a>
     */
    String INSTANCE_ID = "";

    /**
     * 接收数据的Topic，替换成"创建规则动作"中的Topic
     */
    String SUBSCRIBE_TOPIC = "${SUBSCRIBE_TOPIC}";
}
