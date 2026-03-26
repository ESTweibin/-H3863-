#include "pinctrl.h"
#include "soc_osal.h"
#include "i2c.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "adc_porting.h"


#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "adc.h" // 引入 PH 传感器相关的头文件

#define I2C_MASTER_ADDR                   0x0
#define I2C_SET_BAUDRATE                  400000
#define I2C_MASTER_PIN_MODE               2

#define I2C_TASK_STACK_SIZE               0x1000
#define I2C_TASK_DURATION_MS              1000
#define I2C_TASK_PRIO                     (osPriority_t)(17)

#define CONFIG_I2C_MASTER_BUS_ID          1
#define CONFIG_I2C_SCL_MASTER_PIN         15
#define CONFIG_I2C_SDA_MASTER_PIN         16
#define PH_ADC_CHANNEL                    ADC_CHANNEL_1  // PH传感器的ADC通道
#define PH_READ_TIMES                     10             // PH传感器读取次数

/**
 * @brief 初始化PH传感器的ADC模块
 */
void PH_Init(void)
{
    osal_printk("Initializing PH sensor ADC...\n");

    // 初始化 ADC 时钟
    if (uapi_adc_init(ADC_CLOCK_500KHZ) != ERRCODE_SUCC) {
        osal_printk("ADC initialization failed!\n");
        return;
    }

    // 启用 ADC 电源
    uapi_adc_power_en(AFE_GADC_MODE, true);

    // 打开指定的 ADC 通道
    if (uapi_adc_open_channel(PH_ADC_CHANNEL) != ERRCODE_SUCC) {
        osal_printk("Failed to open ADC channel %d\n", PH_ADC_CHANNEL);
        return;
    }

    osal_printk("PH sensor ADC initialized successfully.\n");
}

/**
 * @brief 读取PH传感器的PH值
 * @return 返回计算后的PH值
 */
float PH_GetData(void)
{
    float tempData = 0;
    float PH_DAT;

    uint16_t adc_value = 0;

    osal_printk("Starting PH data acquisition...\n");

    for (uint8_t i = 0; i < PH_READ_TIMES; i++) 
    {
        // 使用 adc_port_read 读取 ADC 值
        if (adc_port_read(PH_ADC_CHANNEL, &adc_value) != ERRCODE_SUCC) {
            osal_printk("ADC read failed at sample %d!\n", i);
            return 0.0;
        }

        osal_printk("ADC Value [%d]: %d\n", i, adc_value); // 打印每次采样值
        tempData += adc_value;
        osal_msleep(10); // 增加延时，确保采样稳定
    }

    // 计算平均值
    tempData /= PH_READ_TIMES;
    osal_printk("Average ADC Value: %.2f\n", tempData); // 打印平均值

    // 转换为电压值
    PH_DAT = (tempData / 4096.0) * 3.3; // 假设参考电压为 3.3V
    osal_printk("Voltage: %.2f V\n", PH_DAT);

    // 根据标定公式计算 PH 值
    PH_DAT = -5.7541 * PH_DAT + 14.654;

    // 限制 PH 值范围
    if (PH_DAT > 14.0) PH_DAT = 14.0;
    if (PH_DAT < 0) PH_DAT = 0.0;

    osal_printk("Calculated PH Value: %.2f\n", PH_DAT); // 打印最终 PH 值

    return PH_DAT;
}

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
//  * @brief I2C 主任务，显示 PH 数据
//  */
// static void *i2c_master_task(const char *arg)
// {
//     UNUSED(arg);

//     // 初始化 I2C 和 PH 传感器
//     app_i2c_init_pin();
//     uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID, I2C_SET_BAUDRATE, I2C_MASTER_ADDR);
//     PH_Init();

//     // 初始化 OLED
//     ssd1306_Init();
//     ssd1306_Fill(Black);

//     while (1) {
//         // 获取 PH 数据
//         float ph_value = PH_GetData();

//         // 在 OLED 上显示 PH 数据
//         ssd1306_Fill(Black);
//         ssd1306_SetCursor(0, 0);
//         ssd1306_DrawString("PH Sensor Data:", Font_7x10, White);

//         char ph_str[32];
//         snprintf(ph_str, sizeof(ph_str), "PH: %.2f", ph_value);
//         ssd1306_SetCursor(0, 20);
//         ssd1306_DrawString(ph_str, Font_7x10, White);

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