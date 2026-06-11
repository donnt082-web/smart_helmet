#include "esp01s.h"

/* ── 初始化状态标志 ───────────────────────────────────────── */
static uint8_t esp_ready = 0;   /* 1=初始化成功，可以发送数据 */


/**
 * @brief   ESP-01S 初始化（恢复原版时序 + 调试输出）
 */
void esp_init(void)
{
    esp_ready = 0;

    my_printf(&huart1, "[ESP] init start\r\n");

    /* 1. 复位模块 */
    my_printf(&huart3, "AT+RST\r\n");
    HAL_Delay(3000);

    /* 2. 关闭回显 */
    my_printf(&huart3, "ATE0\r\n");
    HAL_Delay(500);

    /* 3. Station 模式 */
    my_printf(&huart3, "AT+CWMODE=1\r\n");
    HAL_Delay(2000);

    /* 4. 连接 WiFi */
    my_printf(&huart1, "[ESP] WiFi: %s ...\r\n", WIFI_SSID);
    my_printf(&huart3, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(8000);

    /* 5. 连接 TCP 服务器 */
    my_printf(&huart1, "[ESP] TCP %s:%s ...\r\n", TCP_SERVER_IP, TCP_SERVER_PORT);
    my_printf(&huart3, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",
              TCP_SERVER_IP, TCP_SERVER_PORT);
    HAL_Delay(3000);

    /* 6. 启动 USART3 RX 中断（下行命令 + 断线检测）*/
    HAL_UART_Receive_IT(&huart3, &uart3_rx_byte, 1);

    esp_ready = 1;
    my_printf(&huart1, "[ESP] init done\r\n");
}


static char send_buf[256];

/* TCP 断线标志：USART3 收到 CLOSED / link is not 等关键字时被置 1 */
static volatile uint8_t tcp_dead = 0;

/* 重连 TCP（不重启 WiFi，省时间）*/
static void esp_reconnect_tcp(void)
{
    my_printf(&huart1, "[ESP] TCP dead, reconnect...\r\n");

    /* 清缓冲区，避免旧数据干扰检查 */
    uart3_rx_index = 0;
    memset(uart3_rx_buffer, 0, sizeof(uart3_rx_buffer));

    my_printf(&huart3, "AT+CIPCLOSE\r\n");
    HAL_Delay(500);

    my_printf(&huart3, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",
              TCP_SERVER_IP, TCP_SERVER_PORT);
    HAL_Delay(3000);

    /* 检查应答：有 OK/ALREADY 才算成功 */
    if (strstr((char *)uart3_rx_buffer, "OK")
     || strstr((char *)uart3_rx_buffer, "ALREADY"))
    {
        tcp_dead = 0;
        my_printf(&huart1, "[ESP] reconnect ok\r\n");
    }
    else
    {
        my_printf(&huart1, "[ESP] reconnect fail, retry later\r\n");
    }

    /* 清理缓冲区，避免误判 */
    uart3_rx_index = 0;
    memset(uart3_rx_buffer, 0, sizeof(uart3_rx_buffer));
}

/**
 * @brief   通过 AT+CIPSEND 发送一条数据
 */
static void esp_tcp_send(const char *json)
{
    if (!esp_ready)
        return;
    if (tcp_dead)
    {
        esp_reconnect_tcp();
        if (tcp_dead) return;
    }
    uint16_t len = strlen(json);
    my_printf(&huart3, "AT+CIPSEND=%d\r\n", len);
    HAL_Delay(80);
    my_printf(&huart3, "%s", json);
    HAL_Delay(120);
}

/**
 * @brief   上报 1：血氧、心率、气体、跌倒、撞击
 */
void esp_report1(void)
{
    /* 直接上报真实测量值：未测到手指时 dis_spo2/dis_hr 为 0 */
    sprintf(send_buf,
        "{\"spO2\":%d,\"heart_rate\":%d,\"density\":%.2f,\"fall_flag\":%d,\"collision_flag\":%d}\n",
        dis_spo2, dis_hr, ppm, fall_flag, collision_flag);
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
 * @brief   处理 ESP-01S 上行的下发命令
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
