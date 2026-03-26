#include "pinctrl.h"
#include "gpio.h"
#include "uart.h"
#include "i2c.h"
#include "adc.h"
#include "adc_porting.h"
#include "watchdog.h"
#include "soc_osal.h"
#include "app_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "dma.h"
#include "hal_dma.h"
#include "bmi2.h"
#include "bmi270.h"
#include "common.h"
#include "bmi270_init.h"
#include "common_def.h"
#include "tcxo.h"


#define UART_BAUDRATE                      115200  //波特率
#define CONFIG_UART_INT_WAIT_MS            5       //UART中断等待时间

#define CAR_SAMPLE0_TASK_PRIO              24      //任务优先级
#define CAR_SAMPLE0_TASK_STACK_SIZE        0x1000  //任务空间

#define I2C_SCL_MASTER_PIN                 16      //I2C SCL引脚
#define I2C_SDA_MASTER_PIN                 15      //I2C SDA引脚
#define I2C_MASTER_PIN_MODE                2       //I2C引脚模式

#define ADC_CHANNEL                        2

#define UART0_BUS_ID                       0       //UART0总线ID
#define UART0_TXD_PIN                      17      //UART0 TXD引脚
#define UART0_RXD_PIN                      18      //UART0 RXD引脚
#define UART0_TXD_PIN_MODE                 1       //UART0 TXD引脚模式
#define UART0_RXD_PIN_MODE                 1       //UART0 RXD引脚模式

#define UART2_BUS_ID                       2       //UART2总线ID
#define UART2_TXD_PIN                      8       //UART2 TXD引脚
#define UART2_RXD_PIN                      7       //UART2 RXD引脚
#define UART2_TXD_PIN_MODE                 2       //UART2 TXD引脚模式
#define UART2_RXD_PIN_MODE                 2       //UART2 RXD引脚模式

#define UART0_TRANSFER_SIZE                32      //UART0传输数据长度
#define UART0_RECEIVE_SIZE                 4       //UART0接收数据长度
#define UART2_TRANSFER_SIZE                4       //UART2传输数据长度
#define UART2_RECEIVE_SIZE                 4      //UART2接收数据长度


typedef struct {
    float angle_x, angle_y, angle_z, rate_x, rate_y, rate_z;
    float wheel0_rpm, wheel1_rpm, wheel2_rpm, wheel3_rpm;
    float voltage;
} ss928_frame_t;

static ss928_frame_t ss928_t;

typedef struct {
    int wheel0_rpm, wheel1_rpm, wheel2_rpm, wheel3_rpm;
} H21E_frame_t;

//UART接收和发送缓冲区
static uint8_t g_app_uart0_ss928_rx_buff[UART0_RECEIVE_SIZE] = { 0 };
// static uint8_t g_app_uart0_ss928_tx_buff[UART0_TRANSFER_SIZE] = { 0 };
static uint8_t g_app_uart2_rx_buff[UART2_RECEIVE_SIZE] = { 0 };
// static uint8_t g_app_uart2_tx_buff[UART2_TRANSFER_SIZE] = { 0 };


static uart_buffer_config_t uart0_buffer_config = {
    .rx_buffer = g_app_uart0_ss928_rx_buff,
    .rx_buffer_size = UART0_RECEIVE_SIZE
};
static uart_buffer_config_t uart2_buffer_config = {
    .rx_buffer = g_app_uart2_rx_buff,
    .rx_buffer_size = UART2_RECEIVE_SIZE
};

// static uart_write_dma_config_t g_uart_dma_928_cfg = {0};
static uart_write_dma_config_t g_uart_dma_928_cfg = {
    .src_width = HAL_DMA_TRANSFER_WIDTH_8,
    .dest_width = HAL_DMA_TRANSFER_WIDTH_8,
    .burst_length = HAL_DMA_BURST_TRANSACTION_LENGTH_4,
    .priority = HAL_DMA_CH_PRIORITY_0
};

