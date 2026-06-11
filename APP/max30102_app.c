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

// 显示的心率和血氧值（0 表示未测到 / 无手指）
uint8_t dis_hr = 0;   // 显示的心率值
uint8_t dis_spo2 = 0; // 显示的血氧值

// 调试用：最近一次采集的 IR 直流均值（塞进上报 JSON，服务端可见）
uint32_t dbg_ir_mean = 0;

// 手指检测阈值：IR 直流分量低于此值认为没有放手指
// 注意：这个值要按服务端看到的 dbg_ir_mean 实测来调。
// 先用一个较低的保守值，确认能进测量后再往上调。
#define FINGER_IR_MIN   5000

/**
 * @brief  读一个 FIFO 样本，返回 IR 值（用于手指探测）
 *         先等 INT 拉低（数据就绪）再读，避免读到空 FIFO 的垃圾值。
 *         带 50ms 超时，传感器无响应时不卡死。
 */
static uint32_t read_one_ir(void)
{
    uint8_t temp[6];
    uint32_t t0 = HAL_GetTick();
    while (MAX30102_INT == 1)
    {
        if (HAL_GetTick() - t0 > 50)
            return 0;   // 超时，无数据
    }
    max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
    return ((uint32_t)((uint32_t)temp[3] & 0x03) << 16) | ((uint32_t)temp[4] << 8) | temp[5];
}

/**
 * @brief  MAX30102 任务函数：持续测量心率/血氧
 *
 * 设计思路（基于原版"连续读 500 样本"机制，已验证能出数据）：
 *   1. 先快速探测手指：读几个样本看 IR 强度。没手指 → 立刻上报 0 返回，不阻塞。
 *   2. 有手指 → 连续读满 500 样本（约 5 秒，期间会阻塞调度器），
 *      数据连续无空洞，Maxim 算法才能算出有效值。
 *   3. 每次等 INT 带超时，绝不会因传感器无响应而永久卡死。
 */
void max30102_task(void)
{
    uint8_t temp[6];

    // ── 1. 手指探测：快速读几个样本判断 IR 强度 ──
    uint32_t probe = 0;
    for (int i = 0; i < 4; i++)
        probe += read_one_ir();
    probe /= 4;
    dbg_ir_mean = probe;   // 记录给服务端看

    if (probe < FINGER_IR_MIN)
    {
        // 没放手指：清零，不阻塞，下个周期再探
        dis_hr = 0;
        dis_spo2 = 0;
        heartrate_flag = 0;
        spo2_flag = 0;
        return;
    }

    // ── 2. 有手指：连续采满 500 样本（数据必须连续，算法才有效）──
    max30102_data.min_value = 0x3FFFF;
    max30102_data.max_value = 0;

    for (int i = 0; i < BUFFER_LENGTH; i++)
    {
        // 等数据就绪（INT 拉低），带超时防止永久卡死
        uint32_t t0 = HAL_GetTick();
        while (MAX30102_INT == 1)
        {
            if (HAL_GetTick() - t0 > 50)   // 50ms 还没来，放弃本次测量
            {
                dis_hr = 0;
                dis_spo2 = 0;
                return;
            }
        }

        max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
        max30102_data.red_buffer[i] =
            ((uint32_t)((uint32_t)temp[0] & 0x03) << 16) | ((uint32_t)temp[1] << 8) | temp[2];
        max30102_data.ir_buffer[i] =
            ((uint32_t)((uint32_t)temp[3] & 0x03) << 16) | ((uint32_t)temp[4] << 8) | temp[5];

        if (max30102_data.min_value > max30102_data.red_buffer[i])
            max30102_data.min_value = max30102_data.red_buffer[i];
        if (max30102_data.max_value < max30102_data.red_buffer[i])
            max30102_data.max_value = max30102_data.red_buffer[i];

        // 采样会阻塞约 5 秒，期间穿插处理服务器下发命令（风扇/LED），
        // 否则测量时控制命令要等采样结束才生效（表现为"反应慢"）。
        // 每 20 个样本（约 200ms）处理一次，兼顾响应和采样效率。
        if ((i % 20) == 0)
            esp_process_cmd();
    }

    // ── 3. 跑 Maxim 官方算法（自带滤波）──
    maxim_heart_rate_and_oxygen_saturation(
        max30102_data.ir_buffer, BUFFER_LENGTH, max30102_data.red_buffer,
        &max30102_data.spO2, &max30102_data.spO2_valid,
        &max30102_data.heart_rate, &max30102_data.heart_rate_valid);

    // 心率：有效且在生理合理范围（30~200）才采用，否则置 0
    if (max30102_data.heart_rate_valid &&
        max30102_data.heart_rate > 30 && max30102_data.heart_rate < 200)
        dis_hr = (uint8_t)max30102_data.heart_rate;
    else
        dis_hr = 0;

    // 血氧：有效且 0~100 才采用，否则置 0
    if (max30102_data.spO2_valid &&
        max30102_data.spO2 > 0 && max30102_data.spO2 <= 100)
        dis_spo2 = (uint8_t)max30102_data.spO2;
    else
        dis_spo2 = 0;

    // ── 告警标志：仅在测到有效值时判断 ──
    heartrate_flag = (dis_hr != 0) && (dis_hr < 60 || dis_hr > 100);
    spo2_flag      = (dis_spo2 != 0) && (dis_spo2 < 95);
}
