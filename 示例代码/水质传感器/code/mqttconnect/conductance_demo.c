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

#define TDS_ADC_CHANNEL                   ADC_CHANNEL_2  // TDS传感器的ADC通道
#define TDS_READ_TIMES                    10             // TDS传感器读取次数

/**
 * @brief 初始化 TDS 传感器的 ADC 模块
 */
void TDS_Init(void)
{
    osal_printk("Initializing TDS sensor ADC...\n");

    // 初始化 ADC 时钟
    if (uapi_adc_init(ADC_CLOCK_500KHZ) != ERRCODE_SUCC) {
        osal_printk("ADC initialization failed!\n");
        return;
    }

    // 启用 ADC 电源
    uapi_adc_power_en(AFE_GADC_MODE, true);

    // 打开指定的 ADC 通道
    if (uapi_adc_open_channel(TDS_ADC_CHANNEL) != ERRCODE_SUCC) {
        osal_printk("Failed to open ADC channel %d\n", TDS_ADC_CHANNEL);
        return;
    }

    osal_printk("TDS sensor ADC initialized successfully.\n");
}

/**
 * @brief 获取 TDS 传感器的原始 ADC 数据
 * @return 返回 ADC 平均值
 */
uint16_t TDS_GetData(void)
{
    uint32_t tempData = 0;
    uint16_t adc_value = 0;

    for (uint8_t i = 0; i < TDS_READ_TIMES; i++) {
        // 使用 adc_port_read 读取 ADC 值
        if (adc_port_read(TDS_ADC_CHANNEL, &adc_value) != ERRCODE_SUCC) {
            osal_printk("ADC read failed at sample %d!\n", i);
            return 0;
        }

        tempData += adc_value;
        osal_msleep(5); // 增加延时，确保采样稳定
    }

    // 计算平均值
    tempData /= TDS_READ_TIMES;
    return (uint16_t)tempData;
}

/**
 * @brief 获取 TDS 传感器的电导率值（PPM）
 * @return 返回计算后的 TDS 值（单位：PPM）
 */
float TDS_GetData_PPM(void)
{
    float tempData = 0;
    float TDS_DAT;

    for (uint8_t i = 0; i < TDS_READ_TIMES; i++) {
        tempData += TDS_GetData();
        osal_msleep(5);
    }

    tempData /= TDS_READ_TIMES;

    // 转换为电压值
    TDS_DAT = (tempData / 4096.0) * 3.3; // 假设参考电压为 3.3V

    // 根据标定公式计算 TDS 值（PPM）
    TDS_DAT = 66.71 * TDS_DAT * TDS_DAT * TDS_DAT - 127.93 * TDS_DAT * TDS_DAT + 428.7 * TDS_DAT;

    // 限制 TDS 值范围
    if (TDS_DAT < 20) TDS_DAT = 0;

    return TDS_DAT;
}