static uart_write_dma_config_t g_app_dma_cfg = {
    .src_width = HAL_DMA_TRANSFER_WIDTH_8,
    .dest_width = HAL_DMA_TRANSFER_WIDTH_8,
    .burst_length = HAL_DMA_BURST_TRANSACTION_LENGTH_4,
    .priority = HAL_DMA_CH_PRIORITY_0
};

static void app_i2c_init_pin(void) {
    uapi_pin_set_mode(I2C_SCL_MASTER_PIN, I2C_MASTER_PIN_MODE);
    uapi_pin_set_mode(I2C_SDA_MASTER_PIN, I2C_MASTER_PIN_MODE);
}

static void app_uart_init_pin(void)
{
    uapi_pin_set_mode(UART0_TXD_PIN, UART0_TXD_PIN_MODE);
    uapi_pin_set_mode(UART0_RXD_PIN, UART0_RXD_PIN_MODE);
    uapi_pin_set_mode(UART2_TXD_PIN, UART2_TXD_PIN_MODE);
    uapi_pin_set_mode(UART2_RXD_PIN, UART2_RXD_PIN_MODE);
}

static errcode_t app_uart_init_config(void)
{
    errcode_t errcode;

    uart_attr_t attr = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE
    };

    uart_pin_config_t pin_config1 = {
        .tx_pin = UART0_TXD_PIN,
        .rx_pin = UART0_RXD_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE
    };

    uart_pin_config_t pin_config2 = {
        .tx_pin = UART2_TXD_PIN,
        .rx_pin = UART2_RXD_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE
    };

    uart_extra_attr_t extra_attr = {
        .tx_dma_enable = true,
        .tx_int_threshold = UART_FIFO_INT_TX_LEVEL_EQ_0_CHARACTER,
        .rx_dma_enable = true,
        .rx_int_threshold = UART_FIFO_INT_RX_LEVEL_1_CHARACTER
    };

    uapi_dma_init();
    uapi_dma_open();
    uapi_uart_deinit(UART0_BUS_ID);

    errcode = uapi_uart_init(UART0_BUS_ID, &pin_config1, &attr, &extra_attr, &uart0_buffer_config);
    if (errcode != ERRCODE_SUCC) {
        osal_printk("uart init fail\r\n");
        return errcode;
    }

    uapi_uart_deinit(UART2_BUS_ID);
    errcode = uapi_uart_init(UART2_BUS_ID, &pin_config2, &attr, &extra_attr, &uart2_buffer_config);
    if (errcode != ERRCODE_SUCC) {
        osal_printk("uart init fail\r\n");
        return errcode;
    }

    return errcode;
} 




