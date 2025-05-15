#include "sys.h"
#include "delay.h"
#include "gpio.h"
#include "OLED_I2C.h"
#include "HC_SR04.h"
#include "usart1.h"
#include "usart3.h"
#include "timer.h"
#include "iic.h"
#include "adxl345.h"
#include "wt588d.h"
#include "GPS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 闪存保存地址定义
#define FLASH_SAVE_ADDR  ((u32)0x0800F000)

// 串口接收缓冲区定义
#define STM32_RX3_BUF       Usart3RecBuf
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

// GPS信息结构和相关标志
#define GPS_STR_LEN 48
GPS_INFO GPS;
extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
u8 GPS_rx_flag = 0;
u8 gpsInitFlag = 0;

// 系统状态变量
u16 safetyDistance = 10;      // 安全距离阈值(cm)
float currentDistance = 1000;    // 100cm，避免一上电就报警
u8 distanceWarning = 0;       // 距离警告标志
u8 displayTwinkle = 0;        // 显示闪烁标志
u8 systemInitFlag = 1;        // 系统初始化标志
u8 settingMode = 0;           // 设置模式(0:正常,1:设置安全距离)
u8 refreshFlag = 0;           // 刷新显示标志
u8 playTimeCounter = 0;       // 播放时间计数器
unsigned char secondCounter = 0; // 秒计数器
unsigned char displayBuffer[16]; // 显示缓冲区
u8 tiltDetected = 0;          // 倾斜检测标志
u8 fallDetected = 0;          // 跌倒检测标志
u8 fallTimer = 10;            // 跌倒计时器
u8 sendFlag = 0x00;           // 发送标志
float accelX, accelY, accelZ; // 加速度计数据
float accelMagnitude, accelMagnitude2; // 加速度幅值
bool emergencyMode = 0;       // 紧急模式标志

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
 * 显示主界面
 */
void ShowHomePage(void) {
    char i;
    
    // 如果需要初始化界面
    if(systemInitFlag == 1) {
        systemInitFlag = 0;
        OLED_CLS(); // 清屏
        
        // 显示设置区域
        OLED_ShowStr(54, 0, "SET:", 2);
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(87, 0, displayBuffer, 2);
        
        // 显示固定文本内容
        for(i = 0; i < 2; i++) OLED_ShowCN(i*16, 2, i+26, 0);
        for(i = 0; i < 2; i++) OLED_ShowCN(i*16, 4, i+28, 0);
        OLED_ShowChar(32, 2, ':', 2, 0);
        OLED_ShowChar(32, 4, ':', 2, 0);
    }
}

/**
 * 显示设置值
 */
void DisplaySetValue(void) {
    u8 add = 2, i;
    
    // 在设置安全距离模式下显示星号
    if(settingMode >= 2) {
        for(i = 0; i < 11; i++) {
            OLED_ShowChar((add++)*8, 4, '*', 2, 0);
        }
    }
    
    // 根据当前设置模式更新显示
    if(settingMode == 0) {
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(87, 0, displayBuffer, 2);
    } else if(settingMode == 1) {
        SprintfIntNum(safetyDistance, (char *)displayBuffer);
        OLED_ShowStr(50, 4, displayBuffer, 2);
    }
}

/**
 * 按键设置处理
 */
void KeySettings(void) {
    char i;
    
    // KEY1: 切换设置模式
    if(KEY1 == 0) {
        delay_ms(20); // 消抖
        if(KEY1 == 0) {
            while(KEY1 == 0); // 等待按键释放
            settingMode++;
            
            // 超出设置模式范围则重置
            if(settingMode > 1) {
                settingMode = 0;
                // 保存安全距离到闪存
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
                systemInitFlag = 1; // 标记需要重新初始化界面
            }
            
            // 进入设置安全距离模式
            if(settingMode == 1) {
                OLED_CLS(); // 清屏
                for(i = 0; i < 6; i++) OLED_ShowCN(i*16+16, 0, i+30, 0);
            }
            
            DisplaySetValue(); // 更新显示
        }
    }
    
    // KEY2: 增加安全距离
    if(KEY2 == 0) {
        delay_ms(50); // 消抖
        if(KEY2 == 0 && settingMode == 1) {
            if(safetyDistance < 450) safetyDistance++;
            DisplaySetValue(); // 更新显示
        }
    }
    
    // KEY3: 减少安全距离
    if(KEY3 == 0) {
        delay_ms(50); // 消抖
        if(KEY3 == 0 && settingMode == 1) {
            if(safetyDistance > 0) safetyDistance--;
            DisplaySetValue(); // 更新显示
        }
    }
    
    // KEY4: 进入紧急模式
    if(KEY4 == 0) {
        delay_ms(20); // 消抖
        if(KEY4 == 0 && settingMode == 0) {
            while(KEY4 == 0); // 等待按键释放
            emergencyMode = 1;
            if(fallDetected == 0) playTimeCounter = 0;
            StartBeep(2); // 启动紧急模式蜂鸣
        }
    }
    
    // KEY5: 退出紧急模式
    if(KEY5 == 0) {
        delay_ms(20); // 消抖
        if(KEY5 == 0 && settingMode == 0) {
            while(KEY5 == 0); // 等待按键释放
            emergencyMode = 0;
            OLED_ShowStr(54, 0, "SET:", 2);
            SprintfIntNum(safetyDistance, (char *)displayBuffer);
            OLED_ShowStr(87, 0, displayBuffer, 2);
        }
    }
}

