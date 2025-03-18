/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.iot.mqtt.example.client;

import com.iot.mqtt.example.client.listener.ActionListener;

import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttToken;

import lombok.Getter;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class DefaultSubscribeListenerImpl implements IMqttActionListener {
    private final String topic;

    @Getter
    private final ActionListener listener;

    public DefaultSubscribeListenerImpl(String topic, ActionListener listener) {
        this.topic = topic;
        this.listener = listener;
    }

    @Override
    public void onSuccess(IMqttToken iMqttToken) {
        log.info("Subscribe mqtt topic qos: {}", iMqttToken.getGrantedQos()[0]);
        //AT_MOST_ONCE(0), AT_LEAST_ONCE(1), EXACTLY_ONCE(2), FAILURE(128)
        if (iMqttToken.getGrantedQos()[0] > 2) {
            log.warn("Subscribe mqtt topic {} failed.", topic);
            if (listener != null) {
                listener.onFailure(topic, new IllegalStateException("qos: " + iMqttToken.getGrantedQos()[0]));
            }
        } else {
            if (listener != null) {
                listener.onSuccess(topic);
            }
        }
    }

    @Override
    public void onFailure(IMqttToken iMqttToken, Throwable throwable) {
        log.error("subscribe topic failed:" + topic);
        if (listener != null) {
            listener.onFailure(topic, throwable);
        }
    }
}
