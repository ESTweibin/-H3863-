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
#include "aux_bmp280.h"
#include "bmi270_init.h"

// 定义数组存储数据
#define DATA_SAMPLES 5
#define VALUES_PER_SAMPLE 6 // roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate
float imu_data[DATA_SAMPLES * VALUES_PER_SAMPLE];

char buffer[100];

#include "uart.h"
#include <string.h>
// #include "pm_clock.h"

#define CONFIG_UART_TXD_PIN 17
#define CONFIG_UART_RXD_PIN 18
#define CONFIG_SLE_UART_BUS 1
#define CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT 1

#include "sle_low_latency.h"
#if defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER)

#elif defined(CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT)
#define SLE_UART_TASK_STACK_SIZE 0x600
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"
#include "sle_uart_client.h"
#endif /* CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT */

#define SLE_UART_TASK_DURATION_MS 2000
#define SLE_UART_BAUDRATE 115200
#define SLE_UART_TRANSFER_SIZE 512

#define CONFIG_UART_TXD_PIN 17
#define CONFIG_UART_RXD_PIN 18
#define CONFIG_SLE_UART_BUS 1
#define CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT 1

#if defined(CONFIG_PWM_SUPPORT_LPM)
#include "pm_veto.h"
#endif

#include "common_def.h"
#include "pwm.h"
#include "tcxo.h"

#include "pid.h"

#define TEST_TCXO_DELAY_1000MS 100

#define FLY_TASK_PRIO 26
#define FLY_TASK_STACK_SIZE 0x10000

// PWM相关引脚宏定义
#define PWM_PIN_MODE 1

#define PWM_CHANNEL1 1
#define PWM_PIN1 1
#define PWM_GROUP_ID1 0

#define PWM_CHANNEL2 3
#define PWM_PIN2 3
#define PWM_GROUP_ID2 1

#define PWM_CHANNEL3 2
#define PWM_PIN3 10
#define PWM_GROUP_ID3 2

#define PWM_CHANNEL4 7
#define PWM_PIN4 7
#define PWM_GROUP_ID4 3

#define CONFIG_PWM_USING_V151 1

// 定义PID对象
PID_Object pidAngleRoll;
PID_Object pidAnglePitch;
PID_Object pidAngleYaw;
PID_Object pidRateX;
PID_Object pidRateY;
PID_Object pidRateZ;

// 初始化PID参数
pidInit_t angleParam = {.kp = 0.8f, .ki = 0.0f, .kd = 0.0f};
pidInit_t rateParam = {.kp = 1.0f, .ki = 0.0f, .kd = 0.0f};

typedef struct {
    int X1;
    int Y1;
    int X2;
    int Y2;
    int test_mode;
} SLEData;

// 全局变量，存储接收到的数据
SLEData g_received_data = {0};
volatile int g_data_received = 0; // 标志，表示数据是否已接收
float pwm[4];

int parse_sle_data(const char *input, SLEData *data)
{
    // 使用 sscanf 解析字符串
    int result = sscanf(input, "X1:%d,Y1:%d,X2:%d,Y2:%d,test_mode:%d", &data->X1, &data->Y1, &data->X2, &data->Y2,
                        &data->test_mode);

    // 检查解析结果
    if (result == 5) {
        return 1; // 解析成功
    } else {
        return 0; // 解析失败
    }
}

static uint8_t g_app_uart_rx_buff[SLE_UART_TRANSFER_SIZE] = {0};

static uart_buffer_config_t g_app_uart_buffer_config = {.rx_buffer = g_app_uart_rx_buff,
                                                        .rx_buffer_size = SLE_UART_TRANSFER_SIZE};

static void uart_init_pin(void)
{
    if (CONFIG_SLE_UART_BUS == 0) {
        uapi_pin_set_mode(CONFIG_UART_TXD_PIN, PIN_MODE_1);
        uapi_pin_set_mode(CONFIG_UART_RXD_PIN, PIN_MODE_1);
    } else if (CONFIG_SLE_UART_BUS == 1) {
        uapi_pin_set_mode(CONFIG_UART_TXD_PIN, PIN_MODE_1);
        uapi_pin_set_mode(CONFIG_UART_RXD_PIN, PIN_MODE_1);
    }
}

static void uart_init_config(void)
{
    uart_attr_t attr = {.baud_rate = SLE_UART_BAUDRATE,
                        .data_bits = UART_DATA_BIT_8,
                        .stop_bits = UART_STOP_BIT_1,
                        .parity = UART_PARITY_NONE};

    uart_pin_config_t pin_config = {
        .tx_pin = CONFIG_UART_TXD_PIN, .rx_pin = CONFIG_UART_RXD_PIN, .cts_pin = PIN_NONE, .rts_pin = PIN_NONE};
    uapi_uart_deinit(CONFIG_SLE_UART_BUS);
    uapi_uart_init(CONFIG_SLE_UART_BUS, &pin_config, &attr, NULL, &g_app_uart_buffer_config);
}

