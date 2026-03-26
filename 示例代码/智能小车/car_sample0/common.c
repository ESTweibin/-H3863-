/**
 * Copyright (C) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "bmi2_defs.h"
#include "osal_debug.h"
#include "i2c.h"
#include "tcxo.h"

/*! Macro that defines read write length */
#define READ_WRITE_LEN     UINT8_C(32)
/*! Earth's gravity in m/s^2 */
#define GRAVITY_EARTH  (9.80665f)

/*! Structure to hold interface configurations */
static struct hisilicon_intf_config intf_conf = {
    .dev_addr = 0x68,
    .bus = 1 
};
// 创建写入缓冲区: 寄存器地址 + 数据 (最大支持256字节数据)
static uint8_t write_buf[257]; // 1字节地址 + 最大256字节数据
static uint8_t dev_addr; // I2C设备地址或SPI片选

/*!
 * I2C read function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    struct hisilicon_intf_config intf_info = *(struct hisilicon_intf_config *)intf_ptr;

    // 构建I2C数据结构
    i2c_data_t i2c_data = {
        .send_buf = &reg_addr,      // 发送寄存器地址
        .send_len = 1,              // 发送1字节
        .receive_buf = reg_data,    // 接收数据缓冲区
        .receive_len = len          // 接收数据长度
    };

    // 执行I2C写读操作
    errcode_t ret = uapi_i2c_master_writeread(intf_info.bus, intf_info.dev_addr, &i2c_data);
    
    
    if (ret == ERRCODE_SUCC) {
        return BMI2_INTF_RET_SUCCESS;
    } else {
        printf("BMI270 I2C读取失败: 地址=0x%02X, 长度=%d, 错误码=0x%X\n", reg_addr, len, ret);
        return BMI2_E_COM_FAIL;
    }
}

/*!
 * I2C write function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    struct hisilicon_intf_config intf_info = *(struct hisilicon_intf_config *)intf_ptr;

    if (len > 256) {
        printf("BMI270 I2C写入数据长度超限: %d > 256\n", len);
        return BMI2_E_INVALID_INPUT;
    }
    
    write_buf[0] = reg_addr;
    for (uint32_t i = 0; i < len; i++) {
        write_buf[i + 1] = reg_data[i];
    }
    
    // 构建I2C数据结构
    i2c_data_t i2c_data = {
        .send_buf = write_buf,      // 发送缓冲区（地址+数据）
        .send_len = len + 1,        // 发送长度（地址1字节+数据）
        .receive_buf = NULL,        // 无接收数据
        .receive_len = 0            // 无接收长度
    };
    
    // 执行I2C写操作
    errcode_t ret = uapi_i2c_master_write(intf_info.bus, intf_info.dev_addr, &i2c_data);

    if (ret == ERRCODE_SUCC) {
        return BMI2_INTF_RET_SUCCESS;
    } else {
        printf("BMI270 I2C写入失败: 地址=0x%02X, 长度=%d, 错误码=0x%X\n", reg_addr, len, ret);
        return BMI2_E_COM_FAIL;
    }
}

/*!
 * SPI read function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)reg_addr;   // 避免未使用参数警告
    (void)reg_data;   // 避免未使用参数警告  
    (void)len;        // 避免未使用参数警告
    (void)intf_ptr;   // 避免未使用参数警告
    return BMI2_INTF_RET_SUCCESS;
}

/*!
 * SPI write function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)reg_addr;   // 避免未使用参数警告
    (void)reg_data;   // 避免未使用参数警告
    (void)len;        // 避免未使用参数警告
    (void)intf_ptr;   // 避免未使用参数警告
    return BMI2_INTF_RET_SUCCESS;
}

/*!
 * Delay function map to COINES platform
 */
void bmi2_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr; // 避免未使用参数警告
    uapi_tcxo_delay_us(period);
}


/*!
 *  @brief Function to select the interface between SPI and I2C.
 */
