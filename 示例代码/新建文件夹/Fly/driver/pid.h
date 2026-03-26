#ifndef PID_H
#define PID_H

#include <stdint.h>

// PID参数结构体
typedef struct {
    float kp;  // 比例系数
    float ki;  // 积分系数
    float kd;  // 微分系数
} pidInit_t;

// PID对象结构体
typedef struct {
    float target;        // 目标值
    float temp_err;      // 当前误差
    float preError;      // 上一次误差
    float integ;         // 积分项
    float deriv;         // 微分项
    float out_P;         // 比例输出
    float out_I;         // 积分输出
    float out_D;         // 微分输出
    float i_limit_high;  // 积分上限
    float i_limit_low;   // 积分下限
    float dt;            // 采样时间间隔
    float kp;            // 比例系数
    float ki;            // 积分系数
    float kd;            // 微分系数
} PID_Object;

// 姿态和角速度结构体
typedef struct {
    float roll;
    float pitch;
    float yaw;
} attitude_t;

typedef struct {
    float X;
    float Y;
    float Z;
} Axis3f;

typedef struct {
    float roll_input;
    float pitch_input;
    float yaw_input;
} Contrl_t;

// 初始化PID控制器
void PID_Init(PID_Object* pid, const float target, const pidInit_t pidParam, const float dt);

// 更新PID控制器
float PID_Update(PID_Object* pid, const float temp_err);

// 角度环PID控制
void AnglePID(attitude_t* Angle_real, attitude_t* Angle_target, Axis3f* output_Rate, PID_Object* pidRoll, PID_Object* pidPitch, PID_Object* pidYaw);

// 角速度环PID控制
void RatePID(Axis3f* Rate_real, Axis3f* Rate_target, Contrl_t* output, PID_Object* pidRateX, PID_Object* pidRateY, PID_Object* pidRateZ);

#endif // PID_H