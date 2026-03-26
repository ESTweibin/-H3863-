#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT配置
#define MQTT_BROKER    "458080d370.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT      1883
#define MQTT_USERNAME  "684d4bda32771f177b4202e6_ws63"     
#define MQTT_PASSWORD  "684d4bda32771f177b4202e6_ws63" 
#define MQTT_CLIENTID  "684d4bda32771f177b4202e6_ws63_0_0_2025061611"    

// WiFi配置
#define WIFI_SSID     "rongyao"    
#define WIFI_PASSWORD "3f5491322853" 

// 传感器数据结构
typedef struct {
    float ph;
    float tds;
    float turbidity;
    float temperature;
} sensor_data_t;

#endif