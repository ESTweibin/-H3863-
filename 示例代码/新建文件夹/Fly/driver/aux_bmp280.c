
#include "aux_bmp280.h"

// 辅助I2C读取函数（用于BMP280）
int8_t aux_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    struct bmi2_dev *bmi270_dev = (struct bmi2_dev *)intf_ptr;
    int8_t rslt = bmi2_read_aux_man_mode(reg_addr, reg_data, (uint16_t)length, bmi270_dev);
    return rslt;
}

// 辅助I2C写入函数（用于BMP280）
int8_t aux_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    struct bmi2_dev *bmi270_dev = (struct bmi2_dev *)intf_ptr;
    int8_t rslt = bmi2_write_aux_man_mode(reg_addr, reg_data, (uint16_t)length, bmi270_dev);
    return rslt;
}

// 初始化BMP280（通过辅助I2C）
errcode_t init_bmp280(bmp280_calib_param_t *bmp_calib, struct bmi2_dev *bmi270_dev)
{
    printf("BMP280初始化开始...\n");
    
    // 等待BMI270稳定后再初始化BMP280
    osal_mdelay(50);  // 增加延时确保BMI270辅助I2C接口稳定
    
    // 读取芯片ID验证连接，增加重试机制
    uint8_t chip_id = 0;
    int8_t ret = aux_i2c_read(BMP280_REG_CHIP_ID, &chip_id, 1, bmi270_dev);
    if (ret != BMI2_OK) {
        printf("BMP280芯片ID读取失败: %d\n", ret);
        return ERRCODE_FAIL;
    }
    
    printf("BMP280芯片ID: 0x%02X (期望: 0x%02X)\n", chip_id, BMP280_CHIP_ID_VALUE);
    
    if (chip_id != BMP280_CHIP_ID_VALUE) {
        printf("BMP280芯片ID不匹配\n");
        printf("%d\n", chip_id);
        return ERRCODE_FAIL;
    }
    
    // 软复位
    uint8_t reset_cmd = 0xB6;
    ret = aux_i2c_write(BMP280_REG_RESET, &reset_cmd, 1, bmi270_dev);
    if (ret != BMI2_OK) {
        printf("BMP280复位失败: %d\n", ret);
        return ERRCODE_FAIL;
    }
    
    osal_mdelay(20);  // 等待复位完成
    
    // 读取校准参数
    uint8_t calib_data[BMP280_CALIB_DATA_SIZE];
    ret = aux_i2c_read(BMP280_REG_CALIB_START, calib_data, BMP280_CALIB_DATA_SIZE, bmi270_dev);
    if (ret != BMI2_OK) {
        printf("BMP280校准参数读取失败: %d\n", ret);
        return ERRCODE_FAIL;
    }
    
    // 解析校准参数
    bmp_calib->dig_T1 = (calib_data[1] << 8) | calib_data[0];
    bmp_calib->dig_T2 = (calib_data[3] << 8) | calib_data[2];
    bmp_calib->dig_T3 = (calib_data[5] << 8) | calib_data[4];
    bmp_calib->dig_P1 = (calib_data[7] << 8) | calib_data[6];
    bmp_calib->dig_P2 = (calib_data[9] << 8) | calib_data[8];
    bmp_calib->dig_P3 = (calib_data[11] << 8) | calib_data[10];
    bmp_calib->dig_P4 = (calib_data[13] << 8) | calib_data[12];
    bmp_calib->dig_P5 = (calib_data[15] << 8) | calib_data[14];
    bmp_calib->dig_P6 = (calib_data[17] << 8) | calib_data[16];
    bmp_calib->dig_P7 = (calib_data[19] << 8) | calib_data[18];
    bmp_calib->dig_P8 = (calib_data[21] << 8) | calib_data[20];
    bmp_calib->dig_P9 = (calib_data[23] << 8) | calib_data[22];
    
    // 配置测量模式
    uint8_t ctrl_meas = 0x27;  // 正常模式，1x采样
    ret = aux_i2c_write(BMP280_REG_CTRL_MEAS, &ctrl_meas, 1, bmi270_dev);
    if (ret != BMI2_OK) {
        printf("BMP280控制寄存器配置失败: %d\n", ret);
        return ERRCODE_FAIL;
    }
    
    uint8_t config_reg = 0x00;  // 1ms待机，滤波器关闭
    ret = aux_i2c_write(BMP280_REG_CONFIG, &config_reg, 1, bmi270_dev);
    if (ret != BMI2_OK) {
        printf("BMP280配置寄存器设置失败: %d\n", ret);
        return ERRCODE_FAIL;
    }
    
    printf("BMP280初始化成功\n");
    return ERRCODE_SUCC;
}


