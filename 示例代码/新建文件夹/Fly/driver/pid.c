#include "pid.h"

// 初始化PID控制器
void PID_Init(PID_Object* pid, const float target, const pidInit_t pidParam, const float dt) {
    pid->temp_err = 0;
    pid->preError = 0;
    pid->integ = 0;
    pid->deriv = 0;
    pid->target = target;
    pid->kp = pidParam.kp;
    pid->ki = pidParam.ki;
    pid->kd = pidParam.kd;
    pid->i_limit_high = 100.0f;  // 默认积分上限
    pid->i_limit_low = -100.0f; // 默认积分下限
    pid->dt = dt;
}

// 更新PID控制器
float PID_Update(PID_Object* pid, const float temp_err) {
    float output;

    pid->temp_err = temp_err;
    pid->integ += pid->temp_err * pid->dt;

    // 积分限幅
    if (pid->integ > pid->i_limit_high) {
        pid->integ = pid->i_limit_high;
    } else if (pid->integ < pid->i_limit_low) {
        pid->integ = pid->i_limit_low;
    }

    pid->deriv = (pid->temp_err - pid->preError) / pid->dt;

    pid->out_P = pid->kp * pid->temp_err;
    pid->out_I = pid->ki * pid->integ;
    pid->out_D = pid->kd * pid->deriv;

    output = pid->out_P + pid->out_I + pid->out_D;

    pid->preError = pid->temp_err;

    return output;
}

// 角度环PID控制
void AnglePID(attitude_t* Angle_real, attitude_t* Angle_target, Axis3f* output_Rate, PID_Object* pidRoll, PID_Object* pidPitch, PID_Object* pidYaw) {
    output_Rate->X = PID_Update(pidRoll, Angle_target->roll - Angle_real->roll);
    output_Rate->Y = PID_Update(pidPitch, Angle_target->pitch - Angle_real->pitch);

    float YawError = Angle_target->yaw - Angle_real->yaw;
    if (YawError > 180.0f) {
        YawError -= 360.0f;
    } else if (YawError < -180.0f) {
        YawError += 360.0f;
    }
    output_Rate->Z = PID_Update(pidYaw, YawError);
}

// 角速度环PID控制
void RatePID(Axis3f* Rate_real, Axis3f* Rate_target, Contrl_t* output, PID_Object* pidRateX, PID_Object* pidRateY, PID_Object* pidRateZ) {
    output->roll_input = PID_Update(pidRateX, Rate_target->X - Rate_real->X);
    output->pitch_input = PID_Update(pidRateY, Rate_target->Y - Rate_real->Y);
    output->yaw_input = PID_Update(pidRateZ, Rate_target->Z - Rate_real->Z);
}