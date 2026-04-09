/**
 * @file HC_SR04.c
 * @brief HC-SR04超声波测距驱动
 *
 * 引脚: Trig=PB4(触发), Echo=PB5(回波)
 * 原理: 声速340m/s，往返时间/2=单程距离
 * 计时: 使用TIM3(72MHz时基)计算回波脉宽
 */
#include "HC_SR04.h"
#include "delay.h"


/**
 * @brief 初始化HC-SR04引脚（PB5=Echo浮空输入, PB4=Trig推挽输出）
 */
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

u16 TIM3_UPDATA = 0;  // TIM3溢出计数（由TIM3中断累加）
u32 temp = 0;

/**
 * @brief 获取超声波测距结果
 * @return 距离(微米/μm量级)，0xFFFF=超时无回波
 * @note  超时阈值30000次轮询≈数十ms，取决于CPU频率
 */
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
    }

    TIM_SetCounter(TIM3, 0);
    TIM3_UPDATA = 0;

    // 等待回波引脚变低，超时则返回异常
    timeout = 30000;   // 约30ms超时
    while(SR04_Echo) {
        if(--timeout == 0) return 0xFFFF;
    }

    TIM_Cmd(TIM3, DISABLE);
    /* 计算距离: timer_ticks / 72 = 微秒数 (72MHz时钟)
     * 微秒数 / 2 = 单程时间 (声音往返)
     * 单程时间 * 0.034cm/μs ≈ 距离(cm) */
    temp = (int)(((double)(TIM_GetCounter(TIM3) + (7200 * TIM3_UPDATA))) / 72 / 2);
    TIM_Cmd(TIM3, ENABLE);
    return temp;
}
