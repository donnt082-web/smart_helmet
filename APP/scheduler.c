#include "scheduler.h"



// 全局变量，用于存储任务数量

uint8_t task_num;



typedef struct

{

    void (*task_func)(void);

    uint32_t rate_ms;

    uint32_t last_run;

} task_t;



// 静态任务数组，每个任务包含任务函数、执行周期（毫秒）和上次运行时间（毫秒）

static task_t scheduler_task[] =

{

    // MPU6050 姿态传感器任务，每 100ms 执行一次，当前超时计数初始化为 0

    {mpu6050_task, 100, 0},

    // MQ2 气体传感器任务，每 100ms 执行一次，当前超时计数初始化为 0

    {mq2_task, 100, 0},

    // DHT11 温湿度传感器任务，每 100ms 执行一次，当前超时计数初始化为 0

    {dht11_task, 100, 0},

    // MAX30102 心率血氧传感器任务，每 200ms 执行一次，当前超时计数初始化为 0

    {max30102_task, 200, 0},

    // ATGM336H GPS 模块任务，每 100ms 执行一次，当前超时计数初始化为 0

//    {atgm336h_task, 100, 0},

    // 向 ASRpro 发送标志任务，每 1000ms 执行一次，当前超时计数初始化为 0

    {send_flag_to_asr, 1000, 0},

    // 处理 ASRpro 数据任务，每 20ms 执行一次，当前超时计数初始化为 0

    {process_asr_data, 20, 0},

    // 向服务器上报数据任务，每 350ms 执行一次，当前超时计数初始化为 0

    {esp_report1, 350, 0},

	 // 向服务器上报数据任务，每 1000ms 执行一次，当前超时计数初始化为 0

	{esp_report2, 1000, 0},

	// 处理服务器下发控制命令（风扇/LED），每 10ms 执行一次

	{esp_process_cmd, 10, 0},

};

/**

 * @brief 调度器初始化函数

 * 计算任务数组的元素个数，并将结果存储在 task_num 中

 */

void scheduler_init(void)

{

    // 计算任务数组的元素个数，并将结果存储在 task_num 中

    task_num = sizeof(scheduler_task) / sizeof(task_t);

}



/**

 * @brief 调度器运行函数

 * 遍历任务数组，检查是否有任务需要执行。如果当前时间已经超过任务的执行周期，则执行该任务并更新上次运行时间

 */

void scheduler_run(void)

{

    // 遍历任务数组中的所有任务

    for (uint8_t i = 0; i < task_num; i++)

    {

        // 获取当前的系统时间（毫秒）

        uint32_t now_time = HAL_GetTick();



        // 检查当前时间是否达到任务的执行时间

        if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run)

        {

            // 更新任务的上次运行时间为当前时间

            scheduler_task[i].last_run = now_time;



            // 执行任务函数

            scheduler_task[i].task_func();

        }

    }

}

