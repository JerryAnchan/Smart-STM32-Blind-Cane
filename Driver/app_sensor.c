/**
 * @file app_sensor.c
 * @brief 传感器数据处理实现（跌倒检测/测距/GPS）
 */
#include "app_sensor.h"
#include "app_ui.h"
#include "OLED_I2C.h"
#include "VL53L1x_STM32.h"
#include "adxl345.h"
#include "GPS.h"
#include "wt588d.h"
#include "usart2.h"
#include "usart3.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* main.c 中定义的全局变量 */
extern float currentDistance;
extern u16 safetyDistance;
extern u8 distanceWarning;
extern u8 systemInitFlag;
extern u8 settingMode;
extern unsigned char displayBuffer[16];
extern u8 tiltDetected;
extern u8 fallDetected;
extern u8 fallTimer;
extern u8 playTimeCounter;
extern bool emergencyMode;
extern u8 alarmEnable;
extern GPS_INFO GPS;
extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
extern u8 gpsInitFlag;
extern double lbs_longitude;
extern double lbs_latitude;
extern u8 lbs_valid;
extern uint8_t sendSmsFlag;

#define STM32_RX3_BUF       Usart3RecBuf
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

/**
 * 跌倒检测处理（基于ADXL345加速度幅值判定）
 *
 * 策略: 采集5次取平均，任意轴加速度超过190 LSB(±16g模式下约760mg)
 *        则判为倾斜，持续倾斜2秒后确认跌倒
 * 跌倒确认后: 蜂鸣报警 + 设置发短信标志
 */
void FallDetection(void) {
    #define ACCEL_SAMPLE_COUNT 5
    #define FALL_ACCEL_THRESHOLD 190.0f
    #define FALL_TIMER_INIT 2

    float ax = 0, ay = 0, az = 0;
    u8 i;

    adxl345_read_average(&ax, &ay, &az, ACCEL_SAMPLE_COUNT);
    
    if (fabsf(ax) >= FALL_ACCEL_THRESHOLD || fabsf(ay) >= FALL_ACCEL_THRESHOLD) {
        tiltDetected = 1;
    } else {
        tiltDetected = 0;
        fallTimer = FALL_TIMER_INIT;
    }
    if (fallTimer == 0) {
        if (fallDetected == 0) {
            OLED_ShowStr(40, 0, "           ", 2);
            for (i = 0; i < 3; i++) OLED_ShowCN(i * 16 + 70, 0, i + 8, 0);
            fallDetected = 1;
            Serial_SendByte(0x34);
            if(alarmEnable) StartBeep(1);
            sendSmsFlag = 1;
        }
    } else {
        if (fallDetected == 1) {
            fallDetected = 0;
            StopBeep();
            if (emergencyMode == 1) {
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0);
            } else {
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(safetyDistance, (char *)displayBuffer);
                OLED_ShowStr(87, 0, displayBuffer, 2);
            }
        }
    }
}

/**
 * 获取并处理VL53L1X测距数据
 *
 * 返回值含义:
 *   0xFFFF = I2C通信错误，使用上次有效值，连续50次错误触发重初始化
 *   0      = 数据未准备好，直接返回不更新显示
 *   其他  = 有效距离(mm)，限幅至4500mm（VL53L1X量程上限）
 */
void Get_Distance(void) {
    u8 i;
    uint16_t dist_mm;
    static uint16_t last_valid_distance = 500;
    static uint8_t error_count = 0;
    static uint8_t wait_count = 0;

    dist_mm = VL53L1X_GetDistance();

    if (dist_mm == 0xFFFF) {
        error_count++;
        if(error_count > 10) {
            if(error_count > 50) {
                VL53L1X_Init();
                error_count = 0;
            }
        }
        dist_mm = last_valid_distance;
    } 
    else if (dist_mm == 0) {
        wait_count++;
        error_count = 0;
        
        if(wait_count > 150) {
            wait_count = 0;
            VL53L1X_WriteReg16(0x0087, 0x00);
            delay_ms(10);
            VL53L1X_WriteReg16(0x0087, 0x40);
        }
        
        return;
    } 
    else {
        error_count = 0;
        wait_count = 0;
        last_valid_distance = dist_mm;
    }
    
    currentDistance = dist_mm;

    if (currentDistance >= 4500) currentDistance = 4500;
    SprintfIntNum((u16)currentDistance / 10, (char *)displayBuffer);
    OLED_ShowStr(0, 0, displayBuffer, 2);

    if (emergencyMode == 0) {
        if (currentDistance <= ((float)safetyDistance * 10.0f)) {
            if (distanceWarning == 0) {
                distanceWarning = 1;
                if (fallDetected == 0) playTimeCounter = 0;
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 2, 0);
                Serial_SendByte(0x33);
                if(alarmEnable) StartBeep(3);
                delay_ms(200);
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(safetyDistance, (char *)displayBuffer);
                OLED_ShowStr(87, 0, displayBuffer, 2);
                delay_ms(200);
            }
        } else {
            distanceWarning = 0;
        }
    } else {
        for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0);
    }
}

/**
 * 获取并处理GPS数据（每5个周期解析一次，降低解析开销）
 *
 * 显示策略: GPS锁定→正常坐标, LBS有效→基站坐标+"*", 否则→"No Fix"
 */
void Get_GPS(void) {
    static u8 errorNum = 0;
    static u8 timeCount = 0;
    
    if(rev_stop == 1 && timeCount++ >= 5) {
        if(GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) {
            errorNum = 0;
            gps_flag = 0;
            rev_stop = 0;
            gpsInitFlag = 1;
        } else {
            if(errorNum++ >= 30) {
                errorNum = 30;
                gpsInitFlag = 0;
            }
            gps_flag = 0;
            rev_stop = 0;
        }
        timeCount = 0;
    }
    
    if(gpsInitFlag) {
        sprintf((char *)displayBuffer, "%10.6f ", GPS.longitude_Degree);
        OLED_ShowStr(40, 2, (u8*)displayBuffer, 2);
        sprintf((char *)displayBuffer, "%10.6f ", GPS.latitude_Degree);
        OLED_ShowStr(40, 4, (u8*)displayBuffer, 2);
    } else if(lbs_valid) {
        sprintf((char *)displayBuffer, "%10.6f*", lbs_longitude);
        OLED_ShowStr(40, 2, (u8*)displayBuffer, 2);
        sprintf((char *)displayBuffer, "%10.6f*", lbs_latitude);
        OLED_ShowStr(40, 4, (u8*)displayBuffer, 2);
    } else {
        OLED_ShowStr(40, 2, "  No Fix  ", 2);
        OLED_ShowStr(40, 4, "  No Fix  ", 2);
    }
}
