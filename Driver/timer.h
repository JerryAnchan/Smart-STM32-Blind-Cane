/**
 * @file timer.h
 * @brief 定时器初始化接口（TIM2系统节拍 / TIM3超声波计时）
 */
#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

void TIM2_Init(u16 arr,u16 psc);
void TIM3_Init(u16 arr,u16 psc);

#endif
