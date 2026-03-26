#include "pinctrl.h"
#include "soc_osal.h"
#include "i2c.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"

#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "ph_demo.h"
#include "conductance_demo.h"
#include "turbidity_demo.h"
#include "tem_demo.h"

#define I2C_MASTER_ADDR                   0x0
#define I2C_SET_BAUDRATE                  400000
#define I2C_MASTER_PIN_MODE               2

#define I2C_TASK_STACK_SIZE               0x1000
#define I2C_TASK_DURATION_MS              1000
#define I2C_TASK_PRIO                     (osPriority_t)(17)

#define CONFIG_I2C_MASTER_BUS_ID          1
#define CONFIG_I2C_SCL_MASTER_PIN         15
#define CONFIG_I2C_SDA_MASTER_PIN         16

/**
 * @brief 初始化 I2C 引脚
 */
static void app_i2c_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_I2C_SCL_MASTER_PIN, PIN_MODE_2);
    uapi_pin_set_mode(CONFIG_I2C_SDA_MASTER_PIN, PIN_MODE_2);       
    uapi_pin_set_pull(CONFIG_I2C_SCL_MASTER_PIN, PIN_PULL_TYPE_UP);
    uapi_pin_set_pull(CONFIG_I2C_SDA_MASTER_PIN, PIN_PULL_TYPE_UP);
}

/**
 * @brief I2C 主任务，显示 PH 数据
 */
static void *i2c_master_task(const char *arg)
{
    UNUSED(arg);

    // 初始化 I2C 和传感器
    app_i2c_init_pin();
    uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID, I2C_SET_BAUDRATE, I2C_MASTER_ADDR);
    PH_Init();
    TS_Init();
    TDS_Init();
    TEMP_Init();

    // 初始化 OLED
    ssd1306_Init();
    osal_msleep(100);  // 等待OLED初始化完成

    while (1) {
        // 清屏
        ssd1306_Fill(Black);
        osal_msleep(10);  // 等待清屏完成

        // 获取并显示 PH 值
        float ph_value = PH_GetData();
        char ph_str[16];  // 减小缓冲区大小
        snprintf(ph_str, sizeof(ph_str), "PH:%.2f", ph_value);  // 简化显示文本
        ssd1306_SetCursor(0, 0);
        ssd1306_DrawString(ph_str, Font_7x10, White);
        osal_msleep(10);  // 等待显示更新

        // 获取并显示 TDS 值
        float tds_value = TDS_GetData_PPM();
        char tds_str[16];
        snprintf(tds_str, sizeof(tds_str), "TDS:%.2f", tds_value);  // 不显示小数点
        ssd1306_SetCursor(0, 16);  // 调整显示位置
        ssd1306_DrawString(tds_str, Font_7x10, White);
        osal_msleep(10);

        // 获取并显示浊度值
        float ts_value = TS_GetData();        
        char ts_str[32];
        snprintf(ts_str, sizeof(ts_str), "NTU:%.2f", ts_value);
        ssd1306_SetCursor(0, 32);  // 调整显示位置
        ssd1306_DrawString(ts_str, Font_7x10, White);
        osal_msleep(10);

        // 获取并显示浊度值
        float temp_value = TEMP_GetData();        
        char temp_str[32];
        snprintf(temp_str, sizeof(temp_str), "TEMP:%.2f", temp_value);
        ssd1306_SetCursor(0, 48);  // 调整显示位置
        ssd1306_DrawString(temp_str, Font_7x10, White);
        
       // 更新显示
        ssd1306_UpdateScreen();
        osal_msleep(1000);  // 显示更新间隔
    }

    return NULL;
}

/**
 * @brief I2C 主任务入口
 */
static void i2c_master_entry(void)
{
    osThreadAttr_t attr;

    attr.name = "I2cMasterTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = I2C_TASK_STACK_SIZE;
    attr.priority = I2C_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)i2c_master_task, NULL, &attr) == NULL) {
        osal_printk("Failed to create I2C master task.\n");
    }
}

/* Run the i2c_master_entry. */
app_run(i2c_master_entry);



