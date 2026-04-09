/**
 * @file app_utils.h
 * @brief 系统辅助工具接口（闪存初始化/串口清空/数据转换）
 */
#ifndef __APP_UTILS_H
#define __APP_UTILS_H

#include "sys.h"

/* 闪存保存地址定义
 * 布局: +0x10 设备标识串"FDYDZ"(5字节)
 *       +0x40 手机号(11字节)
 *       +0x60 安全距离阈值(1半字) */
#define FLASH_SAVE_ADDR  ((u32)0x0800F000)

void UsartRx1BufClear(void);
float ChangeFloatData(int dat);
void CheckNewMcu(void);

#endif
