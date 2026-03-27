/**
 * @file VL53L1X_Debug.c
 * @brief VL53L1X调试辅助函数
 */

#include "VL53L1x_STM32.h"
#include "OLED_I2C.h"
#include "delay.h"
#include <stdio.h>

// 声明内部函数（需要在VL53L1x_STM32.c中移除static）
extern uint8_t VL53L1X_ReadReg16(uint16_t reg, uint8_t *data);
extern uint8_t VL53L1X_WriteReg16(uint16_t reg, uint8_t data);

/**
 * @brief 详细的I2C通信测试
 */
void VL53L1X_DetailedTest(void)
{
    uint8_t model_id = 0;
    uint8_t result;
    char buf[32];
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "I2C Test", 2);
    delay_ms(1000);
    
    // 测试1: 尝试读取设备ID
    OLED_CLS();
    OLED_ShowStr(0, 0, "Read ID...", 2);
    delay_ms(500);
    
    result = VL53L1X_ReadReg16(0x010F, &model_id);
    
    OLED_CLS();
    if(result == 0) {
        OLED_ShowStr(0, 0, "I2C: OK", 2);
        sprintf(buf, "ID: 0x%02X", model_id);
        OLED_ShowStr(0, 2, buf, 1);
        
        if(model_id == 0xEA || model_id == 0xEB) {
            OLED_ShowStr(0, 3, "ID Check: PASS", 1);
        } else {
            OLED_ShowStr(0, 3, "ID Wrong!", 1);
            OLED_ShowStr(0, 4, "Expect:0xEA", 1);
        }
    } else {
        OLED_ShowStr(0, 0, "I2C: FAIL!", 2);
        OLED_ShowStr(0, 2, "No Response", 1);
        OLED_ShowStr(0, 4, "Check:", 1);
        OLED_ShowStr(0, 5, "Power&Wire", 1);
    }
    
    delay_ms(3000);
}

/**
 * @brief 在main.c中调用此函数测试
 * 
 * 在main函数中VL53L1X_Init()之前添加：
 * VL53L1X_DetailedTest();
 */
