/**
 * @file app_utils.c
 * @brief 系统辅助工具实现（闪存初始化/串口清空/数据转换）
 */
#include "app_utils.h"
#include "stmflash.h"
#include "usart1.h"
#include "delay.h"
#include "stm32f10x_flash.h"
#include <string.h>

/* main.c 中定义的全局变量 */
extern u16 safetyDistance;

/**
 * 清空串口1接收缓冲区
 */
void UsartRx1BufClear(void) {
    memset(Usart1RecBuf, 0, USART1_RXBUFF_SIZE);
    RxCounter = 0;
}

/**
 * 将整数转换为浮点数(除以10)
 * @param dat 输入整数
 * @return 转换后的浮点数
 */
float ChangeFloatData(int dat) {
    return (float)(dat)/10;
}

/**
 * 检查并初始化新MCU（首次上电写入标识串"FDYDZ"+默认安全距离）
 * @note 去闪存+0x10读标识，不存在则为首次上电。
 *       安全距离超过400cm(VL53L1X量程上限4500mm/10)则重置为30cm
 */
void CheckNewMcu(void) {
    u8 comper_str[6];
    
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x10, (u16*)comper_str, 5);
    comper_str[5] = '\0';
    
    if(strstr((char *)comper_str, "FDYDZ") == NULL) {
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x10, (u16*)"FDYDZ", 5);
        delay_ms(50);
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
    }
    
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
    if(safetyDistance > 400) safetyDistance = 30;
    delay_ms(100);
}
