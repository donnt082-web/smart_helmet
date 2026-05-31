#ifndef _ESP01S_H
#define _ESP01S_H

#include "bsp_system.h"

// Wi-Fi 连接参数
#define WIFI_SSID               "admin"      // Wi-Fi 网络名称
#define WIFI_PWD                "123456789" // Wi-Fi 密码

// 华为云物联网平台 MQTT 配置参数
#define HUAWEI_MQTT_ADDRESS     "678d30610a.st1.iotda-device.cn-north-4.myhuaweicloud.com" // MQTT 服务器地址
#define HUAWEI_MQTT_ClientID    "685ca22532771f177b45e75f_smart_helmet_1_0_0_2025062611"     // MQTT 客户端 ID
#define HUAWEI_MQTT_USERNAME    "685ca22532771f177b45e75f_smart_helmet_1"                   // MQTT 用户名
#define HUAWEI_MQTT_PASSWORD    "819d3b6a935d4747f6370b1d240aba4ab2d73c99a8babc4964f770ae63a8bee1" // MQTT 密码
#define HUAWEI_MQTT_PORT        "1883"                                                          // MQTT 端口号
#define HUAWEI_MQTT_PUBLISH_TOPIC "$oc/devices/685ca22532771f177b45e75f_smart_helmet_1/sys/properties/report" // 上报主题

// ESP-01S 模块初始化函数
void esp_init(void);

// ESP-01S 数据上报函数
void esp_report1(void);
void esp_report2(void);
#endif
