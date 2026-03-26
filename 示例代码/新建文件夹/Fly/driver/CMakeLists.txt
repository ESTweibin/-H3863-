#===============================================================================
# @brief    BMI270 sensor driver cmake file
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
#===============================================================================

#===============================================================================
# @brief    BMI270 sensor driver cmake file
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
#===============================================================================

# 将BMI270驱动源文件添加到SOURCES变量
list(APPEND SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/bmi2.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/bmi270.c"
    # "${CMAKE_CURRENT_SOURCE_DIR}/bmi270_example.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/common.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/aux_bmp280.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/bmi270_init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Fly.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/attitude_estimation.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/pid.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_clien.c"
)

# 将当前目录添加到PUBLIC_HEADER变量
list(APPEND PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}")

# 传递给父级作用域
set(SOURCES ${SOURCES} PARENT_SCOPE)
set(PUBLIC_HEADER ${PUBLIC_HEADER} PARENT_SCOPE)