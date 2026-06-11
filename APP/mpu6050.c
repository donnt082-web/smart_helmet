#include "mpu6050.h"



uint8_t i = 10; // 循环计数器

float pitch, roll, yaw;    // 欧拉角（姿态数据）

short aacx, aacy, aacz;    // 加速度计原始数据

short gyrox, gyroy, gyroz; // 陀螺仪原始数据

unsigned long walk;        // 步数

float steplength = 0.3, Distance; // 步距和路程计算参数

uint8_t svm_set = 1;       // 路程计算标志

uint16_t AVM;              // 加速度向量模值

uint16_t GVM;              // 陀螺仪向量模值

bool fall_flag = 0;        // 跌倒标志位

bool collision_flag = 0;   // 碰撞标志位



/**

 * @brief  I2C 写操作函数，用于向 MPU6050 写入多个字节数据

 */

uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)

{

    uint8_t data[1 + len]; // 组合数据缓冲区，第一个字节为寄存器地址

    data[0] = reg;

    for (uint8_t i = 0; i < len; i++)

        data[i + 1] = buf[i];



    // 通过 I2C 发送数据

    if (HAL_I2C_Master_Transmit(&hi2c1, (addr << 1), data, len + 1, HAL_MAX_DELAY) != HAL_OK)

        return 1; // 传输失败

    return 0;     // 传输成功

}



/**

 * @brief  I2C 读操作函数，用于从 MPU6050 读取多个字节数据

 */

uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)

{

    // 发送寄存器地址

    if (HAL_I2C_Master_Transmit(&hi2c1, (addr << 1), &reg, 1, HAL_MAX_DELAY) != HAL_OK)

        return 1; // 发送寄存器地址失败



    // 读取数据

    if (HAL_I2C_Master_Receive(&hi2c1, (addr << 1), buf, len, HAL_MAX_DELAY) != HAL_OK)

        return 1; // 读取数据失败

    return 0;     // 读取成功

}



/**

 * @brief  向 MPU6050 写入单个字节数据

 */

uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data)

{

    uint8_t buf[2] = {reg, data};

    if (HAL_I2C_Master_Transmit(&hi2c1, (MPU_ADDR << 1), buf, 2, HAL_MAX_DELAY) != HAL_OK)

    {

        return 1; // 传输失败

    }

    return 0; // 传输成功

}



/**

 * @brief  从 MPU6050 读取单个字节数据

 */

uint8_t MPU_Read_Byte(uint8_t reg)

{

    uint8_t data;

    // 发送寄存器地址

    if (HAL_I2C_Master_Transmit(&hi2c1, (MPU_ADDR << 1), &reg, 1, HAL_MAX_DELAY) != HAL_OK)

    {

        return 0; // 传输失败

    }



    // 读取数据

    if (HAL_I2C_Master_Receive(&hi2c1, (MPU_ADDR << 1), &data, 1, HAL_MAX_DELAY) != HAL_OK)

    {

        return 0; // 读取失败

    }

    return data; // 返回读取的数据

}



/**

 * @brief  初始化 MPU6050 函数

 */

void MPU_Init(void)

{

    // 解除 MPU6050 休眠状态

    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x00);

    // 设置陀螺仪采样率（典型值 125Hz）

    MPU_Write_Byte(MPU_SAMPLE_RATE_REG, 0x07);

    // 设置低通滤波器（44Hz，能捕获撞击尖峰）

    MPU_Write_Byte(MPU_CFG_REG, 0x03);

    // 配置陀螺仪（不自检，量程 2000deg/s）

    MPU_Write_Byte(MPU_GYRO_CFG_REG, 0x18);

    // 配置加速度计（不自检，量程 2G）

    MPU_Write_Byte(MPU_ACCEL_CFG_REG, 0x00);

}



/**

 * @brief  获取温度值函数

 */

short MPU_Get_Temperature(void)

{

    uint8_t buf[2];

    short raw;

    float temp;

    // 读取温度寄存器的高低 8 位

    MPU_Read_Len(MPU_ADDR, MPU_TEMP_OUTH_REG, 2, buf);

    // 组合 16 位原始数据

    raw = ((uint16_t)buf[0] << 8) | buf[1];

    // 转换为实际温度值（扩大 100 倍返回）

    temp = 36.53 + ((double)raw) / 340;

    return temp * 100;

}