void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    unused(status);
    // osal_printk("\n sle uart recived data : %s\r\n", data->data);
    uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)(data->data), data->data_len, 0);
    // 解析数据
    if (parse_sle_data((const char *)data->data, &g_received_data)) {
        g_data_received = 1; // 设置标志位
    } else {
        osal_printk("Failed to parse data.\r\n");
    }
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    unused(status);
    // osal_printk("\n sle uart recived data : %s\r\n", data->data);
    uapi_uart_write(CONFIG_SLE_UART_BUS, (uint8_t *)(data->data), data->data_len, 0);
    // 解析数据
    if (parse_sle_data((const char *)data->data, &g_received_data)) {
        g_data_received = 1; // 设置标志位
    } else {
        osal_printk("Failed to parse data.\r\n");
    }
}

static void sle_uart_client_read_int_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    ssapc_write_param_t *sle_uart_send_param = get_g_sle_uart_send_param();
    uint16_t g_sle_uart_conn_id = get_g_sle_uart_conn_id();
    sle_uart_send_param->data_len = length;
    sle_uart_send_param->data = (uint8_t *)buffer;
    ssapc_write_req(0, g_sle_uart_conn_id, sle_uart_send_param);
}

static void *fly_task(const char *arg)
{
    UNUSED(arg);

    uart_init_pin();
    uart_init_config();

    // 初始化PID控制器
    PID_Init(&pidAngleRoll, 0.0f, angleParam, 0.011f);
    PID_Init(&pidAnglePitch, 0.0f, angleParam, 0.011f);
    PID_Init(&pidAngleYaw, 0.0f, angleParam, 0.011f);

    PID_Init(&pidRateX, 0.0f, rateParam, 0.011f);
    PID_Init(&pidRateY, 0.0f, rateParam, 0.011f);
    PID_Init(&pidRateZ, 0.0f, rateParam, 0.011f);

    uapi_uart_unregister_rx_callback(CONFIG_SLE_UART_BUS);
    errcode_t ret = uapi_uart_register_rx_callback(CONFIG_SLE_UART_BUS, UART_RX_CONDITION_FULL_OR_IDLE, 1,
                                                   sle_uart_client_read_int_handler);
    sle_uart_client_init(sle_uart_notification_cb, sle_uart_indication_cb);

    if (ret != ERRCODE_SUCC) {
        osal_printk("Register uart callback fail.");
        return NULL;
    }

    if (g_data_received) {
        // 数据已接收，处理数据
        osal_printk("Received Data T:\n");
        osal_printk("X1: %d\n", g_received_data.X1);
        osal_printk("Y1: %d\n", g_received_data.Y1);
        osal_printk("X2: %d\n", g_received_data.X2);
        osal_printk("Y2: %d\n", g_received_data.Y2);
        osal_printk("test_mode: %d\n", g_received_data.test_mode);

        // 重置标志位
        g_data_received = 0;
    }

    // 初始化引脚
    uapi_pin_set_mode(PWM_PIN1, PWM_PIN_MODE);
    uapi_pin_set_mode(PWM_PIN2, PWM_PIN_MODE);
    uapi_pin_set_mode(PWM_PIN3, PWM_PIN_MODE);
    uapi_pin_set_mode(PWM_PIN4, PWM_PIN_MODE);

    // 初始化PWM
    uapi_pwm_deinit();
    uapi_pwm_init();

    pwm_config_t cfg_no_repeat1 = {5000, 0, 0, 0xFF, true};
    pwm_config_t cfg_no_repeat2 = {5000, 0, 0, 0xFF, true};
    pwm_config_t cfg_no_repeat3 = {5000, 0, 0, 0xFF, true};
    pwm_config_t cfg_no_repeat4 = {5000, 0, 0, 0xFF, true};

    // 打开通道
    uapi_pwm_open(PWM_CHANNEL1, &cfg_no_repeat1);
    uapi_pwm_open(PWM_CHANNEL2, &cfg_no_repeat2);
    uapi_pwm_open(PWM_CHANNEL3, &cfg_no_repeat3);
    uapi_pwm_open(PWM_CHANNEL4, &cfg_no_repeat4);

    uint8_t channel_id = PWM_CHANNEL2;
    uapi_pwm_set_group(PWM_GROUP_ID2, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID2);

    channel_id = PWM_CHANNEL3;
    uapi_pwm_set_group(PWM_GROUP_ID3, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID3);

    channel_id = PWM_CHANNEL4;
    uapi_pwm_set_group(PWM_GROUP_ID4, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID4);

    channel_id = PWM_CHANNEL1;
    uapi_pwm_set_group(PWM_GROUP_ID1, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID1);

    uapi_tcxo_delay_ms((uint32_t)TEST_TCXO_DELAY_1000MS);

    while (1) {
        if (g_received_data.test_mode == 0 || g_received_data.test_mode == 1) {

            osal_printk("init\n");
            osal_msleep(200);
            uapi_pwm_close(PWM_GROUP_ID1);
            uapi_pwm_close(PWM_GROUP_ID2);
            uapi_pwm_close(PWM_GROUP_ID3);
            uapi_pwm_close(PWM_GROUP_ID4);

            cfg_no_repeat1 = (pwm_config_t){5000, 0, 0, 0xFF, true};
            uapi_pwm_open(PWM_CHANNEL1, &cfg_no_repeat1);
            uapi_pwm_start_group(PWM_GROUP_ID1);

            cfg_no_repeat2 = (pwm_config_t){5000, 0, 0, 0xFF, true};
            uapi_pwm_open(PWM_CHANNEL2, &cfg_no_repeat2);
            uapi_pwm_start_group(PWM_GROUP_ID2);

            cfg_no_repeat3 = (pwm_config_t){5000, 0, 0, 0xFF, true};
            uapi_pwm_open(PWM_CHANNEL3, &cfg_no_repeat3);
            uapi_pwm_start_group(PWM_GROUP_ID3);

            cfg_no_repeat4 = (pwm_config_t){5000, 0, 0, 0xFF, true};
            uapi_pwm_open(PWM_CHANNEL4, &cfg_no_repeat4);
            uapi_pwm_start_group(PWM_GROUP_ID4);

            read_bmi270_imu_data_array(imu_data, sizeof(imu_data) / sizeof(float), DATA_SAMPLES);

            for (int i = 0; i < 6; i++) {
                snprintf(buffer, 100, "sensor data: Roll=%.2f", imu_data[i]);
                printf("%s\n", buffer);
            }

        }else if (g_received_data.test_mode == 2) {

            // 读取5次数据到数组中
            read_bmi270_imu_data_array(imu_data, sizeof(imu_data) / sizeof(float), DATA_SAMPLES);

            for (int i = 0; i < 6; i++) {
                snprintf(buffer, 100, "sensor data: Roll=%.2f", imu_data[i]);
                printf("%s\n", buffer);
            }

            // 示例数据
            attitude_t Angle_real = {.roll = imu_data[0], .pitch = imu_data[1], .yaw = imu_data[2]};
            attitude_t Angle_target = {.roll = 0, .pitch = 0, .yaw = 0};

            Axis3f Rate_real = {.X = imu_data[3], .Y = imu_data[4], .Z = imu_data[5]};
            Axis3f Rate_target;

            Axis3f output_Rate;
            Contrl_t output;

            // 角度环PID控制
            AnglePID(&Angle_real, &Angle_target, &output_Rate, &pidAngleRoll, &pidAnglePitch, &pidAngleYaw);

            // 设置角速度目标值
            Rate_target.X = output_Rate.X;
            Rate_target.Y = output_Rate.Y;
            Rate_target.Z = output_Rate.Z;

            // 角速度环PID控制
            RatePID(&Rate_real, &Rate_target, &output, &pidRateX, &pidRateY, &pidRateZ);

            // 输出控制信号
            snprintf(buffer, 100,
                     "final output: output.pitch_input = %f, output.roll_input = %f, output.yaw_input = %f\n",
                     output.pitch_input, output.roll_input, output.yaw_input);
            printf("%s\n", buffer);

            // float pitch = (50.0f - g_received_data.Y1);    // 俯仰角
            // float roll = (g_received_data.X1 - 50.0f);     // 横滚角
            // float throttle = (50.0f - g_received_data.Y2); // 油门
            // float yaw = (g_received_data.X2 - 50.0f);   // 航向角

            // float base = throttle * 1.5f;
            // float left = roll * 1.0f;
            // float forward = pitch;

            if (output.pitch_input > 26) {
                output.pitch_input = 26;
            } else if (output.pitch_input < -26) {
                output.pitch_input = -26;
            }
            if (output.roll_input > 26) {
                output.roll_input = 26;
            } else if (output.roll_input < -26) {
                output.roll_input = -26;
            }
            if (output.yaw_input > 26) {
                output.yaw_input = 26;
            } else if (output.yaw_input < -26) {
                output.yaw_input = -26;
            }

            output.pitch_input = output.pitch_input ;
            output.roll_input = output.roll_input ;
            // output.yaw_input = output.yaw_input * 0.5f;
            
            /*pwm[0] = base + left - output.pitch_input - output.roll_input;                     // 机头右3
            pwm[1] = base - output.pitch_input + output.roll_input;                            // 机头左1
            pwm[2] = base + left + forward + output.pitch_input - output.roll_input;           // 机尾右2
            pwm[3] = base + forward + output.pitch_input + output.roll_input;                  // 机尾左4*/
            

            /*pwm[0] = base + forward + output.pitch_input - output.roll_input   ;
            pwm[1] = base + forward - output.roll_input - output.pitch_input ;
            pwm[2] = base - output.pitch_input + output.roll_input  ;
            pwm[3] = base + output.roll_input * 0.9 + output.pitch_input * 0.9 ;*/

            pwm[0] = 50 + output.pitch_input - output.roll_input - output.yaw_input;
            pwm[1] = 50 - output.roll_input - output.pitch_input + output.yaw_input;
            pwm[2] = 50 - output.pitch_input + output.roll_input - output.yaw_input;
            pwm[3] = 50 + output.roll_input + output.pitch_input + output.yaw_input;

            // 输出控制信号
            snprintf(buffer, 100, "final pwm: pwm[0] = %f, pwm[1] = %f, pwm[2] = %f, pwm[3] = %f\n", pwm[0], pwm[1],
                     pwm[2], pwm[3]);
            printf("%s\n", buffer);

            for (int i = 0; i < 4; ++i) {
                if (pwm[i] < 6.1f)
                    pwm[i] = 6.1f;
                if (pwm[i] > 80.1f)
                    pwm[i] = 80.1f;
            }

            uapi_pwm_close(PWM_GROUP_ID1);
            uapi_pwm_close(PWM_GROUP_ID2);

            uapi_pwm_close(PWM_GROUP_ID3);
            uapi_pwm_close(PWM_GROUP_ID4);

            cfg_no_repeat1 = (pwm_config_t){5000 - (int)pwm[1] * 50, (int)pwm[1] * 50, 0, 0xFF, true}; // 机头左
            cfg_no_repeat2 = (pwm_config_t){5000 - (int)pwm[0] * 50, (int)pwm[0] * 50, 0, 0xFF, true}; // 机头右
            cfg_no_repeat3 = (pwm_config_t){5000 - (int)pwm[2] * 50, (int)pwm[2] * 50, 0, 0xFF, true}; // 机尾左
            cfg_no_repeat4 = (pwm_config_t){5000 - (int)pwm[3] * 50, (int)pwm[3] * 50, 0, 0xFF, true}; // 机尾右

            uapi_pwm_open(PWM_CHANNEL1, &cfg_no_repeat1);
            uapi_pwm_open(PWM_CHANNEL2, &cfg_no_repeat2);
            uapi_pwm_open(PWM_CHANNEL3, &cfg_no_repeat3);
            uapi_pwm_open(PWM_CHANNEL4, &cfg_no_repeat4);

            uapi_pwm_start_group(PWM_GROUP_ID1);
            uapi_pwm_start_group(PWM_GROUP_ID2);
            uapi_pwm_start_group(PWM_GROUP_ID3);
            uapi_pwm_start_group(PWM_GROUP_ID4);

            osal_msleep(10);
        }
    }

    return NULL;
}

