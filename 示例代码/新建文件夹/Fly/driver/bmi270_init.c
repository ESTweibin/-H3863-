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
#include "attitude_estimation.h"


// FIFO配置宏定义
#define BMI2_FIFO_RAW_DATA_BUFFER_SIZE  2048
#define BMI2_FIFO_RAW_DATA_USER_LENGTH  2048
#define BMI2_FIFO_ACCEL_FRAME_COUNT     150
#define BMI2_FIFO_GYRO_FRAME_COUNT      150
#define BMI2_FIFO_AUX_FRAME_COUNT       150
#define SENSORTIME_OVERHEAD_BYTE        220

// 使用最新的几帧数据
#define LATEST_FRAMES_TO_PROCESS 20

// 传感器选择宏
#define ACCEL                       0x00
#define GYRO                        0x01
#define AUX                         0x02

uint16_t int_status = 0;

uint8_t sensor_list[3] = {BMI2_ACCEL, BMI2_GYRO, BMI2_AUX};

uint16_t accel_frame_length = BMI2_FIFO_ACCEL_FRAME_COUNT;

uint16_t gyro_frame_length = BMI2_FIFO_GYRO_FRAME_COUNT;

uint16_t aux_frame_length = BMI2_FIFO_AUX_FRAME_COUNT;

uint16_t fifo_length = 0;

int8_t try = 1;

struct bmi2_fifo_frame fifoframe = { 0 };

// 传感器列表
struct bmi2_sens_config config[3];

// 全局变量
static struct bmi2_dev bmi270_dev;
static bmp280_calib_param_t bmp_calib;

// FIFO缓冲区和数据结构
static struct bmi2_sens_axes_data fifo_accel_data[BMI2_FIFO_ACCEL_FRAME_COUNT] = { { 0 } };
static struct bmi2_sens_axes_data fifo_gyro_data[BMI2_FIFO_GYRO_FRAME_COUNT] = { { 0 } };
static struct bmi2_aux_fifo_data fifo_aux_data[BMI2_FIFO_AUX_FRAME_COUNT] = { { .data = { 0 }, .virt_sens_time = 0 } };
static uint8_t fifo_data[BMI2_FIFO_RAW_DATA_BUFFER_SIZE + SENSORTIME_OVERHEAD_BYTE];



