#ifndef __WT588D_H
#define __WT588D_H	 
#include "sys.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 

void WT588D_GPIO_INIT(void);
void Line_1A(unsigned char dat);
#define  P_DATA  PCout(13)
	 				    
#endif

