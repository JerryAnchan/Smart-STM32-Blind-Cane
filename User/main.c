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
#include "gsm.h"  // 添加 gsm.h 头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f10x_iwdg.h" // 添加头文件
#include <math.h> // 添加此行

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
//u8 sendSmsFlag = 0;           // 发送短信标志

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
    // 设置手机号时显示数字，并高亮当前位
    if(settingMode >= 2) {
        for(i = 0; i < 11; i++) {
            if(i == (settingMode - 2)) {
                // 当前正在设置的位高亮显示（如反白或加下划线，具体看你的OLED库支持）
                OLED_ShowChar((add++)*8, 4, PhoneNumber[i], 2, 1); // 1为高亮
            } else {
                OLED_ShowChar((add++)*8, 4, PhoneNumber[i], 2, 0);
            }
        }
        return;
    }
    // 其它模式下的显示
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
void KeySettings(void)
{
    char i;
    // KEY1: 切换设置模式
    if(KEY1 == 0) {
        delay_ms(20);
        if(KEY1 == 0) {
            while(KEY1 == 0);
            settingMode++;
            if(settingMode > 12) {
                settingMode = 0;
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x40, (u16*)PhoneNumber, 11); // 保存手机号
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x60, &safetyDistance, 1); // 保存安全距离
                systemInitFlag = 1;
            }
            if(settingMode == 1) {
                OLED_CLS();
                for(i = 0; i < 6; i++) OLED_ShowCN(i*16+16, 0, i+30, 0); // 设置提醒距离
            }
            if(settingMode == 2) {
                for(i = 0; i < 8; i++) OLED_ShowCN(i*16, 0, i+11, 0); // 设置接收短信号码
            }
            DisplaySetValue();
        }
    }
    // KEY2: 增加
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
    // KEY3: 减少
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
    // KEY4: 一键求助
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
                    StartBeep(2);
                }
            }
        }
    }
    // KEY5: 取消求助
    if(KEY5 == 0) {
        delay_ms(20);
        if(KEY5 == 0) {
            while(KEY5 == 0);
            if(settingMode == 0) {
                if(emergencyMode == 1) {
                    emergencyMode = 0;
                    sendFlag &= 0xFD;
                    StopBeep(); // 立即停止蜂鸣器
                    OLED_ShowStr(54, 0, "SET:", 2);
                    SprintfIntNum(safetyDistance, (char *)displayBuffer);
                    OLED_ShowStr(87, 0, displayBuffer, 2);
                }
            }
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
    #define ACCEL_SAMPLE_COUNT 5
    #define FALL_ACCEL_THRESHOLD 190.0f   // 跌倒判定阈值（可根据实际调整）
    #define FALL_TIMER_INIT 8            // 跌倒计时初值

    float ax = 0, ay = 0, az = 0;
    u8 i;

    // 采集加速度平均值
    //OLED_ShowStr(0, 6, "       FD1", 1);
    adxl345_read_average(&ax, &ay, &az, ACCEL_SAMPLE_COUNT);
    //OLED_ShowStr(0, 6, "       FD2", 1);
    
    // OLED调试显示
    sprintf((char *)displayBuffer, "X:%5.1f", ax);
    OLED_ShowStr(64, 6, displayBuffer, 1);
    sprintf((char *)displayBuffer, "Y:%5.1f", ay);
    OLED_ShowStr(64, 7, displayBuffer, 1);
    OLED_ShowStr(0, 6, "Debug FD", 1);

    // 判断是否倾倒
    if (fabsf(ax) >= FALL_ACCEL_THRESHOLD || fabsf(ay) >= FALL_ACCEL_THRESHOLD) {
        tiltDetected = 1;
    } else {
        tiltDetected = 0;
        fallTimer = FALL_TIMER_INIT; // 恢复计时器
    }

    // 跌倒判定
    if (fallTimer == 0) {
        if (fallDetected == 0) {
            OLED_ShowStr(40, 0, "           ", 2);
            for (i = 0; i < 3; i++) OLED_ShowCN(i * 16 + 70, 0, i + 8, 0);
            fallDetected = 1;
            StartBeep(1);
            sendSmsFlag = 1; // 设置发送短信标志
        }
    } else {
        if (fallDetected == 1) {
            fallDetected = 0;
            StopBeep(); // 跌倒恢复时立即停止蜂鸣器
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
                delay_ms(200); // 延时
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
    char SEND_BUF[400];// 发送短信缓冲区
    char BUF1[50], BUF2[50]; // BUF1为经纬度转换前的字符串，BUF2为转换后的存储字符串
    uint16_t wait_count = 0; 
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
    
    // GSM初始化
    OLED_ShowStr(0,2,"   GSM Init...  ",2);
    gsm_init();

    // 等待GSM模块初始化完成
    wait_count = 0;
    gsm_rev_okflag = 0;
    while(gsm_rev_okflag == 0 && wait_count++ < 5000) {
        delay_ms(1);
    }
    gsm_rev_okflag = 0;

    /*
    WDG_WriteAccessCmd(IWDG_WriteAccess_Enable); // 允许访问IWDG
    IWDG_SetPrescaler(IWDG_Prescaler_64);         // 预分频64
    IWDG_SetReload(1562);                         // 约2秒超时(40kHz/64/1562 ≈ 1.0s，可根据需要调整)
    IWDG_ReloadCounter();                         // 喂一次狗
    IWDG_Enable();                                // 使能看门狗
    */
    // 主循环
    while(1) {
        //Uart1_SendStr("LoopA\r\n");
        // 处理按键输入
        KeySettings();
        
        //Uart1_SendStr("LoopB\r\n");
        // 显示主界面
        ShowHomePage();
        
        //Uart1_SendStr("LoopC\r\n");
        // 在非设置模式下更新传感器数据
        if(settingMode == 0) {
            if(refreshFlag == 1) {
                refreshFlag = 0;
                
                // 更新GPS数据
                Get_GPS();
                
                // 更新跌倒检测数据
                FallDetection();
            
                // 更新距离数据
                //Uart1_SendStr("GD_IN\r\n");
                Get_Distance();
                //Uart1_SendStr("GD_OUT\r\n");

                if(sendSmsFlag != 0) {
                    memset(SEND_BUF, 0, 400);    // 清空缓冲区

                    if(sendSmsFlag == 1) {
                        strcpy(SEND_BUF, "注意,检测到用户跌倒,经度:"); // 直接用UTF-8汉字
                    }
                    if(sendSmsFlag == 2) {
                        strcpy(SEND_BUF, "用户主动求救,需要紧急救援,经度:"); // 直接用UTF-8汉字
                    }

                    // 拼接经度
                    sprintf(BUF1, "%10.6f", GPS.longitude_Degree);
                    strcat(SEND_BUF, BUF1);

                    strcat(SEND_BUF, ",纬度:"); // 逗号用中文逗号

                    // 拼接纬度
                    sprintf(BUF2, "%10.6f", GPS.latitude_Degree);
                    strcat(SEND_BUF, BUF2);

                    sim800_send((unsigned char *)SEND_BUF); // 发送短信
                    memset(BUF1, 0, 50);      // 清空缓冲区
                    memset(BUF2, 0, 50);      // 清空缓冲区
                    sendSmsFlag = 0;
                }
            }
        }
        
        //IWDG_ReloadCounter(); // 喂狗，防止复位
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