int8_t bmi2_interface_init(struct bmi2_dev *bmi, uint8_t intf)
{
    int8_t rslt = BMI2_OK;

    if (bmi == NULL) 
    {
        return BMI2_E_NULL_PTR;
    }

    /* Bus configuration : I2C */
    if (intf == BMI2_I2C_INTF)
    {
        printf("配置I2C接口 \n");

        /* To initialize the user I2C function */
        dev_addr = BMI2_I2C_PRIM_ADDR;
        bmi->intf = BMI2_I2C_INTF;
        bmi->read = bmi2_i2c_read;
        bmi->write = bmi2_i2c_write;

    }
    /* Bus configuration : SPI */
    else if (intf == BMI2_SPI_INTF)
    {
        printf("配置SPI接口 \n");

        /* To initialize the user SPI function */
        // dev_addr = COINES_MINI_SHUTTLE_PIN_2_1;
        bmi->intf = BMI2_SPI_INTF;
        bmi->read = bmi2_spi_read;
        bmi->write = bmi2_spi_write;

    }
    else 
    {
        printf("错误: 不支持的接口类型\n");
        return BMI2_E_INVALID_INPUT;
    }

    bmi->intf_ptr = ((void *)&intf_conf);

    /* Configure delay in microseconds */
    bmi->delay_us = bmi2_delay_us;

    /* Configure max read/write length (in bytes) ( Supported length depends on target machine) */
    bmi->read_write_len = READ_WRITE_LEN;

    /* Assign to NULL to load the default config file. */
    bmi->config_file_ptr = NULL;

    return rslt;
}

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bmi2_error_codes_print_result(int8_t rslt)
{
    switch (rslt)
    {
        case BMI2_OK:
            /* Do nothing */
            break;

        case BMI2_W_FIFO_EMPTY:
            printf("Warning [%d] : FIFO empty\r\n", rslt);
            break;
        case BMI2_W_PARTIAL_READ:
            printf("Warning [%d] : FIFO partial read\r\n", rslt);
            break;
        case BMI2_E_NULL_PTR:
            printf(
                "Error [%d] : Null pointer error. It occurs when the user tries to assign value (not address) to a pointer," " which has been initialized to NULL.\r\n",
                rslt);
            break;

        case BMI2_E_COM_FAIL:
            printf(
                "Error [%d] : Communication failure error. It occurs due to read/write operation failure and also due " "to power failure during communication\r\n",
                rslt);
            break;

        case BMI2_E_DEV_NOT_FOUND:
            printf("Error [%d] : Device not found error. It occurs when the device chip id is incorrectly read\r\n",
                   rslt);
            break;

        case BMI2_E_INVALID_SENSOR:
            printf(
                "Error [%d] : Invalid sensor error. It occurs when there is a mismatch in the requested feature with the " "available one\r\n",
                rslt);
            break;

        case BMI2_E_SELF_TEST_FAIL:
            printf(
                "Error [%d] : Self-test failed error. It occurs when the validation of accel self-test data is " "not satisfied\r\n",
                rslt);
            break;

        case BMI2_E_INVALID_INT_PIN:
            printf(
                "Error [%d] : Invalid interrupt pin error. It occurs when the user tries to configure interrupt pins " "apart from INT1 and INT2\r\n",
                rslt);
            break;

        case BMI2_E_OUT_OF_RANGE:
            printf(
                "Error [%d] : Out of range error. It occurs when the data exceeds from filtered or unfiltered data from " "fifo and also when the range exceeds the maximum range for accel and gyro while performing FOC\r\n",
                rslt);
            break;

        case BMI2_E_ACC_INVALID_CFG:
            printf(
                "Error [%d] : Invalid Accel configuration error. It occurs when there is an error in accel configuration" " register which could be one among range, BW or filter performance in reg address 0x40\r\n",
                rslt);
            break;

        case BMI2_E_GYRO_INVALID_CFG:
            printf(
                "Error [%d] : Invalid Gyro configuration error. It occurs when there is a error in gyro configuration" "register which could be one among range, BW or filter performance in reg address 0x42\r\n",
                rslt);
            break;

        case BMI2_E_ACC_GYR_INVALID_CFG:
            printf(
                "Error [%d] : Invalid Accel-Gyro configuration error. It occurs when there is a error in accel and gyro" " configuration registers which could be one among range, BW or filter performance in reg address 0x40 " "and 0x42\r\n",
                rslt);
            break;

        case BMI2_E_CONFIG_LOAD:
            printf(
                "Error [%d] : Configuration load error. It occurs when failure observed while loading the configuration " "into the sensor\r\n",
                rslt);
            break;

        case BMI2_E_INVALID_PAGE:
            printf(
                "Error [%d] : Invalid page error. It occurs due to failure in writing the correct feature configuration " "from selected page\r\n",
                rslt);
            break;

        case BMI2_E_SET_APS_FAIL:
            printf(
                "Error [%d] : APS failure error. It occurs due to failure in write of advance power mode configuration " "register\r\n",
                rslt);
            break;

        case BMI2_E_AUX_INVALID_CFG:
            printf(
                "Error [%d] : Invalid AUX configuration error. It occurs when the auxiliary interface settings are not " "enabled properly\r\n",
                rslt);
            break;

        case BMI2_E_AUX_BUSY:
            printf(
                "Error [%d] : AUX busy error. It occurs when the auxiliary interface buses are engaged while configuring" " the AUX\r\n",
                rslt);
            break;

        case BMI2_E_REMAP_ERROR:
            printf(
                "Error [%d] : Remap error. It occurs due to failure in assigning the remap axes data for all the axes " "after change in axis position\r\n",
                rslt);
            break;

        case BMI2_E_GYR_USER_GAIN_UPD_FAIL:
            printf(
                "Error [%d] : Gyro user gain update fail error. It occurs when the reading of user gain update status " "fails\r\n",
                rslt);
            break;

        case BMI2_E_SELF_TEST_NOT_DONE:
            printf(
                "Error [%d] : Self-test not done error. It occurs when the self-test process is ongoing or not " "completed\r\n",
                rslt);
            break;

        case BMI2_E_INVALID_INPUT:
            printf("Error [%d] : Invalid input error. It occurs when the sensor input validity fails\r\n", rslt);
            break;

        case BMI2_E_INVALID_STATUS:
            printf("Error [%d] : Invalid status error. It occurs when the feature/sensor validity fails\r\n", rslt);
            break;

        case BMI2_E_CRT_ERROR:
            printf("Error [%d] : CRT error. It occurs when the CRT test has failed\r\n", rslt);
            break;

        case BMI2_E_ST_ALREADY_RUNNING:
            printf(
                "Error [%d] : Self-test already running error. It occurs when the self-test is already running and " "another has been initiated\r\n",
                rslt);
            break;

        case BMI2_E_CRT_READY_FOR_DL_FAIL_ABORT:
            printf(
                "Error [%d] : CRT ready for download fail abort error. It occurs when download in CRT fails due to wrong " "address location\r\n",
                rslt);
            break;

        case BMI2_E_DL_ERROR:
            printf(
                "Error [%d] : Download error. It occurs when write length exceeds that of the maximum burst length\r\n",
                rslt);
            break;

        case BMI2_E_PRECON_ERROR:
            printf(
                "Error [%d] : Pre-conditional error. It occurs when precondition to start the feature was not " "completed\r\n",
                rslt);
            break;

        case BMI2_E_ABORT_ERROR:
            printf("Error [%d] : Abort error. It occurs when the device was shaken during CRT test\r\n", rslt);
            break;

        case BMI2_E_WRITE_CYCLE_ONGOING:
            printf(
                "Error [%d] : Write cycle ongoing error. It occurs when the write cycle is already running and another " "has been initiated\r\n",
                rslt);
            break;

        case BMI2_E_ST_NOT_RUNING:
            printf(
                "Error [%d] : Self-test is not running error. It occurs when self-test running is disabled while it's " "running\r\n",
                rslt);
            break;

        case BMI2_E_DATA_RDY_INT_FAILED:
            printf(
                "Error [%d] : Data ready interrupt error. It occurs when the sample count exceeds the FOC sample limit " "and data ready status is not updated\r\n",
                rslt);
            break;

        case BMI2_E_INVALID_FOC_POSITION:
            printf(
                "Error [%d] : Invalid FOC position error. It occurs when average FOC data is obtained for the wrong" " axes\r\n",
                rslt);
            break;

        default:
            printf("Error [%d] : Unknown error code\r\n", rslt);
            break;
    }
}

/*!
 * @brief This function converts lsb to meter per second squared for 16 bit accelerometer at
 * range 2G, 4G, 8G or 16G.
 */
float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (GRAVITY_EARTH * val * g_range) / half_scale;
}

/*!
 * @brief This function converts lsb to degree per second for 16 bit gyro at
 * range 125, 250, 500, 1000 or 2000dps.
 */
float lsb_to_dps(int16_t val, float dps, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (dps / (half_scale)) * (val);
}
