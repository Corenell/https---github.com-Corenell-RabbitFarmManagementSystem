#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import cv2
import numpy as np
import acl
import atexit
from flask import Flask, request

# ================= 配置区域 =================
MODEL_PATH_1 = "/home/HwHiAiUser/rabbits/models/liuchan.om"
MODEL_PATH_2 = "/home/HwHiAiUser/rabbits/models/paoliao.om"
MODEL_PATH_3 = "/home/HwHiAiUser/rabbits/models/liaocao.om"
SAVE_PATH_YICHANG = "/home/HwHiAiUser/rabbits/images/yichang.jpeg"
SAVE_PATH_LIAOCAO = "/home/HwHiAiUser/rabbits/images/liaocao.jpeg"
CONF_THRESHOLD = 0.5
# ===========================================

app = Flask(__name__)

# 全局变量，用于存放模型实例
yolo_liuchan = None
yolo_paoliao = None
yolo_liaocao = None

# --- 这里放入之前的 YoloOM 类定义 (保持不变) ---
class YoloOM:
    def __init__(self, model_path, device_id=0):
        self.device_id = device_id
        self.model_path = model_path
        self.model_id = None
        self.context = None
        self._init_resource()
        
    def _init_resource(self):
        print(f"[Init] Loading model: {self.model_path} ...")
        # 注意：这里不调用 acl.init()，由主程序统一调用
        acl.rt.set_device(self.device_id)
        self.context, _ = acl.rt.create_context(self.device_id)
        self.model_id, _ = acl.mdl.load_from_file(self.model_path)
        self.model_desc = acl.mdl.create_desc()
        acl.mdl.get_desc(self.model_desc, self.model_id)
        
        self.input_size = acl.mdl.get_input_size_by_index(self.model_desc, 0)
        input_dims, _ = acl.mdl.get_input_dims(self.model_desc, 0)
        self.input_h = input_dims['dims'][2]
        self.input_w = input_dims['dims'][3]

    def preprocess(self, image):
        # Letterbox 预处理
        shape = image.shape[:2]
        new_shape = (self.input_w, self.input_h)
        r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
        new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
        dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]
        dw /= 2; dh /= 2
        if shape[::-1] != new_unpad:
            image = cv2.resize(image, new_unpad, interpolation=cv2.INTER_LINEAR)
        top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
        left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
        img_padded = cv2.copyMakeBorder(image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(114, 114, 114))
        img_rgb = cv2.cvtColor(img_padded, cv2.COLOR_BGR2RGB)
        img_data = img_rgb.astype(np.float32) / 255.0
        img_data = img_data.transpose(2, 0, 1) 
        return np.ascontiguousarray(img_data)

    def infer(self, image_path, conf_threshold=0.5):
        if not os.path.exists(image_path): return False
        
        if self.context:
            acl.rt.set_context(self.context)

        img = cv2.imread(image_path)
        if img is None: return False # 防止坏图导致崩溃

        input_data = self.preprocess(img)
        
        # 准备输入输出
        input_dataset = acl.mdl.create_dataset()
        input_ptr, _ = acl.rt.malloc(self.input_size, 0)
        acl.rt.memcpy(input_ptr, self.input_size, acl.util.bytes_to_ptr(input_data.tobytes()), input_data.nbytes, 1)
        acl.mdl.add_dataset_buffer(input_dataset, acl.create_data_buffer(input_ptr, self.input_size))

        output_dataset = acl.mdl.create_dataset()
        output_size = acl.mdl.get_output_size_by_index(self.model_desc, 0)
        output_ptr, _ = acl.rt.malloc(output_size, 0)
        acl.mdl.add_dataset_buffer(output_dataset, acl.create_data_buffer(output_ptr, output_size))

        # 执行推理
        ret = acl.mdl.execute(self.model_id, input_dataset, output_dataset)
        
        # 获取结果
        host_ptr, _ = acl.rt.malloc_host(output_size)
        acl.rt.memcpy(host_ptr, output_size, output_ptr, output_size, 2)
        raw_data = np.frombuffer(acl.util.ptr_to_bytes(host_ptr, output_size), dtype=np.float32)

        is_detected = False
        try:
            data = raw_data.reshape(5, -1)
            scores = data[4, :]
            max_score = np.max(scores)
            print(f" 最大置信度: {max_score:.4f}")
            if max_score > conf_threshold:
                is_detected = True
        except:
            pass

        # 释放每帧的资源
        acl.rt.free(input_ptr)
        acl.rt.free(output_ptr)
        acl.rt.free_host(host_ptr)
        acl.mdl.destroy_dataset(input_dataset)
        acl.mdl.destroy_dataset(output_dataset)
        
        return is_detected

    def release(self):
        if self.model_id: acl.mdl.unload(self.model_id)
        if self.context: acl.rt.destroy_context(self.context)
        acl.rt.reset_device(self.device_id)

