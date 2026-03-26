#include "pinctrl.h"
#include "soc_osal.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include <math.h> // 用于 roundf

#define PCA9685_TASK_PRIO               (osPriority_t)(17) // 任务优先级，与 Servo_TASK_PRIO 似乎有重复定义，需确认使用哪个
// PCA9685 定义
#define PCA9685_I2C_ADDRESS             0x40  // PCA9685 的I2C从机地址。注意：标准地址通常是0x40，如果A0-A5引脚接地。0x80是7位地址0x40左移一位。请确认硬件配置。
#define PCA9685_MODE1                   0x00  // Mode register 1
#define PCA9685_PRESCALE                0xFE  // Prescaler for PWM output frequency
#define PCA9685_LED0_ON_L               0x06  // Starting address for LED0 (channel 0) ON Low byte register

#define PCA9685_MODE1_RESTART_BIT       (1 << 7) // Restart enabled
#define PCA9685_MODE1_SLEEP_BIT         (1 << 4) // Sleep mode (oscillator off)
#define PCA9685_MODE1_AI_BIT            (1 << 5) // Auto-Increment enabled

// 舵机定义 (针对50Hz PWM, 每周期4096个tick)
// 典型舵机脉冲持续时间：0.5ms (500us) 到 2.5ms (2500us) 对应 0-180 度。
// 50Hz时的Tick持续时间：(1 / 50Hz) / 4096 ticks = 20000us / 4096 = ~4.8828us/tick
#define PWM_FREQUENCY_HZ                50.0f // 目标PWM频率 (Hz)
#define SERVO_MIN_PULSE_US              500   // 舵机0度对应的最小脉冲宽度 (微秒)
#define SERVO_MAX_PULSE_US              2500  // 舵机180度对应的最大脉冲宽度 (微秒)
#define TICKS_PER_CYCLE                 4096  // PCA9685的PWM分辨率 (12-bit)

// 根据脉冲宽度和频率计算ticks
#define SERVO_MIN_TICKS                 (uint16_t)((SERVO_MIN_PULSE_US * TICKS_PER_CYCLE) / (1000000.0f / PWM_FREQUENCY_HZ)) // 0度对应的tick数
#define SERVO_MAX_TICKS                 (uint16_t)((SERVO_MAX_PULSE_US * TICKS_PER_CYCLE) / (1000000.0f / PWM_FREQUENCY_HZ)) // 180度对应的tick数


// I2C 配置
#define CONFIG_I2C_SUPPORT_MASTER       1 // 启用I2C主机支持
#include "i2c.h" // 包含I2C驱动头文件

#define I2C_BAUDRATE                    400000 // I2C总线波特率 (400kHz)
#define I2C_PIN_MODE                    2      // I2C功能引脚模式 (具体值需查阅pinctrl文档)
#define CONFIG_I2C_SCL_MASTER_PIN       16     // I2C SCL引脚号
#define CONFIG_I2C_SDA_MASTER_PIN       15     // I2C SDA引脚号
#define CONFIG_I2C_MASTER_BUS_ID        1      // 使用的I2C总线ID

// 任务配置
#define PCA9685_TASK_STACK_SIZE         0x1000 // PCA9685演示任务的堆栈大小
#define PCA9685_TASK_LOOP_DELAY_MS      20     // 舵机扫描步骤的延迟 (ms)

// --- PCA9685 函数前向声明 ---
static errcode_t pca9685_write_reg_buffer(uint8_t *buffer, uint32_t len);
static errcode_t pca9685_write_reg(uint8_t reg, uint8_t value);
static errcode_t pca9685_read_reg_byte(uint8_t reg, uint8_t *value);
static void pca9685_set_pwm_freq(float freq_hz);
static void pca9685_set_pwm(uint8_t channel, uint16_t on_ticks, uint16_t off_ticks);
static void pca9685_set_servo_angle(uint8_t channel, uint8_t angle);
static errcode_t pca9685_module_init(void);
static uint16_t map_value(uint8_t value, uint8_t in_min, uint8_t in_max, uint16_t out_min, uint16_t out_max); // 前向声明 map_value

/**
 * @brief 初始化I2C通信所需的GPIO引脚。
 * @note  此函数配置SCL和SDA引脚的模式和上拉。
 */
