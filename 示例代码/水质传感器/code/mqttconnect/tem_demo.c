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

#define TEMP_ADC_CHANNEL                  ADC_CHANNEL_1  // PT1000温度传感器的ADC通道
#define TEMP_READ_TIMES                   20             // 温度传感器读取次数

/**
 * @brief 初始化温度传感器的ADC模块
 */
void TEMP_Init(void)
{
    osal_printk("Initializing Temperature sensor ADC...\n");

    // 初始化 ADC 时钟
    if (uapi_adc_init(ADC_CLOCK_500KHZ) != ERRCODE_SUCC) {
        osal_printk("ADC initialization failed!\n");
        return;
    }

    // 启用 ADC 电源
    uapi_adc_power_en(AFE_GADC_MODE, true);

    // 打开指定的 ADC 通道
    if (uapi_adc_open_channel(TEMP_ADC_CHANNEL) != ERRCODE_SUCC) {
        osal_printk("Failed to open ADC channel %d\n", TEMP_ADC_CHANNEL);
        return;
    }

    osal_printk("Temperature sensor ADC initialized successfully.\n");
}

/**
 * @brief 读取温度传感器的温度值
 * @return 返回计算后的温度值（摄氏度）
 */
float TEMP_GetData(void)
{
    float tempData = 0;
    float temp_value;
    uint16_t adc_value = 0;

    osal_printk("Starting temperature data acquisition...\n");

    // 多次采样取平均值
    for (uint8_t i = 0; i < TEMP_READ_TIMES; i++) 
    {
        if (adc_port_read(TEMP_ADC_CHANNEL, &adc_value) != ERRCODE_SUCC) {
            osal_printk("ADC read failed at sample %d!\n", i);
            return 0.0;
        }
        
        tempData += adc_value;
        osal_msleep(10); // 确保采样稳定
    }

    // 计算平均值
    tempData /= TEMP_READ_TIMES;
    
    // 转换为电压值（3.3V参考电压）
    float voltage = (tempData / 4096.0) * 3.3;
    
    // 将电压值转换为温度值
    // 这里使用你原来的转换公式，但可能需要根据实际情况调整系数
    temp_value = voltage * 46.0; // 根据实际标定调整系数

    osal_printk("ADC Value: %.2f\n", tempData);
    osal_printk("Voltage: %.2f V\n", voltage);
    osal_printk("Temperature: %.2f °C\n", temp_value);

    return temp_value;
}

