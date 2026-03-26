#ifndef PCA9685SERVO_H
#define PCA9685SERVO_H

#include "pinctrl.h"
#include "soc_osal.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"


/**
 * @brief 初始化PCA9685舵机驱动所需的I2C和模块。
 * 
 * @return errcode_t ERRCODE_SUCC 表示成功，其他表示失败。
 */
errcode_t Pca9685Servo_GlobalInit(void);

/**
 * @brief 设置指定通道舵机的角度。
 * 
 * @param channel 舵机通道号 (0-15)。
 * @param angle 目标角度 (0-180 度)。
 */
void Pca9685Servo_SetAngle(uint8_t channel, uint8_t angle);


errcode_t Pca9685Servo_ReleaseChannel(uint8_t channel); // <-- 新增：释放舵机通道
#endif




