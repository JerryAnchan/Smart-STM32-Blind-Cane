/**
 * @file wt588d.c
 * @brief 蜂鸣器驱动（软件PWM状态机控制）
 *
 * 引脚: PC13 推挽输出
 * 模式参数表:
 *   mode 0: 不响
 *   mode 1: 快速短响×8 (150ms响/150ms停) — 跌倒报警
 *   mode 2: 长响×4    (500ms响/50ms停)  — 紧急求助
 *   mode 3: 单响×5    (1ms响/1ms停)    — 距离警告
 *   mode 4: 急促短响×8 (15ms响/15ms停)  — 保留
 *   mode 5: 单响×2    (1ms响/1ms停)    — 水位报警
 * 调用: BeepUpdate()应在定时中断中以≥1ms周期调用
 */
#include "wt588d.h"
#include "delay.h"
#include "sys.h"
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

/**
 * @brief 初始化蜂鸣器GPIO（PC13推挽输出，默认高电平关闭）
 */
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
        case 5: // 紧急报警响 5 次
            beepMaxCount = 2;
            beepOnTime = 1;
            beepOffTime = 1;
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

/**
 * @brief 停止蜂鸣器，立即置低并重置状态机
 */
void StopBeep(void) {
    beepState = BEEP_IDLE;
    BEEP_OUT = 0;
}
