#ifndef __WT588D_H
#define __WT588D_H

#include "sys.h"
#include "delay.h"

// 将蜂鸣器连接到 PC13 引脚
#define BEEP_OUT  PCout(13)

void WT588D_GPIO_INIT(void);
void Line_1A(unsigned char mode);

#endif
