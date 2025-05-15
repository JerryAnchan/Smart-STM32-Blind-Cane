/* wt588d.c 文件内容 */
#include "wt588d.h"
#include "delay.h"

// 蜂鸣器状态机相关变量
typedef enum {
    BEEP_IDLE,       // 空闲状态
    BEEP_RINGING,    // 响状态
    BEEP_SILENT      // 静音状态
} BEEP_STATE;

BEEP_STATE beepState = BEEP_IDLE;  // 当前状态
unsigned char beepMode = 0;        // 蜂鸣器模式
unsigned char beepCount = 0;       // 当前响铃次数
unsigned char beepMaxCount = 0;    // 最大响铃次数
unsigned int beepOnTime = 0;       // 响持续时间(ms)
unsigned int beepOffTime = 0;      // 静音持续时间(ms)
unsigned int beepTimer = 0;        // 计时计数器

void WT588D_GPIO_INIT(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // 打开 GPIOC 时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;             // PC13
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_SetBits(GPIOC, GPIO_Pin_13); // 默认高电平
}

/**
 * 启动蜂鸣器
 * @param mode 蜂鸣器模式(0-4)
 */
void StartBeep(unsigned char mode)
{
    if(beepState != BEEP_IDLE) return; // 如果蜂鸣器正在工作，不启动新的
    
    beepMode = mode;
    beepCount = 0;
    
    // 根据不同模式设置参数
    switch(mode) {
        case 0: // 不响
            return;
            
        case 1: // 快速短响 8 次 fall
            beepMaxCount = 8;
            beepOnTime = 150;
            beepOffTime = 150;
            break;
            
        case 2: // emergency
            beepMaxCount = 4;
            beepOnTime = 500;
            beepOffTime = 50;
            break;
            
        case 3: // 慢响 5 次 
            beepMaxCount = 5;
            beepOnTime = 1;
            beepOffTime = 1;
            break;
            
        case 4: // 紧急报警响 5 次
            beepMaxCount = 8;
            beepOnTime = 15;
            beepOffTime = 15;
            break;
            
        default:
            return;
    }
    
    // 开始蜂鸣
    beepState = BEEP_RINGING;
    BEEP_OUT = 1;
    beepTimer = beepOnTime;
}

/**
 * 蜂鸣器状态机更新函数
 * 应在定时器中断中定期调用(如1ms)
 */
void BeepUpdate(void)
{
    if(beepState == BEEP_IDLE) return;
    
    if(beepTimer > 0) {
        beepTimer--;
        return;
    }
    
    switch(beepState) {
        case BEEP_RINGING:
            // 切换到静音状态
            BEEP_OUT = 0;
            beepState = BEEP_SILENT;
            beepTimer = beepOffTime;
            break;
            
        case BEEP_SILENT:
            // 增加计数
            beepCount++;
            
            if(beepCount >= beepMaxCount) {
                // 完成所有响铃，回到空闲状态
                beepState = BEEP_IDLE;
            } else {
                // 继续下一次响铃
                BEEP_OUT = 1;
                beepState = BEEP_RINGING;
                beepTimer = beepOnTime;
            }
            break;
            
        default:
            beepState = BEEP_IDLE;
            break;
    }
}
