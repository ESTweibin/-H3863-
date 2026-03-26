#include "cmsis_os2.h"
#include "app_init.h"
#include "PCA9685Servo.h"
// #include "Button.h" // 不再需要按键，可以注释掉
#include "errcode.h"
#include "watchdog.h"
#include "pinctrl.h" // 新增：用于引脚配置
#include "uart.h"    // 新增：用于UART功能
#include <string.h>  // 新增：用于字符串操作 (strncmp, strcmp)
#include <stdlib.h>  // 新增：用于 atof (将字符串转为浮点数)
#include <stdio.h>   // 新增：用于 sscanf (更安全地解析字符串)
#include <math.h>
#include "kal_printf.h"

// --- 新增：从 em_alg.cpp 移植的机械臂物理尺寸 ---
#define ARM_L1 135.0f // 大臂长度
#define ARM_L2 145.0f // 小臂长度

// --- 新增：从 em_alg.cpp 移植的数学辅助宏和函数 ---
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define degrees(rad) ((rad) * 180.0f / M_PI)
#define radians(deg) ((deg) * M_PI / 180.0f)
#define square(n) ((n) * (n))

/*
static float sgn(float num)
{
    if (num > 0.0f) return 1.0f;
    if (num < 0.0f) return -1.0f;
    return 0.0f;
}
*/

// --- 新增：从 em_alg.cpp 移植的全局变量，用于存储计算出的舵机角度 ---
static float g_angleA = 90.0f; // 基座角度
static float g_angleB = 90.0f; // 肩部角度
static float g_angleC = 90.0f; // 肘部角度

#define ANGLE_ADJUST_STEP           (15) // 每次调整角度的步长 (度)
#define MAX_SERVO_CHANNELS          (16) // PCA9685 支持的最大舵机通道数 (0-15)
#define TASK_LOOP_DELAY_MS          (20) // 主任务循环延时，也用于按键去抖

// --- 舵机通道定义 (已根据您的接线图修正) ---
#define BASE_SERVO_CHANNEL      0 // 基座舵机
#define ELBOW_SERVO_CHANNEL     1 // 肘部舵机 (注意：在通道1)
#define SHOULDER_SERVO_CHANNEL  2 // 肩部舵机 (注意：在通道2)
#define GRIPPER_SERVO_CHANNEL   3 // 爪子舵机

// --- 更新：使用您标定的舵机角度物理限制 ---
#define BASE_ANGLE_MIN      0
#define BASE_ANGLE_MAX      180
#define SHOULDER_ANGLE_MIN  30
#define SHOULDER_ANGLE_MAX  135
#define ELBOW_ANGLE_MIN     45
#define ELBOW_ANGLE_MAX     135
#define GRIPPER_ANGLE_CLOSE 45  // 夹紧 (根据您的表格)
#define GRIPPER_ANGLE_OPEN  105 // 张开 (根据您的表格)

// --- 新增：使用您标定的舵机角度校准偏移量 ---
#define BASE_SERVO_OFFSET      (0)    // 实际90 - 理论90
#define ELBOW_SERVO_OFFSET     (-35)  // 实际145 - 理论180
#define SHOULDER_SERVO_OFFSET  (30)   // 实际30 - 理论0

// --- 新增：基座齿轮比 ---
#define BASE_GEAR_RATIO (2.58f) // 舵机转动角度 / 底盘实际转动角度

// --- UART 相关定义 (已修正) ---
#define ARM_UART_BUS_ID      2 // 修正：根据引脚图，GPIO7/8 对应 UART2
#define ARM_UART_BAUDRATE    115200
#define ARM_UART_TX_PIN      8 
#define ARM_UART_RX_PIN      7
#define ARM_UART_BUFFER_SIZE 128

static uint8_t g_uart_rx_buffer[ARM_UART_BUFFER_SIZE]; // UART 接收缓冲区

static uint8_t g_current_selected_channel = 0;
static uint8_t g_target_angles[MAX_SERVO_CHANNELS]; // 存储每个通道的目标角度

// --- 新增：用于在中断和主任务间传递数据的全局变量 ---
static char g_command_buffer[ARM_UART_BUFFER_SIZE]; // 全局指令缓冲区
static volatile bool g_new_command_received = false; // 新指令标志 (volatile很重要)