static void app_i2c_init_pin(void)
{
    /* I2C 引脚复用 */
    uapi_pin_set_mode(CONFIG_I2C_SCL_MASTER_PIN, I2C_PIN_MODE);
    uapi_pin_set_mode(CONFIG_I2C_SDA_MASTER_PIN, I2C_PIN_MODE);
    uapi_pin_set_pull(CONFIG_I2C_SCL_MASTER_PIN, PIN_PULL_TYPE_UP);
    uapi_pin_set_pull(CONFIG_I2C_SDA_MASTER_PIN, PIN_PULL_TYPE_UP);
}


// --- PCA9685 辅助函数 ---
/**
 * @brief 通过I2C向PCA9685写入一个数据缓冲区。
 * @param buffer 指向要写入数据的缓冲区的指针。第一个字节通常是寄存器地址。
 * @param len 要写入的数据长度。
 * @return errcode_t 操作结果，ERRCODE_SUCC 表示成功。
 */
static errcode_t pca9685_write_reg_buffer(uint8_t *buffer, uint32_t len) {
    i2c_data_t data_to_write;
    data_to_write.send_buf = buffer;
    data_to_write.send_len = len;
    data_to_write.receive_buf = NULL; // 对于纯写操作
    data_to_write.receive_len = 0;
    return uapi_i2c_master_write((i2c_bus_t)CONFIG_I2C_MASTER_BUS_ID, PCA9685_I2C_ADDRESS, &data_to_write);
}

/**
 * @brief 向PCA9685的指定寄存器写入一个字节的值。
 * @param reg 要写入的寄存器地址。
 * @param value 要写入的字节值。
 * @return errcode_t 操作结果，ERRCODE_SUCC 表示成功。
 */
static errcode_t pca9685_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value}; // 第一个字节是寄存器地址，第二个字节是要写入的值
    return pca9685_write_reg_buffer(buffer, sizeof(buffer));
}

/**
 * @brief 从PCA9685的指定寄存器读取一个字节的值。
 * @param reg 要读取的寄存器地址。
 * @param value 指向用于存储读取到的字节值的指针。
 * @return errcode_t 操作结果，ERRCODE_SUCC 表示成功。
 */
static errcode_t pca9685_read_reg_byte(uint8_t reg, uint8_t *value)
{
    errcode_t ret;
    i2c_data_t transfer_data;

    // 准备要发送的数据（要读取的寄存器地址）
    transfer_data.send_buf = &reg;
    transfer_data.send_len = 1;

    // 准备接收数据的缓冲区和长度
    transfer_data.receive_buf = value;
    transfer_data.receive_len = 1;

    // 使用 uapi_i2c_master_writeread 执行写后读操作
    ret = uapi_i2c_master_writeread((i2c_bus_t)CONFIG_I2C_MASTER_BUS_ID, PCA9685_I2C_ADDRESS, &transfer_data);

    if (ret != ERRCODE_SUCC) {
        osal_printk("PCA9685: 使用 writeread 读取寄存器 0x%02X 失败, ret = %d\r\n", reg, ret);
    }
    return ret;
}

/**
 * @brief 设置PCA9685的PWM输出频率。
 * @param freq_hz 期望的PWM频率，单位Hz。
 * @note  根据PCA9685数据手册计算并设置预分频值。
 *        操作期间会短暂将PCA9685置于睡眠模式。
 */
static void pca9685_set_pwm_freq(float freq_hz)
{
    // 公式来自PCA9685数据手册: prescale_value = round(osc_clock / (4096 * update_rate)) - 1
    // PCA9685内部振荡器约为25MHz
    uint8_t prescale_val = (uint8_t)(roundf(25000000.0f / (TICKS_PER_CYCLE * freq_hz)) - 1.0f);
    uint8_t old_mode = 0;
    errcode_t ret;

    ret = pca9685_read_reg_byte(PCA9685_MODE1, &old_mode);
    if (ret != ERRCODE_SUCC) {
        osal_printk("PCA9685: 设置频率前读取MODE1失败。\r\n");
        return;
    }

    uint8_t new_mode = (old_mode & ~PCA9685_MODE1_RESTART_BIT) | PCA9685_MODE1_SLEEP_BIT; // 清除重启位，设置睡眠位

    pca9685_write_reg(PCA9685_MODE1, new_mode);      // 进入睡眠模式
    pca9685_write_reg(PCA9685_PRESCALE, prescale_val); // 设置预分频器
    pca9685_write_reg(PCA9685_MODE1, old_mode & ~PCA9685_MODE1_SLEEP_BIT); // 唤醒 (清除睡眠位)

    osal_msleep(1); // 等待振荡器稳定 (数据手册建议重启后等待 >500us)

    // 设置 RESTART 和 Auto-Increment。RESTART 位将自动清除。
    // 确保AI位被设置，以便连续写入PWM寄存器。
    pca9685_write_reg(PCA9685_MODE1, (old_mode & ~PCA9685_MODE1_SLEEP_BIT) | PCA9685_MODE1_RESTART_BIT | PCA9685_MODE1_AI_BIT);
    // osal_printk("PCA9685: 频率设置为 %.1f Hz, 预分频: %d\r\n", freq_hz, prescale_val);
}

