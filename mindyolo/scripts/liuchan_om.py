#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import os
import cv2
import numpy as np
import acl

# ================= 配置区域 =================
# 1. OM 模型路径 (你的新模型)
MODEL_PATH = "/home/HwHiAiUser/rabbits/models/liuchan_v8.om"

# 2. 图片路径
TARGET_IMAGE_PATH = "/home/HwHiAiUser/rabbits/images/liuchan1.jpeg"

# 3. 置信度阈值
CONF_THRESHOLD = 0.5
# ===========================================

class YoloOM:
    def __init__(self, model_path, device_id=0):
        self.device_id = device_id
        self.model_path = model_path
        self.model_id = None
        self.context = None
        self._init_resource()
        
    def _init_resource(self):
        print("[Info] Init ACL resources...")
        acl.init()
        acl.rt.set_device(self.device_id)
        self.context, _ = acl.rt.create_context(self.device_id)
        self.model_id, _ = acl.mdl.load_from_file(self.model_path)
        self.model_desc = acl.mdl.create_desc()
        acl.mdl.get_desc(self.model_desc, self.model_id)
        
        # 获取输入尺寸 (通常是 640x640)
        self.input_size = acl.mdl.get_input_size_by_index(self.model_desc, 0)
        input_dims, _ = acl.mdl.get_input_dims(self.model_desc, 0)
        self.input_h = input_dims['dims'][2]
        self.input_w = input_dims['dims'][3]
        print(f"【模型加载成功】输入尺寸: {self.input_w}x{self.input_h}")

    def preprocess(self, image):
        # Resize -> RGB -> Normalize -> Transpose(CHW)
        img_resized = cv2.resize(image, (self.input_w, self.input_h))
        img_rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB)
        img_data = img_rgb.astype(np.float32) / 255.0
        img_data = img_data.transpose(2, 0, 1) 
        return np.ascontiguousarray(img_data)

    def infer(self, image_path, conf_threshold=0.8):
        if not os.path.exists(image_path): 
            print(f"图片不存在: {image_path}")
            return False
        
        # 1. 预处理
        img = cv2.imread(image_path)
        input_data = self.preprocess(img)
        
        # 2. 准备输入
        input_dataset = acl.mdl.create_dataset()
        input_ptr, _ = acl.rt.malloc(self.input_size, 0)
        acl.rt.memcpy(input_ptr, self.input_size, acl.util.bytes_to_ptr(input_data.tobytes()), input_data.nbytes, 1)
        acl.mdl.add_dataset_buffer(input_dataset, acl.create_data_buffer(input_ptr, self.input_size))

        # 3. 准备输出 (你的新模型只有 1 个输出)
        output_dataset = acl.mdl.create_dataset()
        output_size = acl.mdl.get_output_size_by_index(self.model_desc, 0)
        output_ptr, _ = acl.rt.malloc(output_size, 0)
        acl.mdl.add_dataset_buffer(output_dataset, acl.create_data_buffer(output_ptr, output_size))

        # 4. 执行推理
        ret = acl.mdl.execute(self.model_id, input_dataset, output_dataset)
        if ret != 0:
            print(f"推理失败错误码: {ret}")
            return False
        
        # 5. 获取结果
        host_ptr, _ = acl.rt.malloc_host(output_size)
        acl.rt.memcpy(host_ptr, output_size, output_ptr, output_size, 2)
        raw_data = np.frombuffer(acl.util.ptr_to_bytes(host_ptr, output_size), dtype=np.float32)

        # 6. 解析结果 [核心修改点]
        # 你的输出 shape 是 [1, 5, 8400]
        # 这里的 5 = [x, y, w, h, score]
        is_detected = False
        try:
            # Reshape 成 [5, 8400] (忽略 batch 维度)
            data = raw_data.reshape(5, -1)
            
            # 第 0,1,2,3 行是坐标，我们这次只需要看第 4 行 (Score)
            scores = data[4, :]
            
            # 找到最大置信度
            max_score = np.max(scores)
            
            print(f"当前图片最大置信度: {max_score:.4f}")
            
            if max_score > conf_threshold:
                is_detected = True

        except Exception as e:
            print(f"解析错误: {e}")

        # 7. 释放资源
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
        acl.finalize()

def main():
    # 检查模型是否存在
    if not os.path.exists(MODEL_PATH):
        print(f"错误：找不到模型文件 {MODEL_PATH}")
        return

    yolo = None
    try:
        # 1. 初始化模型
        yolo = YoloOM(MODEL_PATH)
        
        # 2. 执行一次推理 (去掉 while True 循环)
        print(f"正在处理图片: {TARGET_IMAGE_PATH}")
        t0 = time.time()
        
        is_detected = yolo.infer(TARGET_IMAGE_PATH, CONF_THRESHOLD)
        
        t_cost = (time.time() - t0) * 1000
        print(f"推理耗时: {t_cost:.2f}ms")

        # 3. 根据结果输出并退出
        if is_detected:
            print(">>> 结果: 1 (发现目标)")
            # 这里可以用 sys.exit(0) 表示成功，或者输出特定字符串供外部捕获
        else:
            print(">>> 结果: 0 (未发现目标)")

    except Exception as e:
        print(f"运行出错: {e}")
    finally:
        # 4. 释放资源
        if yolo: 
            yolo.release()
            print("[Info] 资源已释放，程序退出")

if __name__ == "__main__":
    main()