/**
 * @file VL53L1x_STM32.h
 * @brief VL53L1X接口适配STM32
 * 使用PB4(SDA)和PB5(SCL)
 */

#ifndef _VL53L1X_STM32_H_
#define _VL53L1X_STM32_H_

#include "sys.h"
#include "stm32f10x.h"

// VL53L1X I2C地址(默认0x52, 7位地址0x29)
#define VL53L1X_I2C_ADDR  0x52

// 函数声明
void VL53L1X_I2C_Init(void);
uint8_t VL53L1X_Init(void);
uint16_t VL53L1X_GetDistance(void);
uint8_t VL53L1X_DataReady(void);
uint8_t VL53L1X_I2C_Test(void); // I2C通信测试
uint8_t VL53L1X_I2C_Scan(uint8_t *found_addr); // I2C地址扫描
uint8_t VL53L1X_SimpleTest(void); // 简化测试
uint8_t VL53L1X_ReadID(void); // 读取设备ID
uint8_t VL53L1X_TestRegAccess(uint8_t *accessible_regs); // 测试寄存器访问
uint8_t VL53L1X_CheckBootState(void); // 检查启动状态
uint8_t VL53L1X_StartRanging(void); // 启动测距模式
uint8_t VL53L1X_ReadStatus(uint8_t *status_0x0006, uint8_t *status_0x0031); // 读取状态寄存器

// 寄存器读写函数（用于直接访问）
uint8_t VL53L1X_WriteReg16(uint16_t reg, uint8_t data);
uint8_t VL53L1X_ReadReg16(uint16_t reg, uint8_t *data);
uint16_t VL53L1X_ReadReg16_16bit(uint16_t reg);

#endif /* _VL53L1X_STM32_H_ */