// 处理BMP280辅助数据并补偿
void process_bmp280_aux_data(bmp280_calib_param_t bmp_calib, struct bmi2_aux_fifo_data *aux_data, int count)
{
    char buffer[100];
    for (int index = 0; index < count; index++) {
        if (aux_data[index].data[0] != 0 || aux_data[index].data[1] != 0 || aux_data[index].data[2] != 0) {
            // 解析温度和压力数据（从BMP280_REG_PRESS_MSB开始的6个字节）
            // BMI2_AUX_READ_LEN_2配置：读取6字节
            // 字节0-2: 压力数据 (0xF7-0xF9: MSB, LSB, XLSB)  
            // 字节3-5: 温度数据 (0xFA-0xFC: MSB, LSB, XLSB)
            long adc_P = ((long)aux_data[index].data[0] << 12) | ((long)aux_data[index].data[1] << 4) | 
                         ((long)aux_data[index].data[2] >> 4);
            long adc_T = ((long)aux_data[index].data[3] << 12) | ((long)aux_data[index].data[4] << 4) | 
                         ((long)aux_data[index].data[5] >> 4);
            
            // 温度补偿
            long var1 = ((((adc_T >> 3) - ((long)bmp_calib.dig_T1 << 1))) * ((long)bmp_calib.dig_T2)) >> 11;
            long var2 = (((((adc_T >> 4) - ((long)bmp_calib.dig_T1)) * ((adc_T >> 4) - ((long)bmp_calib.dig_T1))) >> 12) * ((long)bmp_calib.dig_T3)) >> 14;
            long t_fine = var1 + var2;
            long T = (t_fine * 5 + 128) >> 8;
            float temperature = T / 100.0f;
            
            // 压力补偿
            var1 = (((long)t_fine) >> 1) - (long)64000;
            var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((long)bmp_calib.dig_P6);
            var2 = var2 + ((var1 * ((long)bmp_calib.dig_P5)) << 1);
            var2 = (var2 >> 2) + (((long)bmp_calib.dig_P4) << 16);
            var1 = (((bmp_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((long)bmp_calib.dig_P2) * var1) >> 1)) >> 18;
            var1 = ((((32768 + var1)) * ((long)bmp_calib.dig_P1)) >> 15);
            
            float pressure = 0;
            float altitude = 0;
            
            if (var1 != 0) {
                unsigned long p = (((unsigned long)(((long)1048576) - adc_P) - (var2 >> 12))) * 3125;
                if (p < 0x80000000) {
                    p = (p << 1) / ((unsigned long)var1);
                } else {
                    p = (p / (unsigned long)var1) * 2;
                }
                
                var1 = (((long)bmp_calib.dig_P9) * ((long)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
                var2 = (((long)(p >> 2)) * ((long)bmp_calib.dig_P8)) >> 13;
                p = (unsigned long)((long)p + ((var1 + var2 + bmp_calib.dig_P7) >> 4));
                
                pressure = p / 100.0f;  // 转换为hPa
                altitude = 44330.0f * (1.0f - pow(pressure / 1013.25f, 0.1903f));
            }

            (void)snprintf(buffer, 100, "BMP280数据[%d], 温度=%.2f°C, 压力=%.2f hPa, 海拔=%.1fm, 时间戳=%lu\n",
                                            index,
                                            temperature,
                                            pressure,
                                            altitude,
                                            aux_data[index].virt_sens_time);
            printf("%s\n", buffer);
        }else {
            printf("%d, 0.00, 0.00\n", count);
        }
    }
}
