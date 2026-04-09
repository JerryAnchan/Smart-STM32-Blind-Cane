/**
 * @file HC_SR04.h
 * @brief HC-SR04超声波测距模块接口
 */
#ifndef __HC_SR04_H
#define __HC_SR04_H	 
#include "sys.h"

#define SR04_Trlg PBout(4) // 触发引脚（PB4, 输出≥10μs高电平启动测量）
#define SR04_Echo PBin(5)  // 回波引脚（PB5, 高电平时长=声波往返时间）

void HC_SR04_IO_Init(void);
u16  Get_SR04_Distance(void);



	 				    
#endif
