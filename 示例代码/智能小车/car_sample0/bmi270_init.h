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

errcode_t bmi270_full_fill_header_mode_init(void);

errcode_t get_bmi270_sensor_data(int8_t count);

errcode_t get_bmi270_attitude_data(float *output_data);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* _BMI270_INIT_H */