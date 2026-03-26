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
#include "attitude_estimation.h"


// FIFO配置宏定义
#define BMI2_FIFO_RAW_DATA_BUFFER_SIZE  2048
#define BMI2_FIFO_RAW_DATA_USER_LENGTH  2048
#define BMI2_FIFO_ACCEL_FRAME_COUNT     200
#define BMI2_FIFO_GYRO_FRAME_COUNT      200
#define SENSORTIME_OVERHEAD_BYTE        220

// 传感器选择宏
#define ACCEL                       0x00
#define GYRO                        0x01

uint16_t int_status = 0;

uint8_t sensor_list[2] = {BMI2_ACCEL, BMI2_GYRO};

uint16_t accel_frame_length = BMI2_FIFO_ACCEL_FRAME_COUNT;

uint16_t gyro_frame_length = BMI2_FIFO_GYRO_FRAME_COUNT;

uint16_t fifo_length = 0;

int8_t try = 1;

struct bmi2_fifo_frame fifoframe = { 0 };

// 传感器列表
struct bmi2_sens_config config[2];

// 全局变量
static struct bmi2_dev bmi270_dev;

// FIFO缓冲区和数据结构
static struct bmi2_sens_axes_data fifo_accel_data[BMI2_FIFO_ACCEL_FRAME_COUNT] = { { 0 } };
static struct bmi2_sens_axes_data fifo_gyro_data[BMI2_FIFO_GYRO_FRAME_COUNT] = { { 0 } };
static uint8_t fifo_data[BMI2_FIFO_RAW_DATA_BUFFER_SIZE + SENSORTIME_OVERHEAD_BYTE];



// 配置传感器具体参数（ODR、量程等）
static int8_t set_accel_gyro_config(struct bmi2_dev *bmi)
{
    int8_t rslt;

    printf("传感器配置开始...\n");
    
    // 初始化传感器配置类型
    config[ACCEL].type = BMI2_ACCEL;
    config[GYRO].type = BMI2_GYRO;
    
     /* Get default configurations for the type of feature selected. */
    rslt = bmi2_get_sensor_config(config, 2, bmi);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK)
    {
        /* NOTE: The user can change the following configuration parameters according to their requirement. */
        /* Set Output Data Rate */
        config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_800HZ;

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
        config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_800HZ;

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

        /* Set the accel and gyro configurations. */
        rslt = bmi2_set_sensor_config(config, 2, bmi);
        bmi2_error_codes_print_result(rslt);


    }

    return rslt;
}

