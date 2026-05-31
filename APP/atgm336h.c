#include "atgm336h.h"



uint16_t point1 = 0; // ���ڼ�¼���յ������ݳ���

float longitude;     // ���ڴ洢����

float latitude;      // ���ڴ洢γ��



// ���屣��GPS���ݵ�ȫ�ֽṹ��ʵ��

_SaveData Save_Data;

// ����洢��γ�����ݵ�ȫ�ֽṹ��ʵ��

LatitudeAndLongitude_s g_LatAndLongData =

{

    .E_W = 0,

    .N_S = 0,

    .latitude = 0.0,

    .longitude = 0.0

};



// ���ڽ��ջ���������ʱ����

char USART_RX_BUF[USART_REC_LEN];

uint8_t uart_A_RX_Buff;



// UART3 接收缓冲区（ESP-01S 下行命令）

uint16_t uart3_rx_index = 0;

uint32_t uart3_rx_ticks = 0;

uint8_t uart3_rx_buffer[256];

uint8_t uart3_rx_byte;



/**

 * @brief   ��ʼ��GPSģ��

 */

void atgm336h_init(void)

{

    clrStruct(); // ����ṹ������

    // ���ô����жϣ�׼����������

    HAL_UART_Receive_IT(&huart2, &uart_A_RX_Buff, 1);

}



/**

 * @brief   ���ڽ�����ɻص�����

 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)

{

    // �����USART1�������

    if (huart->Instance == USART1)

    {

        // ���½���ʱ��������ӽ�������

        uart_rx_ticks = uwTick;

        uart_rx_index++;

        // ����������һ���ֽ�

        HAL_UART_Receive_IT(&huart1, &uart_rx_buffer[uart_rx_index], 1);

    }

    // �����USART2�������

    else if (huart->Instance == USART2)

    {

        // �ж��Ƿ��յ�֡ͷ��־�ַ�'$'

        if (uart_A_RX_Buff == '$')

        {

            point1 = 0; // �������ݳ��ȼ�����

        }

        USART_RX_BUF[point1++] = uart_A_RX_Buff; // �洢���յ�������



        // ����Ƿ��յ�������GPRMC/GNRMC֡����

        if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C')

        {

            // ������յ����з�����ʾһ֡���ݽ������

            if (uart_A_RX_Buff == '\n')

            {

                // ���GPS�����������ƽ��յ�������

                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);

                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1);

                Save_Data.isGetData = true; // ����Ѿ���ȡ��GPS����



                // ������ر�����׼��������һ֡����

                point1 = 0;

                memset(USART_RX_BUF, 0, USART_REC_LEN);

            }

        }

        // ��ֹ���������

        if (point1 >= USART_REC_LEN)

        {

            point1 = USART_REC_LEN;

        }



        // ����������һ���ֽ�

        HAL_UART_Receive_IT(&huart2, &uart_A_RX_Buff, 1);

    }

    // USART3 接收（ESP-01S 下行命令）

    else if (huart->Instance == USART3)

    {

        uart3_rx_ticks = uwTick;

        uart3_rx_buffer[uart3_rx_index++] = uart3_rx_byte;

        if (uart3_rx_index >= sizeof(uart3_rx_buffer) - 1)

            uart3_rx_index = 0;

        HAL_UART_Receive_IT(&huart3, &uart3_rx_byte, 1);

    }

}



/**

 * @brief   ����ṹ�����ݺ���

 */

void clrStruct(void)

{

    Save_Data.isGetData = false; // ���δ��ȡ��GPS����

    Save_Data.isParseData = false; // �������δ����

    Save_Data.isUsefull = false;   // ��Ƕ�λ��Ϣ��Ч

    // ��ո�������

    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);

    memset(Save_Data.UTCTime, 0, UTCTime_Length);

    memset(Save_Data.latitude, 0, latitude_Length);

    memset(Save_Data.N_S, 0, N_S_Length);

    memset(Save_Data.longitude, 0, longitude_Length);

    memset(Save_Data.E_W, 0, E_W_Length);

}



/**

 * @brief   ������־����

 */

void errorLog(int num)

{

    while (1)

    {

        // ��ӡ�����Ų�������ѭ��

        my_printf(&huart1, "ERROR%d\r\n", num);

    }

}

/**

 * @brief   ����GPS���ݻ���������

 */ 

void parseGpsBuffer(void)