// 配置传感器具体参数（ODR、量程等）
static int8_t set_accel_gyro_bmp280_config(struct bmi2_dev *bmi)
{
    int8_t rslt;

    printf("传感器配置开始...\n");
    
    // 初始化传感器配置类型
    config[ACCEL].type = BMI2_ACCEL;
    config[GYRO].type = BMI2_GYRO;
    config[AUX].type = BMI2_AUX;
    
     /* Get default configurations for the type of feature selected. */
    rslt = bmi2_get_sensor_config(config, 3, bmi);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK)
    {
        /* NOTE: The user can change the following configuration parameters according to their requirement. */
        /* Set Output Data Rate */
        config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_400HZ;

        /* Gravity range of the sensor (+/- 2G, 4G, 8G, 16G). */
        config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_8G;

        /* The bandwidth parameter is used to configure the number of sensor samples that are averaged
         * if it is set to 2, then 2^(bandwidth parameter) samples
         * are averaged, resulting in 4 averaged samples.
         * Note1 : For more information, refer the datasheet.
         * Note2 : A higher number of averaged samples will result in a lower noise level of the signal, but
         * this has an adverse effect on the power consumed.
         */
        config[ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;

        /* Enable the filter performance mode where averaging of samples
         * will be done based on above set bandwidth and ODR.
         * There are two modes
         *  0 -> Ultra low power mode
         *  1 -> High performance mode(Default)
         * For more info refer datasheet.
         */
        config[ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

        /* The user can change the following configuration parameters according to their requirement. */
        /* Set Output Data Rate */
        config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_400HZ;

        /* Gyroscope Angular Rate Measurement Range.By default the range is 2000dps. */
        config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;

        /* Gyroscope bandwidth parameters. By default the gyro bandwidth is in normal mode. */
        config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;

        /* Enable/Disable the noise performance mode for precision yaw rate sensing
         * There are two modes
         *  0 -> Ultra low power mode(Default)
         *  1 -> High performance mode
         */
        config[GYRO].cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;

        /* Enable/Disable the filter performance mode where averaging of samples
         * will be done based on above set bandwidth and ODR.
         * There are two modes
         *  0 -> Ultra low power mode
         *  1 -> High performance mode(Default)
         */
        config[GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

        // 配置辅助传感器
        config[AUX].cfg.aux.odr = BMI2_AUX_ODR_400HZ;
        config[AUX].cfg.aux.aux_en = BMI2_ENABLE;
        config[AUX].cfg.aux.i2c_device_addr = BMP280_I2C_ADDR;
        config[AUX].cfg.aux.fcu_write_en = BMI2_ENABLE;
        config[AUX].cfg.aux.man_rd_burst = BMI2_AUX_READ_LEN_2;  // 6字节读取，完美匹配BMP280需求
        config[AUX].cfg.aux.read_addr = BMP280_REG_PRESS_MSB;  // 从压力数据开始读取

        /* Set the accel and gyro configurations. */
        rslt = bmi2_set_sensor_config(config, 3, bmi);
        bmi2_error_codes_print_result(rslt);


    }

    return rslt;
}

// BMI270初始化函数，配置为有辅助传感器、水满检测中断、无硬件中断引脚、帧模式。
errcode_t bmi270_aux_full_fill_header_mode_init(void)
{
    int8_t rslt;

    bmi2_delay_us(2000,NULL);  // 等待系统稳定  

    rslt = bmi2_interface_init(&bmi270_dev, BMI2_I2C_INTF);
    bmi2_error_codes_print_result(rslt);

    rslt = bmi270_init(&bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    // 启用I2C看门狗以防止SCL挂死
    printf("启用I2C看门狗...\n");
    rslt = bmi2_set_i2c_wdt_en(BMI2_ENABLE, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);
    if (rslt == BMI2_OK) {
        // 设置看门狗超时时间为40ms（较长超时，适合复杂操作）
        rslt = bmi2_set_i2c_wdt_sel(BMI2_ENABLE, &bmi270_dev);
        bmi2_error_codes_print_result(rslt);
        if (rslt == BMI2_OK) {
            printf("I2C看门狗启用成功 (40ms超时)\n");
        } else {
            printf("I2C看门狗超时设置失败: %d\n", rslt);
        }
    } else {
        printf("I2C看门狗启用失败: %d\n", rslt);
    }

    // 配置2K上拉电阻
    uint8_t regdata = BMI2_ASDA_PUPSEL_2K;
    rslt = bmi2_set_regs(BMI2_AUX_IF_TRIM, &regdata, 1, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    set_accel_gyro_bmp280_config(&bmi270_dev);
    
    /* NOTE:
     * Accel and gyro enable must be done after setting configurations
     */
    rslt = bmi270_sensor_enable(sensor_list, 3, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

        
    // 初始化BMP280
    if (init_bmp280(&bmp_calib, &bmi270_dev) != ERRCODE_SUCC) {
        printf("BMP280初始化失败\n");
        return ERRCODE_FAIL;
    }
    
    rslt = bmi270_get_sensor_config(config, 3, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    /* Disable manual mode so that the data mode is enabled. */
    config[AUX].cfg.aux.manual_en = BMI2_DISABLE;

    /* Set the aux configurations. */
    rslt = bmi270_set_sensor_config(config, 3, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    /* Before setting FIFO, disable the advance power save mode. */
    rslt = bmi2_set_adv_power_save(BMI2_DISABLE, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    /* Initially disable all configurations in fifo. */
    rslt = bmi2_set_fifo_config(BMI2_FIFO_ALL_EN, BMI2_DISABLE, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    /* Update FIFO structure. */
    /* Mapping the buffer to store the fifo data. */
    fifoframe.data = fifo_data;

    /* Length of FIFO frame. */
    /* To read sensortime, extra 3 bytes are added to fifo user length. */
    fifoframe.length = BMI2_FIFO_RAW_DATA_USER_LENGTH + SENSORTIME_OVERHEAD_BYTE;

    /* Set FIFO configuration by enabling accel, gyro and timestamp.
     * NOTE 1: The header mode is enabled by default.
     * NOTE 2: By default the FIFO operating mode is in FIFO mode.
     * NOTE 3: Sensortime is enabled by default */
    printf("FIFO is configured in header mode\n");
    rslt = bmi2_set_fifo_config(BMI2_FIFO_ALL_EN, BMI2_ENABLE, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    /* Map FIFO full interrupt. */
    fifoframe.data_int_map = BMI2_FFULL_INT;
    rslt = bmi2_map_data_int(fifoframe.data_int_map, BMI2_INT1, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    // 初始化姿态估计模块
    if (rslt == BMI2_OK) {
        printf("初始化姿态估计模块...\n");
        attitude_estimation_init();
        printf("姿态估计模块初始化完成\n");
    }

    return (rslt == BMI2_OK) ? ERRCODE_SUCC : ERRCODE_FAIL;
}


//读取count轮FIFO全满时传感器数据
// count为读取次数
errcode_t get_bmi270_sensor_data(int8_t count)
{
    int8_t rslt = BMI2_E_COM_FAIL; // 初始化为失败状态

    int8_t try = 0;

    char buffer[100];

    while (try < count)
    { 
        /* Read FIFO data on interrupt. */
        rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
        bmi2_error_codes_print_result(rslt);

        if ((rslt == BMI2_OK) && (int_status & BMI2_FFULL_INT_STATUS_MASK))
        {
            printf("\nIteration : %d\n", try);

            accel_frame_length = BMI2_FIFO_ACCEL_FRAME_COUNT;

            gyro_frame_length = BMI2_FIFO_GYRO_FRAME_COUNT;

            aux_frame_length = BMI2_FIFO_AUX_FRAME_COUNT;

            rslt = bmi2_get_fifo_length(&fifo_length, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);

            /* Updating FIFO length to be read based on available length and dummy byte updation */
            fifoframe.length = fifo_length + SENSORTIME_OVERHEAD_BYTE + bmi270_dev.dummy_byte;

            printf("\nFIFO data bytes available : %d \n", fifo_length);
            printf("\nFIFO data bytes requested : %d \n", fifoframe.length);

            /* Read FIFO data. */
            rslt = bmi2_read_fifo_data(&fifoframe, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);

            /* Read FIFO data on interrupt. */
            rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);

            if (rslt == BMI2_OK)
            {
                printf("\nFIFO accel frames requested : %d \n", accel_frame_length);

                /* Parse the FIFO data to extract accelerometer data from the FIFO buffer. */
                rslt = bmi2_extract_accel(fifo_accel_data, &accel_frame_length, &fifoframe, &bmi270_dev);
                printf("\nFIFO accel frames extracted : %d \n", accel_frame_length);

                printf("\nFIFO gyro frames requested : %d \n", gyro_frame_length);

                /* Parse the FIFO data to extract gyro data from the FIFO buffer. */
                (void)bmi2_extract_gyro(fifo_gyro_data, &gyro_frame_length, &fifoframe, &bmi270_dev);
                printf("\nFIFO gyro frames extracted : %d \n", gyro_frame_length);

                printf("\nFIFO aux frames requested : %d \n", aux_frame_length);

                /* Parse the FIFO data to extract aux data from the FIFO buffer. */
                (void)bmi2_extract_aux(fifo_aux_data, &aux_frame_length, &fifoframe, &bmi270_dev);
                printf("\nFIFO aux frames extracted : %d \n", aux_frame_length);

                // printf("\nExtracted accel frames\n");

                // printf("ACCEL_DATA, X, Y, Z\n");

                // int index;

                // /* Print the parsed accelerometer data from the FIFO buffer. */
                // for (index = 0; index < accel_frame_length; index++)
                // {
                //     printf("%d, %d, %d, %d\n",
                //             index,
                //             fifo_accel_data[index].x,
                //             fifo_accel_data[index].y,
                //             fifo_accel_data[index].z);
                // }

                // printf("\nExtracted gyro frames\n");

                // printf("GYRO_DATA, X, Y, Z\n");

                // /* Print the parsed gyro data from the FIFO buffer. */
                // for (index = 0; index < gyro_frame_length; index++)
                // {
                //     printf("%d, %d, %d, %d\n",
                //             index,
                //             fifo_gyro_data[index].x,
                //             fifo_gyro_data[index].y,
                //             fifo_gyro_data[index].z);
                // }

                // FIFO数据处理和姿态估计
                if (accel_frame_length > 0 && gyro_frame_length > 0)
                {
                    printf("\n=== 开始处理 %d 帧FIFO数据 ===\n", accel_frame_length);
                    
                    uint16_t min_frames = (accel_frame_length < gyro_frame_length) ? 
                                        accel_frame_length : gyro_frame_length;
                    
                    // 逐帧处理所有FIFO数据
                    for (uint16_t i = 0; i < min_frames; i++)
                    {
                        attitude_estimation_update(&fifo_accel_data[i], &fifo_gyro_data[i]);
                        
                        // 可选：每10帧打印一次中间状态
                        if ((i + 1) % 10 == 0) {
                            attitude_t temp_attitude = get_current_attitude();
                            snprintf(buffer, 100, "第 %d/%d 帧: Roll=%.1f° Pitch=%.1f° Yaw=%.1f°\n", 
                                i + 1, min_frames, temp_attitude.roll, temp_attitude.pitch, temp_attitude.yaw);
                            printf("%s\n", buffer);
                        }
                    }
                    
                    // 最终结果
                    attitude_t final_attitude = get_current_attitude();
                    angular_rate_t final_rates = get_current_angular_rate();
                    
                    printf("=== 处理完成，最终姿态 ===\n");
                    snprintf(buffer, 100, "最终姿态: Roll=%.2f° Pitch=%.2f° Yaw=%.2f°\n",
                        final_attitude.roll, final_attitude.pitch, final_attitude.yaw);
                    printf("%s\n", buffer);
                    snprintf(buffer, 100, "最终角速度: Roll=%.2f°/s Pitch=%.2f°/s Yaw=%.2f°/s\n",
                        final_rates.roll_rate, final_rates.pitch_rate, final_rates.yaw_rate);
                    printf("%s\n", buffer);
                }

                // printf("\nExtracted BMP280 AUX frames\n");

                // printf("BMP280_DATA, Index, Temperature(°C), Pressure(hPa)\n");


                // process_bmp280_aux_data(bmp_calib, fifo_aux_data, aux_frame_length);

                /* Print control frames like sensor time and skipped frame count. */
                printf("\nSkipped frame count = %d\n", fifoframe.skipped_frame_count);

                
                (void)snprintf(buffer, 100, "Sensor time(in seconds) = %.4lf  s\r\n",
                                            (fifoframe.sensor_time * BMI2_SENSORTIME_RESOLUTION));
                printf("%s\n", buffer);

                try++;
            }
        }
    }

    return (rslt == BMI2_OK) ? ERRCODE_SUCC : ERRCODE_FAIL;
}


/**
 * @brief 读取BMI270传感器的姿态数据到数组中
 * 
 * 功能描述：
 * - 等待FIFO满中断触发，读取N次BMI270的FIFO数据
 * - 对每次读取的数据进行姿态解算（融合加速度计和陀螺仪数据）
 * - 将处理后的姿态角度和角速度数据存储到指定的float数组中
 * - 每次读取使用最新的5帧数据进行姿态估计，提高数据质量
 * - 包含超时保护和错误处理机制
 * 
 * 数据存储格式：
 * 数组中每6个连续的float值表示一次测量结果，按以下顺序存储：
 * [0] - Roll角度 (滚转角，单位：度)
 * [1] - Pitch角度 (俯仰角，单位：度)  
 * [2] - Yaw角度 (偏航角，单位：度)
 * [3] - Roll角速度 (滚转角速度，单位：度/秒)
 * [4] - Pitch角速度 (俯仰角速度，单位：度/秒)
 * [5] - Yaw角速度 (偏航角速度，单位：度/秒)
 * 
 * @param data_array    [输出] 存储姿态数据的float数组指针，数组会被修改
 *                      数组大小必须 >= N * 6 个float元素
 * @param array_size    [输入] data_array数组的实际大小（以float元素个数计算）
 *                      用于边界检查，防止数组越界
 * @param N            [输入] 需要读取的数据次数，取值范围：1-127
 *                      每次读取对应一次FIFO满中断的数据处理
 * 
 * @return 无返回值（void函数）
 *         结果通过修改data_array数组内容返回
 * 
 * @note 使用前提：
 *       1. 必须先调用 bmi270_aux_full_fill_header_mode_init() 初始化BMI270
 *       2. 必须先调用 attitude_estimation_init() 初始化姿态估计模块
 *       3. BMI270必须配置为FIFO满中断模式
 * 
 * @note 错误处理：
 *       - 如果参数无效（空指针、N<=0、数组太小），函数会打印错误信息并返回
 *       - 如果FIFO读取超时，会填充0值并继续处理下一次读取
 *       - 如果数据解析失败，会跳过该次读取并继续
 * 
 * @note 性能说明：
 *       - 每次FIFO满大约包含150帧原始数据
 *       - 函数只使用最新的20帧进行姿态计算，平衡精度和性能,若需修改解算的帧数，则修改LATEST_FRAMES_TO_PROCESS宏定义
 *       - 每次读取大约需要等待FIFO满中断（时间取决于ODR配置）
 */
void read_bmi270_imu_data_array(float *data_array, int array_size, int8_t N)
{
    if (data_array == NULL || N <= 0) {
        printf("错误: 数组指针为空或N值无效\n");
        return;
    }
    
    // 每组数据包含6个float值：roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate
    const int values_per_sample = 6;
    
    if (array_size < N * values_per_sample) {
        printf("错误: 数组大小不足，需要至少 %d 个float元素\n", N * values_per_sample);
        return;
    }
    
    int8_t rslt;
    int8_t successful_reads = 0;
    
    printf("开始读取 %d 次BMI270 FIFO数据...\n", N);
    
    for (int8_t read_count = 0; read_count < N; read_count++) {
        // 等待FIFO满中断
        bool fifo_ready = false;
        int timeout_count = 0;
        const int max_timeout = 1000; // 最大等待1000次
        
        while (!fifo_ready && timeout_count < max_timeout) {
            /* Read FIFO data on interrupt. */
            rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
            
            if ((rslt == BMI2_OK) && (int_status & BMI2_FFULL_INT_STATUS_MASK)) {
                fifo_ready = true;
            } else {
                osal_mdelay(1); // 等待1ms
                timeout_count++;
            }
        }
        
        if (!fifo_ready) {
            printf("警告: 第 %d 次读取FIFO超时\n", read_count + 1);
            // 填充默认值
            int base_index = successful_reads * values_per_sample;
            for (int i = 0; i < values_per_sample; i++) {
                data_array[base_index + i] = 0.0f;
            }
            successful_reads++;
            continue;
        }
        
        printf("第 %d/%d 次FIFO中断触发\n", read_count + 1, N);
        
        // 重置帧长度
        accel_frame_length = BMI2_FIFO_ACCEL_FRAME_COUNT;
        gyro_frame_length = BMI2_FIFO_GYRO_FRAME_COUNT;
        aux_frame_length = BMI2_FIFO_AUX_FRAME_COUNT;
        
        // 获取FIFO长度
        rslt = bmi2_get_fifo_length(&fifo_length, &bmi270_dev);
        if (rslt != BMI2_OK) {
            printf("错误: 第 %d 次获取FIFO长度失败\n", read_count + 1);
            continue;
        }
        
        /* Updating FIFO length to be read based on available length and dummy byte updation */
        fifoframe.length = fifo_length + SENSORTIME_OVERHEAD_BYTE + bmi270_dev.dummy_byte;
        
        printf("FIFO可用字节: %d, 请求字节: %d\n", fifo_length, fifoframe.length);
        
        /* Read FIFO data. */
        rslt = bmi2_read_fifo_data(&fifoframe, &bmi270_dev);
        if (rslt != BMI2_OK) {
            printf("错误: 第 %d 次读取FIFO数据失败\n", read_count + 1);
            continue;
        }
        
        // 解析FIFO数据
        rslt = bmi2_extract_accel(fifo_accel_data, &accel_frame_length, &fifoframe, &bmi270_dev);
        if (rslt != BMI2_OK) {
            printf("错误: 第 %d 次解析加速度数据失败\n", read_count + 1);
            continue;
        }
        
        rslt = bmi2_extract_gyro(fifo_gyro_data, &gyro_frame_length, &fifoframe, &bmi270_dev);
        if (rslt != BMI2_OK) {
            printf("错误: 第 %d 次解析陀螺仪数据失败\n", read_count + 1);
            continue;
        }
        
        printf("解析得到: 加速度 %d 帧, 陀螺仪 %d 帧\n", accel_frame_length, gyro_frame_length);
        
        // 处理数据进行姿态估计
        if (accel_frame_length > 0 && gyro_frame_length > 0) {
            uint16_t min_frames = (accel_frame_length < gyro_frame_length) ? 
                                accel_frame_length : gyro_frame_length;
            

            uint16_t frames_to_process = (min_frames > LATEST_FRAMES_TO_PROCESS) ? 
                                       LATEST_FRAMES_TO_PROCESS : min_frames;
            uint16_t start_index = min_frames - frames_to_process;
            
            // 逐帧更新姿态估计
            for (uint16_t i = start_index; i < min_frames; i++) {
                attitude_estimation_update(&fifo_accel_data[i], &fifo_gyro_data[i]);
            }
            
            // 获取姿态数据并存储到数组中（作为原始float数组）
            attitude_t current_attitude = get_current_attitude();
            angular_rate_t current_rates = get_current_angular_rate();
            
            // 计算在数组中的存储位置
            int base_index = successful_reads * values_per_sample;
            
            // 按顺序存储：roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate
            data_array[base_index + 0] = current_attitude.roll;
            data_array[base_index + 1] = current_attitude.pitch;
            data_array[base_index + 2] = current_attitude.yaw;
            data_array[base_index + 3] = current_rates.roll_rate;
            data_array[base_index + 4] = current_rates.pitch_rate;
            data_array[base_index + 5] = current_rates.yaw_rate;
            
            printf("第 %d 次数据存储: Roll=%.2f° Pitch=%.2f° Yaw=%.2f°, 角速度: Roll=%.2f°/s Pitch=%.2f°/s Yaw=%.2f°/s\n",
                   read_count + 1,
                   data_array[base_index + 0], data_array[base_index + 1], data_array[base_index + 2],
                   data_array[base_index + 3], data_array[base_index + 4], data_array[base_index + 5]);
            
            successful_reads++;
        } else {
            printf("警告: 第 %d 次没有获取到有效的传感器数据\n", read_count + 1);
            // 填充默认值
            int base_index = successful_reads * values_per_sample;
            for (int i = 0; i < values_per_sample; i++) {
                data_array[base_index + i] = 0.0f;
            }
            successful_reads++;
        }
    }
    
    printf("数据读取完成: 成功读取 %d/%d 次数据\n", successful_reads, N);
}