// --- 新增：函数前向声明 ---
static void process_arm_command(const char *command_buffer);
static bool check_kinematic_angles(void);
static void smooth_move_servo(uint8_t channel, uint8_t target_angle);
static void smooth_move_multiple_servos(const uint8_t* channels, const uint8_t* target_angles, uint8_t num_servos);


static uint8_t constrain_angle(uint8_t angle, uint8_t min_angle, uint8_t max_angle)
{
    // --- 恢复角度约束功能 ---
    if (angle < min_angle) return min_angle;
    if (angle > max_angle) return max_angle;
    return angle;
}

/**
 * @brief 【移植】输入绝对坐标，输出舵机角度 (来自 em_alg.cpp)
 * @param x X轴坐标
 * @param y Y轴坐标
 * @param z Z轴坐标
 */
static void inverse_operation(float x, float y, float z)
{
    // --- 坐标系修正：【最终版 - 匹配SolidWorks】 ---
    // 您的坐标系 (X向前, Y向上, Z向左)
    // 算法内部坐标系 (X'向前, Y'向上, Z'向左)
    // 这一次，我们让两个坐标系完全对齐，只做变量名映射
    
    // --- 新增：反转X和Z轴以匹配SolidWorks ---
    float x_alg = -x; // X轴反向
    float y_alg = y;  // Y轴同向 (高度)
    float z_alg = -z; // Z轴反向

    // --- 1. 计算基座角 (angleA) ---
    // 基座角是绕Y轴的旋转，由 X 和 Z 决定
    float geometric_angle_deg = degrees(atan2f(z_alg, x_alg)); // 计算纯几何角度
    
    // 应用齿轮比：几何角度乘以齿轮比，得到舵机需要转动的角度
    // 注意：我们以90度为中心点，所以是从几何角度0度开始计算偏移
    g_angleA = 90.0f - (geometric_angle_deg * BASE_GEAR_RATIO);

    // --- 2. 计算肩部和肘部所需的中间变量 ---
    float dist_xz = sqrtf(square(x_alg) + square(z_alg)); // 水平投影距离
    float term_A = dist_xz - 7.0f - 60.0f;
    float term_B = y_alg - 65.5f; // Y坐标对应垂直高度
    float term_C = sqrtf(square(term_A) + square(term_B));

    // --- 3. 计算肩部角 (angleB) ---
    // 原公式: angleB = 180.0-69.0-temp2-temp1;
    float temp1_rad = atan2f(term_B, term_A);
    
    float acos_arg_B = (square(ARM_L1) + square(term_C) - square(ARM_L2)) / (2.0f * ARM_L1 * term_C);
    if (acos_arg_B < -1.0f || acos_arg_B > 1.0f) {
        kal_printf("Error: Cannot solve angleB, target is unreachable.\r\n");
        return; // 目标点无法到达，提前退出
    }
    float temp2_rad = acosf(acos_arg_B);
    g_angleB = 180.0f - 69.0f - degrees(temp2_rad) - degrees(temp1_rad);

    // --- 4. 计算肘部角 (angleC) ---
    // 原公式: angleC =180.0-(83.5+(180.0-69.0-temp2-temp1)-temp3);
    float acos_arg_C = (square(ARM_L2) + square(ARM_L1) - square(dist_xz - 67.0f) - square(term_B)) / (2.0f * ARM_L2 * ARM_L1);
    if (acos_arg_C < -1.0f || acos_arg_C > 1.0f) {
        kal_printf("Error: Cannot solve angleC, target is unreachable.\r\n");
        return; // 目标点无法到达，提前退出
    }
    float temp3_rad = acosf(acos_arg_C);
    g_angleC = 180.0f - (83.5f + g_angleB - degrees(temp3_rad));

    kal_printf("SW Coords: X=%.2f(fwd), Y=%.2f(up), Z=%.2f(left)\r\n", x, y, z);
    kal_printf("Calculated Angles: A=%.2f, B=%.2f, C=%.2f\r\n", g_angleA, g_angleB, g_angleC);
}

/**
 * @brief 【新增移植】输入舵机角度，输出绝对坐标 (来自 em_alg.cpp)
 * @param angleA        基座舵机的理论角度
 * @param angleB        肩部舵机的理论角度
 * @param angleC        肘部舵机的理论角度
 * @param x             输出X坐标的指针
 * @param y             输出Y坐标的指针
 * @param z             输出Z坐标的指针
 */
