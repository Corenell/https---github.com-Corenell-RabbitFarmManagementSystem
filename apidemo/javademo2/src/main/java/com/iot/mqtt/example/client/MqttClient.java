package com.iot.mqtt.example.client;

import com.iot.mqtt.example.client.listener.ActionListener;
import com.iot.mqtt.example.client.listener.ConnectActionListener;
import com.iot.mqtt.example.client.listener.ConnectListener;
import com.iot.mqtt.example.client.listener.RawMessageListener;

import org.apache.commons.lang3.StringUtils;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.io.InputStream;
import java.security.KeyStore;
import java.security.SecureRandom;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

import lombok.extern.slf4j.Slf4j;

@Slf4j
public class MqttClient implements IMqttClient {
    private static final int DEFAULT_CONNECT_TIMEOUT = 60;

    private static final int DEFAULT_KEEPALIVE = 120;

    /**
     * 只支持qos 0
     */
    private static final int DEFAULT_QOS_LEVEL = 0;

    private static final long MIN_BACKOFF = 1000L;

    private static final long MAX_BACKOFF = 30 * 1000L; // 30 seconds

    private static final long DEFAULT_BACKOFF = 1000L;

    private final SecureRandom random = new SecureRandom();

    private final AtomicBoolean isConnecting = new AtomicBoolean();

    private final AtomicInteger connectResultCode = new AtomicInteger();

    private final AtomicInteger retryTimes = new AtomicInteger();

    private final MqttClientOptions mqttClientOptions;

    private MqttAsyncClient mqttAsyncClient;

    private ConnectListener connectListener;

    private ConnectActionListener connectActionListener;

    private RawMessageListener rawMessageListener;

    private final String url;

    private final Map<String, DefaultSubscribeListenerImpl> subscribeTopics = new ConcurrentHashMap<>();

    public MqttClient(MqttClientOptions mqttClientOptions) {
        this.mqttClientOptions = mqttClientOptions;
        this.url = "ssl://" + mqttClientOptions.getHost() + ":" + mqttClientOptions.getPort();
    }

    public MqttClient(MqttClientOptions mqttClientOptions, RawMessageListener rawMessageListener) {
        this(mqttClientOptions);
        this.rawMessageListener = rawMessageListener;
    }

    public int connect() {
        if (isConnecting.compareAndSet(false, true)) {
            if (doConnect() < 0) {
                return -1;
            }
            synchronized (MqttClient.this) {
                while (isConnecting.get()) {
                    try {
                        wait(DEFAULT_CONNECT_TIMEOUT * 1000);
                    } catch (InterruptedException e) {
                        log.error("InterruptedException occur when mqtt client connect.", e);
                    }
                }
            }
        }
        if (mqttAsyncClient.isConnected()) {
            return 0;
        }
        // 处理paho返回的错误码为0的异常
        if (connectResultCode.get() == 0) {
            log.error("Client encountered an exception");
            return -1;
        }
        return connectResultCode.get();
    }

    private int doConnect() {
        try {
            String userName = generateUserName();
            MqttConnectOptions options = new MqttConnectOptions();
            options.setCleanSession(true);
            options.setKeepAliveInterval(DEFAULT_KEEPALIVE);
            options.setConnectionTimeout(DEFAULT_CONNECT_TIMEOUT);
            options.setAutomaticReconnect(false);
            options.setUserName(userName);
            options.setPassword(mqttClientOptions.getAccessCode().toCharArray());
            if (!mqttClientOptions.isTrustAll()) {
                options.setSocketFactory(getOptionSocketFactory());
            }
            options.setHttpsHostnameVerificationEnabled(false);

            //设置MqttClient
            mqttAsyncClient = new MqttAsyncClient(url, userName, new MemoryPersistence());
            mqttAsyncClient.setCallback(new InnerMqttCallback());
            log.info("try to connect to {}, username = {}", url, userName);
            //建立连接
            mqttAsyncClient.connect(options, null, getCallback());
        } catch (MqttException e) {
            log.error("connect error, the server url is {}.", url, e);
            return -1;
        }
        return 0;
    }

    private String generateUserName() {
        String username = "accessKey=" + mqttClientOptions.getAccessKey() + "|timestamp=" + System.currentTimeMillis();
        if (StringUtils.isNotBlank(mqttClientOptions.getInstanceId())) {
            username = username + "|instanceId=" + mqttClientOptions.getInstanceId();
        }
        return username;
    }

    private IMqttActionListener getCallback() {
        return new IMqttActionListener() {
            @Override
            public void onSuccess(IMqttToken iMqttToken) {
                log.info("connect success, server url: {}", url);
                if (connectActionListener != null) {
                    connectActionListener.onSuccess(iMqttToken);
                }
                synchronized (MqttClient.this) {
                    isConnecting.compareAndSet(true, false);
                    MqttClient.this.notifyAll();
                }
            }

            @Override
            public void onFailure(IMqttToken iMqttToken, Throwable throwable) {
                log.info("connect failed, the reason is {}", throwable.toString(), throwable);
                if (throwable instanceof MqttException) {
                    MqttException me = (MqttException) throwable;
                    connectResultCode.set(me.getReasonCode());
                }
                if (connectActionListener != null) {
                    connectActionListener.onFailure(iMqttToken, throwable);
                }
                synchronized (MqttClient.this) {
                    isConnecting.compareAndSet(true, false);
                    MqttClient.this.notifyAll();
                }
            }
        };
    }

