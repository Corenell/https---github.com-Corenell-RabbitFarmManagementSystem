#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import os
from ultralytics import YOLO

# ================= 配置区域 =================
# 1. 模型路径
MODEL_PATH = "/home/HwHiAiUser/rabbits/models/paoliao.pt"

# 2. 图片路径
IMAGE_PATH = "/home/HwHiAiUser/rabbits/images/liuchan2.jpeg"

# 3. 置信度阈值
CONF_THRESHOLD = 0.5
# ===========================================

def load_model():
    """加载模型并处理可能的加载错误"""
    if not os.path.exists(MODEL_PATH):
        print(f"错误：找不到模型文件 -> {MODEL_PATH}")
        return None
    try:
        print(f"正在加载模型 {MODEL_PATH} (CPU模式)...")
        return YOLO(MODEL_PATH)
    except Exception as e:
        print(f"模型加载失败: {e}")
        return None

def process_image(model, image_path, conf_threshold):
    """
    对输入图片进行检测，并打印详细置信度
    """
    if not os.path.exists(image_path):
        print(f"警告：找不到图片文件 -> {image_path}")
        return False

    try:
        # device='cpu': 强制使用 CPU
        # verbose=False: 关闭详细日志，防止刷屏
        results = model(image_path, conf=0.01, device='cpu', verbose=False)

        is_detected = False

        # 遍历推理结果（通常只有一张图，所以 results[0]）
        for result in results:
            # 检查是否有检测框
            if result.boxes is not None and len(result.boxes) > 0:
                # 遍历每一个检测到的框
                for box in result.boxes:
                    # 提取置信度 (Tensor转float)
                    score = float(box.conf[0])
                    # 提取类别ID (Tensor转int)
                    cls_id = int(box.cls[0])
                    # 获取类别名称 (如果有names映射)
                    cls_name = model.names[cls_id] if model.names else str(cls_id)

                    print(f"  -> [候选] 类别: {cls_name:<10} | 置信度: {score:.4f}", end="")

                    # 再次确认是否大于阈值（虽然 model推理时已经过滤了一次，这里双重保险）
                    if score > conf_threshold:
                        print(f"  -> [详细信息] 类别: {cls_name} (ID:{cls_id}) | 置信度: {score:.4f}")
                        is_detected = True
        
        return is_detected

    except Exception as e:
        print(f"推理过程中出错: {e}")
        return False

def main():
    # 1. 加载模型
    model = load_model()
    if model is None:
        return

    print("系统启动，开始监听...")
    print(f"当前监控图片: {IMAGE_PATH}")

    if process_image(model, IMAGE_PATH, CONF_THRESHOLD):
            # === 信号触发逻辑 ===
        print(f"[{time.strftime('%H:%M:%S')}] *** 信号触发：目标确认！***\n")
    else:
            # 如果没检测到，可以选择打印，也可以选择静默
        print(f"[{time.strftime('%H:%M:%S')}] 未检测到目标")
if __name__ == "__main__":
    main()