static void forward_kinematics(float angleA, float angleB, float angleC, float* x, float* y, float* z)
{
    // --- 1. 移植核心数学公式 ---
    float temp_dist = ARM_L1 * cosf(radians(111.0f - angleB)) + ARM_L2 * sinf(radians((83.5f + angleB - (180.0f - angleC)) - angleB + 21.0f)) + 67.0f;
    
    // --- 2. 坐标系反向转换：【最终版 - 匹配SolidWorks】 ---
    // 根据基座角计算出 X 和 Z
    float angleA_rad = radians(90.0f - angleA) / BASE_GEAR_RATIO; // 反向应用齿轮比
    
    // 计算 Y (高度) - 不再需要重复定义 temp_dist
    *y = 65.5f + ARM_L1 * sinf(radians(111.0f - angleB)) - (ARM_L2 * cosf(radians((83.5f + angleB - (180.0f - angleC)) - angleB + 21.0f)));

    // 分解水平距离到 X 和 Z
    float internal_x = temp_dist * cosf(angleA_rad);
    float internal_z = temp_dist * sinf(angleA_rad);

    // --- 新增：反转X和Z轴以匹配外部坐标系 ---
    *x = -internal_x;
    *z = -internal_z;

    kal_printf("Forward Kinematics: Angles(A:%.1f, B:%.1f, C:%.1f) -> Coords(X:%.2f, Y:%.2f, Z:%.2f)\r\n", angleA, angleB, angleC, *x, *y, *z);
}

/**
 * @brief 【移植】判断计算出的舵机角度是否在可到达范围内 (来自 em_alg.cpp)
 * @return bool true: 角度有效, false: 角度超出范围
 */
static bool check_kinematic_angles(void)
{
    // 临时注释掉所有角度范围检查，允许全范围运动
    /*
    // 检查基座角
    if (g_angleA < 0 || g_angleA > 180) {
        kal_printf("Error: angleA (%.2f) out of algorithm range [0, 180]\r\n", g_angleA);
        return false;
    }

    // 检查肩部角 (根据原 em_alg.cpp 的逻辑)
    if (g_angleB < 0 || g_angleB > 85) {
        kal_printf("Error: angleB (%.2f) out of algorithm range [0, 85]\r\n", g_angleB);
        return false;
    }

    // 检查肘部角 (这是一个与肩部角相关的动态范围)
    float angleCMin = 140.0f - g_angleB;
    // 使用系统提供的 MIN 宏
    float angleCMax = MIN((196.0f - g_angleB), 180.0f); 
    if (g_angleC < angleCMin || g_angleC > angleCMax) {
        kal_printf("Error: angleC (%.2f) out of algorithm range [%.2f, %.2f]\r\n", g_angleC, angleCMin, angleCMax);
        return false;
    }
    */
    
    // 临时直接返回 true，跳过所有检查
    kal_printf("Kinematic check bypassed - allowing all angles\r\n");
    return true;
}

// 平滑运动相关配置
#define SMOOTH_MOTION_ENABLED   1    // 启用平滑运动 (设为0可禁用)
#define MOTION_STEP_DELAY_MS    50   // 每步之间的延时 (毫秒)
#define MOTION_STEP_SIZE        2    // 每步的角度变化 (度)

// 当前角度记录
static uint8_t g_current_servo_angles[MAX_SERVO_CHANNELS] = {90, 90, 90, 105}; // 初始角度

/**
 * @brief 平滑移动单个舵机到目标角度
 */
static void smooth_move_servo(uint8_t channel, uint8_t target_angle)
{
    if (channel >= MAX_SERVO_CHANNELS) {
        return;
    }

#if SMOOTH_MOTION_ENABLED
    uint8_t current_angle = g_current_servo_angles[channel];
    
    kal_printf("Smooth moving servo %d from %d to %d degrees\r\n", channel, current_angle, target_angle);
    
    // 如果角度差异很小，直接移动
    if (abs((int)target_angle - (int)current_angle) <= MOTION_STEP_SIZE) {
        Pca9685Servo_SetAngle(channel, target_angle);
        g_current_servo_angles[channel] = target_angle;
        return;
    }
    
    // 逐步移动到目标角度
    while (current_angle != target_angle) {
        if (current_angle < target_angle) {
            current_angle += MOTION_STEP_SIZE;
            if (current_angle > target_angle) {
                current_angle = target_angle; // 防止超调
            }
        } else {
            current_angle -= MOTION_STEP_SIZE;
            if (current_angle < target_angle) {
                current_angle = target_angle; // 防止超调
            }
        }
        
        Pca9685Servo_SetAngle(channel, current_angle);
        g_current_servo_angles[channel] = current_angle;
        osal_msleep(MOTION_STEP_DELAY_MS);
    }
    
    // 新增：运动完成后释放舵机
    kal_printf("Servo %d movement complete. Releasing PWM.\r\n", channel);
    Pca9685Servo_ReleaseChannel(channel);

#else
    // 禁用平滑运动时，直接移动
    Pca9685Servo_SetAngle(channel, target_angle);
    g_current_servo_angles[channel] = target_angle;
#endif
}

