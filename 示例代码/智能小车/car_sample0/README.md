# car_sample0 项目介绍

## 1. 项目定位

`car_sample0` 是一个运行在海思 MCU 平台上的小车控制示例工程，核心目标是：

- 在两路串口设备之间做控制与状态数据中转；
- 采集 BMI270 IMU（加速度计 + 陀螺仪）数据并完成姿态解算；
- 采集 ADC 电压并与运动状态一起打包上报。

简而言之，这是一个“**底层驱动 + 任务调度 + 传感器融合 + 通信桥接**”的综合样例。

---

## 2. 工程结构

```text
car_sample0/
├─ car_sample0.c              # 主业务任务：串口桥接 + IMU姿态 + 电压采集 + 上报
├─ bmi270_init.c/.h           # BMI270初始化、FIFO读取、姿态数据输出接口
├─ attitude_estimation.c/.h   # 姿态解算（互补滤波）
├─ common.c/.h                # BMI2接口适配层（I2C读写、延时、错误打印）
├─ bmi2.c/.h                  # Bosch BMI2通用驱动
├─ bmi270.c/.h                # Bosch BMI270专用驱动与配置文件
├─ bmi2_defs.h                # BMI2驱动定义与寄存器宏
├─ CMakeLists.txt             # 主工程源码清单
└─ ddd/
   ├─ example.c               # BMI270独立功能测试任务示例
   └─ CMakeLists.txt          # ddd子目录编译清单
```

---

## 3. 系统功能与数据流

### 3.1 主任务职责（`car_sample0.c`）

系统启动后通过 `app_run(car_sample0_entry)` 创建 `car_sample0_task` 线程，持续循环执行：

1. 从 UART0 接收上位机（SS928）下发的 4 字节 PWM；
2. 将 PWM 转发到 UART2（H21E）；
3. 调用 BMI270 接口读取姿态角与角速度；
4. 从 UART2 接收电机侧回传 RPM；
5. 读取 ADC 通道电压；
6. 将姿态 + 角速度 + RPM + 电压组包并发回 UART0。

### 3.2 BMI270 数据链路

- 初始化阶段（`bmi270_full_fill_header_mode_init`）：
  - 配置 I2C 接口与 BMI270；
  - 设置加速度计/陀螺仪量程、ODR、滤波参数；
  - 启用 FIFO（Header 模式 + 时间戳）；
  - 初始化姿态解算模块。
- 运行阶段（`get_bmi270_attitude_data`）：
  - 等待 FIFO 满中断；
  - 读取 FIFO，提取 accel/gyro 帧；
  - 逐帧喂给姿态解算；
  - 输出 6 维结果：
    - `roll, pitch, yaw`
    - `roll_rate, pitch_rate, yaw_rate`

### 3.3 姿态解算方法（`attitude_estimation.c`）

该模块采用互补滤波：

- **加速度计**提供低频稳定姿态参考（roll/pitch）；
- **陀螺仪**提供高频动态响应（角速度积分）；
- 通过 `COMPLEMENTARY_ALPHA` 进行融合，抑制漂移与噪声。

其中 yaw 主要来自陀螺仪积分（当前未融合磁力计）。

---

## 4. 关键外设与接口

- **I2C1**：连接 BMI270（SDA=15, SCL=16）；
- **UART0**：与 SS928 通信；
- **UART2**：与 H21E 通信；
- **ADC 通道 2**：电压采样。

串口收发使用 DMA 配置以降低 CPU 占用。

---

## 5. 任务与启动入口

- 主业务入口：`car_sample0.c` 中 `app_run(car_sample0_entry)`；
- 独立 BMI270 示例入口：`ddd/example.c` 中 `app_run(bmi_entry)`。

> 说明：通常只保留一个主入口参与构建，避免多个示例任务同时注册导致行为冲突。

---

## 6. 编译组织说明

- 根目录 `CMakeLists.txt` 已将主工程源码加入 `SOURCES`；
- `ddd/CMakeLists.txt` 是 BMI270 独立示例的源码清单；
- 当前主工程默认包含 `car_sample0.c`，`ddd/example.c` 处于示例用途。

---

## 7. 适用场景

该项目适合用于：

- 小车/机器人底盘的数据中转控制样板；
- BMI270 在 MCU 上的 FIFO + 姿态解算参考实现；
- 多外设协同（UART + I2C + ADC + DMA + RTOS 线程）的集成示例。

---

## 8. 后续可优化方向

- 增加串口协议帧头/校验（CRC）与异常重传；
- 将姿态解算参数（采样率、滤波系数）配置化；
- 为 yaw 引入磁力计或零偏在线校准；
- 为上报数据定义二进制协议，降低带宽与解析开销；
- 增加模块化日志等级，减少实时环路中 `printf` 影响。

---

## 9. 快速阅读建议（新同学）

建议按以下顺序阅读源码：

1. `car_sample0.c`（先看主循环在做什么）；
2. `bmi270_init.c`（看IMU如何初始化与取数）；
3. `attitude_estimation.c`（看姿态融合逻辑）；
4. `common.c`（看平台I2C适配）；
5. `bmi2.c` / `bmi270.c`（看底层驱动细节）。

这样可以最快建立“业务流程 → 数据来源 → 算法 → 驱动”的全局理解。