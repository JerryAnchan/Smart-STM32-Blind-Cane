#ifndef __GPIO_H
#define __GPIO_H	 
#include "sys.h"

#define BEEP  PCout(13)	

#define KEY1  PBin(12)	
#define KEY2  PBin(13)	
#define KEY3  PBin(14)	
#define KEY4  PBin(15)	
#define KEY5  PAin(8)	

#define LED  PAout(0)	
#define GM   PAin(1)	
#define WATER  PBin(9)	

void BEEP_GPIO_Init(void);
void KEY_GPIO_Init(void);
void LED_GPIO_Init(void);
	 				    
#endif
