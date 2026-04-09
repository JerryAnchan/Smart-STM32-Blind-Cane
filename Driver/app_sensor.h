/**
 * @file app_sensor.h
 * @brief 传感器数据处理接口（跌倒检测/测距/GPS）
 */
#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include "sys.h"

void FallDetection(void);
void Get_Distance(void);
void Get_GPS(void);

#endif
