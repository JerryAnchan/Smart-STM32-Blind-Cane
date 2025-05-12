#include "wt588d.h"

void WT588D_GPIO_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 打开 GPIOA 时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;             // PA0
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    BEEP_OUT = 0; // 初始为低电平（不响）
}

void Line_1A(unsigned char mode)
{
    unsigned char i;

    switch(mode) {
        case 0: break;

        case 1: // 快速短响 8 次 fall
            for(i = 0; i < 8; i++) {
                BEEP_OUT = 1;
                delay_ms(150);
                BEEP_OUT = 0;
                delay_ms(150);
            }
            break;

        case 2: // emergency
            for(i = 0; i < 4; i++) {
                BEEP_OUT = 1;
                delay_ms(500);
                BEEP_OUT = 0;
                delay_ms(50);
            }
            break;

        case 3: // 慢响 2 次 
            for(i = 0; i < 5; i++) {
                BEEP_OUT = 1;
                delay_ms(60);
                BEEP_OUT = 0;
                delay_ms(60);
            }
            break;

        case 4: // 紧急报警响 5 次
            for(i = 0; i < 5; i++) {
                BEEP_OUT = 1;
                delay_ms(150);
                BEEP_OUT = 0;
                delay_ms(150);
            }
            break;

        default: break;
    }
}
