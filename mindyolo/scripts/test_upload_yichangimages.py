#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests
import os
from datetime import datetime

# ================= 配置区域 =================
# 1. 接口地址 (附件2提供的地址)
API_URL = "http://113.44.184.126:8080/rabbit/feedrecords"

# 2. 测试用的图片路径 (必须存在，否则无法发送)
# 如果你本地没有这个文件，脚本会自动创建一个临时的空白图片进行测试
TEST_IMAGE_PATH = "/home/HwHiAiUser/rabbits/images/yichang.jpeg"

# 3. 测试用的 Token (如果接口不需要鉴权，可以留空)
TEST_TOKEN = "eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJIYXJtb255IiwibG9naW5fdXNlcl9rZXkiOiIwZDFhNzcxZS1mYzgzLTQ5ZWEtOGU1YS1iMzcyMGY2OTg1NDMifQ.1c6ucXjL6B3FT5KQyRIP34_abr2so1-NfxMGAdkVun_je7pSbP_EmSfGFKg8KLMI2UCj1rN7NZCkRmAL93EXUw" 
# ===========================================

def test_connection():
    print("=" * 40)
    print("开始测试接口连通性...")
    print(f"目标接口: {API_URL}")
    print("=" * 40)


    # 2. 构造测试数据
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    headers = {
        'Authorization': TEST_TOKEN
    }

    data = {
        'houseId': 9999,             # 使用 9999 标记为测试数据
        'rabbitId': 9999,            # 使用 9999 标记为测试数据
        'warningStatus': "连接性测试(Python脚本)",
        'warningTime': current_time,
        'isDeal': 0,
        'remark': "Debug Test"
    }

    # 3. 发送请求
    try:
        print("[1/2] 正在准备发送请求...")
        with open(TEST_IMAGE_PATH, 'rb') as f:
            files = {
                'imageFile': ('../images/yichang.jpeg', f, 'image/jpeg')
            }
            
            # 设置 5 秒超时，避免卡死
            response = requests.post(API_URL, headers=headers, data=data, files=files, timeout=5)
            
            print("[2/2] 服务器已响应")
            print("-" * 40)
            
            # 4. 判断结果
            if response.status_code == 200:
                print("✅ [成功] 接口连通正常！")
                print(f"服务器返回内容: {response.text}")
            else:
                print(f"❌ [失败] 接口连通，但服务器返回错误代码: {response.status_code}")
                print(f"错误详情: {response.text}")

    except requests.exceptions.ConnectionError:
        print("❌ [致命错误] 无法连接到服务器！")
        print("可能原因：")
        print("  1. OrangePi 没有联网")
        print("  2. 服务器 IP (113.44.184.126) 被防火墙拦截")
        print("  3. 服务器端口 (8080) 未开放")
    except requests.exceptions.Timeout:
        print("❌ [超时] 连接服务器超时 (5秒)")
        print("可能原因：网络质量差或服务器响应过慢")
    except Exception as e:
        print(f"❌ [未知错误] {str(e)}")

if __name__ == "__main__":
    test_connection()