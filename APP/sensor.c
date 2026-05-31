#include "sensor.h"

// 报警取消标志位
bool alarm_cancel_flag = 0;
// 测量标志位，默认 0，需要触发前端按钮来启动测量
bool measure_flag = 0;
// 报警取消计数器
uint8_t alarm_cancel_count = 0;

/**
 * @brief   处理语音识别数据函数
 */
void process_asr_data(void)
{
    // 如果接收索引为 0，说明没有数据需要处理，直接返回
    if (uart_rx_index == 0)
        return;

    // 距离上一次接收到数据的时间已经超过 10ms（表示一帧数据接收完毕）
    if (uwTick - uart_rx_ticks > 10) // 超过 10ms 没有收到新的数据
    {
        uart_rx_buffer[uart_rx_index] = '\0'; // 给字符串加结束符

        // 对接收到的数据进行判断
        if (strcmp((char *)uart_rx_buffer, "good\r\n") == 0)  // 判断是否接收到了 "good"
        {
            // 执行相应的操作
            alarm_cancel_flag = 1; // 设置报警取消标志位
            alarm_cancel_count = 10; // 设置报警取消有效时间（10*1000ms=10s，本函数执行周期为 1000ms）
        }
        else if(strcmp((char *)uart_rx_buffer, "measure\r\n") == 0)
        {
            measure_flag = 1; // 设置测量启动标志位
        }

        // 清空接收缓冲区，重置接收索引
        memset(uart_rx_buffer, 0, uart_rx_index);
        uart_rx_index = 0;

        // 将 UART 接收缓冲区指针重置为接收缓冲区的起始位置
        huart1.pRxBuffPtr = uart_rx_buffer;
    }
}

/**
 * @brief   向语音识别模块发送标志数据
 */
void send_flag_to_asr(void)
{
    // 如果报警取消标志位为 0，说明没有报警取消请求，正常发送传感器报警信息
    if (!alarm_cancel_flag)
    {
        // 根据不同的传感器状态发送相应的信息
        if (fall_flag)
        {
            my_printf(&huart1, "fall"); // 发送跌倒报警信息
        }
        else if (density_flag)
        {
            my_printf(&huart1, "density"); // 发送气体浓度过高报警信息
        }
        else if (heartrate_flag)
        {
            my_printf(&huart1, "heartrate"); // 发送心率异常报警信息
            heartrate_flag = 0; // 清除心率异常标志位
            spo2_flag = 0;      // 清除血氧异常标志位
        }
        else if (spo2_flag)
        {
            my_printf(&huart1, "spo2"); // 发送血氧异常报警信息
            heartrate_flag = 0; // 清除心率异常标志位
            spo2_flag = 0;      // 清除血氧异常标志位
        }
    }
    else
    {
        // 如果报警取消标志位不为 0，说明正在进行报警取消倒计时
        if (--alarm_cancel_count == 0)
        {
            // 当报警取消计数器减到 0 时，清除报警取消标志位
            alarm_cancel_flag = 0;
        }
    }
}
