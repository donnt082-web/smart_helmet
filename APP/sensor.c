#include "sensor.h"

// ����ȡ����־λ
bool alarm_cancel_flag = 0;
// ����������־λ��Ĭ�� 0 ��Ҫ�����ƻ�ǰ�˰�ť������
bool measure_flag = 0;
// ����ȡ��������
uint8_t alarm_cancel_count = 0;

/**
 * @brief   ��������ʶ�����ݺ���
 */
void process_asr_data(void)
{
    // �����������Ϊ0��˵��û��������Ҫ������ֱ�ӷ���
    if (uart_rx_index == 0)
        return;
    
    // ��������һ�ν��յ����ݵ������Ѿ�����10ms
    if (uwTick - uart_rx_ticks > 10) // 100ms��û���յ��µ�����
    {
        uart_rx_buffer[uart_rx_index] = '\0'; // �����ַ���������

        // �Խ��յ������ݽ����ж�
        if (strcmp((char *)uart_rx_buffer, "good\r\n") == 0)  // �ж��Ƿ���յ��� "good"
        {
            // ִ����Ӧ�Ĳ���
            alarm_cancel_flag = 1; // ���ñ���ȡ����־λ
            alarm_cancel_count = 10; // ���ñ���ȡ����Чʱ�䣨10*1000ms=10s������ִ������Ϊ1000ms��
        }
        else if(strcmp((char *)uart_rx_buffer, "measure\r\n") == 0)
        {
            measure_flag = 1; // ���ò���������־λ
        }
        
        // ��ս��ջ�����������������������
        memset(uart_rx_buffer, 0, uart_rx_index);
        uart_rx_index = 0;

        // ��UART���ջ�����ָ������Ϊ���ջ���������ʼλ��
        huart1.pRxBuffPtr = uart_rx_buffer;
    }
}

/**
 * @brief   ������ʶ��ģ�鷢�ͱ�־����
 */
void send_flag_to_asr(void)
{
    // �������ȡ����־λΪ0��˵��û�б���ȡ�������������������ʹ���������
    if (!alarm_cancel_flag)
    {
        // ���ݲ�ͬ�Ĵ�����״̬������Ӧ����Ϣ
        if (fall_flag)
        {
            my_printf(&huart1, "fall"); // ���͵���������Ϣ
        }
        else if (density_flag)
        {
            my_printf(&huart1, "density"); // ����Ũ�ȹ��߱�����Ϣ
        }
        else if (heartrate_flag)
        {
            my_printf(&huart1, "heartrate"); // ���������쳣������Ϣ
            heartrate_flag = 0; // ���������쳣��־λ
            spo2_flag = 0;      // ����Ѫ���쳣��־λ
        }
        else if (spo2_flag)
        {
            my_printf(&huart1, "spo2"); // ����Ѫ���쳣������Ϣ
            heartrate_flag = 0; // ���������쳣��־λ
            spo2_flag = 0;      // ����Ѫ���쳣��־λ
        }
    }
    else
    {
        // �������ȡ����־λ��Ϊ0��˵�����ڽ��б���ȡ������
        if (--alarm_cancel_count == 0)
        {
            // ����ȡ������������0ʱ�����ñ���ȡ����־λ
            alarm_cancel_flag = 0;
        }
    }
}