/**
 * @brief 设置PCA9685指定通道的PWM信号的ON和OFF时刻。
 * @param channel 要设置的PWM通道 (0-15)。
 * @param on_ticks PWM信号变为高电平的时刻 (0-4095)。
 * @param off_ticks PWM信号变为低电平的时刻 (0-4095)。
 * @note  通常对于舵机，on_ticks 设置为0。
 */
static void pca9685_set_pwm(uint8_t channel, uint16_t on_ticks, uint16_t off_ticks)
{
    if (channel > 15) {
        osal_printk("PCA9685: 无效通道 %d\r\n", channel);
        return;
    }

    uint8_t buffer[5]; // 寄存器地址 + 4字节数据 (ON_L, ON_H, OFF_L, OFF_H)
    buffer[0] = PCA9685_LED0_ON_L + (4 * channel); // 计算通道的起始寄存器地址
    buffer[1] = on_ticks & 0xFF;         // ON_L
    buffer[2] = (on_ticks >> 8) & 0x0F;  // ON_H (PCA9685是12位分辨率, 高4位在第二个字节的低4位)
    buffer[3] = off_ticks & 0xFF;        // OFF_L
    buffer[4] = (off_ticks >> 8) & 0x0F; // OFF_H

    if (pca9685_write_reg_buffer(buffer, sizeof(buffer)) != ERRCODE_SUCC) {
        osal_printk("PCA9685: 设置通道 CH%d 的PWM失败\r\n", channel);
    }
}

/**
 * @brief 将一个输入范围内的值线性映射到输出范围。
 * @param value 要映射的输入值。
 * @param in_min 输入范围的最小值。
 * @param in_max 输入范围的最大值。
 * @param out_min 输出范围的最小值。
 * @param out_max 输出范围的最大值。
 * @return uint16_t 映射后的输出值。
 */
static uint16_t map_value(uint8_t value, uint8_t in_min, uint8_t in_max, uint16_t out_min, uint16_t out_max)
{
    if (value <= in_min) return out_min;
    if (value >= in_max) return out_max;
    // 为了获得更好的精度，中间计算使用浮点数
    float mapped = ((float)(value - in_min) / (float)(in_max - in_min)) * (out_max - out_min) + out_min;
    return (uint16_t)roundf(mapped);
}

/**
 * @brief 设置指定舵机通道的角度。
 * @param channel 要控制的舵机通道 (0-15)。
 * @param angle 目标角度 (0-180 度)。
 * @note  此函数将角度转换为PCA9685所需的tick数，并设置PWM。
 */
static void pca9685_set_servo_angle(uint8_t channel, uint8_t angle)
{
    if (angle > 180) angle = 180; // 将角度限制在0-180度
    uint16_t pulse_ticks = map_value(angle, 0, 180, SERVO_MIN_TICKS, SERVO_MAX_TICKS);

    // 对于标准舵机，脉冲从tick 0开始 (ON_ticks = 0)
    // 并在pulse_ticks结束 (OFF_ticks = pulse_ticks)。
    pca9685_set_pwm(channel, 0, pulse_ticks);
    // osal_printk("PCA9685: 舵机 CH%d 设置角度 %d (%u ticks)\r\n", channel, angle, pulse_ticks);
}

/**
 * @brief 初始化PCA9685模块。
 * @note  此函数设置PCA9685的MODE1寄存器（启用自动增量），
 *        设置PWM频率，并将所有舵机通道初始化到90度（中间位置）。
 * @return errcode_t 操作结果，ERRCODE_SUCC 表示成功。
 */
