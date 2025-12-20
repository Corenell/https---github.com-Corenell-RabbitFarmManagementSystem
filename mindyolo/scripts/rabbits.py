#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
示例脚本：使用 best.pt 模型每隔一段时间检测一张图片，
如果检测到目标置信度大于 0.8，则返回一个信号（此处打印触发信息）。

运行此脚本需要：
1. Python 3.6 或更高版本
2. 安装 ultralytics 包：pip install ultralytics
3. best.pt 模型文件
4. 图片文件
"""

import time
from ultralytics import YOLO

# 加载训练后得到的最佳权重模型 best.pt
model = YOLO("best.pt")

def process_image(image_path, conf_threshold=0.8):
    """
    对输入图片进行检测，如果存在置信度大于 conf_threshold 的检测结果则返回 True。
    
    :param image_path: 图片文件路径
    :param conf_threshold: 置信度阈值（默认为 0.8）
    :return: 如果检测到目标且置信度满足要求，则返回 True，否则返回 False
    """
    # 使用模型进行推理，此处传入的 conf 参数会过滤掉置信度低于该值的目标
    results = model(image_path, conf=conf_threshold)
    
    # 遍历每个推理结果
    for result in results:
        # 如果存在检测框，则表示至少有一个目标满足置信度要求
        if result.boxes is not None and len(result.boxes) > 0:
            return True
    return False

def main():
    # 这里假设每隔一段时间传入同一张图片，如 test.jpg
    # 如果你的图片是动态获取的（例如摄像头捕获），请替换此处逻辑
    image_path = r"E:\rabitstraining\NR8AVjViQ1FBeU16UTVNamMyTmpNa2dvaG5vNmJqSHchIQUAcXVuZ3o!.jpeg"
    
    while True:
        print(f"正在处理图片：{image_path}")
        if process_image(image_path, conf_threshold=0.8):
            # 当检测到目标且置信度大于 0.8 时触发信号（此处以打印信息模拟）
            print("信号触发：检测到高于 0.8 置信度的目标！")
            # 此处可以加入其他信号处理逻辑，例如调用回调函数、发送网络请求等
        else:
            print("未检测到高于 0.8 置信度的目标。")
        
        # 每隔 5 秒检测一次，可根据需要调整时间间隔
        time.sleep(5)

if __name__ == "__main__":
    main()
