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
//  * @brief I2C 主任务，显示 TDS 数据
//  */
// static void *i2c_master_task(const char *arg)
// {
//     UNUSED(arg);

//     // 初始化 I2C 和 TDS 传感器
//     app_i2c_init_pin();
//     uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID,I2C_SET_BAUDRATE,I2C_MASTER_ADDR);
//     TDS_Init();

//     // 初始化 OLED
//     ssd1306_Init();
//     ssd1306_Fill(Black);

//     while (1) {
//         // 获取 TDS 数据
//         float tds_value = TDS_GetData_PPM();

//         // 在 OLED 上显示 TDS 数据
//         ssd1306_Fill(Black);
//         ssd1306_SetCursor(0, 0);
//         ssd1306_DrawString("TDS Sensor Data:", Font_7x10, White);

//         char tds_str[32];
//         snprintf(tds_str, sizeof(tds_str), "TDS: %.2f PPM", tds_value);
//         ssd1306_SetCursor(0, 20);
//         ssd1306_DrawString(tds_str, Font_7x10, White);

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