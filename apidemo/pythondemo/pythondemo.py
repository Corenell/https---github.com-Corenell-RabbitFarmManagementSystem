import requests # http请求库
import json # json库

# post请求进行身份认证，获取token，24小时内可用
def iot_authentication():
    # 请求鉴权连接地址
    url = "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens"
    # 云端的身份验证信息，需要填写的地方有4个，数据结构是json
    payload = json.dumps({
        "auth": {
            "identity": {
                "methods": [
                    "password"
                ],
                "password": {
                    "user": {
                        "name": "qing_lan",
                        "password": "cy383245",
                        "domain": {
                            "name": "qing_lan"
                        }
                    }
                }
            },
            "scope": {
                "project": {
                    "name": "cn-north-4"
                }
            }
        }
    })
    headers = {
        'Content-Type': 'application/json'
    }
    # 发起网络请求获取华为云鉴权，post请求，
    response = requests.post(url, headers=headers, data=payload)
    # json方式提取到的json信息，是必要的鉴权信息
    token = response.headers["X-Subject-Token"]
    return token


#发送一个get请求到云端，云端返回json数据
def iot_shadow_data(token):
    # 请求url，网址是获取设备影子数据
    url = "https://ef861ca468.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/b3318d2e70ab4740b267ed8955fd7275/devices/67b683d83f28ab3d0384f27e_rabbit/shadow"
    # headers需要添加请求的鉴权信息
    headers = {
            "Content-Type": "application/json",
            "X-Auth-Token": token
        }
    response = requests.request("GET", url, headers=headers)
    # 转化为json数据
    data = response.json()
    return data



    
# 获取身份验证token
token = iot_authentication()
#获取设备影子数据
data = iot_shadow_data(token)
print(data)
