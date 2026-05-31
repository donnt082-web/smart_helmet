#include "atgm336h.h"



uint16_t point1 = 0; // 用于记录接收到的数据长度

// UART3 接收缓冲区（ESP01S）
uint16_t uart3_rx_index = 0;
uint32_t uart3_rx_ticks = 0;
uint8_t uart3_rx_buffer[256];
uint8_t uart3_rx_byte;

float longitude;     // 用于存储经度

float latitude;      // 用于存储纬度



// 定义保存GPS数据的全局结构体实例

_SaveData Save_Data;

// 定义存储经纬度数据的全局结构体实例

LatitudeAndLongitude_s g_LatAndLongData =

{

    .E_W = 0,

    .N_S = 0,

    .latitude = 0.0,

    .longitude = 0.0

};



// 串口接收缓冲区及临时变量

char USART_RX_BUF[USART_REC_LEN];

uint8_t uart_A_RX_Buff;



/**

 * @brief   初始化GPS模块

 */

void atgm336h_init(void)

{

    clrStruct(); // 清除结构体数据

    // 启用串口中断，准备接收数据

    HAL_UART_Receive_IT(&huart2, &uart_A_RX_Buff, 1);

}



/**

 * @brief   串口接收完成回调函数

 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)

{

    // 如果是USART1接收完成

    if (huart->Instance == USART1)

    {

        // 更新接收时间戳并增加接收索引

        uart_rx_ticks = uwTick;

        uart_rx_index++;

        // 继续接收下一个字节

        HAL_UART_Receive_IT(&huart1, &uart_rx_buffer[uart_rx_index], 1);

    }

    // 如果是USART2接收完成

    else if (huart->Instance == USART2)

    {

        // 判断是否收到帧头标志字符'$'

        if (uart_A_RX_Buff == '$')

        {

            point1 = 0; // 重置数据长度计数器

        }

        USART_RX_BUF[point1++] = uart_A_RX_Buff; // 存储接收到的数据



        // 检查是否收到完整的GPRMC/GNRMC帧数据

        if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C')

        {

            // 如果接收到换行符，表示一帧数据接收完成

            if (uart_A_RX_Buff == '\n')

            {

                // 清空GPS缓冲区并复制接收到的数据

                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);

                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1);

                Save_Data.isGetData = true; // 标记已经获取到GPS数据



                // 重置相关变量，准备接收下一帧数据

                point1 = 0;

                memset(USART_RX_BUF, 0, USART_REC_LEN);

            }

        }

        // 防止缓冲区溢出

        if (point1 >= USART_REC_LEN)

        {

            point1 = USART_REC_LEN;

        }



        // 继续接收下一个字节

        HAL_UART_Receive_IT(&huart2, &uart_A_RX_Buff, 1);

    }

    // USART3 接收（ESP01S）
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

 * @brief   清除结构体数据函数

 */

void clrStruct(void)

{

    Save_Data.isGetData = false; // 标记未获取到GPS数据

    Save_Data.isParseData = false; // 标记数据未解析

    Save_Data.isUsefull = false;   // 标记定位信息无效

    // 清空各缓冲区

    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);

    memset(Save_Data.UTCTime, 0, UTCTime_Length);

    memset(Save_Data.latitude, 0, latitude_Length);

    memset(Save_Data.N_S, 0, N_S_Length);

    memset(Save_Data.longitude, 0, longitude_Length);

    memset(Save_Data.E_W, 0, E_W_Length);

}



/**

 * @brief   错误日志函数

 */

void errorLog(int num)

{

    while (1)

    {

        // 打印错误编号并进入死循环

        my_printf(&huart1, "ERROR%d\r\n", num);

    }

}

/**

 * @brief   解析GPS数据缓冲区函数

 */ 

void parseGpsBuffer(void)

