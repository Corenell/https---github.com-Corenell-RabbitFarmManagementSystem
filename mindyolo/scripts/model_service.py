#!/usr/bin/env python
# -*- coding: utf-8 -*-
# model_service.py

import os
import cv2
import numpy as np
import acl

# ================= 配置区域 =================
# 模型文件绝对路径
MODEL_PATH_PAOLIAO = "/home/HwHiAiUser/rabbits/models/paoliao.om"
MODEL_PATH_LIUCHAN = "/home/HwHiAiUser/rabbits/models/liuchan.om"
MODEL_PATH_LIAOCAO = "/home/HwHiAiUser/rabbits/models/liaocao.om"

# 全局置信度阈值 (可根据需要单独为每个模型调整)
CONF_THRESHOLD = 0.5
# ===========================================

class YoloOM:
    """
    YOLO模型底层封装类：负责 ACL 资源管理、图像预处理和单次推理执行
    """
    def __init__(self, model_path, device_id=0):
        self.device_id = device_id
        self.model_path = model_path
        self.model_id = None
        self.context = None
        self._init_resource()

    def _init_resource(self):
        """初始化 ACL 资源并加载模型"""
        print(f"[Model] Loading: {os.path.basename(self.model_path)} ...")
        acl.rt.set_device(self.device_id)
        self.context, _ = acl.rt.create_context(self.device_id)
        self.model_id, _ = acl.mdl.load_from_file(self.model_path)
        self.model_desc = acl.mdl.create_desc()
        acl.mdl.get_desc(self.model_desc, self.model_id)

        # 获取模型输入尺寸
        self.input_size = acl.mdl.get_input_size_by_index(self.model_desc, 0)
        input_dims, _ = acl.mdl.get_input_dims(self.model_desc, 0)
        self.input_h = input_dims['dims'][2]
        self.input_w = input_dims['dims'][3]

    def preprocess(self, image):
        """Letterbox 图像预处理 (保持长宽比缩放 + 填充)"""
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

    def infer(self, image_path):
        """
        执行推理
        Returns: 
            dict: {"detected": bool, "score": float}
        """
        result = {"detected": False, "score": 0.0}

        if not os.path.exists(image_path):
            print(f"[Error] Image not found: {image_path}")
            return result

        if self.context:
            acl.rt.set_context(self.context)

        img = cv2.imread(image_path)
        if img is None: 
            print(f"[Error] Failed to read image: {image_path}")
            return result

        # 1. 预处理
        input_data = self.preprocess(img)

        # 2. 准备内存
        input_dataset = acl.mdl.create_dataset()
        input_ptr, _ = acl.rt.malloc(self.input_size, 0)
        acl.rt.memcpy(input_ptr, self.input_size, acl.util.bytes_to_ptr(input_data.tobytes()), input_data.nbytes, 1)
        acl.mdl.add_dataset_buffer(input_dataset, acl.create_data_buffer(input_ptr, self.input_size))

        output_dataset = acl.mdl.create_dataset()
        output_size = acl.mdl.get_output_size_by_index(self.model_desc, 0)
        output_ptr, _ = acl.rt.malloc(output_size, 0)
        acl.mdl.add_dataset_buffer(output_dataset, acl.create_data_buffer(output_ptr, output_size))

        # 3. 执行推理
        acl.mdl.execute(self.model_id, input_dataset, output_dataset)

        # 4. 后处理结果
        host_ptr, _ = acl.rt.malloc_host(output_size)
        acl.rt.memcpy(host_ptr, output_size, output_ptr, output_size, 2)
        raw_data = np.frombuffer(acl.util.ptr_to_bytes(host_ptr, output_size), dtype=np.float32)

        try:
            # 假设 YOLO 输出格式，取最大置信度
            # 注意：如果模型输出结构不同（如 batch=1, anchors, ...），可能需要调整 reshape
            data = raw_data.reshape(5, -1) 
            scores = data[4, :] 
            max_score = float(np.max(scores))
            
            result["score"] = max_score
            if max_score > CONF_THRESHOLD:
                result["detected"] = True
                
            print(f"   -> [{os.path.basename(self.model_path)}] Score: {max_score:.4f} ({'Yes' if result['detected'] else 'No'})")
            
        except Exception as e:
            print(f"[Error] Post-processing failed: {e}")

        # 5. 释放资源
        acl.rt.free(input_ptr)
        acl.rt.free(output_ptr)
        acl.rt.free_host(host_ptr)
        acl.mdl.destroy_dataset(input_dataset)
        acl.mdl.destroy_dataset(output_dataset)

        return result

    def release(self):
        if self.model_id: acl.mdl.unload(self.model_id)
        if self.context: acl.rt.destroy_context(self.context)
        acl.rt.reset_device(self.device_id)


