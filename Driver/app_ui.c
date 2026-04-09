/**
 * @file app_ui.c
 * @brief 界面显示与按键设置实现
 */
#include "app_ui.h"
#include "app_utils.h"
#include "OLED_I2C.h"
#include "gpio.h"
#include "delay.h"
#include "stmflash.h"
#include "wt588d.h"
#include "gsm.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* main.c 中定义的全局变量 */
extern u16 safetyDistance;
extern u8 systemInitFlag;
extern u8 settingMode;
extern unsigned char displayBuffer[16];
extern u8 fallDetected;
extern u8 playTimeCounter;
extern bool emergencyMode;
extern u8 sendFlag;
extern u8 alarmEnable;
extern u8 distanceWarning;

/**
 * 将整数格式化为距离字符串(带单位cm)
 * @param data 距离数据(cm)
 * @param str 输出字符串缓冲区
 */
void SprintfIntNum(u16 data, char *str) {
    if(data > 99) {
        sprintf(str, "%dcm", data);
    } else if(data > 9) {
        sprintf(str, "%dcm ", data);
    } else {
        sprintf(str, "%dcm  ", data);
    }
}

/**
 * 显示主界面（距离 + 经纬度 + SET阈值）
 * @note 仅在systemInitFlag==1时重绘全屏，调用后自动清除该标志
 */
void ShowHomePage(void) {
    char i;
    
    if(systemInitFlag == 1) {
        systemInitFlag = 0;
        OLED_CLS();
        
        OLED_ShowStr(54, 0, "SET:", 2);
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(87, 0, displayBuffer, 2);
        
        for(i = 0; i < 2; i++) OLED_ShowCN(i*16, 2, i+26, 0);
        for(i = 0; i < 2; i++) OLED_ShowCN(i*16, 4, i+28, 0);
        OLED_ShowChar(32, 2, ':', 2, 0);
        OLED_ShowChar(32, 4, ':', 2, 0);
    }
}

/**
 * 显示当前设置值（根据settingMode状态机切换显示内容）
 * settingMode: 0=显示SET阈值, 1=显示设置中阈值, 2~12=显示手机号并高亮当前位
 */
void DisplaySetValue(void) {
    u8 add = 2, i;
    if(settingMode >= 2) {
        for(i = 0; i < 11; i++) {
            if(i == (settingMode - 2)) {
                OLED_ShowChar((add++)*8, 4, PhoneNumber[i], 2, 1);
            } else {
                OLED_ShowChar((add++)*8, 4, PhoneNumber[i], 2, 0);
            }
        }
        return;
    }
    if(settingMode == 0) {
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(87, 0, displayBuffer, 2);
    } else if(settingMode == 1) {
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(50, 4, displayBuffer, 2);
    }
}

/**
 * 按键设置处理（状态机驱动）
 *
 * KEY1: 切换设置模式 (0→阈值→手机号11位→保存并返回0)
 * KEY2: 增加当前设置项
 * KEY3: 减小当前设置项
 * KEY4: 一键求助（触发emergencyMode并发送短信）
 * KEY5: 取消求助
 */
void KeySettings(void)
{
    char i;
    if(KEY1 == 0) {
        delay_ms(20);
        if(KEY1 == 0) {
            while(KEY1 == 0);
            settingMode++;
            if(settingMode > 12) {
                settingMode = 0;
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x40, (u16*)PhoneNumber, 11);
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
                systemInitFlag = 1;
            }
            if(settingMode == 1) {
                OLED_CLS();
                for(i = 0; i < 6; i++) OLED_ShowCN(i*16+16, 0, i+30, 0);
            }
            if(settingMode == 2) {
                for(i = 0; i < 8; i++) OLED_ShowCN(i*16, 0, i+11, 0);
            }
            DisplaySetValue();
        }
    }
    if(KEY2 == 0) {
        if(settingMode != 0) delay_ms(80);
        else delay_ms(50);
        if(KEY2 == 0) {
            if(settingMode == 1) {
                if(safetyDistance < 450) safetyDistance++;
                DisplaySetValue();
            }
            if(settingMode >= 2) {
                PhoneNumber[settingMode-2]++;
                if(PhoneNumber[settingMode-2] > '9') PhoneNumber[settingMode-2] = '0';
                DisplaySetValue();
            }
        }
    }
    if(KEY3 == 0) {
        if(settingMode != 0) delay_ms(80);
        else delay_ms(50);
        if(KEY3 == 0) {
            if(settingMode == 1) {
                if(safetyDistance > 0) safetyDistance--;
                DisplaySetValue();
            }
            if(settingMode >= 2) {
                PhoneNumber[settingMode-2]--;
                if(PhoneNumber[settingMode-2] < '0') PhoneNumber[settingMode-2] = '9';
                DisplaySetValue();
            }
        }
    }
    if(KEY4 == 0) {
        delay_ms(20);
        if(KEY4 == 0) {
            while(KEY4 == 0);
            if(settingMode == 0) {
                if(emergencyMode == 0) {
                    if(!(sendFlag & 0x02)) {
                        sendFlag |= 0x02;
                        sendSmsFlag = 2;
                    }
                    if(fallDetected == 0) playTimeCounter = 0;
                    emergencyMode = 1;
                    if(alarmEnable) StartBeep(2);
                }
            }
        }
    }
    if(KEY5 == 0) {
        delay_ms(20);
        if(KEY5 == 0) {
            while(KEY5 == 0);
            if(settingMode == 0) {
                if(emergencyMode == 1) {
                    emergencyMode = 0;
                    sendFlag &= 0xFD;
                    StopBeep();
                    OLED_ShowStr(54, 0, "SET:", 2);
                    SprintfIntNum(safetyDistance, (char *)displayBuffer);
                    OLED_ShowStr(87, 0, displayBuffer, 2);
                }
            }
        }
    }
}
