# ******************************************************************************
#  Copyright (c) 2024 Orbbec 3D Technology, Inc
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# ******************************************************************************
import time

import cv2
import numpy as np

from pyorbbecsdk import *
from detect import YOLO11_Detect, draw_detection, class_names  # 新增：导入 YOLO 相关模块
from utils import frame_to_bgr_image  # 新增：用于彩色帧转图像

ESC_KEY = 27
PRINT_INTERVAL = 1  # seconds
MIN_DEPTH = 20  # 20mm
MAX_DEPTH = 10000  # 10000mm

# 新增：YOLO 模型配置
MODEL_PATH = "models/yolo11n_detect_bayese_640x640_nv12.bin"
CONF_THRESHOLD = 0.25
IOU_THRESHOLD = 0.45
yolo_model = YOLO11_Detect(MODEL_PATH, CONF_THRESHOLD, IOU_THRESHOLD)  # 初始化 YOLO 模型

class TemporalFilter:
    def __init__(self, alpha):
        self.alpha = alpha
        self.previous_frame = None

    def process(self, frame):
        if self.previous_frame is None:
            result = frame
        else:
            result = cv2.addWeighted(frame, self.alpha, self.previous_frame, 1 - self.alpha, 0)
        self.previous_frame = result
        return result


def main():
    config = Config()
    pipeline = Pipeline()
    temporal_filter = TemporalFilter(alpha=0.5)

    #新增：相机内参变量（假设已获取，与之前修改一致）
    fx, fy, cx, cy = 0.0, 0.0, 0.0, 0.0


    try:
        # 新增：启用彩色传感器（原仅启用深度）
        depth_profile_list = pipeline.get_stream_profile_list(OBSensorType.DEPTH_SENSOR)
        color_profile_list = pipeline.get_stream_profile_list(OBSensorType.COLOR_SENSOR)
        depth_profile = depth_profile_list.get_default_video_stream_profile()
        color_profile = color_profile_list.get_default_video_stream_profile()
        config.enable_stream(depth_profile)
        config.enable_stream(color_profile)  # 新增：启用彩色流

        # 新增：获取深度摄像头内参（焦距和光心）
        intrinsic = depth_profile.get_intrinsic()  # 假设SDK提供此方法获取内参
        fx, fy = intrinsic.fx, intrinsic.fy  # 焦距（像素单位）
        cx, cy = intrinsic.cx, intrinsic.cy  # 光心（像素坐标）

    except Exception as e:
        print(e)
        return
    pipeline.start(config)
    last_print_time = time.time()
    while True:
        try:
            frames = pipeline.wait_for_frames(100)
            if frames is None:
                continue
            # 新增：获取彩色帧和深度帧
            color_frame = frames.get_color_frame()
            depth_frame = frames.get_depth_frame()
            if not color_frame or not depth_frame:  # 原仅检查深度帧，现需同时检查彩色帧
                continue

            # 处理彩色帧（用于检测）
            color_image = frame_to_bgr_image(color_frame)  # 彩色帧转 BGR 图像
            input_tensor = yolo_model.bgr2nv12(color_image)  # 转换为 YOLO 输入格式
            outputs = yolo_model.c2numpy(yolo_model.forward(input_tensor))  # 模型推理
            ids, scores, bboxes = yolo_model.postProcess(outputs)  # 后处理获取检测结果

            # 处理深度帧（用于测距）
            # depth_format = depth_frame.get_format()
            # if depth_format != OBFormat.Y16:
            #     print("depth format is not Y16")
            #     continue
            # width = depth_frame.get_width()
            # height = depth_frame.get_height()
            # scale = depth_frame.get_depth_scale()
            # depth_data = np.frombuffer(depth_frame.get_data(), dtype=np.uint16).reshape((height, width))
            # depth_data = depth_data.astype(np.float32) * scale
            # depth_data = np.where((depth_data > MIN_DEPTH) & (depth_data < MAX_DEPTH), depth_data, 0)
            # depth_data = depth_data.astype(np.uint16)
            # depth_data = temporal_filter.process(depth_data)

            depth_format = depth_frame.get_format()
            if depth_format != OBFormat.Y16:
                print("depth format is not Y16")
                continue
            width = depth_frame.get_width()
            height = depth_frame.get_height()
            scale = depth_frame.get_depth_scale()  # 关键：深度值转实际距离的比例（单位：mm/pixel）
            if scale <=0:
                print("Invalid depth scale, skip")  # 新增：检查比例有效性
                continue
            depth_data = np.frombuffer(depth_frame.get_data(), dtype=np.uint16).reshape((height, width))
            depth_data = depth_data.astype(np.float32) * scale  # 转换为实际距离（mm）

            # 新增：空间滤波（中值滤波去噪，3x3核）
            depth_data = cv2.medianBlur(depth_data.astype(np.float32), 3)
            # 保留有效深度范围（原有逻辑）
            depth_data = np.where((depth_data > MIN_DEPTH) & (depth_data <MAX_DEPTH), depth_data, 0)
            # 时间滤波（原有逻辑）
            depth_data = temporal_filter.process(depth_data)

            # 新增：对每个检测目标计算距离并绘制
            # results = []
            # for class_id, score, bbox in zip(ids, scores, bboxes):
            #     x1, y1, x2, y2 = bbox
            #     # 计算目标中心点的深度值（注意：需确保 bbox 坐标在深度图范围内）
            #     center_x = int((x1 + x2) // 2)
            #     center_y = int((y1 + y2) // 2)
            #     if 0 <= center_x < width and 0 <= center_y < height:
            #         distance = depth_data[center_y, center_x]
            #         results.append((bbox, score, distance))
            #     else:
            #         distance = 0  # 超出深度图范围时设为 0

            # 新增：对每个检测目标计算距离并绘制
            # results=[]
            # for class_id, score, bbox in zip(ids, scores, bboxes):
            #     x1, y1, x2, y2 = bbox
            #     # 扩展采样区域（中心点周围3x3像素）
            #     center_x = int((x1 + x2) // 2)
            #     center_y = int((y1 + y2) // 2)
            #     # 定义采样区域边界（避免越界）
            #     x_start = max(0, center_x - 1)
            #     x_end = min(width - 1, center_x + 1)
            #     y_start = max(0, center_y - 1)
            #     y_end = min(height - 1, center_y + 1)
            #     # 提取区域内的深度值（排除0值）
            #     region_depths = depth_data[y_start:y_end + 1, x_start:x_end + 1]
            #     valid_depths = region_depths[region_depths >0]
            #     if len(valid_depths)>0:
            #         # 取中值（抗噪声更强）或平均值
            #         distance = np.median(valid_depths)  # 或 np.mean(valid_depths)
            #     else:
            #         distance = 0  # 无有效数据
            #     results.append((bbox, score, distance))

            # 新增：计算三维坐标并收集结果
            results = []
            for class_id, score, bbox in zip(ids, scores, bboxes):
                x1, y1, x2, y2 = bbox
                # 计算目标中心点（原有逻辑）
                center_x = int((x1 + x2) // 2)
                center_y = int((y1 + y2) // 2)
                # 扩展采样区域（原有逻辑）
                x_start = max(0, center_x - 1)
                x_end = min(width - 1, center_x + 1)
                y_start = max(0, center_y - 1)
                y_end = min(height - 1, center_y + 1)
                region_depths = depth_data[y_start:y_end + 1, x_start:x_end + 1]
                valid_depths = region_depths[region_depths > 0]
                # 计算距离（原有逻辑）
                distance = np.median(valid_depths) if len(valid_depths) > 0 else 0
                # 新增：计算三维坐标（单位：毫米→米）
                if distance > 0 and fx != 0 and fy != 0:
                    x = (center_x - cx) * distance / fx / 1000  # 转换为米
                    y = (center_y - cy) * distance / fy / 1000
                    z = distance / 1000
                else:
                    x, y, z = 0.0, 0.0, 0.0
                results.append((bbox, score, distance, (x, y, z)))  # 保存三维坐标

            # 新增：每隔1秒打印三维坐标和距离（英文）
            current_time = time.time()
            if current_time - last_print_time > PRINT_INTERVAL:
                for idx, (bbox, score, distance, (x, y, z)) in enumerate(results):
                    if distance > 0:
                        print(
                            f"Object {idx + 1}: Distance={distance:.1f}mm | 3D Coords: (X={x:.2f}m, Y={y:.2f}m, Z={z:.2f}m)")
                last_print_time = current_time




            # 新增：在彩色图像上绘制检测框和距离信息
                # 修改：显示距离和三维坐标（英文标签）
            for bbox, score, distance, (x, y, z) in results:
                draw_detection(color_image, bbox, score, 0)
                x1, y1, _, _ = bbox
                # 显示距离（毫米）和三维坐标（米）
                label = f"Dist: {distance:.1f}mm | X:{x:.2f}m Y:{y:.2f}m Z:{z:.2f}m"
                cv2.putText(color_image, label, (x1, y1 + 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)

            # 原：显示深度图像 → 改为显示彩色图像（含检测框和距离）
            cv2.imshow("Depth and Detection Viewer", color_image)

            # 检查退出键
            key = cv2.waitKey(1)
            if key == ord('q') or key == ESC_KEY:
                break
        except KeyboardInterrupt:
            break
    cv2.destroyAllWindows()
    pipeline.stop()


if __name__ == "__main__":
    main()