/**
 * @brief 平滑移动多个舵机到目标角度 (并行插值)
 * @param channels 舵机通道数组
 * @param target_angles 目标角度数组
 * @param num_servos 舵机数量
 */
static void smooth_move_multiple_servos(const uint8_t* channels, const uint8_t* target_angles, uint8_t num_servos)
{
    if (num_servos == 0 || num_servos > MAX_SERVO_CHANNELS) {
        return;
    }

#if SMOOTH_MOTION_ENABLED
    bool motion_complete = false;
    uint8_t current_angles[MAX_SERVO_CHANNELS];
    
    // 初始化当前角度
    for (uint8_t i = 0; i < num_servos; i++) {
        if (channels[i] < MAX_SERVO_CHANNELS) {
            current_angles[i] = g_current_servo_angles[channels[i]];
        }
    }
    
    kal_printf("Smooth moving %d servos simultaneously\r\n", num_servos);
    
    // 并行插值移动
    while (!motion_complete) {
        motion_complete = true;
        
        for (uint8_t i = 0; i < num_servos; i++) {
            uint8_t channel = channels[i];
            if (channel >= MAX_SERVO_CHANNELS) continue;
            
            uint8_t target = target_angles[i];
            uint8_t current = current_angles[i];
            
            if (current != target) {
                motion_complete = false;
                
                if (current < target) {
                    current += MOTION_STEP_SIZE;
                    if (current > target) {
                        current = target;
                    }
                } else {
                    current -= MOTION_STEP_SIZE;
                    if (current < target) {
                        current = target;
                    }
                }
                
                current_angles[i] = current;
                Pca9685Servo_SetAngle(channel, current);
                g_current_servo_angles[channel] = current;
            }
        }
        
        if (!motion_complete) {
            osal_msleep(MOTION_STEP_DELAY_MS);
        }
    }
    
    // 新增：所有舵机运动完成后，逐个释放
    kal_printf("Multi-servo movement complete. Releasing all PWMs.\r\n");
    for (uint8_t i = 0; i < num_servos; i++) {
        Pca9685Servo_ReleaseChannel(channels[i]);
    }

#else
    // 禁用平滑运动时，直接移动所有舵机
    for (uint8_t i = 0; i < num_servos; i++) {
        if (channels[i] < MAX_SERVO_CHANNELS) {
            Pca9685Servo_SetAngle(channels[i], target_angles[i]);
            g_current_servo_angles[channels[i]] = target_angles[i];
        }
    }
#endif
}

// --- 舵机控制相关代码保持不变 ---

/**
 * @brief 串口接收中断处理函数 (只接收数据，不处理)
 * @note  当UART接收到数据时，此函数被系统自动调用。
 *        此函数在中断上下文中运行，必须快速执行完毕。
 */
static void arm_uart_rx_handler(const void *buffer, uint16_t length, bool error)
{
    // 添加调试输出 - 这样就能知道中断是否被触发
    kal_printf("UART RX triggered: length=%d, error=%d\r\n", length, error);
    
    if (error || buffer == NULL || length == 0 || g_new_command_received) {
        kal_printf("UART RX: Ignoring data due to error or busy\r\n");
        return;
    }

    // 添加更多调试信息
    kal_printf("UART RX: Data received, copying to buffer\r\n");
    
    // --- 生产者逻辑 ---
    uint16_t copy_len = (length < sizeof(g_command_buffer) - 1) ? length : (sizeof(g_command_buffer) - 1);
    memcpy(g_command_buffer, buffer, copy_len);
    g_command_buffer[copy_len] = '\0';

    kal_printf("UART RX: Command copied: '%s'\r\n", g_command_buffer);

    g_new_command_received = true;
    kal_printf("UART RX: Flag set, exiting handler\r\n");
}

/**
 * @brief 初始化 UART (已修正)
 */
