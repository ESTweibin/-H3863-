#include "ph_demo.h"
#include "tem_demo.h"
#include "turbidity_demo.h"
#include "conductance_demo.h"
#include "uart.h"
#include "soc_osal.h"
#include "pinctrl.h"
#include "app_init.h"
#include <stdio.h>
#include <string.h>

// =================================================================================
// =================== 请根据您的硬件修改以下引脚配置 ===================
// =================================================================================
#define CONFIG_UART_BUS_ID 0 // 指定使用UART0，如需UART1请改为1
#define CONFIG_UART_TXD_PIN 9 // <<<--- 修改这里: 设置正确的TXD引脚编号
#define CONFIG_UART_TXD_PIN_MODE PIN_MODE_1 // <<<--- 修改这里: 设置正确的引脚模式 (例如 PIN_MODE_1)
#define CONFIG_UART_RXD_PIN 10 // <<<--- 修改这里: 设置正确的RXD引脚编号
#define CONFIG_UART_RXD_PIN_MODE PIN_MODE_1 // <<<--- 修改这里: 设置正确的引脚模式 (例如 PIN_MODE_1)
// =================================================================================

#define UART_SEND_INTERVAL_MS 5000
#define UART_SEND_BUFFER_SIZE 256
#define UART_BAUDRATE 115200
#define UART_TASK_STACK_SIZE 0x1000
#define UART_TASK_PRIO 24

static void app_uart_init_pin(void)
{
#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    uapi_pin_set_ie(CONFIG_UART_RXD_PIN, PIN_IE_1);
#endif
    uapi_pin_set_mode(CONFIG_UART_TXD_PIN, CONFIG_UART_TXD_PIN_MODE);
    uapi_pin_set_mode(CONFIG_UART_RXD_PIN, CONFIG_UART_RXD_PIN_MODE);
}

static void app_uart_init_config(void)
{
    uart_attr_t attr = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE
    };

    uart_pin_config_t pin_config = {
        .tx_pin = CONFIG_UART_TXD_PIN,
        .rx_pin = CONFIG_UART_RXD_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE
    };

    uapi_uart_deinit(CONFIG_UART_BUS_ID);
    uapi_uart_init(CONFIG_UART_BUS_ID, &pin_config, &attr, NULL, NULL);
}

static void send_sensor_data_via_uart(void)
{
    float ph = PH_GetData();
    float temp = TEMP_GetData();
    float tds = TDS_GetData_PPM();
    float ts = TS_GetData();
    char send_buf[UART_SEND_BUFFER_SIZE];
    int len = snprintf(send_buf, sizeof(send_buf),
        "{\"services\": [{\"service_id\": \"ws63\", \"properties\": {\"temp\": %.2f, \"ph\": %.2f, \"TDS\": %.2f, \"TS\": %.2f}}]}",
        temp, ph, tds, ts);
    if (len > 0 && len < UART_SEND_BUFFER_SIZE) {
        uapi_uart_write(CONFIG_UART_BUS_ID, (uint8_t*)send_buf, len, 0);
        uapi_uart_write(CONFIG_UART_BUS_ID, (uint8_t*)"\r\n", 2, 0); // 发送结束符
    }
}

static void *uart_task(const char *arg)
{
    unused(arg); // 使用 unused 宏避免编译器产生“未使用参数”的警告

    app_uart_init_pin();
    app_uart_init_config();

    while (1) {
        send_sensor_data_via_uart();
        osal_msleep(UART_SEND_INTERVAL_MS);
    }

    return NULL;
}

static void uart_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)uart_task, 0, "UartTask", UART_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, UART_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the uart_entry. */
app_run(uart_entry);