{

    char *subString;      // ָ��ǰ����λ��

    char *subStringNext;  // ָ���¸�����λ��

    char i = 0;           // ѭ��������



    uint16_t Number = 0, Integer = 0, Decimal = 0; // ���ھ�γ����ֵת��



    if (Save_Data.isGetData) // ����Ѿ���ȡ��GPS����

    {

        Save_Data.isGetData = false;

        // ��ӡ������Ϣ

//        my_printf(&huart1, "**************\r\n");

//        my_printf(&huart1, "%s\r\n", Save_Data.GPS_Buffer);



        for (i = 0; i <= 6; i++) // ѭ���������ֶ�

        {

            if (i == 0)

            {

                // ���ҵ�һ�����ţ��ָ��ֶ�

                if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)

                    errorLog(1); // ����Ҳ������ţ���¼����

            }

            else

            {

                subString++;

                // ������һ������

                if ((subStringNext = strstr(subString, ",")) != NULL)

                {

                    char usefullBuffer[2]; // ���ڴ洢������Ч����Ϣ

                    switch (i)

                    {

                        case 1: // ����ʱ���ֶ�

                            memcpy(Save_Data.UTCTime, subString, subStringNext - subString);

                            break;

                        case 2: // ����������Ч���ֶ�

                            memcpy(usefullBuffer, subString, subStringNext - subString);

                            break;

                        case 3: // ����γ���ֶ�

                            memcpy(Save_Data.latitude, subString, subStringNext - subString);

                            break;

                        case 4: // ����γ�ȷ����ֶ�

                            memcpy(Save_Data.N_S, subString, subStringNext - subString);

                            break;

                        case 5: // ����������ֶ�

                            memcpy(Save_Data.longitude, subString, subStringNext - subString);

                            break;

                        case 6: // �������ȷ����ֶ�

                            memcpy(Save_Data.E_W, subString, subStringNext - subString);

                            break;

                        default:

                            break;

                    }

                    subString = subStringNext; // ���µ�ǰ����λ��

                    Save_Data.isParseData = true; // ��������ѽ���

                    // �ж�������Ч��

                    if (usefullBuffer[0] == 'A')

                        Save_Data.isUsefull = true; // ������Ч

                    else if (usefullBuffer[0] == 'V')

                        Save_Data.isUsefull = false; // ������Ч

                }

                else

                {

//                    errorLog(2); // ����Ҳ������ţ���¼����

                }

            }

        }



        if (Save_Data.isParseData) // ��������ѽ���

        {

            if (Save_Data.isUsefull) // ���������Ч

            {

                // ��ȡγ�ȷ���;��ȷ���

                g_LatAndLongData.N_S = Save_Data.N_S[0];

                g_LatAndLongData.E_W = Save_Data.E_W[0];



                // ת��γ������

                for (uint8_t i = 0; i < 9; i++)

                {

                    if (i < 2)

                    {

                        Number *= 10;

                        Number += Save_Data.latitude[i] - '0'; // ��ȡ�Ȳ���

                    }

                    else if (i < 4)

                    {

                        Integer *= 10;

                        Integer += Save_Data.latitude[i] - '0'; // ��ȡ�ֵ���������

                    }

                    else if (i == 4);

                    else if (i < 9)

                    {

                        Decimal *= 10;

                        Decimal += Save_Data.latitude[i] - '0'; // ��ȡ�ֵ�С������

                    }

                }

                // ת��Ϊʮ���ƶ���

                g_LatAndLongData.latitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;



                Number = 0;

                Integer = 0;

                Decimal = 0;



                // ת����������

                for (uint8_t i = 0; i < 10; i++)

                {

                    if (i < 3)

                    {

                        Number *= 10;

                        Number += Save_Data.longitude[i] - '0'; // ��ȡ�Ȳ���

                    }

                    else if (i < 5)

                    {

                        Integer *= 10;

                        Integer += Save_Data.longitude[i] - '0'; // ��ȡ�ֵ���������

                    }

                    else if (i == 5);

                    else if (i < 10)

                    {

                        Decimal *= 10;

                        Decimal += Save_Data.longitude[i] - '0'; // ��ȡ�ֵ�С������

                    }

                }

                // ת��Ϊʮ���ƶ���

                g_LatAndLongData.longitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;



                // ����ȫ�־�γ�ȱ���

                longitude = g_LatAndLongData.longitude;

                latitude = g_LatAndLongData.latitude;

				

				//�����ϱ�γ������γ����

				if(g_LatAndLongData.E_W=='W')

					 latitude = -latitude;

				if(g_LatAndLongData.N_S=='S')

					 latitude = -latitude;

            }

        }

    }

}



/**

 * @brief   ��ӡGPS���ݺ���

 */

void printGpsBuffer(void)

{

    if (Save_Data.isParseData) // ��������ѽ���

    {

        Save_Data.isParseData = false;



        // ��ӡUTCʱ��

        my_printf(&huart1, "Save_Data.UTCTime = %s\r\n", Save_Data.UTCTime);



        if (Save_Data.isUsefull) // ���������Ч

        {

            Save_Data.isUsefull = false;

            // ��ӡԭʼ��γ������

            my_printf(&huart1, "Save_Data.latitude = %s\r\n", Save_Data.latitude);

            my_printf(&huart1, "Save_Data.N_S = %s", Save_Data.N_S);

            my_printf(&huart1, "Save_Data.longitude = %s", Save_Data.longitude);

            my_printf(&huart1, "Save_Data.E_W = %s\r\n", Save_Data.E_W);



            // ��ӡת����ľ�γ������

            my_printf(&huart1, "latitude: %c,%.4f\r\n", g_LatAndLongData.N_S, g_LatAndLongData.latitude);

            my_printf(&huart1, "longitude: %c,%.4f\r\n", g_LatAndLongData.E_W, g_LatAndLongData.longitude);

        }

        else

        {

            // ��ʾGPS������Ч

            my_printf(&huart1, "GPS DATA is not usefull!\r\n");

        }

    }

}



/**

 * @brief   GPSģ��������

 */

void atgm336h_task(void)

{

    parseGpsBuffer(); // ����GPS����

//    printGpsBuffer(); // ��ӡGPS����

}

