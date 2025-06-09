#include "stm32f10x.h"

u16 flag_send=0;
u8 com_data;
u16 USART2_RX_STA=0;
int flag;

void USART2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStrue;
    USART_InitTypeDef USART2_InitStrue;
    NVIC_InitTypeDef NVIC_InitStrue;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // PA2/PA3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // PA2: TX
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStrue.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStrue);

    // PA3: RX
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStrue.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStrue);

    USART2_InitStrue.USART_BaudRate = 9600;
    USART2_InitStrue.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART2_InitStrue.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
    USART2_InitStrue.USART_Parity = USART_Parity_No;
    USART2_InitStrue.USART_StopBits = USART_StopBits_1;
    USART2_InitStrue.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &USART2_InitStrue);

    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    NVIC_InitStrue.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStrue.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStrue.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStrue);
}

void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART2, Byte);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}
void Usart2_SendString(char *str)
{
    while(*str)
    {
        Serial_SendByte(*str++);
    }
}