# --- Flask 路由逻辑 ---

@app.route("/upload_yichang", methods=["POST"])
def upload_yichang():
    data = request.get_data()
    # 1. 保存图片
    with open(SAVE_PATH_YICHANG, "wb") as f:
        f.write(data)
    print(f"[Web] Received yichang.jpeg ({len(data)} bytes)")

    # 2. 触发推理 (直接使用全局模型对象)
    results = []
    
    # 检测liuchan
    if yolo_liuchan:
        res1 = yolo_liuchan.infer(SAVE_PATH_YICHANG, CONF_THRESHOLD)
        results.append(f"Liuchan:{'Yes' if res1 else 'No'}")
        
    # 检测跑料
    if yolo_paoliao:
        res2 = yolo_paoliao.infer(SAVE_PATH_YICHANG, CONF_THRESHOLD)
        results.append(f"Paoliao:{'Yes' if res2 else 'No'}")

    # 3. 返回结果给ESP32 (或仅返回OK)
    result_str = " | ".join(results)
    print(f"[Inference] {result_str}")
    
    return f"OK: {result_str}"

@app.route("/upload_liaocao", methods=["POST"])
def upload_liaocao():
    data = request.get_data()
    # 1. 保存图片
    with open(SAVE_PATH_LIAOCAO, "wb") as f:
        f.write(data)
    print(f"[Web] Received liaocao.jpeg ({len(data)} bytes)")
    # 2. 触发推理 (直接使用全局模型对象)
    results = []
    
    # 检测liaocao
    if yolo_liaocao:
        res1 = yolo_liaocao.infer(SAVE_PATH_LIAOCAO, CONF_THRESHOLD)
        results.append(f"Liaocao:{'Yes' if res1 else 'No'}")

    # 3. 返回结果给ESP32 (或仅返回OK)
    result_str = " | ".join(results)
    print(f"[Inference] {result_str}")
    
    return f"OK: {result_str}"

# --- 资源清理注册 ---
def cleanup():
    print("[Exit] Cleaning up ACL resources...")
    if yolo_liuchan: yolo_liuchan.release()
    if yolo_paoliao: yolo_paoliao.release()
    if yolo_liaocao: yolo_liaocao.release()
    acl.finalize()

# --- 主程序启动 ---
if __name__ == "__main__":
    # 注册退出时的清理函数
    atexit.register(cleanup)

    # 1. 全局初始化 ACL
    acl.init()
    
    # 2. 加载模型 (在Web服务启动前完成，避免每次请求都加载)
    print(">>> 正在加载模型，请稍候...")
    if os.path.exists(MODEL_PATH_1):
        yolo_liuchan = YoloOM(MODEL_PATH_1)
        # res = yolo_liuchan.infer(SAVE_PATH_YICHANG, CONF_THRESHOLD)  # 预热
        # print(f"Liuchan : {'Yes' if res else 'No'}")
    else:
        print(f"Warning: Model 1 not found at {MODEL_PATH_1}")

    if os.path.exists(MODEL_PATH_2):
        yolo_paoliao = YoloOM(MODEL_PATH_2)
        # res = yolo_paoliao.infer(SAVE_PATH_YICHANG, CONF_THRESHOLD)  # 预热
        # print(f"Paoliao : {'Yes' if res else 'No'}")
    else:
        print(f"Warning: Model 2 not found at {MODEL_PATH_2}")

    if os.path.exists(MODEL_PATH_3):
        yolo_liaocao = YoloOM(MODEL_PATH_3)
    else:
        print(f"Warning: Model 3 not found at {MODEL_PATH_3}")

    print(">>> 模型加载完毕，Web服务启动 (Port 80)")
    
    # 3. 启动 Flask 
    # 【关键】必须设置 threaded=False 或者 processes=1
    # 因为 ACL context 是线程绑定的，多线程模式会导致路由函数找不到上下文报错
    app.run(host="0.0.0.0", port=5000, threaded=False)