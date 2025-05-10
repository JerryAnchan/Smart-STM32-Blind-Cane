#include "HC_SR04.h"
//#include "timer.h"
#include "delay.h"

//////////////////////////////////////////////////////////////////////////////////	 

void HC_SR04_IO_Init(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;				 // 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOB.12
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;				 // 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOB.13
	
}

u16 TIM3_UPDATA = 0;
u32 temp = 0;
u16  Get_SR04_Distance(void)
{

   SR04_Trlg=0;       //触发信号是高电平脉冲，宽度大于10us
   delay_ms(20);
   SR04_Trlg=1	;
   while(!SR04_Echo); //等待高电平
	 TIM_SetCounter(TIM3,0);  //重填计数器值
	 TIM3_UPDATA = 0;         //溢出次数清零
   while(SR04_Echo);  //等待低电平
	 TIM_Cmd(TIM3,DISABLE);  //暂时关闭定时器，保证数据正确性
   temp = (int)(((double)(TIM_GetCounter(TIM3) + (7200* TIM3_UPDATA)))/72/2);  //得到高电平总时间，单位us（定时器的计数值加上溢出的值才是高电平的时间），除以2是计算单程的时间
	 //（7200* TIM3_UPDATA这里为溢出的时间us，可以转换为100* TIM3_UPDATA,因为在公式后面除以了72。可最终理解为TIM3_UPDATA个100us）
	 TIM_Cmd(TIM3,ENABLE);
	 return temp;
}
 

