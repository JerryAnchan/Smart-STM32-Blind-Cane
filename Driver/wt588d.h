/* wt588d.h 文件内容 */
#ifndef __WT588D_H
#define __WT588D_H

#include "stm32f10x.h"

// 蜂鸣器IO口定义
#define BEEP_OUT    PCout(13)

void WT588D_GPIO_INIT(void);
void StartBeep(unsigned char mode);
void BeepUpdate(void);
void StopBeep(void);
#endif