class InferenceSystem:
    """
    推理业务管理类：管理所有模型的生命周期和任务分发
    """
    def __init__(self):
        self.model_paoliao = None
        self.model_liuchan = None
        self.model_liaocao = None
        self.is_initialized = False

    def initialize(self):
        """初始化 ACL 并加载所有模型"""
        if self.is_initialized: return
        
        print(">>> [System] Initializing Inference System...")
        ret = acl.init()
        if ret != 0:
            print(f"ACL init failed, ret={ret}")
            return

        # 加载 Paoliao 模型
        if os.path.exists(MODEL_PATH_PAOLIAO):
            self.model_paoliao = YoloOM(MODEL_PATH_PAOLIAO)
        else:
            print(f"[Warning] Model not found: {MODEL_PATH_PAOLIAO}")

        # 加载 Liuchan 模型
        if os.path.exists(MODEL_PATH_LIUCHAN):
            self.model_liuchan = YoloOM(MODEL_PATH_LIUCHAN)
        else:
            print(f"[Warning] Model not found: {MODEL_PATH_LIUCHAN}")

        # 加载 Liaocao 模型
        if os.path.exists(MODEL_PATH_LIAOCAO):
            self.model_liaocao = YoloOM(MODEL_PATH_LIAOCAO)
        else:
            print(f"[Warning] Model not found: {MODEL_PATH_LIAOCAO}")
            
        self.is_initialized = True
        print(">>> [System] All models loaded.")

    def run_inference_task(self, yichang_img_path, liaocao_img_path):
        """
        执行组合推理任务
        
        Args:
            yichang_img_path: 异常检测图片路径 (用于 paoliao 和 liuchan 模型)
            liaocao_img_path: 料槽图片路径 (用于 liaocao 模型)
            
        Returns:
            dict: 包含三个模型的检测结果
        """
        results = {
            "paoliao": {"detected": False, "score": 0.0},
            "liuchan": {"detected": False, "score": 0.0},
            "liaocao": {"detected": False, "score": 0.0}
        }

        # 1. 对 'yichang.jpeg' 运行两个模型：跑料 & 流产
        if os.path.exists(yichang_img_path):
            print(f"--- Inferencing on {os.path.basename(yichang_img_path)} ---")
            if self.model_paoliao:
                results["paoliao"] = self.model_paoliao.infer(yichang_img_path)
            
            if self.model_liuchan:
                results["liuchan"] = self.model_liuchan.infer(yichang_img_path)
        else:
            print(f"[Error] File not found: {yichang_img_path}")

        # 2. 对 'liaocao.jpeg' 运行一个模型：料槽
        if os.path.exists(liaocao_img_path):
            print(f"--- Inferencing on {os.path.basename(liaocao_img_path)} ---")
            if self.model_liaocao:
                results["liaocao"] = self.model_liaocao.infer(liaocao_img_path)
        else:
            print(f"[Error] File not found: {liaocao_img_path}")
            
        return results

    def cleanup(self):
        """释放所有资源"""
        print("[System] Cleaning up inference resources...")
        if self.model_paoliao: self.model_paoliao.release()
        if self.model_liuchan: self.model_liuchan.release()
        if self.model_liaocao: self.model_liaocao.release()
        acl.finalize()