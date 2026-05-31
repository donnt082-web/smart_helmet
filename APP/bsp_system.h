#ifndef _BSP_SYSTEM_H
#define _BSP_SYSTEM_H

#include "main.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include "math.h"

#include "sys.h"
#include "adc.h"
#include "tim.h"
#include "i2c.h"
#include "usart.h"

#include "mq2.h"
#include "dht11.h"
#include "atgm336h.h"
#include "scheduler.h"
#include "ringbuffer.h"

//mpu6050相关
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "dmpKey.h"
#include "dmpmap.h"

//max30102相关
#include "myiic.h"
#include "max30102.h"
#include "max30102_app.h"
#include "algorithm.h"

#include "sensor.h"
#include "esp01s.h"

extern uint32_t dma_buff[30];//mq2模块相关

//atgm336h相关
extern uint16_t uart_rx_index;
extern uint32_t uart_rx_ticks;
extern uint8_t uart_rx_buffer[10];

extern uint8_t uart_rx_dma_buffer[1000];//接收缓存

//uart3 接收相关（ESP01S）
extern uint16_t uart3_rx_index;
extern uint32_t uart3_rx_ticks;
extern uint8_t uart3_rx_buffer[256];
extern uint8_t uart3_rx_byte;

extern bool measure_flag;
extern bool density_flag;
extern bool fall_flag;
extern bool collision_flag;
extern bool heartrate_flag;
extern bool spo2_flag;

extern float ppm;
extern uint8_t humi;
extern uint8_t temp;
extern uint8_t dis_hr;   // 显示的心率值
extern uint8_t dis_spo2; // 显示的血氧值
extern float longitude; // 经度
extern float latitude;  // 纬度

#endif
