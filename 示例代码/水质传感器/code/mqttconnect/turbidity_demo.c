#include "pinctrl.h"
#include "soc_osal.h"
#include "i2c.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "adc_porting.h"
#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "adc.h"

#define I2C_MASTER_ADDR                   0x0
#define I2C_SET_BAUDRATE                  400000
#define I2C_MASTER_PIN_MODE               2

#define I2C_TASK_STACK_SIZE               0x1000
#define I2C_TASK_DURATION_MS              1000
#define I2C_TASK_PRIO                     (osPriority_t)(17)

#define CONFIG_I2C_MASTER_BUS_ID          1
#define CONFIG_I2C_SCL_MASTER_PIN         15
#define CONFIG_I2C_SDA_MASTER_PIN         16

#define TS_ADC_CHANNEL                    ADC_CHANNEL_3  // 浊度传感器的ADC通道
#define TS_READ_TIMES                     10             // 浊度传感器读取次数
#define TS_K                              350        // 浊度传感器标定系数

/**
 * @brief 初始化浊度传感器的 ADC 模块
 */
void TS_Init(void)
{
    osal_printk("Initializing Turbidity sensor ADC...\n");

    // 初始化 ADC 时钟
    if (uapi_adc_init(ADC_CLOCK_500KHZ) != ERRCODE_SUCC) {
        osal_printk("ADC initialization failed!\n");
        return;
    }

    // 启用 ADC 电源
    uapi_adc_power_en(AFE_GADC_MODE, true);

    // 打开指定的 ADC 通道
    if (uapi_adc_open_channel(TS_ADC_CHANNEL) != ERRCODE_SUCC) {
        osal_printk("Failed to open ADC channel %d\n", TS_ADC_CHANNEL);
        return;
    }

    osal_printk("Turbidity sensor ADC initialized successfully.\n");
}

/**
 * @brief 获取浊度传感器的浊度值
 * @return 返回计算后的浊度值
 */
float TS_GetData(void)
{
    float tempData = 0;
    float TS_DAT;

    uint16_t adc_value = 0;

    for (uint8_t i = 0; i < TS_READ_TIMES; i++) {
        // 使用 adc_port_read 读取 ADC 值
            adc_port_read(TS_ADC_CHANNEL, &adc_value);
        }

        tempData += adc_value;
        osal_msleep(5); // 增加延时，确保采样稳定

    // 计算平均值
    tempData /= TS_READ_TIMES;

    // 转换为电压值
    TS_DAT = (tempData / 4096.0) * 5; // 假设参考电压为 3.3V

    // 根据标定公式计算浊度值
    TS_DAT = -865.68 * TS_DAT + TS_K;

    // 限制浊度值范围
    if (TS_DAT < 35) TS_DAT = 0;

    return TS_DAT;
}