/**
 * 检查并初始化新MCU
 */
void CheckNewMcu(void) {
    u8 comper_str[6];
    
    // 读取闪存中的标识字符串
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x10, (u16*)comper_str, 5);
    comper_str[5] = '\0';
    
    // 如果标识字符串不存在，则初始化
    if(strstr((char *)comper_str, "FDYDZ") == NULL) {
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x10, (u16*)"FDYDZ", 5);
        delay_ms(50);
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
    }
    
    // 读取保存的安全距离
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1);
    if(safetyDistance > 400) safetyDistance = 30; // 安全距离超出范围则重置
    delay_ms(100);
}

/**
 * 跌倒检测处理
 */
void FallDetection(void) {
    u8 i;
    
    // 读取加速度计数据(平均10次采样)
    adxl345_read_average(&accelX, &accelY, &accelZ, 10);
    
    // 计算加速度幅值
    accelMagnitude = accelY;
    accelMagnitude2 = accelX;
    if(accelMagnitude < 0) accelMagnitude = -accelMagnitude;
    if(accelMagnitude2 < 0) accelMagnitude2 = -accelMagnitude2;
    
    // 检测倾斜状态
    if(((u16)accelMagnitude) >= 190 || ((u16)accelMagnitude2) >= 190) {
        tiltDetected = 1;
    } else {
        tiltDetected = 0;
        fallTimer = 10; // 重置跌倒计时器
    }
    
    // 在OLED上显示加速度计数据(调试信息)
    sprintf((char *)displayBuffer, "X:%5.1f", accelX);
    OLED_ShowStr(64, 6, displayBuffer, 1);
    sprintf((char *)displayBuffer, "Y:%5.1f", accelY);
    OLED_ShowStr(64, 7, displayBuffer, 1);
    OLED_ShowStr(0, 6, "Debug FD", 1);
    
    // 处理跌倒状态
    if(fallTimer == 0) {
        if(fallDetected == 0) {
            // 显示跌倒警告
            OLED_ShowStr(40, 0, "           ", 2);
            for(i = 0; i < 3; i++) OLED_ShowCN(i * 16 + 70, 0, i + 8, 0);
            fallDetected = 1;
            StartBeep(1); // 启动跌倒模式蜂鸣
        }
    } else {
        if(fallDetected == 1) {
            fallDetected = 0;
            if(emergencyMode == 1) {
                // 显示紧急模式信息
                for(i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0);
            } else {
                // 恢复正常显示
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(safetyDistance, (char *)displayBuffer);
                OLED_ShowStr(87, 0, displayBuffer, 2);
            }
        }
    }
}

/**
 * 获取并处理距离数据
 */
void Get_Distance(void) {
    u8 i;
    currentDistance = (Get_SR04_Distance() * 331) * 1.0 / 1000; // 获取距离并转换为毫米

    // 1. 检查超声波异常值
    if (currentDistance == 0xFFFF) {
        OLED_ShowStr(0, 6, "SR04 Error", 1);
        delay_ms(500);
        return;
    }

    if (currentDistance >= 4500) currentDistance = 4500; // 限制最大距离
    SprintfIntNum((u16)currentDistance / 10, (char *)displayBuffer);
    OLED_ShowStr(0, 0, displayBuffer, 2);
    OLED_ShowStr(0, 6, "Debug GD", 1);

    // 2. 处理距离警告
    if (emergencyMode == 0) {
        if (currentDistance / 10 <= safetyDistance) {
            if (distanceWarning == 0) {
                distanceWarning = 1;
                if (fallDetected == 0) playTimeCounter = 0;
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 2, 0);
                StartBeep(3); // 启动蜂鸣
                delay_ms(2000); // 延时
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(safetyDistance, (char *)displayBuffer);
                OLED_ShowStr(87, 0, displayBuffer, 2);
            }
        } else {
            distanceWarning = 0;
        }
    } else {
        for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0);
    }
}