static void arm_uart_init(void)
{
    // 1. 引脚复用配置 - 恢复到原始设置
    uapi_pin_set_mode(ARM_UART_TX_PIN, PIN_MODE_1); 
    uapi_pin_set_mode(ARM_UART_RX_PIN, PIN_MODE_1);

    // 2. 简化的UART配置
    uart_attr_t attr = {
        .baud_rate = ARM_UART_BAUDRATE,
        .data_bits = UART_DATA_BIT_8,
        .stop_bits = UART_STOP_BIT_1,
        .parity = UART_PARITY_NONE
    };
    
    uart_pin_config_t pin_config = { 
        .tx_pin = ARM_UART_TX_PIN, 
        .rx_pin = ARM_UART_RX_PIN 
    };
    
    uart_buffer_config_t buffer_config = { 
        .rx_buffer = g_uart_rx_buffer, 
        .rx_buffer_size = ARM_UART_BUFFER_SIZE 
    };

    // 3. 初始化 UART
    uapi_uart_deinit(ARM_UART_BUS_ID);
    errcode_t ret = uapi_uart_init(ARM_UART_BUS_ID, &pin_config, &attr, NULL, &buffer_config);
    if (ret != ERRCODE_SUCC) {
        kal_printf("UART init failed with error: %d\r\n", ret);
        return;
    }

    // 4. 注册接收回调 - 尝试不同的参数
    ret = uapi_uart_register_rx_callback(ARM_UART_BUS_ID, UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE, 1, arm_uart_rx_handler);
    if (ret != ERRCODE_SUCC) {
        kal_printf("UART callback register failed with error: %d\r\n", ret);
    } else {
        kal_printf("UART2 initialized successfully\r\n");
    }
}


/**
 * @brief PCA9685舵机控制任务 (现在也负责处理指令)
 */
void pca9685_servo_task(char *arg)
{
    UNUSED(arg); // 标记参数未使用，避免编译器警告
    errcode_t ret;

    // 初始化PCA9685驱动
    ret = Pca9685Servo_GlobalInit();
    if (ret != ERRCODE_SUCC) {
        kal_printf("PCA9685 Task: Global Init Failed. Exiting task.\r\n");
        return; // 如果初始化失败，任务可以提前退出
    }
    kal_printf("PCA9685 Task: PCA9685 Global Init Succeeded.\r\n");

    // --- 新增：初始化 UART ---
    arm_uart_init();
    kal_printf("Arm controller ready. Waiting for commands via UART%d...\r\n", ARM_UART_BUS_ID);

    // --- 修改：初始化机械臂到指定的舵机角度 ---
    kal_printf("Setting arm to initial angles...\r\n");
    
    // 1. 直接定义初始角度
    uint8_t init_angle_base = 90;     // 基座 (您指定)
    uint8_t init_angle_shoulder = 50; // 肩部 (您指定)
    uint8_t init_angle_elbow = 120;   // 肘部 (您指定)
    uint8_t init_gripper_angle = 50;  // 夹爪 (保持上次的设置)

    // 2. 更新软件记录的当前角度，防止上电跳动
    g_current_servo_angles[BASE_SERVO_CHANNEL] = init_angle_base;
    g_current_servo_angles[SHOULDER_SERVO_CHANNEL] = init_angle_shoulder;
    g_current_servo_angles[ELBOW_SERVO_CHANNEL] = init_angle_elbow;
    g_current_servo_angles[GRIPPER_SERVO_CHANNEL] = init_gripper_angle;

    kal_printf("Initial Target Angles -> Base:%d, Shoulder:%d, Elbow:%d, Gripper:%d\r\n", 
               init_angle_base, init_angle_shoulder, init_angle_elbow, init_gripper_angle);

    // 3. 直接设置舵机到目标位置，因为这是“上电复位”，不需要平滑过程
    Pca9685Servo_SetAngle(BASE_SERVO_CHANNEL, init_angle_base);
    Pca9685Servo_SetAngle(SHOULDER_SERVO_CHANNEL, init_angle_shoulder);
    Pca9685Servo_SetAngle(ELBOW_SERVO_CHANNEL, init_angle_elbow);
    Pca9685Servo_SetAngle(GRIPPER_SERVO_CHANNEL, init_gripper_angle);
    
    // 4. 延时一小段时间确保舵机到位
    osal_msleep(500); 
    
    // 5. 释放所有舵机，使其进入“放松”状态
    Pca9685Servo_ReleaseChannel(BASE_SERVO_CHANNEL);
    Pca9685Servo_ReleaseChannel(SHOULDER_SERVO_CHANNEL);
    Pca9685Servo_ReleaseChannel(ELBOW_SERVO_CHANNEL);
    Pca9685Servo_ReleaseChannel(GRIPPER_SERVO_CHANNEL);

    // 主循环现在负责检查和处理后续指令
    while (1) {
        uapi_watchdog_kick();

        // --- 消费者逻辑：检查是否有新指令 ---
        if (g_new_command_received) {
            // 在任务上下文中安全地处理指令
            // kal_printf("CMD: %s\r\n", g_command_buffer); // 这行可以注释掉，因为 process_arm_command 内部会打印

            // 调用函数来处理指令
            process_arm_command(g_command_buffer);

            // 清除标志，表示指令已处理，可以接收下一条
            g_new_command_received = false;
        }
        
        osal_msleep(20); // 短暂延时，让出CPU，降低占用率
    }
}