static errcode_t pca9685_module_init(void)
{
    errcode_t ret;
    // 将MODE1重置为已知状态（正常模式）并启用自动增量
    ret = pca9685_write_reg(PCA9685_MODE1, PCA9685_MODE1_AI_BIT);
    if (ret != ERRCODE_SUCC) {
        osal_printk("PCA9685: 初始化设置MODE1失败, ret = %d\r\n", ret);
        return ret;
    }
    osal_msleep(1); // 如果振荡器关闭，则短暂延迟以使其稳定

    pca9685_set_pwm_freq(PWM_FREQUENCY_HZ);

    // 可选地，将所有舵机设置到默认位置 (例如，90度 - 中间位置)
    for (uint8_t i = 0; i < 16; i++) {
        pca9685_set_servo_angle(i, 90);
    }
    osal_printk("PCA9685 初始化完成并且舵机已居中。\r\n");
    return ERRCODE_SUCC;
}

/**
 * @brief 全局初始化函数，用于PCA9685舵机驱动。
 * @note  此函数负责初始化I2C引脚、I2C主机以及PCA9685模块本身。
 *        应在任何舵机控制操作之前调用。
 * @return errcode_t 操作结果，ERRCODE_SUCC 表示成功。
 */
errcode_t Pca9685Servo_GlobalInit(void)
{
    errcode_t ret;

    // 1. 初始化 I2C 引脚
    app_i2c_init_pin();

    // 2. 初始化 I2C 主机
    // 第三个参数 hscode 对于WS63通常为0，表示不支持高速模式
    ret = uapi_i2c_master_init((i2c_bus_t)CONFIG_I2C_MASTER_BUS_ID, I2C_BAUDRATE, 0 /* hscode */);
    if (ret != ERRCODE_SUCC) {
        osal_printk("PCA9685_GlobalInit: 初始化I2C主机失败, ret = %d\r\n", ret);
        return ret;
    }
    osal_printk("PCA9685_GlobalInit: I2C 主机已初始化 (总线 %d, 波特率 %d)\r\n", CONFIG_I2C_MASTER_BUS_ID, I2C_BAUDRATE);

    // 3. 初始化 PCA9685 模块
    ret = pca9685_module_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("PCA9685_GlobalInit: 模块初始化失败, ret = %d\r\n", ret);
        // 可选: 如果模块初始化失败，可以考虑反初始化I2C主机
        // uapi_i2c_deinit((i2c_bus_t)CONFIG_I2C_MASTER_BUS_ID);
        return ret;
    }
    
    osal_printk("PCA9685_GlobalInit: PCA9685 驱动初始化完成。\r\n");
    return ERRCODE_SUCC;
}

/**
 * @brief 设置指定舵机通道的角度 (公开API)。
 * @param channel 要控制的舵机通道 (0-15)。
 * @param angle 目标角度 (0-180 度)。
 */
void Pca9685Servo_SetAngle(uint8_t channel, uint8_t angle)
{
    // 直接调用内部的舵机角度设置函数
    pca9685_set_servo_angle(channel, angle);
    // osal_printk("Pca9685Servo_SetAngle: CH%d set to %d degrees\r\n", channel, angle);
}

/**
 * @brief 释放指定通道的舵机 (关闭PWM输出)
 * @param channel 要释放的舵机通道 (0-15)
 * @return errcode_t 
 */
errcode_t Pca9685Servo_ReleaseChannel(uint8_t channel)
{
    if (channel > 15) {
        return ERRCODE_INVALID_PARAM;
    }
    
    // 设置 ON 寄存器为 0, OFF 寄存器的高位为 1 (完全关闭)
    // 这是 PCA9685 关闭特定通道 PWM 输出的标准方法
    uint8_t off_reg_val = 0x10; // bit 4 = 1
    
    // 修正：使用正确的宏名称和函数调用
    // PCA9685_LED0_ON_L 是 ON_L 寄存器地址
    // PCA9685_LED0_ON_L + 2 是 OFF_L 寄存器地址
    // PCA9685_LED0_ON_L + 3 是 OFF_H 寄存器地址
    errcode_t ret = pca9685_write_reg(PCA9685_LED0_ON_L + 4 * channel, 0x00);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    
    // 修正：使用正确的函数调用，并计算 OFF_H 寄存器的地址
    ret = pca9685_write_reg(PCA9685_LED0_ON_L + 3 + 4 * channel, off_reg_val);
    return ret;
}

