#ifndef _AUX_BMP280_H
#define _AUX_BMP280_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <math.h>
#include "cmsis_os2.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "app_init.h"
#include "pinctrl.h"
#include "i2c.h"
#include "pinctrl.h"
#include "gpio.h"

// BMI270驱动头文件
#include "bmi2.h"
#include "bmi270.h"
#include "common.h"

// BMP280 I2C地址和寄存器定义
#define BMP280_I2C_ADDR             0x76
#define BMP280_REG_CHIP_ID          0xD0
#define BMP280_REG_RESET            0xE0
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_PRESS_MSB        0xF7
#define BMP280_REG_TEMP_MSB         0xFA
#define BMP280_REG_CALIB_START      0x88
#define BMP280_CALIB_DATA_SIZE      24
#define BMP280_CHIP_ID_VALUE        0x58


// BMP280校准参数结构体
typedef struct {
    unsigned short dig_T1;
    short dig_T2, dig_T3;
    unsigned short dig_P1;
    short dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
} bmp280_calib_param_t;

int8_t aux_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr);

int8_t aux_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr);

// 初始化BMP280（通过辅助I2C）
errcode_t init_bmp280(bmp280_calib_param_t *bmp_calib, struct bmi2_dev *bmi270_dev);

// 处理BMP280辅助数据并补偿
void process_bmp280_aux_data(bmp280_calib_param_t bmp_calib, struct bmi2_aux_fifo_data *aux_data, int count);



#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* _AUX_BMP280_H */
