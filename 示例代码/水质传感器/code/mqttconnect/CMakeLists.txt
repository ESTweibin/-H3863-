#===============================================================================
# @brief    cmake file
# Copyright (c) CompanyNameMagicTag 2023-2023. All rights reserved.
#===============================================================================
if(DEFINED CONFIG_SAMPLE_SUPPORT_MQTTCONNECT)
set(SOURCES_LIST   
    ${CMAKE_CURRENT_SOURCE_DIR}/ph_demo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/conductance_demo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/turbidity_demo.c  
    ${CMAKE_CURRENT_SOURCE_DIR}/tem_demo.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/ssd1306_fonts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ssd1306.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_huawei_cloud_sender.c
)
set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)
endif()