/**
 * @file gpio.h
 * @brief 系统GPIO引脚宏定义（蜂鸣器/按键/LED/光敏/水位）
 */
#ifndef __GPIO_H
#define __GPIO_H	 
#include "sys.h"

#define BEEP_OUT PCout(13)	// 蜂鸣器输出（PC13）

#define KEY1  PBin(12)	// 模式切换键
#define KEY2  PBin(13)	// 加/增键
#define KEY3  PBin(14)	// 减/降键
#define KEY4  PBin(15)	// 一键求助
#define KEY5  PAin(8)	// 取消求助

#define LED  PAout(0)	// LED指示灯输出
#define GM   PAin(1)	// 光敏传感器输入（高=暗）
#define WATER  PBin(9)	// 水位传感器输入（0=浸水）

void BEEP_GPIO_Init(void);
void KEY_GPIO_Init(void);
void LED_GPIO_Init(void);
	 				    
#endif
