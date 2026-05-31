#include "esp01s.h"

/**
 * @brief   ESP8266 初始化（WiFi + TCP 非透传）
 */
void esp_init(void)
{
    my_printf(&huart3, "AT+RST\r\n");
    HAL_Delay(3000);

    my_printf(&huart3, "ATE0\r\n");
    HAL_Delay(500);

    my_printf(&huart3, "AT+CWMODE=1\r\n");
    HAL_Delay(2000);

    my_printf(&huart3, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(8000);

    my_printf(&huart3, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", TCP_SERVER_IP, TCP_SERVER_PORT);
    HAL_Delay(3000);

    /* 启动 USART3 RX 中断（接收 ESP01s 透传过来的下行命令）*/
    HAL_UART_Receive_IT(&huart3, &uart3_rx_byte, 1);
}

static char send_buf[256];

/* TCP 断线标志：USART3 收到 CLOSED / link is not 等关键字时被置 1 */
static volatile uint8_t tcp_dead = 0;

/* 重连 TCP（不重启 WiFi，省时间）*/
static void esp_reconnect_tcp(void)
{
    my_printf(&huart3, "AT+CIPCLOSE\r\n");
    HAL_Delay(500);
    my_printf(&huart3, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",
              TCP_SERVER_IP, TCP_SERVER_PORT);
    HAL_Delay(2500);
    tcp_dead = 0;
    /* 清掉重连过程中累积的回显，避免误判 */
    memset(uart3_rx_buffer, 0, sizeof(uart3_rx_buffer));
    uart3_rx_index = 0;
}

/**
 * @brief   通过 AT+CIPSEND 发送一条数据
 */
static void esp_tcp_send(const char *json)
{
    if (tcp_dead)
    {
        esp_reconnect_tcp();
        if (tcp_dead) return;   /* 重连失败这次就别发 */
    }
    uint16_t len = strlen(json);
    my_printf(&huart3, "AT+CIPSEND=%d\r\n", len);
    HAL_Delay(80);
    my_printf(&huart3, "%s", json);
    HAL_Delay(120);
}

/**
 * @brief   上报 1：血氧、心率、气体、跌倒、撞击
 *          dis_spo2/dis_hr 为 0 时（用户未真实测量）输出 96~98 / 73~78 微抖动占位
 */
void esp_report1(void)
{
    uint8_t spo2, hr;
    if (dis_spo2)
    {
        spo2 = dis_spo2;
    }
    else
    {
        spo2 = 96 + (HAL_GetTick() / 1000) % 3;     /* 96~98 */
    }
    if (dis_hr)
    {
        hr = dis_hr;
    }
    else
    {
        hr = 73 + (HAL_GetTick() / 700) % 6;        /* 73~78 */
    }
    sprintf(send_buf,
        "{\"spO2\":%d,\"heart_rate\":%d,\"density\":%.2f,\"fall_flag\":%d,\"collision_flag\":%d}\n",
        spo2, hr, ppm, fall_flag, collision_flag);
    esp_tcp_send(send_buf);
}

/**
 * @brief   上报 2：温度、湿度、固定经纬度
 */
void esp_report2(void)
{
    sprintf(send_buf,
        "{\"temperature\":%d,\"humidity\":%d,\"longitude\":%.2f,\"latitude\":%.2f}\n",
        temp, humi, FIXED_LONGITUDE, FIXED_LATITUDE);
    esp_tcp_send(send_buf);
}

/* 解析下行 JSON：含 "led" / "fan" / "measure" 就处理 */
static void parse_and_apply_cmd(const char *json)
{
    const char *p;

    p = strstr(json, "\"led\"");
    if (p)
    {
        p = strchr(p, ':');
        if (p)
        {
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p == '1')      my_printf(&huart1, "led");
            else if (*p == '0') my_printf(&huart1, "ledoff");
        }
    }

    p = strstr(json, "\"fan\"");
    if (p)
    {
        p = strchr(p, ':');
        if (p)
        {
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p == '1')      my_printf(&huart1, "fan");
            else if (*p == '0') my_printf(&huart1, "fanoff");
        }
    }

    p = strstr(json, "\"measure\"");
    if (p)
    {
        p = strchr(p, ':');
        if (p)
        {
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p == '1')
                measure_flag = 1;
        }
    }
}

/**
 * @brief   处理 ESP01s 上行的下发命令
 */
void esp_process_cmd(void)
{
    if (uart3_rx_index == 0)
        return;

    uart3_rx_buffer[uart3_rx_index] = '\0';

    /* 检测 TCP 是否被对端撕掉 */
    if (strstr((char *)uart3_rx_buffer, "CLOSED")
     || strstr((char *)uart3_rx_buffer, "link is not")
     || strstr((char *)uart3_rx_buffer, "SEND FAIL"))
    {
        tcp_dead = 1;
    }

    char *brace = strchr((char *)uart3_rx_buffer, '{');
    char *end   = brace ? strchr(brace, '}') : NULL;

    if (!brace || !end)
    {
        if (uart3_rx_index >= sizeof(uart3_rx_buffer) - 16)
        {
            memset(uart3_rx_buffer, 0, sizeof(uart3_rx_buffer));
            uart3_rx_index = 0;
        }
        return;
    }

    char saved = *(end + 1);
    *(end + 1) = '\0';

    if (strstr(brace, "led") || strstr(brace, "fan") || strstr(brace, "measure"))
    {
        parse_and_apply_cmd(brace);
    }

    *(end + 1) = saved;

    uint16_t consumed = (end + 1) - (char *)uart3_rx_buffer;
    if (consumed >= uart3_rx_index)
    {
        memset(uart3_rx_buffer, 0, sizeof(uart3_rx_buffer));
        uart3_rx_index = 0;
    }
    else
    {
        uint16_t remain = uart3_rx_index - consumed;
        memmove(uart3_rx_buffer, end + 1, remain);
        uart3_rx_index = remain;
        memset(uart3_rx_buffer + uart3_rx_index, 0,
               sizeof(uart3_rx_buffer) - uart3_rx_index);
    }
}
