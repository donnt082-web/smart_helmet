#ifndef __MAX30102_APP_H__
#define __MAX30102_APP_H__

#include "bsp_system.h"

// 数据缓存长度（100Hz 采样 × 5 秒 = 500 样本，与 Maxim 算法要求一致）
#define BUFFER_LENGTH 500

// MAX30102 数据结构：存储采集数据和计算结果
typedef struct
{
  // 红外 LED 数据（心率计算用）
  uint32_t ir_buffer[BUFFER_LENGTH];
  // 红光 LED 数据（血氧计算用）
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
  // 信号亮度
  int32_t brightness;
  // 数据缓冲区长度
  uint32_t buffer_length;
} MAX30102_Data;

// MAX30102 任务：非阻塞持续测量心率/血氧
void max30102_task(void);

// 数据结构（供外部引用）
extern MAX30102_Data max30102_data;

// 调试用：最近一次采集的 IR 直流均值（上报给服务端看，用于调阈值）
extern uint32_t dbg_ir_mean;

#endif
