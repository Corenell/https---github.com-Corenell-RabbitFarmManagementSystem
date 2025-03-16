package com.iot.mqtt.example.client;

import com.iot.mqtt.example.client.listener.ActionListener;
import com.iot.mqtt.example.client.listener.ConnectActionListener;
import com.iot.mqtt.example.client.listener.ConnectListener;
import com.iot.mqtt.example.client.listener.RawMessageListener;

public interface IMqttClient {
    /**
     * 建立连接
     *
     * @return 连接建立结果，0表示成功，其他表示失败
     */
    int connect();

    /**
     * 关闭连接
     */
    void close();

    /**
     * 是否连接中
     *
     * @return true表示在连接中，false表示断连
     */
    boolean isConnected();

    /**
     * 设置链路监听器
     *
     * @param connectListener 链路监听器
     */
    void setConnectListener(ConnectListener connectListener);

    /**
     * @param topic          订阅自定义topic。注意SDK会自动订阅平台定义的topic
     * @param actionListener 监听订阅是否成功
     */
    void subscribeTopic(String topic, ActionListener actionListener);

    /**
     * 设置连接动作监听器
     *
     * @param connectActionListener 连接动作监听器
     */
    void setConnectActionListener(ConnectActionListener connectActionListener);

    /**
     * 设置消息处理监听器
     *
     * @param rawMessageListener 消息处理监听器
     */
    void setRawMessageListener(RawMessageListener rawMessageListener) ;
}
