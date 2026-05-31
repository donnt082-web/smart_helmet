#include "esp01s.h"

/**
 * @brief   ESP 初始化函数
 */
void esp_init(void)
{
    // 发送 AT+RST 命令，复位 ESP 模块
    my_printf(&huart3, "AT+RST\r\n");
    HAL_Delay(1000);
    
    // 发送 AT+CWMODE=1 命令，设置模块为 STA（客户端）模式
    my_printf(&huart3, "AT+CWMODE=1\r\n");
    HAL_Delay(1000);
    
    // 发送 AT+CWJAP 命令，连接到指定的 Wi-Fi 网络
    my_printf(&huart3, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(4000);
    
    // 配置 MQTT 用户信息
    my_printf(&huart3, "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n", HUAWEI_MQTT_USERNAME, HUAWEI_MQTT_PASSWORD);
    HAL_Delay(1000);
    
    // 设置 MQTT 客户端 ID
    my_printf(&huart3, "AT+MQTTCLIENTID=0,\"%s\"\r\n", HUAWEI_MQTT_ClientID);
    HAL_Delay(1000);
    
    // 连接到 MQTT 服务器
    my_printf(&huart3, "AT+MQTTCONN=0,\"%s\",1883,1\r\n", HUAWEI_MQTT_ADDRESS);
    HAL_Delay(1000);
}

/**
 * @brief   ESP 数据上报函数1
 */
void esp_report1(void)
{
    // 上报传感器数据（血氧、浓度、心率、跌倒标志）
	my_printf(&huart3, "AT+MQTTPUB=0,\"smart_helmet/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"BasicData\\\"\\,\\\"properties\\\":{\\\"spO2\\\":%d\\,\\\"density\\\":%.2f\\,\\\"heart_rate\\\":%d\\,\\\"fall_flag\\\":%d\\,\\\"collision_flag\\\":%d}}]}\",0,0\r\n", dis_spo2, ppm, dis_hr, fall_flag, collision_flag);
//	HAL_Delay(50);
}

/**
 * @brief   ESP数据上报函数2
 */
void esp_report2(void)
{
	// 上报环境数据（温度、湿度、固定经纬度）
	my_printf(&huart3, "AT+MQTTPUB=0,\"smart_helmet/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"BasicData\\\"\\,\\\"properties\\\":{\\\"longitude\\\":%.2f\\,\\\"latitude\\\":%.2f\\,\\\"temperature\\\":%d\\,\\\"humidity\\\":%d}}]}\",0,0\r\n", longitude, latitude, temp, humi);
	HAL_Delay(50);
}