#define Servo_TASK_STACK_SIZE 0x1000 // 舵机任务堆栈大小
#define Servo_TASK_PRIO (osPriority_t)(24) // 舵机任务优先级


/**
 * @brief 应用程序入口点，用于创建和启动舵机控制任务。
 */
void Servo_entry(void)
{
    osThreadAttr_t attr1; // CMSIS OS V2 线程属性结构体
    attr1.name = "ServoTASK"; // 任务名称
    attr1.attr_bits = 0U;    // 线程属性位 (通常为0)
    attr1.cb_mem = NULL;     // 控制块内存 (由OS分配)
    attr1.cb_size = 0U;      // 控制块大小
    attr1.stack_mem = NULL;  // 堆栈内存 (由OS分配)
    attr1.stack_size = Servo_TASK_STACK_SIZE; // 任务堆栈大小
    attr1.priority = Servo_TASK_PRIO;         // 任务优先级

    // 创建新线程 (任务)
    if (osThreadNew((osThreadFunc_t)pca9685_servo_task, NULL, &attr1) == NULL)
    {
        /* Create task fail. */
        kal_printf("Create ServoTASK Fail!");
    }
}

// 注册应用程序入口函数 (如果编译出错可以注释掉这行)
app_run(Servo_entry);

static void process_arm_command(const char *command_buffer)
{
    // --- 清理代码保持不变 ---
    char clean_buffer[ARM_UART_BUFFER_SIZE];
    strncpy(clean_buffer, command_buffer, sizeof(clean_buffer) - 1);
    clean_buffer[sizeof(clean_buffer) - 1] = '\0';

    int len = strlen(clean_buffer);
    while (len > 0 && (clean_buffer[len - 1] == '\n' || clean_buffer[len - 1] == '\r')) {
        clean_buffer[--len] = '\0';
    }

    float x, y, z;
    int channel = -1, angle = -1;
    
    // --- 修改：更新用于存储当前机械臂位置的静态变量 ---
    static float current_x = 107.0f; // 初始X位置
    static float current_y = 48.0f;  // 初始Y位置  
    static float current_z = 60.0f;  // 初始Z位置

    // 现有的完整坐标控制
    if (sscanf(clean_buffer, "%f,%f,%f", &x, &y, &z) == 3) {
        kal_printf("CMD: GOTO -> x=%.2f, y=%.2f, z=%.2f\r\n", x, y, z);
        
        // 更新当前位置记录
        current_x = x;
        current_y = y;
        current_z = z;
        
        inverse_operation(x, y, z);
        
        if (check_kinematic_angles()) {
            kal_printf("Angles are valid within kinematic limits. Proceeding to move servos...\r\n");
            
            // --- 修改：应用标定偏移量 ---
            uint8_t final_angleA = constrain_angle((uint8_t)g_angleA + BASE_SERVO_OFFSET, BASE_ANGLE_MIN, BASE_ANGLE_MAX);
            uint8_t final_angleB = constrain_angle((uint8_t)g_angleB + SHOULDER_SERVO_OFFSET, SHOULDER_ANGLE_MIN, SHOULDER_ANGLE_MAX);
            uint8_t final_angleC = constrain_angle((uint8_t)g_angleC + ELBOW_SERVO_OFFSET, ELBOW_ANGLE_MIN, ELBOW_ANGLE_MAX);

            kal_printf("Final Calibrated & Constrained Angles -> A:%d, B:%d, C:%d\r\n", final_angleA, final_angleB, final_angleC);

            // 使用平滑运动控制
            uint8_t channels[] = {BASE_SERVO_CHANNEL, SHOULDER_SERVO_CHANNEL, ELBOW_SERVO_CHANNEL};
            uint8_t angles[] = {final_angleA, final_angleB, final_angleC};
            smooth_move_multiple_servos(channels, angles, 3);
            
        } else {
            kal_printf("Error: Calculated angles are out of kinematic range. Arm will not move.\r\n");
        }

    // --- 单轴控制指令也使用平滑运动 ---
    } else if (sscanf(clean_buffer, "X%f", &x) == 1) {
        // 只改变X坐标，Y和Z保持不变
        kal_printf("CMD: Move X to %.2f (Y=%.2f, Z=%.2f)\r\n", x, current_y, current_z);
        current_x = x;
        inverse_operation(current_x, current_y, current_z);
        
        if (check_kinematic_angles()) {
            uint8_t final_angleA = constrain_angle((uint8_t)g_angleA, BASE_ANGLE_MIN, BASE_ANGLE_MAX);
            uint8_t final_angleB = constrain_angle((uint8_t)g_angleB, SHOULDER_ANGLE_MIN, SHOULDER_ANGLE_MAX);
            uint8_t final_angleC = constrain_angle((uint8_t)g_angleC, ELBOW_ANGLE_MIN, ELBOW_ANGLE_MAX);
            
            uint8_t channels[] = {BASE_SERVO_CHANNEL, SHOULDER_SERVO_CHANNEL, ELBOW_SERVO_CHANNEL};
            uint8_t angles[] = {final_angleA, final_angleB, final_angleC};
            smooth_move_multiple_servos(channels, angles, 3);
        } else {
            kal_printf("Error: X=%.2f is unreachable with current Y,Z\r\n", x);
        }

    } else if (sscanf(clean_buffer, "Y%f", &y) == 1) {
        // 只改变Y坐标，X和Z保持不变
        kal_printf("CMD: Move Y to %.2f (X=%.2f, Z=%.2f)\r\n", y, current_x, current_z);
        current_y = y;
        inverse_operation(current_x, current_y, current_z);
        
        if (check_kinematic_angles()) {
            uint8_t final_angleA = constrain_angle((uint8_t)g_angleA, BASE_ANGLE_MIN, BASE_ANGLE_MAX);
            uint8_t final_angleB = constrain_angle((uint8_t)g_angleB, SHOULDER_ANGLE_MIN, SHOULDER_ANGLE_MAX);
            uint8_t final_angleC = constrain_angle((uint8_t)g_angleC, ELBOW_ANGLE_MIN, ELBOW_ANGLE_MAX);
            
            uint8_t channels[] = {BASE_SERVO_CHANNEL, SHOULDER_SERVO_CHANNEL, ELBOW_SERVO_CHANNEL};
            uint8_t angles[] = {final_angleA, final_angleB, final_angleC};
            smooth_move_multiple_servos(channels, angles, 3);
        } else {
            kal_printf("Error: Y=%.2f is unreachable with current X,Z\r\n", y);
        }

    } else if (sscanf(clean_buffer, "Z%f", &z) == 1) {
        // 只改变Z坐标，X和Y保持不变
        kal_printf("CMD: Move Z to %.2f (X=%.2f, Y=%.2f)\r\n", z, current_x, current_y);
        current_z = z;
        inverse_operation(current_x, current_y, current_z);
        
        if (check_kinematic_angles()) {
            uint8_t final_angleA = constrain_angle((uint8_t)g_angleA, BASE_ANGLE_MIN, BASE_ANGLE_MAX);
            uint8_t final_angleB = constrain_angle((uint8_t)g_angleB, SHOULDER_ANGLE_MIN, SHOULDER_ANGLE_MAX);
            uint8_t final_angleC = constrain_angle((uint8_t)g_angleC, ELBOW_ANGLE_MIN, ELBOW_ANGLE_MAX);
            
            uint8_t channels[] = {BASE_SERVO_CHANNEL, SHOULDER_SERVO_CHANNEL, ELBOW_SERVO_CHANNEL};
            uint8_t angles[] = {final_angleA, final_angleB, final_angleC};
            smooth_move_multiple_servos(channels, angles, 3);
        } else {
            kal_printf("Error: Z=%.2f is unreachable with current X,Y\r\n", z);
        }

    // --- 新增：查询当前位置指令 (已增强) ---
    } else if (strcmp(clean_buffer, "POS") == 0) {
        kal_printf("Current Position: X=%.2f, Y=%.2f, Z=%.2f\r\n", current_x, current_y, current_z);
        kal_printf("Current Angles (Actual): Base=%d, Shoulder=%d, Elbow=%d, Gripper=%d\r\n",
                   g_current_servo_angles[BASE_SERVO_CHANNEL],
                   g_current_servo_angles[SHOULDER_SERVO_CHANNEL],
                   g_current_servo_angles[ELBOW_SERVO_CHANNEL],
                   g_current_servo_angles[GRIPPER_SERVO_CHANNEL]);

    // --- 爪子控制修改：改为直接移动，不使用平滑运动 ---
    } else if (strcmp(clean_buffer, "GRAB") == 0) {
        kal_printf("CMD: GRAB (Direct Move)\r\n");
        // 1. 直接设置角度
        Pca9685Servo_SetAngle(GRIPPER_SERVO_CHANNEL, GRIPPER_ANGLE_CLOSE);
        // 2. 更新软件记录的当前角度
        g_current_servo_angles[GRIPPER_SERVO_CHANNEL] = GRIPPER_ANGLE_CLOSE;
        // 3. 短暂延时以确保舵机到位
        osal_msleep(200); 
        // 4. 释放舵机PWM
        Pca9685Servo_ReleaseChannel(GRIPPER_SERVO_CHANNEL);
        kal_printf("Gripper PWM released.\r\n");

    } else if (strcmp(clean_buffer, "RELEASE") == 0) {
        kal_printf("CMD: RELEASE (Direct Move)\r\n");
        // 1. 直接设置角度
        Pca9685Servo_SetAngle(GRIPPER_SERVO_CHANNEL, GRIPPER_ANGLE_OPEN);
        // 2. 更新软件记录的当前角度
        g_current_servo_angles[GRIPPER_SERVO_CHANNEL] = GRIPPER_ANGLE_OPEN;
        // 3. 短暂延时以确保舵机到位
        osal_msleep(200);
        // 4. 释放舵机PWM
        Pca9685Servo_ReleaseChannel(GRIPPER_SERVO_CHANNEL);
        kal_printf("Gripper PWM released.\r\n");

    } else if (sscanf(clean_buffer, "S%d %d", &channel, &angle) == 2) {
        if (channel >= 0 && channel < MAX_SERVO_CHANNELS && angle >= 0 && angle <= 180) {
            kal_printf("CMD: Set Servo %d to %d degrees\r\n", channel, angle);
            
            if (channel == GRIPPER_SERVO_CHANNEL) {
                // 夹爪使用直接移动
                kal_printf("Gripper direct move to %d.\r\n", angle);
                Pca9685Servo_SetAngle(GRIPPER_SERVO_CHANNEL, (uint8_t)angle);
                g_current_servo_angles[GRIPPER_SERVO_CHANNEL] = (uint8_t)angle;
                osal_msleep(200); // 等待舵机到位
                Pca9685Servo_ReleaseChannel(GRIPPER_SERVO_CHANNEL);
                kal_printf("Gripper PWM released.\r\n");
            } else {
                // 其他舵机使用平滑移动
                smooth_move_servo((uint8_t)channel, (uint8_t)angle);

                // --- 新增：移动完成后，更新坐标 ---
                // 1. 获取三个关节的当前实际角度
                uint8_t actual_A = g_current_servo_angles[BASE_SERVO_CHANNEL];
                uint8_t actual_B = g_current_servo_angles[SHOULDER_SERVO_CHANNEL];
                uint8_t actual_C = g_current_servo_angles[ELBOW_SERVO_CHANNEL];

                // 2. 反向应用偏移量，得到理论角度
                float theoretical_A = (float)actual_A - BASE_SERVO_OFFSET;
                float theoretical_B = (float)actual_B - SHOULDER_SERVO_OFFSET;
                float theoretical_C = (float)actual_C - ELBOW_SERVO_OFFSET;

                // 3. 调用正解算函数更新坐标
                forward_kinematics(theoretical_A, theoretical_B, theoretical_C, &current_x, &current_y, &current_z);
            }
        } else {
            kal_printf("Error: Invalid channel or angle for direct control.\r\n");
        }
    } else {
        kal_printf("Error: Unknown command format.\r\n");
    }
}
