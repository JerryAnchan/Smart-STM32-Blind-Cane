/**
 * @file gpio.c
 * @brief 系统GPIO初始化（蜂鸣器、按键、LED/光敏、水位传感器）
 *
 * 引脚分配:
 *   蜂鸣器  PC13  推挽输出
 *   KEY1~4  PB12~15  上拉输入
 *   KEY5    PA8      上拉输入
 *   LED     PA0      输出
 *   GM(光敏) PA1      下拉输入
 *   WATER   PB9      由硬件外部定义
 */
#include "gpio.h"

//////////////////////////////////////////////////////////////////////////////////	 
//蜂鸣器，按键的GPIO设置								  
////////////////////////////////////////////////////////////////////////////////// 	   

/**
 * @brief 初始化蜂鸣器GPIO（PC13推挽输出，默认低电平关闭）
 * @note  需同时开启AFIO时钟并禁用JTAG，否则PB3/PB4不可用
 */
void BEEP_GPIO_Init(void)
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);	 //使能PA端口时钟
	 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);  //关闭JTAG模式 使PB3，PB4变成普通IO口
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOC, &GPIO_InitStructure);		
   GPIO_ResetBits(GPIOC,GPIO_Pin_13);   //输出低电平
}

/**
 * @brief 初始化按键GPIO（PB12~15 + PA8 上拉输入，按下为低）
 */
void KEY_GPIO_Init(void)
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //上拉输入
	 GPIO_Init(GPIOB, &GPIO_InitStructure);		
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //下拉输入
	 GPIO_Init(GPIOB, &GPIO_InitStructure);	

   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //下拉输入
	 GPIO_Init(GPIOA, &GPIO_InitStructure);			
}

/**
 * @brief 初始化LED及光敏传感器GPIO（PA1下拉输入读取光敏电平）
 */
void LED_GPIO_Init(void)
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 		 //下拉输入
	 GPIO_Init(GPIOA, &GPIO_InitStructure);		

	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 		 //下拉输入
	 GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOA, &GPIO_InitStructure);		
   GPIO_ResetBits(GPIOA,GPIO_Pin_0);   //输出低电平
}

