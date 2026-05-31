#ifndef _ESP01S_H
#define _ESP01S_H

#include "bsp_system.h"

// Wi-Fi 连接参数
#define WIFI_SSID               "admin"      // Wi-Fi 网络名称
#define WIFI_PWD                "123456789" // Wi-Fi 密码

// 本地 MQTT 服务器配置参数
// 注意：MQTT_ADDRESS 要填运行服务器那台电脑在 admin 热点下的局域网 IP，
//       演示前用 ipconfig 查 192.168.x.x（无线网卡 IPv4 地址）填到这里。
#define HUAWEI_MQTT_ADDRESS     "192.168.0.100"   // 本地服务器 IP（演示前按实际修改）
#define HUAWEI_MQTT_ClientID    "smart_helmet_1"  // MQTT 客户端 ID（本地 broker 不校验，可任意）
#define HUAWEI_MQTT_USERNAME    "smart_helmet_1"  // MQTT 用户名（本地 broker 不校验）
#define HUAWEI_MQTT_PASSWORD    "123456"          // MQTT 密码（本地 broker 不校验）
#define HUAWEI_MQTT_PORT        "1883"            // MQTT 端口号
#define HUAWEI_MQTT_PUBLISH_TOPIC "smart_helmet/report" // 上报主题（本地 broker 不挑 topic）

// ESP-01S 模块初始化函数
void esp_init(void);

// ESP-01S 数据上报函数
void esp_report1(void);
void esp_report2(void);
#endif