/**
 * 获取并处理GPS数据
 */
void Get_GPS(void) {
    static u8 errorNum = 0;
    static u8 timeCount = 0;
    
    // 每5个周期处理一次GPS数据
    if(rev_stop == 1 && timeCount++ >= 5) {
        if(GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) {
            // GPS解析成功
            errorNum = 0;
            gps_flag = 0;
            rev_stop = 0;
            gpsInitFlag = 1;
        } else {
            // GPS解析失败
            if(errorNum++ >= 30) {
                errorNum = 30;
                gpsInitFlag = 0;
                OLED_ShowStr(0, 2, "GPS ERR", 2);
            }
            gps_flag = 0;
            rev_stop = 0;
        }
        timeCount = 0;
    }
    
    // 显示GPS经纬度信息
    sprintf((char *)displayBuffer, "%10.6f ", GPS.longitude_Degree);
    OLED_ShowStr(40, 2, (u8*)displayBuffer, 2);
    sprintf((char *)displayBuffer, "%10.6f ", GPS.latitude_Degree);
    OLED_ShowStr(40, 4, (u8*)displayBuffer, 2);
}

/**
 * 主函数
 */
int main(void) {
    // 系统初始化
    delay_init();
    NVIC_Configuration();
    delay_ms(200);
    I2C_Configuration();
    GPS_rx_flag = 0;
    
    // 检查并初始化MCU
    CheckNewMcu();
    
    // 外设初始化
    BEEP_GPIO_Init();
    KEY_GPIO_Init();
    IIC_init();
    adxl345_init();
    HC_SR04_IO_Init();
    LED_GPIO_Init();
    WT588D_GPIO_INIT();
    
    // OLED初始化
    OLED_Init();
    OLED_CLS();
    
    // 串口初始化
    uart1_Init(9600);
    USART3_Init(9600);
    OLED_CLS();
    UsartRx1BufClear();
    GPS_rx_flag = 1;
    
    // 定时器初始化
    TIM2_Init(500-1, 7199);  // 10ms中断
    TIM3_Init(7199, 0);      // 用于其他定时功能
    
    // 主循环
    while(1) {
        OLED_ShowStr(0, 7, "Loop A", 1);
        
        // 处理按键输入
        KeySettings();
        
        OLED_ShowStr(0, 7, "Loop B", 1);
        
        // 显示主界面
        ShowHomePage();
        
        // 在非设置模式下更新传感器数据
        if(settingMode == 0) {
            if(refreshFlag == 1) {
                refreshFlag = 0;
                
                OLED_ShowStr(0, 7, "Loop C", 1);
                // 更新GPS数据
                Get_GPS();
                
                OLED_ShowStr(0, 7, "Loop D", 1);
                // 更新跌倒检测数据
                FallDetection();
                
                OLED_ShowStr(0, 7, "Loop E", 1);
                // 更新距离数据
                Get_Distance();
                
                OLED_ShowStr(0, 7, "Loop F", 1);
            }
        }
        
        delay_ms(1); // 短暂延时，防止CPU占用过高
    }
}

/**
 * 定时器2中断处理函数
 * 10ms中断一次，用于系统定时任务
 */
void TIM2_IRQHandler(void) {
    static u8 time_count1s = 0;    // 1秒计数器
    static unsigned int timeCount = 0; // 时间计数器
    
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        LED = GM; // 更新LED状态
        
        // 更新蜂鸣器状态
        BeepUpdate();
        
        // 每100ms(10*10ms)设置刷新标志
        if(timeCount++ >= 10) {
            timeCount = 0;
            refreshFlag = 1;
        }
        
        // 每1秒(20*50ms)执行一次
        if(time_count1s++ >= 20) {
            time_count1s = 0;
            
            // 处理跌倒检测计时
            if(tiltDetected && fallTimer > 0) fallTimer--;
            
            // 更新秒计数器
            if(secondCounter > 0) secondCounter--;
        }
    }
}
