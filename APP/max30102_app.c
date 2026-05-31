#include "max30102_app.h"

// 心率和血氧标志位，用于指示是否超出正常范围
bool heartrate_flag = 0;
bool spo2_flag = 0;

// 初始化 MAX30102 数据结构
MAX30102_Data max30102_data = {
    .buffer_length = BUFFER_LENGTH, // 设置缓冲区长度
    .min_value = 0x3FFFF,           // 设置初始最小值
    .max_value = 0,                 // 设置初始最大值
    .brightness = 0                 // 设置初始亮度值
};

// 滑动平均滤波器的缓存和索引
int hr_buffer[WINDOW_SIZE] = {0};   // 心率的滑动窗口
int spo2_buffer[WINDOW_SIZE] = {0}; // 血氧的滑动窗口
int hr_index = 0, spo2_index = 0;   // 缓存索引

// 低通滤波器的先前值
int prev_hr = 0, prev_spo2 = 0; // 上一时刻的平滑心率和血氧值

/**
 * @brief  滑动平均滤波函数
 */
int SmoothData(int new_value, int *buffer, int *index)
{
  // 更新滑动窗口
  buffer[*index] = new_value;
  // 循环索引
  *index = (*index + 1) % WINDOW_SIZE;

  // 计算窗口内的平均值
  int sum = 0;
  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    sum += buffer[i];
  }

  // 返回平均值
  return sum / WINDOW_SIZE;
}

/**
 * @brief  低通滤波函数
 */
int LowPassFilter(int new_value, int previous_filtered_value)
{
  // 按公式进行滤波
  return (int)(ALPHA * new_value + (1 - ALPHA) * previous_filtered_value);
}

/**
 * @brief  读取 MAX30102 传感器数据，并进行心率补偿
 */
void MAX30102_Read_Data(void)
{
  uint8_t temp[6]; // 临时缓冲区，用于存储从 FIFO 读取的数据

  // 读取前 500 个样本，确定信号范围
  for (int i = 0; i < max30102_data.buffer_length; i++)
  {
    // 等待中断信号
    while (MAX30102_INT == 1);

    // 从 FIFO 读取数据
    max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
    // 解析红光数据
    max30102_data.red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2];
    // 解析红外数据
    max30102_data.ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];

    // 更新信号的最小值和最大值
    if (max30102_data.min_value > max30102_data.red_buffer[i])
      max30102_data.min_value = max30102_data.red_buffer[i];
    if (max30102_data.max_value < max30102_data.red_buffer[i])
      max30102_data.max_value = max30102_data.red_buffer[i];
  }

  // 更新上一数据点
  volatile uint32_t un_prev_data = max30102_data.red_buffer[max30102_data.buffer_length - 1];

  // 调整补偿值：减少补偿并应用在心率计算前
  max30102_data.heart_rate += HEART_RATE_COMPENSATION;
}

/**
 * @brief  计算心率和血氧值
 */
void Calculate_Heart_Rate_and_SpO2(void)
{
  // 使用 Maxim 提供的算法计算心率和血氧
  maxim_heart_rate_and_oxygen_saturation(max30102_data.ir_buffer, max30102_data.buffer_length,
                                         max30102_data.red_buffer, &max30102_data.spO2, &max30102_data.spO2_valid,
                                         &max30102_data.heart_rate, &max30102_data.heart_rate_valid);
}

/**
 * @brief  更新信号的最小值和最大值，并应用滤波
 */
void Update_Signal_Min_Max(void)
{
  // 获取上一数据点
  uint32_t un_prev_data = max30102_data.red_buffer[max30102_data.buffer_length - 1];

  // 处理数据缓冲区
  for (int i = 100; i < max30102_data.buffer_length; i++)
  {
    // 移位数据
    max30102_data.red_buffer[i - 100] = max30102_data.red_buffer[i];
    max30102_data.ir_buffer[i - 100] = max30102_data.ir_buffer[i];

    // 低通滤波处理
    max30102_data.red_buffer[i - 100] = LowPassFilter(max30102_data.red_buffer[i - 100], un_prev_data);
    max30102_data.ir_buffer[i - 100] = LowPassFilter(max30102_data.ir_buffer[i - 100], un_prev_data);

    // 滑动平均滤波处理
    max30102_data.red_buffer[i - 100] = SmoothData(max30102_data.red_buffer[i - 100], hr_buffer, &hr_index);
    max30102_data.ir_buffer[i - 100] = SmoothData(max30102_data.ir_buffer[i - 100], spo2_buffer, &spo2_index);

    // 更新信号的最小值和最大值
    if (max30102_data.min_value > max30102_data.red_buffer[i - 100])
      max30102_data.min_value = max30102_data.red_buffer[i - 100];
    if (max30102_data.max_value < max30102_data.red_buffer[i - 100])
      max30102_data.max_value = max30102_data.red_buffer[i - 100];
  }
}

// 显示的心率和血氧值
uint8_t dis_hr = 0;   // 显示的心率值
uint8_t dis_spo2 = 0; // 显示的血氧值

/**
 * @brief  数据处理与打印函数
 */
void Process_And_Display_Data(void)
{
  // 检查心率数据是否有效且心率小于 120
  if (max30102_data.heart_rate_valid == 1 && max30102_data.heart_rate < 120)
  {
    // 更新显示的心率和血氧数据
    dis_hr = max30102_data.heart_rate;
    dis_spo2 = max30102_data.spO2;
    // 打印心率和血氧数据
//    my_printf(&huart1,"dis_hr:%d  ,dis_spo2:%d\r\n",dis_hr,dis_spo2);
  }
//  else
//  {
//    // 如果数据无效，将显示的心率和血氧置零
//    dis_hr = 0;
//    dis_spo2 = 0;
//    // 打印零值数据
//    my_printf(&huart1,"dis_hr:%d  ,dis_spo2:%d\r\n",dis_hr,dis_spo2);
//  }
}

/**
 * @brief  MAX30102 任务函数，负责读取和处理传感器数据
 */
void max30102_task(void)
{
  // 如果测量标志位为 0，直接返回
  if (measure_flag == 0)
    return;

  // 重置测量标志位
  measure_flag = 0;

  // 初始化显示的心率和血氧为零
  dis_hr = dis_spo2 = 0;

  // 循环直到获取有效的心率和血氧数据
  while (dis_hr == 0 && dis_spo2 == 0)
  {
    // 禁用中断，防止数据读取过程中被中断
    __disable_irq();

    // 读取 MAX30102 传感器数据
    MAX30102_Read_Data();

    // 计算心率和血氧
    Calculate_Heart_Rate_and_SpO2();

    // 更新信号的最小最大值
    Update_Signal_Min_Max();

    // 处理并显示数据
    Process_And_Display_Data();
  }

  // 启用中断
  __enable_irq();

  // 打印任务结束标志
//  my_printf(&huart1, "over");

  // 根据心率范围设置心率标志位
  heartrate_flag = (dis_hr < 60 || dis_hr > 100);

  // 根据血氧范围设置血氧标志位
  spo2_flag = (dis_spo2 < 95 || dis_spo2 > 100);
}