    /**
     * 退避重连
     */
    private void reConnect() {
        int ret = -1;
        while (ret != 0) {
            // 退避重连
            int lowBound = (int) (DEFAULT_BACKOFF * 0.8);
            int highBound = (int) (DEFAULT_BACKOFF * 1.0);
            long randomBackOff = random.nextInt(highBound - lowBound);
            int powParameter = retryTimes.get() & 0x0F;
            long backOffWithJitter = (long) (Math.pow(2.0, powParameter)) * (randomBackOff + lowBound);
            long waitTimeUntilNextRetry = Math.min(MIN_BACKOFF + backOffWithJitter, MAX_BACKOFF);
            try {
                Thread.sleep(waitTimeUntilNextRetry);
            } catch (InterruptedException e) {
                log.error("sleep failed, the reason is {}", e.getMessage());
            }
            retryTimes.getAndIncrement();
            ret = connect();
        }
        retryTimes.set(0);
    }

    @Override
    public void subscribeTopic(String topic, ActionListener listener) {
        DefaultSubscribeListenerImpl defaultSubscribeListener = new DefaultSubscribeListenerImpl(topic,
            listener);
        subscribeTopics.compute(topic, (key, value) -> {
            if (value == null) {
                try {
                    mqttAsyncClient.subscribe(topic, DEFAULT_QOS_LEVEL, null, defaultSubscribeListener);
                } catch (MqttException e) {
                    log.error("subscribeTopic error. topic = {}", topic, e);
                    if (listener != null) {
                        listener.onFailure(topic, e);
                    }
                }
            }
            return defaultSubscribeListener;
        });
    }

    /**
     * 加载SSL证书
     *
     * @return SocketFactory
     */
    private SocketFactory getOptionSocketFactory() {
        SSLContext sslContext;
        String jksSourceRootPath = mqttClientOptions.getJksSourceRootPath();
        try (InputStream stream = MqttClient.class.getClassLoader().getResourceAsStream(jksSourceRootPath)) {
            sslContext = SSLContext.getInstance("TLS");
            KeyStore ts = KeyStore.getInstance("JKS");
            ts.load(stream, mqttClientOptions.getJksPassword());
            TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            tmf.init(ts);
            TrustManager[] tm = tmf.getTrustManagers();
            sslContext.init(null, tm, SecureRandom.getInstanceStrong());
        } catch (Exception e) {
            log.error("getOptionSocketFactory error.", e);
            return null;
        }
        return sslContext.getSocketFactory();
    }

    public void close() {
        if (mqttAsyncClient.isConnected()) {
            try {
                subscribeTopics.clear();
                mqttAsyncClient.disconnect();
                mqttAsyncClient.close();
            } catch (MqttException e) {
                log.error("mqttAsyncClient close error.", e);
            }
        }
    }

    @Override
    public boolean isConnected() {
        if (mqttAsyncClient == null) {
            return false;
        }
        return mqttAsyncClient.isConnected();
    }

    @Override
    public void setConnectListener(ConnectListener connectListener) {
        this.connectListener = connectListener;
    }

    @Override
    public void setConnectActionListener(ConnectActionListener connectActionListener) {
        this.connectActionListener = connectActionListener;
    }

    @Override
    public void setRawMessageListener(RawMessageListener rawMessageListener) {
        this.rawMessageListener = rawMessageListener;
    }

    /**
     * Mqtt回调
     */
    class InnerMqttCallback implements MqttCallbackExtended {
        @Override
        public void connectionLost(Throwable cause) {
            log.error("Connection lost.", cause);
            if (connectListener != null) {
                connectListener.connectionLost(cause);
            }
            reConnect();
            // reSubscribe
            subscribeTopics.keySet().forEach(topic -> {
                DefaultSubscribeListenerImpl actionListener = subscribeTopics.remove(topic);
                if (actionListener != null) {
                    subscribeTopic(topic, actionListener.getListener());
                }
            });
        }

        @Override
        public void messageArrived(String topic, MqttMessage message) {
            log.debug("messageArrived topic =  {}, msg = {}", topic, message.toString());
            RawMessage rawMessage = new RawMessage(topic, message.toString());
            try {
                if (rawMessageListener != null) {
                    rawMessageListener.onMessageReceived(rawMessage);
                }
            } catch (Exception e) {
                log.error("message handler error.", e);
            }

        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken token) {

        }

        @Override
        public void connectComplete(boolean reconnect, String serverURI) {
            log.info("Mqtt client connected. address is {}", serverURI);
            if (connectListener != null) {
                connectListener.connectComplete(reconnect, serverURI);
            }
        }
    }
}