{

    char *subString;      // 指向当前解析位置

    char *subStringNext;  // 指向下个解析位置

    char i = 0;           // 循环计数器



    uint16_t Number = 0, Integer = 0, Decimal = 0; // 用于经纬度数值转换



    if (Save_Data.isGetData) // 如果已经获取到GPS数据

    {

        Save_Data.isGetData = false;

        // 打印调试信息

//        my_printf(&huart1, "**************\r\n");

//        my_printf(&huart1, "%s\r\n", Save_Data.GPS_Buffer);



        for (i = 0; i <= 6; i++) // 循环解析各字段

        {

            if (i == 0)

            {

                // 查找第一个逗号，分割字段

                if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)

                    errorLog(1); // 如果找不到逗号，记录错误

            }

            else

            {

                subString++;

                // 查找下一个逗号

                if ((subStringNext = strstr(subString, ",")) != NULL)

                {

                    char usefullBuffer[2]; // 用于存储数据有效性信息

                    switch (i)

                    {

                        case 1: // 解析时间字段

                            memcpy(Save_Data.UTCTime, subString, subStringNext - subString);

                            break;

                        case 2: // 解析数据有效性字段

                            memcpy(usefullBuffer, subString, subStringNext - subString);

                            break;

                        case 3: // 解析纬度字段

                            memcpy(Save_Data.latitude, subString, subStringNext - subString);

                            break;

                        case 4: // 解析纬度方向字段

                            memcpy(Save_Data.N_S, subString, subStringNext - subString);

                            break;

                        case 5: // 解解析经度字段

                            memcpy(Save_Data.longitude, subString, subStringNext - subString);

                            break;

                        case 6: // 解析经度方向字段

                            memcpy(Save_Data.E_W, subString, subStringNext - subString);

                            break;

                        default:

                            break;

                    }

                    subString = subStringNext; // 更新当前解析位置

                    Save_Data.isParseData = true; // 标记数据已解析

                    // 判断数据有效性

                    if (usefullBuffer[0] == 'A')

                        Save_Data.isUsefull = true; // 数据有效

                    else if (usefullBuffer[0] == 'V')

                        Save_Data.isUsefull = false; // 数据无效

                }

                else

                {

//                    errorLog(2); // 如果找不到逗号，记录错误

                }

            }

        }



        if (Save_Data.isParseData) // 如果数据已解析

        {

            if (Save_Data.isUsefull) // 如果数据有效

            {

                // 获取纬度方向和经度方向

                g_LatAndLongData.N_S = Save_Data.N_S[0];

                g_LatAndLongData.E_W = Save_Data.E_W[0];



                // 转换纬度数据

                for (uint8_t i = 0; i < 9; i++)

                {

                    if (i < 2)

                    {

                        Number *= 10;

                        Number += Save_Data.latitude[i] - '0'; // 提取度部分

                    }

                    else if (i < 4)

                    {

                        Integer *= 10;

                        Integer += Save_Data.latitude[i] - '0'; // 提取分的整数部分

                    }

                    else if (i == 4);

                    else if (i < 9)

                    {

                        Decimal *= 10;

                        Decimal += Save_Data.latitude[i] - '0'; // 提取分的小数部分

                    }

                }

                // 转换为十进制度数

                g_LatAndLongData.latitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;



                Number = 0;

                Integer = 0;

                Decimal = 0;



                // 转换经度数据

                for (uint8_t i = 0; i < 10; i++)

                {

                    if (i < 3)

                    {

                        Number *= 10;

                        Number += Save_Data.longitude[i] - '0'; // 提取度部分

                    }

                    else if (i < 5)

                    {

                        Integer *= 10;

                        Integer += Save_Data.longitude[i] - '0'; // 提取分的整数部分

                    }

                    else if (i == 5);

                    else if (i < 10)

                    {

                        Decimal *= 10;

                        Decimal += Save_Data.longitude[i] - '0'; // 提取分的小数部分

                    }

                }

                // 转换为十进制度数

                g_LatAndLongData.longitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;



                // 更新全局经纬度变量

                longitude = g_LatAndLongData.longitude;

                latitude = g_LatAndLongData.latitude;

				

				//进行南北纬，东西纬处理

				if(g_LatAndLongData.E_W=='W')

					 latitude = -latitude;

				if(g_LatAndLongData.N_S=='S')

					 latitude = -latitude;

            }

        }

    }

}



/**

 * @brief   打印GPS数据函数

 */

void printGpsBuffer(void)

{

    if (Save_Data.isParseData) // 如果数据已解析

    {

        Save_Data.isParseData = false;



        // 打印UTC时间

        my_printf(&huart1, "Save_Data.UTCTime = %s\r\n", Save_Data.UTCTime);



        if (Save_Data.isUsefull) // 如果数据有效

        {

            Save_Data.isUsefull = false;

            // 打印原始经纬度数据

            my_printf(&huart1, "Save_Data.latitude = %s\r\n", Save_Data.latitude);

            my_printf(&huart1, "Save_Data.N_S = %s", Save_Data.N_S);

            my_printf(&huart1, "Save_Data.longitude = %s", Save_Data.longitude);

            my_printf(&huart1, "Save_Data.E_W = %s\r\n", Save_Data.E_W);



            // 打印转换后的经纬度数据

            my_printf(&huart1, "latitude: %c,%.4f\r\n", g_LatAndLongData.N_S, g_LatAndLongData.latitude);

            my_printf(&huart1, "longitude: %c,%.4f\r\n", g_LatAndLongData.E_W, g_LatAndLongData.longitude);

        }

        else

        {

            // 提示GPS数据无效

            my_printf(&huart1, "GPS DATA is not usefull!\r\n");

        }

    }

}



/**

 * @brief   GPS模块任务函数

 */

void atgm336h_task(void)

{

    parseGpsBuffer(); // 解析GPS数据

//    printGpsBuffer(); // 打印GPS数据

}

