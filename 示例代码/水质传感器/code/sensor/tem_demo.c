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

// // I2C 和 OLED 相关定义
// #define I2C_MASTER_ADDR                   0x0
// #define I2C_SET_BAUDRATE                  400000
// #define I2C_MASTER_PIN_MODE               2
// #define I2C_TASK_STACK_SIZE               0x1000
// #define I2C_TASK_DURATION_MS              1000
// #define I2C_TASK_PRIO                     (osPriority_t)(17)
// #define CONFIG_I2C_MASTER_BUS_ID          1
// #define CONFIG_I2C_SCL_MASTER_PIN         15
// #define CONFIG_I2C_SDA_MASTER_PIN         16

// /**
//  * @brief 初始化 I2C 引脚
//  */
// static void app_i2c_init_pin(void)
// {
//     uapi_pin_set_mode(CONFIG_I2C_SCL_MASTER_PIN, PIN_MODE_2);
//     uapi_pin_set_mode(CONFIG_I2C_SDA_MASTER_PIN, PIN_MODE_2);       
//     uapi_pin_set_pull(CONFIG_I2C_SCL_MASTER_PIN, PIN_PULL_TYPE_UP);
//     uapi_pin_set_pull(CONFIG_I2C_SDA_MASTER_PIN, PIN_PULL_TYPE_UP);
// }

// /**
//  * @brief I2C 主任务，显示温度数据
//  */
// static void *i2c_master_task(const char *arg)
// {
//     UNUSED(arg);

//     // 初始化 I2C 和温度传感器
//     app_i2c_init_pin();
//     uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID, I2C_SET_BAUDRATE, I2C_MASTER_ADDR);
//     TEMP_Init();

//     // 初始化 OLED
//     ssd1306_Init();
//     ssd1306_Fill(Black);

//     while (1) {
//         // 获取温度数据
//         float temp_value = TEMP_GetData();

//         // 在 OLED 上显示温度数据
//         ssd1306_Fill(Black);
//         ssd1306_SetCursor(0, 0);
//         ssd1306_DrawString("Temperature:", Font_7x10, White);

//         char temp_str[32];
//         snprintf(temp_str, sizeof(temp_str), "%.2f C", temp_value);
//         ssd1306_SetCursor(0, 20);
//         ssd1306_DrawString(temp_str, Font_7x10, White);

//         ssd1306_UpdateScreen();

//         // 延时 1 秒
//         osal_msleep(I2C_TASK_DURATION_MS);
//     }

//     return NULL;
// }

// /**
//  * @brief I2C 主任务入口
//  */
// static void i2c_master_entry(void)
// {
//     osThreadAttr_t attr;

//     attr.name = "I2cMasterTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = I2C_TASK_STACK_SIZE;
//     attr.priority = I2C_TASK_PRIO;

//     if (osThreadNew((osThreadFunc_t)i2c_master_task, NULL, &attr) == NULL) {
//         osal_printk("Failed to create I2C master task.\n");
//     }
// }

// /* Run the i2c_master_entry. */
// app_run(i2c_master_entry);


