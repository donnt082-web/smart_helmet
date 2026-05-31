#ifndef __MAX30102_APP_H__
#define __MAX30102_APP_H__

#include "bsp_system.h"

// 心率补偿值，每次补偿固定的心率值
#define HEART_RATE_COMPENSATION 10
// 滑动窗口大小
#define WINDOW_SIZE 20
// 低通滤波器的滤波系数
#define ALPHA 0.05     

// 数据缓存长度
#define BUFFER_LENGTH 500

// MAX30102 数据结构，用于存储传感器数据和相关参数
typedef struct
{
  // 红外LED数据（用于血氧计算）
  uint32_t ir_buffer[BUFFER_LENGTH];
  // 红色LED数据（用于心率计算）
  uint32_t red_buffer[BUFFER_LENGTH];
  // 血氧饱和度
  int32_t spO2;
  // 血氧有效性指示
  int8_t spO2_valid;
  // 心率
  int32_t heart_rate;
  // 心率有效性指示
  int8_t heart_rate_valid;
  // 信号最小值
  int32_t min_value;
  // 信号最大值
  int32_t max_value;
  // 上一数据点
  int32_t prev_data;
  // 信号亮度（用于心率计算）
  int32_t brightness;
  // 数据缓冲区长度
  uint32_t buffer_length;
} MAX30102_Data;

// 滑动平均滤波函数
int SmoothData(int new_value, int *buffer, int *index);

// 低通滤波函数
int LowPassFilter(int new_value, int previous_filtered_value);

// 读取MAX30102传感器数据，并进行心率补偿
void MAX30102_Read_Data(void);

// 计算心率和血氧值
void Calculate_Heart_Rate_and_SpO2(void);

// 更新信号的最小值和最大值，并应用滤波
void Update_Signal_Min_Max(void);

// 数据处理与显示函数，处理心率和血氧数据并将其打印出来
void Process_And_Display_Data(void);

// MAX30102 任务函数，负责读取和处理传感器数据
void max30102_task(void);

// 初始化数据结构
extern MAX30102_Data max30102_data;

#endif