static void *car_sample0_task(const char *arg)
{
    unused(arg);

    osal_msleep(20);

    int32_t ret0 = UART0_RECEIVE_SIZE;
    int32_t ret2 = UART2_RECEIVE_SIZE;
    uint8_t wheel_pwm[4] = {0};
    uint8_t wheel_rpm[4] = {0};
    float bmi270_data[6] = {0};
    uint8_t adc_channel = ADC_CHANNEL;
    uint16_t voltage = 0;
    bool rx_flag = 1;

    app_i2c_init_pin();
    uapi_i2c_master_init(I2C_BUS_1, 400000, 0);
    osal_msleep(2);

    uapi_adc_init(ADC_CLOCK_NONE);

    app_uart_init_pin();
    app_uart_init_config();

    bmi270_full_fill_header_mode_init();

    osal_msleep(2);

    while (1) {
        rx_flag = 1;

        memset(g_app_uart0_ss928_rx_buff, 0, sizeof(g_app_uart0_ss928_rx_buff));

        osal_printk("开始从928接收PWM值\r\n");
        ret0 = uapi_uart_read_by_dma(UART0_BUS_ID, g_app_uart0_ss928_rx_buff, UART0_RECEIVE_SIZE,
            &g_uart_dma_928_cfg);
        if (ret0 == UART0_RECEIVE_SIZE) {
            osal_printk("接收成功");
        }
        else {
            rx_flag = 0;
        }

        if(rx_flag == 1) {
            // 处理接收到的PWM值
            memcpy(wheel_pwm, g_app_uart0_ss928_rx_buff, sizeof(wheel_pwm));

            osal_printk("开始向H21E发送PWM值\r\n");
            ret0 = uapi_uart_write_by_dma(UART2_BUS_ID, wheel_pwm, UART0_RECEIVE_SIZE,
                &g_app_dma_cfg);
            if (ret0 == UART0_RECEIVE_SIZE) {
                osal_printk("发送成功");
            }
        }

        rx_flag = 1;

        osal_printk("开始获取bmi270的角度和角速度\r\n");
        if (get_bmi270_attitude_data(bmi270_data) != ERRCODE_SUCC) {
            rx_flag = 0;
        }
        if(rx_flag == 1) {
            // 处理bmi270数据
            ss928_t.angle_x = bmi270_data[0];
            ss928_t.angle_y = bmi270_data[1];
            ss928_t.angle_z = bmi270_data[2];
            ss928_t.rate_x  = bmi270_data[3];
            ss928_t.rate_y  = bmi270_data[4];
            ss928_t.rate_z  = bmi270_data[5];
        }

        osal_printk("开始从H21E接收RPM值\r\n");
        memset(g_app_uart2_rx_buff, 0, sizeof(g_app_uart2_rx_buff));
        
        ret2 = uapi_uart_read_by_dma(UART2_BUS_ID, g_app_uart2_rx_buff, UART2_RECEIVE_SIZE, &g_app_dma_cfg);
        if (ret2 == UART2_RECEIVE_SIZE) {
            osal_printk("接收成功");
        }
        else {
            rx_flag = 0;
        }

        // 读取ADC电压值
        if(adc_port_read(adc_channel, &voltage) == ERRCODE_SUCC) {
            osal_printk("voltage: %.d mv\r\n", (int)voltage);
            ss928_t.voltage = 0.37 + voltage / 9.8 * (62 + 9.8);
        }
        else {
            rx_flag = 0;
        }

        if(rx_flag == 1) {
            // 处理接收到的RPM值
            memcpy(wheel_rpm, g_app_uart2_rx_buff, sizeof(wheel_rpm));
            ss928_t.wheel0_rpm = wheel_rpm[0];
            ss928_t.wheel1_rpm = wheel_rpm[1];
            ss928_t.wheel2_rpm = wheel_rpm[2];
            ss928_t.wheel3_rpm = wheel_rpm[3];
            
            char upload_buf[256];
            int len = snprintf(upload_buf, sizeof(upload_buf),
                "angle_x=%.2f,angle_y=%.2f,angle_z=%.2f,"
                "rate_x=%.2f,rate_y=%.2f,rate_z=%.2f,"
                "wheel0_rpm=%.2f,wheel1_rpm=%.2f,wheel2_rpm=%.2f,wheel3_rpm=%.2f"
                "voltage=%.2f\n",
                ss928_t.angle_x, ss928_t.angle_y, ss928_t.angle_z,
                ss928_t.rate_x, ss928_t.rate_y, ss928_t.rate_z,
                ss928_t.wheel0_rpm, ss928_t.wheel1_rpm, ss928_t.wheel2_rpm, ss928_t.wheel3_rpm,
                ss928_t.voltage
            );

            osal_printk("开始向ss928发送转速\r\n");
            if (uapi_uart_write_by_dma(UART0_BUS_ID, upload_buf, len, &g_uart_dma_928_cfg) == len) {
                osal_printk("发送成功");
            }
        }
    }

    return NULL;
}

static void car_sample0_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)car_sample0_task, 0, "CarSample0Task", CAR_SAMPLE0_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, CAR_SAMPLE0_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the car_sample0_entry. */
app_run(car_sample0_entry);