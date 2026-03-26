# 福寿螺定位系统 - 使用Gemini355深度相机

## 项目概述
本项目基于Gemini355深度相机实现福寿螺(Pomacea canaliculata)的单一目标定位，集成了目标识别与三维测距功能。系统通过YOLO11模型进行目标检测，并利用深度相机数据计算目标的三维坐标，适用于农业生态监测与有害生物防治场景。

## 环境配置要求
### 硬件要求
- Gemini355深度相机
- 兼容的计算平台(推荐RDK系列开发板)

### 软件依赖
- Python 3.x
- OpenCV
- NumPy
- pyorbbecsdk (深度相机SDK)
- hobot_dnn (BPU加速库)
- scipy

## 文件结构说明
- `detect.py`: YOLO11模型推理实现，负责图像预处理、模型前向传播和检测结果后处理
- `detect_final.py`: 主程序，集成相机数据流获取、目标检测与三维坐标计算
- `models/`: 存放YOLO模型文件(yolo11n_detect_bayese_640x640_nv12.bin)

## 使用方法
### 1. 准备模型文件
将YOLO11模型文件放置于`models/`目录下

### 2. 运行主程序
```bash
python detect_final.py
```

### 3. 程序控制
- 检测窗口显示彩色图像与实时检测结果
- 按'q'键或ESC键退出程序

## 核心功能说明
### 目标检测 (detect.py)
- 实现YOLO11检测模型的加载与推理
- 支持BGR到NV12格式转换
- 包含NMS非极大值抑制后处理
- 检测阈值配置：置信度阈值0.25，IOU阈值0.45

### 相机数据处理与三维定位 (detect_final.py)
- 深度相机数据流获取与同步
- 深度数据滤波处理(中值滤波+时间滤波)
- 目标中心点3x3区域深度采样，提高测距稳定性
- 三维坐标计算：
  ```python
  x = (center_x - cx) * distance / fx / 1000
  y = (center_y - cy) * distance / fy / 1000
  z = distance / 1000
  ```
- 实时显示检测框、距离(mm)和三维坐标(m)

## 输出信息
程序每秒打印一次检测结果，格式如下：
```
Object 1: Distance=XXX.Xmm | 3D Coords: (X=X.XXm, Y=Y.YYm, Z=Z.ZZm)
```

## 注意事项
- 确保深度相机驱动正确安装
- 模型文件路径需与代码中`MODEL_PATH`参数匹配
- 检测目标需在20mm-10000mm有效深度范围内