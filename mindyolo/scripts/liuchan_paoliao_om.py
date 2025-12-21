#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import os
import cv2
import numpy as np
import acl

# ================= 配置区域 (修改点 1) =================
# 定义两个模型路径
MODEL_PATH_1 = "/home/HwHiAiUser/rabbits/models/liuchan.om"
MODEL_PATH_2 = "/home/HwHiAiUser/rabbits/models/paoliao.om"

# 统一图片路径
TARGET_IMAGE_PATH = "/home/HwHiAiUser/rabbits/images/yichang.jpg"

# 置信度阈值
CONF_THRESHOLD = 0.5
# =====================================================

class YoloOM:
    def __init__(self, model_path, device_id=0):
        self.device_id = device_id
        self.model_path = model_path
        self.model_id = None
        self.context = None
        self._init_resource()
        
    def _init_resource(self):
        print(f"[Info] Loading model: {self.model_path} ...")
        # [修改点 2] 注释掉 acl.init()，移至 main 函数，防止多次初始化报错
        # acl.init() 
        
        acl.rt.set_device(self.device_id)
        self.context, _ = acl.rt.create_context(self.device_id)
        self.model_id, _ = acl.mdl.load_from_file(self.model_path)
        self.model_desc = acl.mdl.create_desc()
        acl.mdl.get_desc(self.model_desc, self.model_id)
        
        self.input_size = acl.mdl.get_input_size_by_index(self.model_desc, 0)
        input_dims, _ = acl.mdl.get_input_dims(self.model_desc, 0)
        self.input_h = input_dims['dims'][2]
        self.input_w = input_dims['dims'][3]
        print(f"【模型加载成功】输入尺寸: {self.input_w}x{self.input_h}")

    def preprocess(self, image):
        # 保持原有的 Letterbox 预处理逻辑不变
        shape = image.shape[:2]
        new_shape = (self.input_w, self.input_h)
        r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
        new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
        dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]
        dw /= 2  
        dh /= 2
        if shape[::-1] != new_unpad:
            image = cv2.resize(image, new_unpad, interpolation=cv2.INTER_LINEAR)
        top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
        left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
        img_padded = cv2.copyMakeBorder(image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(114, 114, 114))
        img_rgb = cv2.cvtColor(img_padded, cv2.COLOR_BGR2RGB)
        img_data = img_rgb.astype(np.float32) / 255.0
        img_data = img_data.transpose(2, 0, 1) 
        return np.ascontiguousarray(img_data)
    
    def infer(self, image_path, conf_threshold=0.8):
        if not os.path.exists(image_path): 
            print(f"图片不存在: {image_path}")
            return False
        
        img = cv2.imread(image_path)
        input_data = self.preprocess(img)
        
        input_dataset = acl.mdl.create_dataset()
        input_ptr, _ = acl.rt.malloc(self.input_size, 0)
        acl.rt.memcpy(input_ptr, self.input_size, acl.util.bytes_to_ptr(input_data.tobytes()), input_data.nbytes, 1)
        acl.mdl.add_dataset_buffer(input_dataset, acl.create_data_buffer(input_ptr, self.input_size))

        output_dataset = acl.mdl.create_dataset()
        output_size = acl.mdl.get_output_size_by_index(self.model_desc, 0)
        output_ptr, _ = acl.rt.malloc(output_size, 0)
        acl.mdl.add_dataset_buffer(output_dataset, acl.create_data_buffer(output_ptr, output_size))

        ret = acl.mdl.execute(self.model_id, input_dataset, output_dataset)
        if ret != 0:
            print(f"推理失败错误码: {ret}")
            return False
        
        host_ptr, _ = acl.rt.malloc_host(output_size)
        acl.rt.memcpy(host_ptr, output_size, output_ptr, output_size, 2)
        raw_data = np.frombuffer(acl.util.ptr_to_bytes(host_ptr, output_size), dtype=np.float32)

        is_detected = False
        try:
            data = raw_data.reshape(5, -1)
            scores = data[4, :]
            max_score = np.max(scores)
            print(f"最大置信度: {max_score:.4f}")
            if max_score > conf_threshold:
                is_detected = True
        except Exception as e:
            print(f"解析错误: {e}")

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
        # [修改点 3] 注释掉 acl.finalize()，防止释放第一个模型时导致整个ACL不可用
        # acl.finalize()

def main():
    if not os.path.exists(MODEL_PATH_1) or not os.path.exists(MODEL_PATH_2):
        print("错误：找不到部分模型文件")
        return

    # [修改点 4] 全局初始化 ACL (只执行一次)
    acl.init()
    
    yolo_liuchan = None
    yolo_paoliao = None

    try:
        # 1. 分别初始化两个模型
        print("=== 初始化模型 1 (liuchan.om) ===")
        yolo_liuchan = YoloOM(MODEL_PATH_1)
        
        print("=== 初始化模型 2 (paoliao.om) ===")
        yolo_paoliao = YoloOM(MODEL_PATH_2)
        
        print(f"\n正在处理图片: {TARGET_IMAGE_PATH}")
        
        # 2. 模型 1 推理
        print(">>> [Liuchan] 开始推理...")
        t0 = time.time()
        res_liuchan = yolo_liuchan.infer(TARGET_IMAGE_PATH, CONF_THRESHOLD)
        print(f"    [Liuchan] 耗时: {(time.time() - t0) * 1000:.2f}ms, 结果: {'发现目标' if res_liuchan else '未发现'}")

        # 3. 模型 2 推理
        print(">>> [Paoliao] 开始推理...")
        t1 = time.time()
        res_paoliao = yolo_paoliao.infer(TARGET_IMAGE_PATH, CONF_THRESHOLD)
        print(f"    [Paoliao] 耗时: {(time.time() - t1) * 1000:.2f}ms, 结果: {'发现目标' if res_paoliao else '未发现'}")

    except Exception as e:
        print(f"运行出错: {e}")
    finally:
        # 4. 依次释放模型资源
        if yolo_liuchan: yolo_liuchan.release()
        if yolo_paoliao: yolo_paoliao.release()
        
        # [修改点 5] 最后全局去初始化 ACL
        print("[Info] 所有资源释放完毕")
        acl.finalize()

if __name__ == "__main__":
    main()