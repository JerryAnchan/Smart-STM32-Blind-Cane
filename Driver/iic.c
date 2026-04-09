/**
 * @file iic.c
 * @brief 软件模拟I2C总线驱动（ADXL345等外设使用）
 *
 * 引脚: SCL=PC14, SDA=PC15
 * 速率: 约100kHz（受delay_us精度影响）
 * 协议: 标准I2C，7位地址，MSB先发
 */
#include "iic.h"
#include "delay.h"

/**
 * @brief 设置SDA为推挽输出模式（写数据时调用）
 */
void I2C_SDA_OUT(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;	
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//SDA推挽输出
	GPIO_Init(GPIOC,&GPIO_InitStructure); 						
}

/**
 * @brief 设置SDA为上拉输入模式（读数据/等待ACK时调用）
 */
void I2C_SDA_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU; // 上拉输入，空闲时SDA保持高
	GPIO_Init(GPIOC,&GPIO_InitStructure);
}
/**
 * @brief 初始化I2C总线GPIO（PC14=SCL, PC15=SDA，推挽输出）
 * @note  初始化后SCL/SDA均拉高，总线处于空闲状态
 */
void IIC_init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14|GPIO_Pin_15; // PC14=SCL, PC15=SDA
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC,GPIO_Pin_14|GPIO_Pin_15); // 空闲态：SCL=1, SDA=1
}
/**
 * @brief 发送I2C起始信号（SCL高时SDA下降沿）
 */
void IIC_start()
{
	I2C_SDA_OUT();
	IIC_SDA=1;	  	  
	IIC_SCL=1;
	delay_us(5);
	IIC_SDA=0;
	delay_us(5);
	IIC_SCL=0;
}
/**
 * @brief 发送I2C停止信号（SCL高时SDA上升沿）
 */
void IIC_stop()
{
	I2C_SDA_OUT();
	IIC_SCL=0;
	IIC_SDA=0;
	delay_us(5);
	IIC_SCL=1; 
	IIC_SDA=1;
	delay_us(5);
}
/**
 * @brief 主机发送ACK应答（SDA=0）
 */
void IIC_ack()
{
	IIC_SCL=0;
	I2C_SDA_OUT();
  IIC_SDA=0;
   delay_us(2);
   IIC_SCL=1;
   delay_us(5);
   IIC_SCL=0;	
}
/**
 * @brief 主机发送NACK非应答（SDA=1），用于读取最后一个字节后
 */
void IIC_noack()
{
	IIC_SCL=0;
	I2C_SDA_OUT();
   IIC_SDA=1;
   delay_us(2);
   IIC_SCL=1;
   delay_us(2);
   IIC_SCL=0;
}
/**
 * @brief 等待从机ACK应答
 * @return 0=收到ACK  1=超时无应答（已自动发送STOP）
 * @note   超时阈值约250次轮询，防止总线死锁
 */
u8 IIC_wait_ack()
{
	u8 tempTime=0;
	I2C_SDA_IN();
	IIC_SDA=1;
	delay_us(1);
	IIC_SCL=1;
	delay_us(1);

	while(READ_SDA)
	{
		tempTime++;
		if(tempTime>250) // 约250次轮询超时，避免总线挂死
		{
			IIC_stop();
			return 1;
		}	 
	}

	IIC_SCL=0;
	return 0;
}
/**
 * @brief 发送一个字节（MSB先发）
 * @param txd 待发送字节
 */
void IIC_send_byte(u8 txd)
{
	u8 i=0;
	I2C_SDA_OUT();
	IIC_SCL=0; // 拉低时钟开始数据传输
	for(i=0;i<8;i++)
	{
		IIC_SDA=(txd&0x80)>>7;//读取字节
		txd<<=1;
		IIC_SCL=1;
		delay_us(5); // 发送数据
		IIC_SCL=0;
		delay_us(5);
	}
}
/**
 * @brief 读取一个字节（MSB先收）
 * @param ack 1=读完后发送ACK继续读  0=发送NACK结束读取
 * @return 读取到的字节
 */
u8 IIC_read_byte(u8 ack)
{
	u8 i=0,receive=0;
	I2C_SDA_IN();
   for(i=0;i<8;i++)
   {
   		IIC_SCL=0;
		delay_us(5);
		IIC_SCL=1;
		receive<<=1;//左移
		if(READ_SDA)
		   receive++;//连续读取八位
		delay_us(1);	
   }

   	if(!ack)
	   	IIC_noack();
	else
		IIC_ack();

	return receive;//返回读取到的字节
}

