#include "wt588d.h"

void WT588D_GPIO_INIT(void)
{

 GPIO_InitTypeDef  GPIO_InitStructure;
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	 //使能PB端口时钟
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;				 //端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOC, &GPIO_InitStructure);					 //根据设定参数初始化GPIOB.11
}

void Line_1A(unsigned char dat)//播报地址内容
{
	unsigned char i;
	
	P_DATA = 0;

    delay_ms(5);   

	for(i=0;i<8;i++)
	{

		P_DATA = 1;			
		if(dat&0X01)
		{	
		    delay_us(600); 
			  P_DATA = 0;
		    delay_us(200);	    
		}
		else
		{  
			delay_us(200);
			P_DATA = 0;
		    delay_us(600);
		}
	    dat>>=1;
	  }
	P_DATA = 1;		
}


