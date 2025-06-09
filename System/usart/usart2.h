#ifndef __USART2_H
#define __USART2_H

#include <stdio.h>
#include "stm32f10x.h"


#define USART2_MAX_RECV_LEN		200			
#define USART2_MAX_SEND_LEN		200			

extern u8  USART2_RX_BUF[USART2_MAX_RECV_LEN]; 	
extern u8  USART2_TX_BUF[USART2_MAX_SEND_LEN]; 		
extern u16 USART2_RX_STA;
extern int flag;

void USART2_Init(void);
void USART2_IRQHandler(void);
void Serial_SendByte(uint8_t Byte);
void Usart2_SendString(char *str);
int fputc(int ch,FILE *f);	

void Usart_SendString(USART_TypeDef* pUSARTx, char *str);
#endif
