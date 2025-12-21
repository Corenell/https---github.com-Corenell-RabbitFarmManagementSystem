## YOLO 模型转换指南：PyTorch (.pt) → OrangePi Kunpeng Pro (.om)

本指南详细说明了如何将 YOLO 系列模型（PyTorch `.pt` 格式）转换为适配 OrangePi Kunpeng Pro（搭载 Ascend 310B4 NPU）的离线推理模型（`.om` 格式）。

## 🗓 转换流程概览

整个转换过程分为两个主要阶段：
1.  **PC 端**：将 `.pt` 权重文件导出为标准 `.onnx` 格式。
2.  **开发板端**：将 `.onnx` 文件转换为昇腾 NPU 专用的 `.om` 格式。

---

## 🛠 步骤一：导出 ONNX 模型 (PC端)

在安装了 YOLO 环境（Ultralytics）的 PC 或服务器上执行此步骤。

假设你的原始模型文件名为 `liuchan.pt`，请运行以下命令：

```bash
# 导出模型
yolo export model=path/to/liuchan.pt format=onnx opset=11 simplify=True dynamic=False
```
✅ 结果：转换完成后，当前目录下会生成 liuchan.onnx 文件。

## 步骤二：环境连接与文件传输
1. 连接 OrangePi
确保 OrangePi Kunpeng Pro 已连接网络。

    获取 IP：在开发板终端输入 ifconfig 或在路由器后台查询 IP 地址。

    SSH 登录：在 PC 端终端执行：

    ssh HwHiAiUser@<开发板IP地址>
    例如: ssh HwHiAiUser@192.168.1.100

2. 传输文件
使用 SFTP 客户端（如 FileZilla, MobaXterm）或 scp 命令，将生成的 liuchan.onnx 上传至开发板的工作目录。

3. 配置环境变量 (开发板端)
重要：在执行转换工具前，必须加载 CANN 工具包的环境变量。在开发板终端执行：
source /usr/local/Ascend/ascend-toolkit/set_env.sh

## ⚙️ 步骤三：转换为 OM 模型 (开发板端)
在开发板上使用 atc (Ascend Tensor Compiler) 工具进行最终转换。

请直接复制并运行以下命令：

```Bash
atc --model=liuchan.onnx \
    --framework=5 \
    --output=liuchan \
    --input_format=NCHW \
    --input_shape="images:1,3,640,640" \
    --log=error \
    --soc_version=Ascend310B4 \
    --op_select_implmode=high_precision
```
⏳ 注意：此步骤涉及图编译和算子优化，耗时较长（可能需要几分钟），请耐心等待。

✅ 结果验证
当终端显示 ATC run success 时，表示转换成功。当前目录下将生成目标文件：
```bash
liuchan.om
```

现在，该模型即可用于在 OrangePi Kunpeng Pro 上进行 NPU 加速推理。


## 模型推理开机自启动：
## 1 打开服务配置文件：
```bash
sudo nano /etc/systemd/system/yolo_web.service
```
## 2 编辑此文件（已经编辑好了，可以使用cat查看）
## 3 刷新配置并重启服务
### 1. 刷新配置
```bash
sudo systemctl daemon-reload
```
### 2. 重启服务
```bash
sudo systemctl restart yolo_web.service
```
### 3. 查看状态
```bash
sudo systemctl status yolo_web.service
```
### 4 如何查看程序的 print 输出？
由于程序在后台运行，你看不到屏幕打印。如果需要查看 print 的内容（比如推理结果、报错信息），请使用以下命令查看实时日志：
```bash
sudo journalctl -u yolo_web.service -f
```
临时停止：
```bash
sudo systemctl stop yolo_web.service
```
## 彻底停止 (禁止开机自启)：
### 1. 现在停止运行
```bash
sudo systemctl stop yolo_web.service
```

### 2. 禁用开机自启
```bash
sudo systemctl disable yolo_web.service
```

# 项目进展日志

### 2025年1月17日，早上5:10

刚刚完成了以下任务：
- 配置好了 **YOLOv8** 环境。
- 选择了适合的数据集工具，确保工具简单易用。

## 数据集结构要求

整理后的数据集结构如下：

```
dataset/
├── images/
│   ├── train/
│   │   ├── image1.jpg
│   │   └── image2.jpg
│   └── val/
│       ├── image3.jpg
│       └── image4.jpg
├── labels/
│   ├── train/
│   │   ├── image1.txt
│   │   └── image2.txt
│   └── val/
│       ├── image3.txt
│       └── image4.txt
```

### 标签文件格式

YOLO 的标签文件为 `.txt` 格式，与对应图片同名。标签内容格式如下：

```plaintext
class_id x_center y_center width height
```

- **class_id**: 目标类别的ID，血迹可用 `0` 表示。
- **x_center, y_center**: 目标的中心点坐标（归一化到 0~1）。
- **width, height**: 目标的宽和高（归一化到 0~1）。

#### 示例
如果 `image1.jpg` 中有一个血迹标注，则 `image1.txt` 的内容可能如下：

```plaintext
0 0.5 0.5 0.3 0.2
```

---

## 今日任务：运行 **Atlas 200 Developer Kit**

上午测试了 **Atlas 200 Developer Kit**，发现其相关的 **MindSpore** 生态目前仍不完善，文档中存在不少自相矛盾之处，让人困惑（尤其文档编写的逻辑，真是让人头疼）。

### 当前思路
计划使用以下流程：
1. 使用 **YOLOv8** 进行训练。
2. 从 **PyTorch** 导出 **ONNX** 格式。
3. 将 **ONNX** 转换为 **OM** 格式。

由于 **MindSpore** 工具链尚未完善，相比之下，**TensorFlow** 和 **PyTorch** 在工具支持上更加丰富，这样的处理流程对目前的需求更为合适。

---

### 设备与资源

考虑到协会的 **Atlas 200 DK** 都已经被使用，加上 **208+** 不再开放使用，且设备被锁在柜子里无法取出，手头资源有限。目前只有：
- 一个没有 **microSD 卡** 的 **i 创街**设备。
- 两个川哥用于人数识别的设备。

为了解决硬件问题，已购买了一个读卡器和 **128G SD 卡**，预计可以满足开发需求。

---

### 总结
未来几天将继续探索如何更有效地使用现有设备和工具链，以确保项目顺利进行。