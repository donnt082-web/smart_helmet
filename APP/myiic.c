#include "myiic.h"

//初始化IIC
void MAX30102_IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	// RCC->APB2ENR|=1<<4;//先使能外设IO PORTC时钟
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStructure.Pin = MAX30102_IIC_SCL_PIN | MAX30102_IIC_SDA_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(MAX30102_IIC_PORT, &GPIO_InitStructure);

	MAX30102_IIC_SCL = 1;
	MAX30102_IIC_SDA = 1;
}

// MAX30102引脚输出模式控制
void MAX30102_IIC_SDA_OUT(void) // SDA输出方向配置
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = MAX30102_IIC_SDA_PIN;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; // SDA推挽输出
	HAL_GPIO_Init(MAX30102_IIC_PORT, &GPIO_InitStructure);
}

void MAX30102_IIC_SDA_IN(void) // SDA输入方向配置
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = MAX30102_IIC_SDA_PIN;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT; // SCL上拉输入
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(MAX30102_IIC_PORT, &GPIO_InitStructure);
}

// 产生IIC起始信号
void MAX30102_IIC_Start(void)
{
	MAX30102_IIC_SDA_OUT(); // sda线输出
	MAX30102_IIC_SDA = 1;
	MAX30102_IIC_SCL = 1;
	Delay_us(4);
	MAX30102_IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
	Delay_us(4);
	MAX30102_IIC_SCL = 0; // 钳住I2C总线，准备发送或接收数据
}
// 产生IIC停止信号
void MAX30102_IIC_Stop(void)
{
	MAX30102_IIC_SDA_OUT(); // sda线输出
	MAX30102_IIC_SCL = 0;
	MAX30102_IIC_SDA = 0; // STOP:when CLK is high DATA change form low to high
	Delay_us(4);
	MAX30102_IIC_SCL = 1;
	MAX30102_IIC_SDA = 1; // 发送I2C总线结束信号
	Delay_us(4);
}
// 等待应答信号到来
// 返回值：1，接收应答失败
//         0，接收应答成功
uint8_t MAX30102_IIC_Wait_Ack(void)
{
	uint8_t ucErrTime = 0;
	MAX30102_IIC_SDA_IN(); // SDA设置为输入
	MAX30102_IIC_SDA = 1;
	Delay_us(1);
	MAX30102_IIC_SCL = 1;
	Delay_us(1);
	while (MAX30102_READ_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			MAX30102_IIC_Stop();
			return 1;
		}
	}
	MAX30102_IIC_SCL = 0; // 时钟输出0
	return 0;
}
// 产生ACK应答
void MAX30102_IIC_Ack(void)
{
	MAX30102_IIC_SCL = 0;
	MAX30102_IIC_SDA_OUT();
	MAX30102_IIC_SDA = 0;
	Delay_us(2);
	MAX30102_IIC_SCL = 1;
	Delay_us(2);
	MAX30102_IIC_SCL = 0;
}
// 不产生ACK应答
void MAX30102_IIC_NAck(void)
{
	MAX30102_IIC_SCL = 0;
	MAX30102_IIC_SDA_OUT();
	MAX30102_IIC_SDA = 1;
	Delay_us(2);
	MAX30102_IIC_SCL = 1;
	Delay_us(2);
	MAX30102_IIC_SCL = 0;
}
// IIC发送一个字节
// 返回从机有无应答
// 1，有应答
// 0，无应答
void MAX30102_IIC_Send_Byte(uint8_t txd)
{
	uint8_t t;
	MAX30102_IIC_SDA_OUT();
	MAX30102_IIC_SCL = 0; // 拉低时钟开始数据传输
	for (t = 0; t < 8; t++)
	{
		MAX30102_IIC_SDA = (txd & 0x80) >> 7;
		txd <<= 1;
		Delay_us(2); // 对TEA5767这三个延时都是必须的
		MAX30102_IIC_SCL = 1;
		Delay_us(2);
		MAX30102_IIC_SCL = 0;
		Delay_us(2);
	}
}
// 读1个字节，ack=1时，发送ACK，ack=0，发送nACK
uint8_t MAX30102_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	MAX30102_IIC_SDA_IN(); // SDA设置为输入
	for (i = 0; i < 8; i++)
	{
		MAX30102_IIC_SCL = 0;
		Delay_us(2);
		MAX30102_IIC_SCL = 1;
		receive <<= 1;
		if (MAX30102_READ_SDA)
			receive++;
		Delay_us(1);
	}
	if (!ack)
		MAX30102_IIC_NAck(); // 发送nACK
	else
		MAX30102_IIC_Ack(); // 发送ACK
	return receive;
}

void MAX30102_IIC_WriteBytes(uint8_t WriteAddr, uint8_t *data, uint8_t dataLength)
{
	uint8_t i;
	MAX30102_IIC_Start();

	MAX30102_IIC_Send_Byte(WriteAddr); // 发送写命令
	MAX30102_IIC_Wait_Ack();

	for (i = 0; i < dataLength; i++)
	{
		MAX30102_IIC_Send_Byte(data[i]);
		MAX30102_IIC_Wait_Ack();
	}
	MAX30102_IIC_Stop(); // 产生一个停止条件
	HAL_Delay(10);
}

void MAX30102_IIC_ReadBytes(uint8_t deviceAddr, uint8_t writeAddr, uint8_t *data, uint8_t dataLength)
{
	uint8_t i;
	MAX30102_IIC_Start();

	MAX30102_IIC_Send_Byte(deviceAddr); // 发送写命令
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(writeAddr);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(deviceAddr | 0X01); // 进入接收模式
	MAX30102_IIC_Wait_Ack();

	for (i = 0; i < dataLength - 1; i++)
	{
		data[i] = MAX30102_IIC_Read_Byte(1);
	}
	data[dataLength - 1] = MAX30102_IIC_Read_Byte(0);
	MAX30102_IIC_Stop(); // 产生一个停止条件
	HAL_Delay(10);
}

void MAX30102_IIC_Read_One_Byte(uint8_t daddr, uint8_t addr, uint8_t *data)
{
	MAX30102_IIC_Start();

	MAX30102_IIC_Send_Byte(daddr); // 发送写命令
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(addr); // 发送地址
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(daddr | 0X01); // 进入接收模式
	MAX30102_IIC_Wait_Ack();
	*data = MAX30102_IIC_Read_Byte(0);
	MAX30102_IIC_Stop(); // 产生一个停止条件
}

void MAX30102_IIC_Write_One_Byte(uint8_t daddr, uint8_t addr, uint8_t data)
{
	MAX30102_IIC_Start();

	MAX30102_IIC_Send_Byte(daddr); // 发送写命令
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(addr); // 发送地址
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(data); // 发送字节
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Stop(); // 产生一个停止条件
	HAL_Delay(10);
}
