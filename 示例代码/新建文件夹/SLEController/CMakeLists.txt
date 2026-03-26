list(APPEND SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_server/sle_uart_server_adv.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_server/sle_uart_server.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/SLEController.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/SSD1306_OLED.c"
    
)

# 将当前目录添加到PUBLIC_HEADER变量
list(APPEND PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}")

# 传递给父级作用域
set(SOURCES ${SOURCES} PARENT_SCOPE)
set(PUBLIC_HEADER ${PUBLIC_HEADER} PARENT_SCOPE)