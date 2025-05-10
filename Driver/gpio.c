#include "gpio.h"

//////////////////////////////////////////////////////////////////////////////////	 
//蜂鸣器，按键的GPIO设置								  
////////////////////////////////////////////////////////////////////////////////// 	   

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

