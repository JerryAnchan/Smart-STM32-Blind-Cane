#include "HC_SR04.h"
//#include "timer.h"
#include "delay.h"
#include "OLED_I2C.h"
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
    u32 timeout = 0;
    SR04_Trlg = 0;
    delay_us(10);      // 建议用us级延时
    SR04_Trlg = 1;
    delay_us(15);      // 保证高电平宽度大于10us
    SR04_Trlg = 0;

    // 等待回波引脚变高，超时则返回异常
    timeout = 30000;   // 约30ms超时
    while(!SR04_Echo) {
        if(--timeout == 0) return 0xFFFF;
        //OLED_ShowStr(0, 7, "Loop E1", 1);
    }

    TIM_SetCounter(TIM3, 0);
    TIM3_UPDATA = 0;

    // 等待回波引脚变低，超时则返回异常
    timeout = 30000;   // 约30ms超时
    while(SR04_Echo) {
        if(--timeout == 0) return 0xFFFF;
        //OLED_ShowStr(0, 7, "Loop E2", 1);
    }

    TIM_Cmd(TIM3, DISABLE);
    temp = (int)(((double)(TIM_GetCounter(TIM3) + (7200 * TIM3_UPDATA))) / 72 / 2);
    TIM_Cmd(TIM3, ENABLE);
    return temp;
}