// FIFO数据处理主任务
static void *bmi_task(const char *arg)
{
    (void)arg; // 避免未使用参数警告

    int8_t rslt;

    bmi2_delay_us(2000, NULL); // 等待系统稳定
    // 配置I2C引脚
    uapi_pin_set_mode(15, 2); // I2C1_SDA
    uapi_pin_set_mode(16, 2); // I2C1_SCL

    // 初始化I2C
    uapi_i2c_master_init(I2C_BUS_1, 400000, 0);

    rslt = bmi270_aux_full_fill_header_mode_init();
    bmi2_error_codes_print_result(rslt);

    rslt = get_bmi270_sensor_data(5);
    bmi2_error_codes_print_result(rslt);

    osal_printk("BMI Task: BMI270功能测试完成\n");

    return NULL;
}

static void Fly_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();

    task_handle = osal_kthread_create((osal_kthread_handler)fly_task, 0, "FlyTask", FLY_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, FLY_TASK_PRIO);
    }

    task_handle = osal_kthread_create((osal_kthread_handler)bmi_task, 0, "BmiTask", 0x4000);
    if (task_handle != NULL) {
        osal_printk("bmi_entry: 任务创建成功\n");
        /* 设置合适的优先级 */
        osal_kthread_set_priority(task_handle, 28);
        osal_kfree(task_handle);
    } else {
        osal_printk("bmi_entry: 任务创建失败\n");
    }

    osal_kthread_unlock();
}

/* 注册在系统启动时运行 */
app_run(Fly_entry);
