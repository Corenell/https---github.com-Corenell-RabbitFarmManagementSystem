
---

# 今天的困境

今天真的可以说是个“悲惨”的日子，完全没能顺利配置出 MindSpore 环境。整个过程卡在了从 `best.onnx` 转换为 `best.ms` 的环节，本来应该是简单的 YOLO 任务，但一碰到 MindSpore，整个人直接瘫痪了，真的是呜呜呜呜呜。

## 问题概述

我花了一整天的时间调试，面对成千上万的报错，心情可以说是非常糟糕。每次解决一个问题，另一个问题就浮现出来。要么是我的版本不对，要么是他的版本不兼容，完全没有交集，真的是把人逼疯了。最后，还得在 **Win11 + WSL2 + Ubuntu 22.04 + Conda** 的环境中，配置出一个合理的版本。

## 环境问题

虽然配置了相对合适的版本，但依然卡在了 **APU** 和 **GPU** 之间的转换问题上。CUDA（Compute Unified Device Architecture）和 CuDNN 是 NVIDIA GPU 的并行计算框架，而 CANN（Compute Architecture for Neural Networks）则是华为 NPU 的异构计算架构，这两个之间的差异也让我陷入了困境。

## 未来的计划

考虑到这些问题，我决定尝试使用 **英伟达的 Jetson Nano Developer Kit** 和 **TX2**，它们可能会更加适合我的需求。希望能在这些设备上顺利实现项目目标。

---
