#include "attitude_estimation.h"
#include <math.h>
#include <stdio.h>

// 姿态解算相关全局变量
static attitude_t current_attitude = {0.0f, 0.0f, 0.0f};
static angular_rate_t current_angular_rate = {0.0f, 0.0f, 0.0f};

// 内部函数：从加速度计计算姿态角
static void calculate_attitude_from_accel(struct bmi2_sens_axes_data *acc_data, 
                                        float *roll_acc, float *pitch_acc)
{
    // 将原始数据转换为g值
    float acc_x_g = (float)acc_data->x * ACCEL_SCALE_8G;
    float acc_y_g = (float)acc_data->y * ACCEL_SCALE_8G;
    float acc_z_g = (float)acc_data->z * ACCEL_SCALE_8G;
    
    // 计算加速度计姿态角（弧度）
    float roll_rad = atan2f(acc_y_g, acc_z_g);
    float pitch_rad = atan2f(-acc_x_g, sqrtf(acc_y_g * acc_y_g + acc_z_g * acc_z_g));
    
    // 转换为度
    *roll_acc = roll_rad * RAD_TO_DEG;
    *pitch_acc = pitch_rad * RAD_TO_DEG;
}

// 内部函数：从陀螺仪计算角速度
static void calculate_angular_rate(struct bmi2_sens_axes_data *gyro_data,
                                 float *roll_rate, float *pitch_rate, float *yaw_rate)
{
    // 将原始数据转换为度/秒
    *roll_rate = (float)gyro_data->x * GYRO_SCALE_2000DPS;
    *pitch_rate = (float)gyro_data->y * GYRO_SCALE_2000DPS;
    *yaw_rate = (float)gyro_data->z * GYRO_SCALE_2000DPS;
}

// 内部函数：互补滤波器更新
static void complementary_filter_update(float roll_acc, float pitch_acc,
                                      float roll_rate, float pitch_rate, float yaw_rate)
{
    // 互补滤波器：融合加速度计和陀螺仪数据
    // 陀螺仪积分（高频特性好，但有漂移）
    current_attitude.roll += roll_rate * SAMPLE_PERIOD;
    current_attitude.pitch += pitch_rate * SAMPLE_PERIOD;
    current_attitude.yaw += (yaw_rate * YAW_SENSITIVITY) * SAMPLE_PERIOD;  // 应用灵敏度系数
    
    // 与加速度计数据融合（低频特性好，但有噪声）
    current_attitude.roll = COMPLEMENTARY_ALPHA * current_attitude.roll + 
                           (1.0f - COMPLEMENTARY_ALPHA) * roll_acc;
    current_attitude.pitch = COMPLEMENTARY_ALPHA * current_attitude.pitch + 
                            (1.0f - COMPLEMENTARY_ALPHA) * pitch_acc;
    
    // 偏航角只能通过陀螺仪积分（加速度计无法提供偏航角信息）
    // 这里可以后续添加磁力计融合
    
    // 更新角速度
    current_angular_rate.roll_rate = roll_rate;
    current_angular_rate.pitch_rate = pitch_rate;
    current_angular_rate.yaw_rate = yaw_rate * YAW_SENSITIVITY;  // 输出的角速度也应用灵敏度系数
    
    // 添加调试信息来观察Yaw值变化
    static int debug_count = 0;
    if (++debug_count % 50 == 0) {  // 每50帧打印一次
        printf("[Yaw Debug] 原始yaw_rate=%.3f°/s, 放大后=%.3f°/s, yaw=%.3f°, 增量=%.6f\n", 
               yaw_rate, yaw_rate * YAW_SENSITIVITY, current_attitude.yaw, 
               (yaw_rate * YAW_SENSITIVITY) * SAMPLE_PERIOD);
    }
    
    // 角度限制在合理范围内
    if (current_attitude.roll > 180.0f) current_attitude.roll -= 360.0f;
    if (current_attitude.roll < -180.0f) current_attitude.roll += 360.0f;
    if (current_attitude.pitch > 90.0f) current_attitude.pitch = 90.0f;
    if (current_attitude.pitch < -90.0f) current_attitude.pitch = -90.0f;
    if (current_attitude.yaw > 180.0f) current_attitude.yaw -= 360.0f;
    if (current_attitude.yaw < -180.0f) current_attitude.yaw += 360.0f;
}

// 公共函数：初始化姿态解算
void attitude_estimation_init(void)
{
    // 初始化姿态角为零
    current_attitude.roll = 0.0f;
    current_attitude.pitch = 0.0f;
    current_attitude.yaw = 0.0f;
    
    // 初始化角速度为零
    current_angular_rate.roll_rate = 0.0f;
    current_angular_rate.pitch_rate = 0.0f;
    current_angular_rate.yaw_rate = 0.0f;
    
    printf("姿态解算系统初始化完成\n");
}

// 公共函数：更新姿态解算
void attitude_estimation_update(struct bmi2_sens_axes_data *acc_data, 
                              struct bmi2_sens_axes_data *gyro_data)
{
    float roll_acc, pitch_acc;
    float roll_rate, pitch_rate, yaw_rate;
    
    // 计算加速度计姿态角
    calculate_attitude_from_accel(acc_data, &roll_acc, &pitch_acc);
    
    // 计算陀螺仪角速度
    calculate_angular_rate(gyro_data, &roll_rate, &pitch_rate, &yaw_rate);
    
    // 互补滤波器融合
    complementary_filter_update(roll_acc, pitch_acc, roll_rate, pitch_rate, yaw_rate);
    
    // 打印姿态信息
    // printf("加速度计角度: Roll=%.2f°, Pitch=%.2f°\n", roll_acc, pitch_acc);
    // printf("陀螺仪角速度: Roll=%.2f°/s, Pitch=%.2f°/s, Yaw=%.2f°/s\n", 
    //        roll_rate, pitch_rate, yaw_rate);
    // printf("融合姿态角度: Roll=%.2f°, Pitch=%.2f°, Yaw=%.2f°\n",
    //        current_attitude.roll, current_attitude.pitch, current_attitude.yaw);
    // printf("当前角速度: Roll=%.2f°/s, Pitch=%.2f°/s, Yaw=%.2f°/s\n",
    //        current_angular_rate.roll_rate, current_angular_rate.pitch_rate, 
    //        current_angular_rate.yaw_rate);
}

// 公共函数：获取当前姿态角度
attitude_t get_current_attitude(void)
{
    return current_attitude;
}

// 公共函数：获取当前角速度
angular_rate_t get_current_angular_rate(void)
{
    return current_angular_rate;
}
