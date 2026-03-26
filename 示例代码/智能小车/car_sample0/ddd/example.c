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
#include "bmi270_init.h"


// FIFO数据处理主任务
static void *bmi_task(const char *arg)
{
    (void)arg;  // 避免未使用参数警告

    int8_t rslt;
    
    
    bmi2_delay_us(2000,NULL);  // 等待系统稳定  
    // 配置I2C引脚
    uapi_pin_set_mode(15, 2);   // I2C1_SDA
    uapi_pin_set_mode(16, 2);   // I2C1_SCL
    
    // 初始化I2C
    uapi_i2c_master_init(I2C_BUS_1, 400000, 0);

    rslt = bmi270_full_fill_header_mode_init();
    bmi2_error_codes_print_result(rslt);

    rslt = get_bmi270_sensor_data(5);
    bmi2_error_codes_print_result(rslt);

    osal_printk("BMI Task: BMI270功能测试完成\n");
    return NULL;
}


static void bmi_entry(void)
{
    osal_printk("bmi_entry: 开始创建BMI270任务\n");
    
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    
    /* 创建任务，使用更大的栈空间和合适的优先级 */
    task_handle = osal_kthread_create((osal_kthread_handler)bmi_task, 0, "BmiTask", 0x4000);
    if (task_handle != NULL) {
        osal_printk("bmi_entry: 任务创建成功\n");
        /* 设置合适的优先级 */
        osal_kthread_set_priority(task_handle, 22);
        osal_kfree(task_handle);
    } else {
        osal_printk("bmi_entry: 任务创建失败\n");
    }
    osal_kthread_unlock();

    osal_printk("bmi_entry: 函数执行完成\n");
}

/* 注册在系统启动时运行 */
app_run(bmi_entry);
 