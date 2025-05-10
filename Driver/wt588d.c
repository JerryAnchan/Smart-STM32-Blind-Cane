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

        case 1: // 快速短响 3 次 fall
            for(i = 0; i < 3; i++) {
                BEEP_OUT = 1;
                delay_ms(100);
                BEEP_OUT = 0;
                delay_ms(100);
            }
            break;

        case 2: // 长响 1 次 emergency
            BEEP_OUT = 1;
            delay_ms(500);
            BEEP_OUT = 0;
            break;

        case 3: // 慢响 2 次 
            for(i = 0; i < 2; i++) {
                BEEP_OUT = 1;
                delay_ms(300);
                BEEP_OUT = 0;
                delay_ms(300);
            }
            break;

        case 4: // 紧急报警响 5 次
            for(i = 0; i < 5; i++) {
                BEEP_OUT = 1;
                delay_ms(200);
                BEEP_OUT = 0;
                delay_ms(150);
            }
            break;

        default: break;
    }
}