/**

 * @brief  获取陀螺仪原始数据函数

 */

uint8_t MPU_Get_Gyroscope(short *gx, short *gy, short *gz)

{

    uint8_t buf[6], res;

    res = MPU_Read_Len(MPU_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);

    if (res == 0)

    {

        // 组合各轴的高低 8 位数据

        *gx = ((uint16_t)buf[0] << 8) | buf[1];

        *gy = ((uint16_t)buf[2] << 8) | buf[3];

        *gz = ((uint16_t)buf[4] << 8) | buf[5];

    }

    return res;

}



/**

 * @brief  获取加速度计原始数据函数

 */

uint8_t MPU_Get_Accelerometer(short *ax, short *ay, short *az)

{

    uint8_t buf[6], res;

    res = MPU_Read_Len(MPU_ADDR, MPU_ACCEL_XOUTH_REG, 6, buf);

    if (res == 0)

    {

        // 组合各轴的高低 8 位数据

        *ax = ((uint16_t)buf[0] << 8) | buf[1];

        *ay = ((uint16_t)buf[2] << 8) | buf[3];

        *az = ((uint16_t)buf[4] << 8) | buf[5];

    }

    return res;

}



/**

 * @brief  MPU6050 任务处理函数

 */

void mpu6050_task(void)

{

    // 获取姿态数据（欧拉角）

    mpu_dmp_get_data(&pitch, &roll, &yaw);

    // 获取加速度计数据

    MPU_Get_Accelerometer(&aacx, &aacy, &aacz);

    // 获取陀螺仪数据

    MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);

    

    // 计算加速度向量模值

    AVM = sqrt(pow(aacx, 2) + pow(aacy, 2) + pow(aacz, 2));

    // 计算陀螺仪向量模值

    GVM = sqrt(pow(gyrox, 2) + pow(gyroy, 2) + pow(gyroz, 2));

    // ── 跌倒检测（自校准重力方向偏离角，不依赖 DMP/FIFO）──────
    // DMP 的 pitch/roll 依赖 FIFO，一旦读取失败角度会冻结、翻转也不变。
    // 这里只用加速度原始值（单纯 I2C 读，MPU 在线就有数据）。
    //
    // 原理：开机静止 1 秒，记录当前重力方向作为基准 g0（含方向/符号）。
    //       之后实时重力 g 与 g0 的夹角 = acos(g·g0/|g|)。
    //       夹角 > 45° → cos < 0.707 → 触发。翻转 180° 夹角=180°，必触发。
    //       每周期重新判定，恢复原姿态自动清零。
    //
    // MPU6050 ±2g → 1g = 16384 LSB

    #define GRAV_1G   16384.0f

    static float    g0x = 0, g0y = 0, g0z = 0;   // 基准重力方向（已归一化）
    static uint8_t  calib_cnt = 0;               // 校准采样计数
    static uint8_t  tilt_cnt  = 0;

    float ax = (float)aacx, ay = (float)aacy, az = (float)aacz;
    float mag = sqrtf(ax*ax + ay*ay + az*az);
    if (mag < 1.0f) mag = 1.0f;   // 防除零

    // 开机前 10 次接近 1g 的采样作为基准姿态
    if (calib_cnt < 10)
    {
        if (mag > GRAV_1G * 0.7f && mag < GRAV_1G * 1.3f)
        {
            g0x += ax; g0y += ay; g0z += az;
            calib_cnt++;
        }
        return;   // 校准期间不检测
    }
    else if (calib_cnt == 10)
    {
        float gm = sqrtf(g0x*g0x + g0y*g0y + g0z*g0z);
        if (gm < 1.0f) gm = 1.0f;
        g0x /= gm; g0y /= gm; g0z /= gm;   // 归一化
        calib_cnt = 11;
    }

    // 当前方向与基准的夹角余弦（g0 已归一化）
    float cosang = (ax*g0x + ay*g0y + az*g0z) / mag;

    if (cosang < 0.707f)        // 夹角 > 45°
    {
        if (++tilt_cnt >= 2)    // 持续 200ms 确认
            fall_flag = 1;
    }
    else
    {
        tilt_cnt = 0;
        fall_flag = 0;          // 恢复姿态自动清零
    }
}
