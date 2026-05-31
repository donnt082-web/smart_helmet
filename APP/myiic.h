#ifndef __MYIIC_H
#define __MYIIC_H
#include "bsp_system.h"	  

#define		MAX30102_IIC_PORT				GPIOB
#define		MAX30102_IIC_SCL_PIN		    GPIO_PIN_15
#define		MAX30102_IIC_SDA_PIN		    GPIO_PIN_14

#define 	MAX30102_IIC_SCL				PBout(15)
#define 	MAX30102_IIC_SDA				PBout(14)
#define 	MAX30102_READ_SDA   		    PBin(14)  //输入SDA 

#define		MAX30102_INT_PORT				GPIOB
#define		MAX30102_INT_PIN		        GPIO_PIN_13
#define		MAX30102_INT                    PBin(13)


//IIC所有操作函数
void MAX30102_IIC_Init(void);                //初始化IIC的IO口				 
void MAX30102_IIC_Start(void);				//发送IIC开始信号
void MAX30102_IIC_Stop(void);	  			//发送IIC停止信号
void MAX30102_IIC_Send_Byte(uint8_t txd);			//IIC发送一个字节
uint8_t MAX30102_IIC_Read_Byte(unsigned char ack);//IIC读取一个字节
uint8_t MAX30102_IIC_Wait_Ack(void); 				//IIC等待ACK信号
void MAX30102_IIC_Ack(void);					//IIC发送ACK信号
void MAX30102_IIC_NAck(void);				//IIC不发送ACK信号

void MAX30102_IIC_Write_One_Byte(uint8_t daddr,uint8_t addr,uint8_t data);
void MAX30102_IIC_Read_One_Byte(uint8_t daddr,uint8_t addr,uint8_t* data);

void MAX30102_IIC_WriteBytes(uint8_t WriteAddr,uint8_t* data,uint8_t dataLength);
void MAX30102_IIC_ReadBytes(uint8_t deviceAddr, uint8_t writeAddr,uint8_t* data,uint8_t dataLength);

#endif
