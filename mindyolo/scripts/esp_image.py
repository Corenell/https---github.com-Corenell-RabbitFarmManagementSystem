#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import requests
import os
import time

# ==========================================
# 基础类：通用单台相机客户端 (低耦合)
# ==========================================
class ESP32CamClient:
    def __init__(self, base_url, timeout=10):
        """
        初始化单台相机客户端
        """
        self.base_url = base_url.rstrip('/')
        self.capture_url = f"{self.base_url}/capture"
        self.timeout = timeout

    def capture_save(self, save_path):
        """
        发送POST请求拍照并覆盖保存文件
        """
        try:
            # 1. 自动创建父目录
            folder = os.path.dirname(save_path)
            if folder and not os.path.exists(folder):
                os.makedirs(folder, exist_ok=True)

            print(f"[Cam] Connecting to {self.base_url} ...")
            
            # 2. 发起请求 (POST)
            start_t = time.time()
            response = requests.post(self.capture_url, timeout=self.timeout)
            
            # 3. 保存文件
            if response.status_code == 200:
                with open(save_path, 'wb') as f:
                    f.write(response.content)
                cost = time.time() - start_t
                print(f"[Cam] Saved: {save_path} ({len(response.content)/1024:.1f}KB, {cost:.2f}s)")
                return True
            else:
                print(f"[Cam] Error: HTTP {response.status_code} from {self.base_url}")
                return False

        except Exception as e:
            print(f"[Cam] Failed to connect {self.base_url}: {e}")
            return False

# ==========================================
# 业务类：双相机管理系统 (封装你的具体需求)
# ==========================================
class RabbitDualCameraSystem:
    def __init__(self):
        # 配置相机地址
        self.cam_fenban = ESP32CamClient("http://esp32-cam-fenban.local")
        self.cam_liaocao = ESP32CamClient("http://esp32-cam-liaocao.local")
        
        # 配置保存路径
        self.base_dir = "/home/HwHiAiUser/rabbits/images"
        self.path_yichang = os.path.join(self.base_dir, "yichang.jpeg")
        self.path_liaocao = os.path.join(self.base_dir, "liaocao.jpeg") 

    def capture_all(self):
        """
        一键调用两台相机拍照
        Returns: (bool, bool) 分别代表 [粪板/异常] 和 [料槽] 是否成功
        """
        print(">>> 启动双摄拍照任务...")
        
        # 粪板相机 -> yichang.jpeg
        res_fenban = self.cam_fenban.capture_save(self.path_yichang)
        
        # 为了防止网络并发拥堵或电源波动，建议微小间隔
        # time.sleep(0.5) 
        
        # 料槽相机 -> liaocao.jpeg
        res_liaocao = self.cam_liaocao.capture_save(self.path_liaocao)
        
        return res_fenban, res_liaocao

# ==========================================
# 单元测试模块
# ==========================================
if __name__ == "__main__":
    print("=== 开始测试双相机系统 ===")
    system = RabbitDualCameraSystem()
    
    # 模拟执行
    success1, success2 = system.capture_all()
    
    if success1 and success2:
        print("\n✅ 测试通过：两台相机均已保存图片")
    else:
        print(f"\n❌ 测试失败：粪板({success1}), 料槽({success2})")