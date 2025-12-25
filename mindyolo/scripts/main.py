#!/usr/bin/env python
# -*- coding: utf-8 -*-
# main.py

import sys
import os
import time
import json
import atexit
import requests 
from datetime import datetime
import paho.mqtt.client as mqtt
from model_service import InferenceSystem
from esp_image import RabbitDualCameraSystem 

# ================= 1. 华为云 MQTT 配置 =================
MQTT_SERVER = "0e7519d568.iot-mqtts.cn-north-4.myhuaweicloud.com"
MQTT_PORT = 1883
DEVICE_ID = "694a6484cfb5ee5b35742750_orange_pi_0_0_2025122404"
MQTT_USER = "694a6484cfb5ee5b35742750_orange_pi"
MQTT_PASSWORD = "0b08f7b5a1875ec59c0474efc77dc61c634746e8fe195cfb1048686acd0e9e3d"
# ===============================================================

# Topic 定义
TOPIC_DEVICE_ID = "694a6484cfb5ee5b35742750_orange_pi"
TOPIC_COMMAND = f"$oc/devices/{TOPIC_DEVICE_ID}/sys/commands/#"
TOPIC_REPORT = f"$oc/devices/{TOPIC_DEVICE_ID}/sys/properties/report"
TOPIC_CMD_RESP_PREFIX = f"$oc/devices/{TOPIC_DEVICE_ID}/sys/commands/response/request_id="

# 全局实例
inference_system = InferenceSystem()
cam_system = RabbitDualCameraSystem() 

# ================= 异常数据上报函数 =================
def upload_rabbit_warning(token, house_id, rabbit_id, warning_status):
    """
    当检测到异常时，调用此函数向服务器发送数据
    接口地址: http://113.44.184.126:8080/rabbit/feedrecords
    """
    api_url = "http://113.44.184.126:8080/rabbit/feedrecords"
    image_path = "/home/HwHiAiUser/rabbits/images/yichang.jpeg"
    
    # 构造当前时间字符串
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # 构造Header
    headers = {
        'Authorization': token
    }

    # 构造表单数据 (Form Data)
    data = {
        'houseId': house_id,         # 栋号
        'rabbitId': rabbit_id,       # 笼号
        'warningStatus': warning_status, # 异常描述
        'warningTime': current_time, # 当前时间
        'isDeal': 0,                 # 未处理
        'remark': ""                 # 备注为空
    }

    print(f"[上传] 正在上报异常: {warning_status} -> {api_url}")

    try:
        if os.path.exists(image_path):
            with open(image_path, 'rb') as f:
                # 构造文件部分
                files = {
                    'imageFile': (os.path.basename(image_path), f, 'image/jpeg')
                }
                # 发送请求
                response = requests.post(api_url, headers=headers, data=data, files=files, timeout=5)
                
                if response.status_code == 200:
                    print(f"[上传] 成功! 服务器响应: {response.text}")
                else:
                    print(f"[上传] 失败. 状态码: {response.status_code}, 响应: {response.text}")
        else:
            print(f"[上传] 错误: 图片文件不存在 {image_path}")

    except Exception as e:
        print(f"[上传] 发生异常: {e}")
# ============================================================

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[MQTT] 已连接到华为云! 代码: {rc}")
        client.subscribe(TOPIC_COMMAND)
        print(f"[MQTT] 已订阅 {TOPIC_COMMAND}")
    else:
        print(f"[MQTT] 连接失败，代码: {rc}")

def on_message(client, userdata, msg):
    print(f"\n[MQTT] 收到消息，主题: {msg.topic}")
    payload_str = msg.payload.decode('utf-8')

    try:
        data = json.loads(payload_str)
        request_id = ""
        if "request_id=" in msg.topic:
            request_id = msg.topic.split("request_id=")[1]

        service_id = data.get("service_id")
        command_name = data.get("command_name")
        paras = data.get("paras", {})

        if service_id == "orangepi" and command_name == "detect":
            handle_detect_command(client, request_id, paras)
        else:
            print(f"[MQTT] 忽略命令: 服务={service_id}, 命令={command_name}")

    except Exception as e:
        print(f"[错误] JSON 解析错误: {e}")

def handle_detect_command(client, request_id, paras):
    """
    处理 'detect' 命令：提取参数 -> 拍照 -> 推理 -> 响应 -> [新增]异常上报
    """
    token = paras.get("token")
    cage_id = paras.get("cageid")
    house_id = paras.get("houseid")
    
    print("="*40)
    print(f"任务开始: [检测] 笼位ID: {cage_id}, 栋号ID: {house_id}")
    print("="*40)

    # [步骤 1] 双摄一键拍照
    print(">>> 步骤 1: 正在调用双摄拍照...")
    is_fenban_ok, is_liaocao_ok = cam_system.capture_all()

    # if not (is_fenban_ok and is_liaocao_ok):
    #     print("[错误] 图片获取失败，终止推理。")
    #     send_response(client, request_id, 500, "Image fetch failed")
    #     return

    # [步骤 2] 调用推理
    print(">>> 步骤 2: 正在运行推理...")
    save_path_yichang = cam_system.path_yichang
    save_path_liaocao = "../images/yichang.jpeg"
    
    # 获取推理结果字典
    infer_results = inference_system.run_inference_task(save_path_yichang, save_path_liaocao)

    # 提取具体的检测结果
    res_paoliao = infer_results["paoliao"]
    res_liuchan = infer_results["liuchan"]
    res_liaocao = infer_results["liaocao"]

    # 打印简要日志
    print(f"[推理详情] 跑料: {res_paoliao['score']:.2f}, 流产: {res_liuchan['score']:.2f}, 料槽: {res_liaocao['score']:.2f}")

    # ================= 异常数据上报逻辑 =================
    # 如果检测到【】，上报数据
    if res_paoliao['detected']:
        print(">>> 检测到[跑料]，正在上报数据...")
        upload_rabbit_warning(token, house_id, cage_id, "出现刨料行为")

    # 如果检测到【流产】，上报数据
    if res_liuchan['detected']:
        print(">>> 检测到[流产]，正在上报数据...")
        upload_rabbit_warning(token, house_id, cage_id, "出现流产行为")
    # ========================================================

    # 构造返回给华为云的数据包
    result_data = {
        "weight":1
    }
    print(f"[结果] {result_data}")

    # [步骤 3] 发送响应和上报
    print(">>> 步骤 3: 发送响应给云端...")
    send_response(client, request_id, 0, result_data)
    report_properties(client, result_data)

def send_response(client, request_id, result_code, result_data):
    if not request_id: return
    resp_topic = f"{TOPIC_CMD_RESP_PREFIX}{request_id}"
    resp_payload = {
        "result_code": result_code,
        "response_name": "detect_response",
        "paras": result_data if isinstance(result_data, dict) else {"msg": result_data}
    }
    client.publish(resp_topic, json.dumps(resp_payload))
    print(f"[MQTT] 响应已发送。")

def report_properties(client, data):
    payload = {"services": [{"service_id": "orangepi", "properties": data}]}
    client.publish(TOPIC_REPORT, json.dumps(payload))
    print(f"[MQTT] 属性已上报。")

def main():
    # 初始化系统
    inference_system.initialize()
    atexit.register(inference_system.cleanup)

    # MQTT 设置
    client = mqtt.Client(client_id=DEVICE_ID)
    client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"[系统] 正在连接华为云 IoT...")
    try:
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
        print("[系统] 等待命令中...")
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[系统] 正在停止...")
    except Exception as e:
        print(f"\n[错误] {e}")

if __name__ == "__main__":
    main()