// BMI270初始化函数，配置为FIFO满检测中断、无硬件中断引脚、帧模式。
errcode_t bmi270_full_fill_header_mode_init(void)
{
    int8_t rslt;

    bmi2_delay_us(2000,NULL);  // 等待系统稳定  

    rslt = bmi2_interface_init(&bmi270_dev, BMI2_I2C_INTF);
    bmi2_error_codes_print_result(rslt);

    rslt = bmi270_init(&bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    // BMI270 I2C看门狗配置 - 检测I2C总线是否挂死
    if (rslt == BMI2_OK)
    {
        printf("配置BMI270 I2C看门狗...\n");
        
        // 启用I2C看门狗
        rslt = bmi2_set_i2c_wdt_en(BMI2_ENABLE, &bmi270_dev);
        if (rslt == BMI2_OK)
        {
            printf("I2C看门狗启用成功\n");
            
            // 设置I2C看门狗超时时间 (BMI2_ENABLE = 40ms, BMI2_DISABLE = 1.25ms)
            rslt = bmi2_set_i2c_wdt_sel(BMI2_ENABLE, &bmi270_dev);
            if (rslt == BMI2_OK)
            {
                printf("I2C看门狗超时时间设置为40ms\n");
            }
            else
            {
                printf("I2C看门狗超时时间设置失败: %d\n", rslt);
            }
        }
        else
        {
            printf("I2C看门狗启用失败: %d\n", rslt);
        }
        bmi2_error_codes_print_result(rslt);
    }

    set_accel_gyro_config(&bmi270_dev);
    
    /* NOTE:
     * Accel and gyro enable must be done after setting configurations
     */
    rslt = bmi2_sensor_enable(sensor_list, 2, &bmi270_dev);
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
    rslt = bmi2_set_fifo_config(BMI2_FIFO_ACC_EN | BMI2_FIFO_GYR_EN | BMI2_FIFO_HEADER_EN | BMI2_FIFO_TIME_EN, BMI2_ENABLE, &bmi270_dev);
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

            rslt = bmi2_get_fifo_length(&fifo_length, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);

            /* Updating FIFO length to be read based on available length and dummy byte updation */
            fifoframe.length = fifo_length + SENSORTIME_OVERHEAD_BYTE + bmi270_dev.dummy_byte;

            printf("\nFIFO data bytes available : %d \n", fifo_length);
            printf("\nFIFO data bytes requested : %d \n", fifoframe.length);

            /* Read FIFO data with validation */
            if (fifoframe.data != NULL && fifoframe.length > 0) {
                rslt = bmi2_read_fifo_data(&fifoframe, &bmi270_dev);
                bmi2_error_codes_print_result(rslt);
            } else {
                printf("Error: FIFO frame data pointer invalid\n");
                rslt = BMI2_E_NULL_PTR;
                break;
            }

            /* Read FIFO data on interrupt. */
            if (rslt == BMI2_OK) {
                rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
                bmi2_error_codes_print_result(rslt);
            }

            if (rslt == BMI2_OK)
            {
                printf("\nFIFO accel frames requested : %d \n", accel_frame_length);

                /* Parse the FIFO data to extract accelerometer data from the FIFO buffer. */
                if (fifo_accel_data != NULL && accel_frame_length > 0) {
                    rslt = bmi2_extract_accel(fifo_accel_data, &accel_frame_length, &fifoframe, &bmi270_dev);
                    if (rslt != BMI2_OK) {
                        printf("Error extracting accel data: %d\n", rslt);
                        accel_frame_length = 0;
                    } else {
                        printf("\nFIFO accel frames extracted : %d \n", accel_frame_length);
                    }
                }

                printf("\nFIFO gyro frames requested : %d \n", gyro_frame_length);

                /* Parse the FIFO data to extract gyro data from the FIFO buffer. */
                if (fifo_gyro_data != NULL && gyro_frame_length > 0) {
                    int8_t gyro_rslt = bmi2_extract_gyro(fifo_gyro_data, &gyro_frame_length, &fifoframe, &bmi270_dev);
                    if (gyro_rslt != BMI2_OK) {
                        printf("Error extracting gyro data: %d\n", gyro_rslt);
                        gyro_frame_length = 0;
                    } else {
                        printf("\nFIFO gyro frames extracted : %d \n", gyro_frame_length);
                    }
                }

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

                // FIFO数据处理和姿态估计 - 添加更严格的数据验证
                if (accel_frame_length > 0 && gyro_frame_length > 0 && 
                    accel_frame_length <= BMI2_FIFO_ACCEL_FRAME_COUNT && 
                    gyro_frame_length <= BMI2_FIFO_GYRO_FRAME_COUNT)
                {
                    printf("\n=== Processing %d FIFO frames ===\n", accel_frame_length);
                    
                    uint16_t min_frames = (accel_frame_length < gyro_frame_length) ? 
                                        accel_frame_length : gyro_frame_length;
                    
                    // 逐帧处理所有FIFO数据
                    for (uint16_t i = 0; i < min_frames; i++)
                    {
                        attitude_estimation_update(&fifo_accel_data[i], &fifo_gyro_data[i]);
                        
                        // 可选：每10帧打印一次中间状态
                        if ((i + 1) % 10 == 0) {
                            attitude_t temp_attitude = get_current_attitude();
                            snprintf(buffer, 100, "Frame %d/%d: Roll=%.1f Pitch=%.1f Yaw=%.1f\n", 
                                i + 1, min_frames, temp_attitude.roll, temp_attitude.pitch, temp_attitude.yaw);
                            printf("%s", buffer);
                        }
                    }
                    
                    // 最终结果
                    attitude_t final_attitude = get_current_attitude();
                    angular_rate_t final_rates = get_current_angular_rate();
                    
                    printf("=== Processing Complete, Final Attitude ===\n");
                    snprintf(buffer, 100, "Final Attitude: Roll=%.2f Pitch=%.2f Yaw=%.2f\n",
                        final_attitude.roll, final_attitude.pitch, final_attitude.yaw);
                    printf("%s", buffer);
                    snprintf(buffer, 100, "Final Angular Rate: Roll=%.2f Pitch=%.2f Yaw=%.2f\n",
                        final_rates.roll_rate, final_rates.pitch_rate, final_rates.yaw_rate);
                    printf("%s", buffer);
                }

                /* Print control frames like sensor time and skipped frame count. */
                printf("\nSkipped frame count = %d\n", fifoframe.skipped_frame_count);

                // 安全地检查和显示传感器时间
                if (fifoframe.sensor_time > 0) {
                    double sensor_time_seconds = (double)(fifoframe.sensor_time) * BMI2_SENSORTIME_RESOLUTION;
                    snprintf(buffer, 100, "Sensor time(in seconds) = %.4f s\n", sensor_time_seconds);
                    printf("%s", buffer);
                } else {
                    printf("Sensor time: Not available (value: %u)\n", (unsigned int)fifoframe.sensor_time);
                }

                try++;
            }
        }
    }

    return (rslt == BMI2_OK) ? ERRCODE_SUCC : ERRCODE_FAIL;
}

