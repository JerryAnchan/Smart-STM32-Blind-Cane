/**
 * @file timer.c
 * @brief 通用定时器配置（TIM2系统节拍 + TIM3超声波计时）
 *
 * 时钟源: APB1=36MHz, 定时器时钟=APB1×2=72MHz
 * TIM2: 系统10ms节拍，优先级(0,2)
 * TIM3: 超声波回波计时，优先级(0,1)，初始化后处于禁止状态
 */
#include "timer.h"
#include "delay.h"

/**
 * @brief 初始化TIM2（系统节拍定时器）
 * @param arr 自动重装值
 * @param psc 预分频系数
 * @note  中断周期 = (arr+1)*(psc+1)/72MHz
 *        例: arr=499, psc=7199 → 500*7200/72M = 50ms…实际代码使用arr=499,psc=7199得50ms
 */
void TIM2_Init(u16 arr,u16 psc)
{	 
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//使能TIM2时钟
	
	//初始化定时器2	 
	TIM_TimeBaseStructure.TIM_Period = arr; //设定计数器自动重装值 
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 	//预分频器   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  

	
	//中断分组初始化
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器 
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//允许更新中断 ,
	
  TIM_Cmd(TIM2,ENABLE ); 	//使能定时器2
}

/**
 * @brief 初始化TIM3（超声波回波计时用，初始化后处于禁止状态）
 * @param arr 自动重装值
 * @param psc 预分频系数
 * @note  中断优先级(0,1)高于TIM2(0,2)，确保计时精度
 */
void TIM3_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为500ms
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(  //使能或者失能指定的TIM中断
		TIM3, //TIM3
		TIM_IT_Update ,
		ENABLE  //使能
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //从优先级2级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM3, DISABLE);  //失能TIMx外设						 
}


//定时器3中断服务程序	               主要记录溢出次数
// VL53L1X替换HC_SR04后，TIM3_UPDATA变量定义在此处
u16 TIM3_UPDATA = 0;
void TIM3_IRQHandler(void)
{ 
		if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
				TIM3_UPDATA++;//当回响信号很长时，记录溢出次数，每加一次代表加100us
				TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //清除中断标志位
		}
}

