/**
 * @file VL53L1X_Test.c
 * @brief VL53L1X传感器测试程序示例
 * 
 * 此文件提供一个简单的测试函数，可用于调试VL53L1X传感器
 */

#include "VL53L1x_STM32.h"
#include "delay.h"
#include "OLED_I2C.h"
#include <stdio.h>

/**
 * @brief VL53L1X基础功能测试
 * @details 调用此函数进行传感器连接测试
 */
void VL53L1X_BasicTest(void)
{
    char buf[32];
    uint16_t distance;
    uint8_t test_count = 0;
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "VL53L1X Test", 2);
    
    // 1. 初始化I2C
    OLED_ShowStr(0, 2, "Init I2C...", 1);
    VL53L1X_I2C_Init();
    delay_ms(100);
    OLED_ShowStr(0, 2, "Init I2C...OK  ", 1);
    
    // 2. 初始化传感器
    OLED_ShowStr(0, 3, "Init Sensor...", 1);
    if(VL53L1X_Init() != 0) {
        OLED_ShowStr(0, 4, "FAILED!", 2);
        OLED_ShowStr(0, 6, "Check:", 1);
        OLED_ShowStr(0, 7, "1.Wiring", 1);
        return;
    }
    OLED_ShowStr(0, 3, "Init Sensor...OK", 1);
    delay_ms(500);
    
    // 3. 连续测量10次
    OLED_CLS();
    OLED_ShowStr(0, 0, "Measuring...", 2);
    
    while(test_count < 10) {
        distance = VL53L1X_GetDistance();
        
        if(distance == 0xFFFF) {
            OLED_ShowStr(0, 2, "Error:0xFFFF", 1);
        } else {
            sprintf(buf, "D:%4dmm  ", distance);
            OLED_ShowStr(0, 2, buf, 2);
            
            sprintf(buf, "Test:%d/10", test_count + 1);
            OLED_ShowStr(0, 4, buf, 1);
        }
        
        test_count++;
        delay_ms(200);
    }
    
    OLED_ShowStr(0, 6, "Test Complete!", 1);
}

/**
 * @brief VL53L1X高级测试
 * @details 测试传感器在不同距离下的性能
 */
void VL53L1X_AdvancedTest(void)
{
    char buf[32];
    uint16_t distance;
    uint16_t min_dist = 9999, max_dist = 0;
    uint32_t sum_dist = 0;
    uint8_t valid_count = 0;
    uint8_t i;
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "Advanced Test", 2);
    OLED_ShowStr(0, 2, "Sampling...", 1);
    
    // 采样20次
    for(i = 0; i < 20; i++) {
        distance = VL53L1X_GetDistance();
        
        if(distance != 0xFFFF) {
            sum_dist += distance;
            valid_count++;
            
            if(distance < min_dist) min_dist = distance;
            if(distance > max_dist) max_dist = distance;
            
            sprintf(buf, "Sample:%d/20", i + 1);
            OLED_ShowStr(0, 3, buf, 1);
        }
        delay_ms(100);
    }
    
    // 显示统计结果
    OLED_CLS();
    OLED_ShowStr(0, 0, "Statistics:", 2);
    
    if(valid_count > 0) {
        sprintf(buf, "Avg:%dmm", (uint16_t)(sum_dist / valid_count));
        OLED_ShowStr(0, 2, buf, 1);
        
        sprintf(buf, "Min:%dmm", min_dist);
        OLED_ShowStr(0, 3, buf, 1);
        
        sprintf(buf, "Max:%dmm", max_dist);
        OLED_ShowStr(0, 4, buf, 1);
        
        sprintf(buf, "Valid:%d/20", valid_count);
        OLED_ShowStr(0, 5, buf, 1);
        
        sprintf(buf, "Range:%dmm", max_dist - min_dist);
        OLED_ShowStr(0, 6, buf, 1);
    } else {
        OLED_ShowStr(0, 2, "No Valid Data!", 2);
    }
    
    delay_ms(5000);
}

/**
 * @brief I2C通信测试
 * @details 测试I2C通信是否正常
 */
void VL53L1X_I2C_Test(void)
{
    uint8_t model_id = 0;
    uint8_t ret;
    char buf[32];
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "I2C Test", 2);
    
    VL53L1X_I2C_Init();
    delay_ms(100);
    
    // 尝试读取设备ID
    OLED_ShowStr(0, 2, "Read ID...", 1);
    
    // VL53L1X的Model ID位于0x010F
    // 应该读到0xEA或0xEB
    extern uint8_t VL53L1X_ReadReg16(uint16_t reg, uint8_t *data);
    ret = VL53L1X_ReadReg16(0x010F, &model_id);
    
    if(ret == 0) {
        sprintf(buf, "ID:0x%02X", model_id);
        OLED_ShowStr(0, 3, buf, 1);
        
        if(model_id == 0xEA || model_id == 0xEB) {
            OLED_ShowStr(0, 4, "ID Check: OK!", 1);
        } else {
            OLED_ShowStr(0, 4, "ID Wrong!", 1);
        }
    } else {
        OLED_ShowStr(0, 3, "Read Failed!", 1);
        OLED_ShowStr(0, 4, "I2C Error!", 1);
    }
    
    delay_ms(3000);
}

/**
 * @brief 如何在main.c中使用测试函数
 * 
 * 在main函数的适当位置添加以下代码:
 * 
 * // 基础测试
 * VL53L1X_BasicTest();
 * 
 * // 或者高级测试
 * VL53L1X_AdvancedTest();
 * 
 * // 或者I2C测试
 * VL53L1X_I2C_Test();
 */
