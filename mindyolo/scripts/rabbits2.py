import time
import cv2
from ultralytics import YOLO
import numpy as np
from collections import deque
from threading import Thread

# 加载 YOLOv8 模型
model = YOLO("path/to/your/best.pt")

# 设置置信度阈值
CONFIDENCE_THRESHOLD = 0.8
ALERT_THRESHOLD = 3
TIME_WINDOW = 30  # 时间窗口（秒）

# 记录超过阈值的目标检测次数
detections = deque()  # 存储时间戳

# 发送警报函数
def send_alert():
    print("警报：目标检测次数超过阈值！")

# 处理单张图片的函数
def process_image(image):
    # 使用 YOLOv8 模型进行目标检测
    results = model(image)
    
    # 获取检测到的目标
    for result in results:
        boxes = result.boxes
        for box in boxes:
            confidence = box.conf[0].item()  # 获取置信度
            if confidence > CONFIDENCE_THRESHOLD:
                # 目标检测到，记录时间
                detections.append(time.time())

# 每隔五秒获取一张图片的函数
def capture_images():
    cap = cv2.VideoCapture(0)  # 0 是默认的摄像头
    while True:
        # 使用 OpenCV 从摄像头捕获一张图片（假设你有一个摄像头） 
        ret, frame = cap.read()
        if not ret:
            print("无法获取摄像头图像")
            continue
        
        # 处理这张图片
        process_image(frame)

        # 关闭摄像头
        cap.release()

        # 每 5 秒处理一次图片
        time.sleep(5)

# 检查是否触发警报的函数
def check_alert():
    while True:
        # 删除过期的检测记录，超过 TIME_WINDOW 的时间戳
        current_time = time.time()
        while detections and detections[0] < current_time - TIME_WINDOW:
            detections.popleft()
        
        # 如果在时间窗口内的检测次数超过阈值，则触发警报
        if len(detections) >= ALERT_THRESHOLD:
            send_alert()

        # 每秒检查一次
        time.sleep(1)

# 主函数，启动捕获和检查线程
def main():
    # 启动一个线程来捕获图片
    capture_thread = Thread(target=capture_images)
    capture_thread.daemon = True
    capture_thread.start()

    # 启动一个线程来检查警报
    alert_thread = Thread(target=check_alert)
    alert_thread.daemon = True
    alert_thread.start()

    # 保持主线程运行
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
