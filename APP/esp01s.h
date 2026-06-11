#ifndef _ESP01S_H
#define _ESP01S_H

#include "bsp_system.h"

// Wi-Fi 连接参数
#define WIFI_SSID               "admin"        // Wi-Fi 网络名称
#define WIFI_PWD                "123456789"    // Wi-Fi 密码

// TCP 服务器配置（PC 上 terminal-system 后端监听 3001）
// 注意：TCP_SERVER_IP 要填运行服务器那台电脑在 admin 热点下的局域网 IP，
//       演示前用 ipconfig 查无线网卡 IPv4 地址填到这里。
#define TCP_SERVER_IP           "10.239.198.178"
#define TCP_SERVER_PORT         "3001"

// 固定经纬度（黑龙江科技大学）
#define FIXED_LONGITUDE         126.62f
#define FIXED_LATITUDE          45.78f

// ESP-01S 模块初始化（连 WiFi + TCP）
void esp_init(void);

// 数据上报（TCP 发 JSON）
void esp_report1(void);
void esp_report2(void);

// 处理服务器下发命令（控制 PA5/PA6）
void esp_process_cmd(void);

#endif
