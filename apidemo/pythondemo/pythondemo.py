# 导入库
import requests

# 发起鉴权请求并返回响应头中的鉴权 Token 数据
def fetch_auth_token():
    # 请求鉴权的 URL
    url = "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens"
    headers = {"Content-Type": "application/json"}

    # 构造 JSON 格式的请求数据
    json_payload = {
        "auth": {
            "identity": {
                "methods": ["password"],
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
    }

    # 构造 POST 请求
    response = requests.post(url, json=json_payload, headers=headers)
    if response.status_code != 201:
        raise Exception(f"请求失败: {response.status_code} {response.text}")

    # 从响应头中提取鉴权 Token 数据
    token = response.headers.get("X-Subject-Token")
    return token

# 获取设备属性并返回响应体中的 JSON 数据
def get_device_properties(token):
    # 请求 URL
    url = "https://ef861ca468.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/b3318d2e70ab4740b267ed8955fd7275/devices/67b683d83f28ab3d0384f27e_rabbit/properties?service_id=environment"
    headers = {
        "Content-Type": "application/json",
        "X-Auth-Token": token  # 这里传递 fetch_auth_token() 方法获取的 token
    }

    # 构造 GET 请求
    response = requests.get(url, headers=headers)
    if response.status_code != 200:
        raise Exception(f"请求失败: {response.status_code} {response.text}")

    # 提取返回数据
    return response.json()

# 发送消息并返回状态码
def post_message(token):
    # 请求 URL
    url = "https://ef861ca468.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/b3318d2e70ab4740b267ed8955fd7275/devices/67b683d83f28ab3d0384f27e_rabbit/messages"
    headers = {
        "Content-Type": "application/json",
        "X-Auth-Token": token  # 这里传递 fetch_auth_token() 方法获取的 token
    }

    # 构造 JSON 格式的请求数据
    json_payload = {
        "message": {
            "pwm": 50,
            "num": 1
        }
    }

    # 构造 POST 请求
    response = requests.post(url, json=json_payload, headers=headers)
    status = response.status_code
    if response.status_code != 200 and response.status_code != 201:
        raise Exception(f"请求失败: {response.status_code} {response.text}")

    return status

def main():
    try:
        token = fetch_auth_token()
        print("Token:", token)

        response = get_device_properties(token)
        print("Response:", response)

        status = post_message(token)
        print("Status:", status)

    except Exception as e:
        print("请求发生错误:", e)

if __name__ == "__main__":
    main() 
