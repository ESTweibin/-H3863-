#ifndef ATTITUDE_ESTIMATION_H
#define ATTITUDE_ESTIMATION_H

#include "bmi2.h"
#include "bmi270.h"

// 姿态解算相关宏定义
#define DEG_TO_RAD                  0.017453292f    // π/180
#define RAD_TO_DEG                  57.29577951f    // 180/π
#define ACCEL_SCALE_8G              (8.0f / 32768.0f)    // 8g量程转换
#define GYRO_SCALE_2000DPS          (2000.0f / 32768.0f) // 2000dps量程转换
#define SAMPLE_FREQ_HZ              400.0f          // 采样频率400Hz
#define SAMPLE_PERIOD               (1.0f / SAMPLE_FREQ_HZ)  // 采样周期
#define COMPLEMENTARY_ALPHA         0.98f           // 互补滤波器系数
#define YAW_SENSITIVITY             5.0f            // Yaw轴灵敏度系数，增加Yaw响应

// 姿态角结构体
typedef struct {
    float roll;     // 横滚角 (度)
    float pitch;    // 俯仰角 (度)
    float yaw;      // 偏航角 (度)
} attitude_t;

// 角速度结构体
typedef struct {
    float roll_rate;    // 横滚角速度 (度/秒)
    float pitch_rate;   // 俯仰角速度 (度/秒)
    float yaw_rate;     // 偏航角速度 (度/秒)
} angular_rate_t;

// 函数声明
void attitude_estimation_init(void);
void attitude_estimation_update(struct bmi2_sens_axes_data *acc_data, 
                              struct bmi2_sens_axes_data *gyro_data);
attitude_t get_current_attitude(void);
angular_rate_t get_current_angular_rate(void);

#endif // ATTITUDE_ESTIMATION_H