// 获取BMI270传感器数据并填充到float数组
// output_data: 输出数组[roll_angle, pitch_angle, yaw_angle, roll_rate, pitch_rate, yaw_rate]
errcode_t get_bmi270_attitude_data(float *output_data)
{
    int8_t rslt = BMI2_E_COM_FAIL; // 初始化为失败状态

    if (output_data == NULL) {
        printf("Error: Output data array is NULL\n");
        return ERRCODE_FAIL;
    }

    /* Read FIFO data on interrupt. */
    rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
    bmi2_error_codes_print_result(rslt);

    while(!(int_status & BMI2_FFULL_INT_STATUS_MASK)){
        osal_udelay(2);
        rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
        bmi2_error_codes_print_result(rslt);
    }
        

    if ((rslt == BMI2_OK) && (int_status & BMI2_FFULL_INT_STATUS_MASK))
    {
        printf("\nReading FIFO for attitude calculation\n");

        accel_frame_length = BMI2_FIFO_ACCEL_FRAME_COUNT;
        gyro_frame_length = BMI2_FIFO_GYRO_FRAME_COUNT;

        rslt = bmi2_get_fifo_length(&fifo_length, &bmi270_dev);
        bmi2_error_codes_print_result(rslt);

        /* Updating FIFO length to be read based on available length and dummy byte updation */
        fifoframe.length = fifo_length + SENSORTIME_OVERHEAD_BYTE + bmi270_dev.dummy_byte;

        printf("\nFIFO data bytes available : %d \n", fifo_length);
        printf("\nFIFO data bytes requested : %d \n", fifoframe.length);

        /* Read FIFO data with validation */
        if (fifoframe.data != NULL && fifoframe.length > 0) {
            rslt = bmi2_read_fifo_data(&fifoframe, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);
        } else {
            printf("Error: FIFO frame data pointer invalid\n");
            rslt = BMI2_E_NULL_PTR;
            return ERRCODE_FAIL;
        }

        /* Read FIFO data on interrupt. */
        if (rslt == BMI2_OK) {
            rslt = bmi2_get_int_status(&int_status, &bmi270_dev);
            bmi2_error_codes_print_result(rslt);
        }

        if (rslt == BMI2_OK)
        {
            printf("\nFIFO accel frames requested : %d \n", accel_frame_length);

            /* Parse the FIFO data to extract accelerometer data from the FIFO buffer. */
            
            bmi2_extract_accel(fifo_accel_data, &accel_frame_length, &fifoframe, &bmi270_dev);
        
            printf("\nFIFO accel frames extracted : %d \n", accel_frame_length);
                

            printf("\nFIFO gyro frames requested : %d \n", gyro_frame_length);

            /* Parse the FIFO data to extract gyro data from the FIFO buffer. */

            bmi2_extract_gyro(fifo_gyro_data, &gyro_frame_length, &fifoframe, &bmi270_dev);
            
            printf("\nFIFO gyro frames extracted : %d \n", gyro_frame_length);

            // FIFO数据处理和姿态估计 - 添加更严格的数据验证
            if (accel_frame_length > 0 && gyro_frame_length > 0 && 
                accel_frame_length <= BMI2_FIFO_ACCEL_FRAME_COUNT && 
                gyro_frame_length <= BMI2_FIFO_GYRO_FRAME_COUNT)
            {
                printf("\n=== Processing %d FIFO frames ===\n", accel_frame_length);
                
                uint16_t min_frames = (accel_frame_length < gyro_frame_length) ? 
                                    accel_frame_length : gyro_frame_length;
                
                // 逐帧处理所有FIFO数据
                for (uint16_t i = 0; i < min_frames; i++)
                {
                    attitude_estimation_update(&fifo_accel_data[i], &fifo_gyro_data[i]);
                }
                
                // 获取最终姿态和角速度结果
                attitude_t final_attitude = get_current_attitude();
                angular_rate_t final_rates = get_current_angular_rate();
                
                // 填充输出数组 [roll_angle, pitch_angle, yaw_angle, roll_rate, pitch_rate, yaw_rate]
                output_data[0] = final_attitude.roll;
                output_data[1] = final_attitude.pitch;
                output_data[2] = final_attitude.yaw;
                output_data[3] = final_rates.roll_rate;
                output_data[4] = final_rates.pitch_rate;
                output_data[5] = final_rates.yaw_rate;
                
                return ERRCODE_SUCC;
            } else {
                printf("Error: Insufficient FIFO data for attitude calculation\n");
                return ERRCODE_FAIL;
            }
        }
    } else {
        printf("No FIFO interrupt or FIFO not full\n");
        return ERRCODE_FAIL;
    }

    return (rslt == BMI2_OK) ? ERRCODE_SUCC : ERRCODE_FAIL;
}