#ifndef _BMI270_INIT_H
#define _BMI270_INIT_H

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
  #include "aux_bmp280.h"


typedef struct {
    float roll;         // 滚转角 (度)
    float pitch;        // 俯仰角 (度) 
    float yaw;          // 偏航角 (度)
    float roll_rate;    // 滚转角速度 (度/秒)
    float pitch_rate;   // 俯仰角速度 (度/秒)
    float yaw_rate;     // 偏航角速度 (度/秒)
} imu_data_t;

errcode_t bmi270_aux_full_fill_header_mode_init(void);

errcode_t get_bmi270_sensor_data(int8_t count);

void read_bmi270_imu_data_array(float *data_array, int array_size, int8_t N);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* _BMI270_INIT_H */