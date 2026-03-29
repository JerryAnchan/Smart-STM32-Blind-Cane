#include "sys.h"
#include "delay.h"
#include "gpio.h"
#include "OLED_I2C.h"
#include "VL53L1x_STM32.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "timer.h"
#include "iic.h"
#include "adxl345.h"
#include "wt588d.h"
#include "GPS.h"
#include "gsm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h> 

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
float currentDistance = 10000;    // 以0.1cm为单位保存，初值1000cm避免上电误报
u16 safetyDistance = 10;          // 安全距离阈值(cm)
u8 distanceWarning = 0;           // 距离预警状态位
u8 displayTwinkle = 0;            // 显示闪烁标志（预留）
u8 systemInitFlag = 1;            // 首页重绘请求标志
u8 settingMode = 0;               // 设置模式(0:正常,1:阈值,2~12:手机号各位)
u8 refreshFlag = 0;               // 周期任务触发标志，由定时中断置位
u8 playTimeCounter = 0;           // 播放时间计数器
unsigned char secondCounter = 0; // 秒计数器
unsigned char displayBuffer[16]; // 显示缓冲区
u8 tiltDetected = 0;          // 倾斜检测标志
u8 fallDetected = 0;          // 跌倒检测标志
u8 fallTimer = 10;            // 跌倒计时器
u8 sendFlag = 0x00;           // 发送标志
float accelX, accelY, accelZ; // 加速度计数据
float accelMagnitude, accelMagnitude2; // 加速度幅值
bool emergencyMode = 0;       // 紧急模式标志
u8 alarmEnable = 0;           // 报警使能(0=初始化静音,1=允许蜂鸣)

// 基站定位回退坐标（GPS未锁定时使用）
double lbs_longitude = 0;
double lbs_latitude = 0;
u8 lbs_valid = 0;

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
                    if(alarmEnable) StartBeep(2);
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
    #define FALL_TIMER_INIT 2            // 跌倒计时初值

    float ax = 0, ay = 0, az = 0;
    u8 i;

    // 采集加速度平均值
    adxl345_read_average(&ax, &ay, &az, ACCEL_SAMPLE_COUNT);
    

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
            if(alarmEnable) StartBeep(1);
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
    uint16_t dist_mm;
    static uint16_t last_valid_distance = 500;
    static uint8_t error_count = 0;
    static uint8_t wait_count = 0;

    dist_mm = VL53L1X_GetDistance();

    // 检查VL53L1X返回值
    if (dist_mm == 0xFFFF) {
        // I2C通信错误：暂用上次有效值并累计错误计数
        error_count++;
        if(error_count > 10) {
            // 连续错误后执行重初始化，降低死锁概率
            if(error_count > 50) {
                VL53L1X_Init();
                error_count = 0;
            }
        }
        dist_mm = last_valid_distance; // 使用上次值
    } 
    else if (dist_mm == 0) {
        // 数据未准备好：只等待，不改写显示值
        wait_count++;
        error_count = 0;
        
        if(wait_count > 150) {
            wait_count = 0;
            // 长时间未出新数据则重启测距状态机
            VL53L1X_WriteReg16(0x0087, 0x00);
            delay_ms(10);
            VL53L1X_WriteReg16(0x0087, 0x40);
        }
        
        return; // 直接返回，不更新距离
    } 
    else {
        // 有效数据（含0mm）
        error_count = 0;
        wait_count = 0;
        last_valid_distance = dist_mm;
    }
    
    // currentDistance使用0.1cm单位保存，1mm 正好等于 0.1cm
    currentDistance = dist_mm;

    if (currentDistance >= 4500) currentDistance = 4500; // 限制最大距离
    SprintfIntNum((u16)currentDistance / 10, (char *)displayBuffer);
    OLED_ShowStr(0, 0, displayBuffer, 2);

    // 处理距离警告
    if (emergencyMode == 0) {
        if (currentDistance <= ((float)safetyDistance * 10.0f)) {
            if (distanceWarning == 0) {
                distanceWarning = 1;
                if (fallDetected == 0) playTimeCounter = 0;
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 2, 0);
                if(alarmEnable) StartBeep(3); // 启动蜂鸣
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
    
    // 优先显示GPS坐标，未锁定时回退到基站定位坐标
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
    LED_GPIO_Init();
    WT588D_GPIO_INIT();
    
    // OLED初始化(必须先初始化，才能显示错误信息)
    OLED_Init();
    OLED_CLS();
    
    // 初始化VL53L1X传感器
    VL53L1X_I2C_Init();
    
    // I2C通信测试
    OLED_CLS();
    OLED_ShowStr(0, 0, "I2C Test...", 2);
    delay_ms(300);
    
    {
        uint8_t test_result;
        char test_buf[20];
        uint8_t found_addr = 0;
        uint8_t device_count;
        uint8_t simple_result;
        uint8_t device_id;
        uint8_t accessible_regs = 0;
        uint8_t reg_count;
        uint8_t boot_state;
        
        test_result = VL53L1X_I2C_Test();
        
        if(test_result != 0) {
            OLED_CLS();
            OLED_ShowStr(0, 0, "I2C Test Fail", 2);
            sprintf(test_buf, "Err:%d", test_result);
            OLED_ShowStr(0, 2, test_buf, 2);
            
            // 尝试扫描I2C总线
            device_count = VL53L1X_I2C_Scan(&found_addr);
            sprintf(test_buf, "Dev:0x%02X", found_addr);
            OLED_ShowStr(0, 3, test_buf, 1);
            
            // 测试寄存器访问
            reg_count = VL53L1X_TestRegAccess(&accessible_regs);
            sprintf(test_buf, "Reg:0x%02X(%d)", accessible_regs, reg_count);
            OLED_ShowStr(0, 4, test_buf, 1);
            
            // 简
            simple_result = VL53L1X_SimpleTest();
            sprintf(test_buf, "Step:%d/6", simple_result);
            OLED_ShowStr(0, 5, test_buf, 1);
            
            if(accessible_regs == 0x01) {
                // 只有0x00可访问,检查启动状态
                boot_state = VL53L1X_CheckBootState();
                sprintf(test_buf, "Boot:0x%02X", boot_state);
                OLED_ShowStr(0, 6, test_buf, 1);
                
                if(boot_state == 0x01) {
                    OLED_ShowStr(0, 7, "Boot OK!", 1);
                } else if(boot_state == 0xFF) {
                    OLED_ShowStr(0, 7, "Boot Rd Fail", 1);
                } else {
                    OLED_ShowStr(0, 7, "Booting...", 1);
                }
            } else if(accessible_regs > 0x01) {
                OLED_ShowStr(0, 6, "Multi-reg OK", 1);
            }
            
            while(1) delay_ms(1000);
        }
        
        // I2C测试通过，读取实际ID
        OLED_ShowStr(0, 0, "I2C Test OK!", 2);
        delay_ms(300);
        
        device_id = VL53L1X_ReadID();
        sprintf(test_buf, "ID:0x%02X", device_id);
        OLED_ShowStr(0, 2, test_buf, 2);
        
        if(device_id == 0xEA || device_id == 0xEB) {
            OLED_ShowStr(0, 4, "VL53L1X OK!", 1);
        } else if(device_id == 0xCC) {
            OLED_ShowStr(0, 4, "VL53L1 (old)", 1);
        } else {
            OLED_ShowStr(0, 4, "Unknown ID!", 1);
            OLED_ShowStr(0, 5, "Try anyway..", 1);
        }
        
        delay_ms(1500);
    }
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "VL53L1X Test", 2);
    delay_ms(500);
    
    // 多次尝试初始化
    {
        uint8_t init_result = 1;
        uint8_t try_count;
        char error_buf[20];
        
        OLED_CLS();
        OLED_ShowStr(0, 0, "Initializing", 2);
        OLED_ShowStr(0, 2, "Please wait...", 1);
        delay_ms(300);
        
        for(try_count = 0; try_count < 3; try_count++) {
            init_result = VL53L1X_Init();
            if(init_result == 0) break;
            
            // 显示尝试次数
            OLED_CLS();
            sprintf(error_buf, "Try %d/3...", try_count + 1);
            OLED_ShowStr(0, 0, error_buf, 2);
            sprintf(error_buf, "Err:%d", init_result);
            OLED_ShowStr(0, 2, error_buf, 1);
            delay_ms(500);
        }
        
        if(init_result != 0) {
            OLED_CLS();
            OLED_ShowStr(0, 0, "INIT FAIL!", 2);
            
            // 显示错误代码
            sprintf(error_buf, "ErrCode:%d", init_result);
            OLED_ShowStr(0, 2, error_buf, 1);
            
            if(init_result == 1) {
                OLED_ShowStr(0, 3, "I2C Fail", 1);
                OLED_ShowStr(0, 4, "Check:", 1);
                OLED_ShowStr(0, 5, "SDA->PB4", 1);
                OLED_ShowStr(0, 6, "SCL->PB5", 1);
                OLED_ShowStr(0, 7, "Pullup 4.7K", 1);
            } else if(init_result == 2) {
                OLED_ShowStr(0, 3, "Boot Timeout", 1);
                OLED_ShowStr(0, 4, "Check:", 1);
                OLED_ShowStr(0, 5, "VDD Power", 1);
                OLED_ShowStr(0, 6, "XSHUT->VCC", 1);
                OLED_ShowStr(0, 7, "Try Reset", 1);
            }
            
            while(1) delay_ms(1000); // 停止运行
        }
    }
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "VL53L1X OK!", 2);
    OLED_ShowStr(0, 2, "Start Range..", 1);
    delay_ms(300);
    
    // 再次确保测距启动
    {
        uint8_t start_result;
        uint8_t retry;
        char msg_buf[20];
        uint8_t stat_06, stat_31;
        
        for(retry = 0; retry < 5; retry++) {
            start_result = VL53L1X_StartRanging();
            if(start_result == 0) {
                OLED_ShowStr(0, 3, "Range Started!", 1);
                break;
            }
            delay_ms(100);
        }
        
        if(start_result != 0) {
            OLED_ShowStr(0, 3, "Range Fail!", 1);
            sprintf(msg_buf, "Try:%d", retry);
            OLED_ShowStr(0, 4, msg_buf, 1);
        }
        
        // 显示状态寄存器值用于诊断
        VL53L1X_ReadStatus(&stat_06, &stat_31);
        sprintf(msg_buf, "S06:%02X S31:%02X", stat_06, stat_31);
        OLED_ShowStr(0, 5, msg_buf, 1);
        
        delay_ms(1000);
    }
    
    // OLED已在前面完成初始化，这里只清屏
    OLED_CLS();
    
    // 串口初始化
    uart1_Init(9600);
    USART2_Init();
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
    gsm_rev_okflag = 1;
    // 等待GSM模块初始化完成
    wait_count = 0;
    gsm_rev_okflag = 0;
    while(gsm_rev_okflag == 0 && wait_count++ < 5000) {
        delay_ms(1);
    }
    gsm_rev_okflag = 0;

    // 开启基站定位并获取初始坐标，作为GPS未锁定时的回退
    OLED_ShowStr(0, 2, " LBS Init...    ", 2);
    gsm_lbs_init();
    if(gsm_get_lbs(&lbs_longitude, &lbs_latitude)) {
        lbs_valid = 1;
    }
    OLED_CLS();

    /* 初始化阶段结束，解除静音门控 */
    alarmEnable = 1;


    // 主循环
    while(1) {
        // 处理按键输入
        KeySettings();
        // 显示主界面
        ShowHomePage();
        // 在非设置模式下更新传感器数据
        if(settingMode == 0) {
            if(refreshFlag == 1) {
                refreshFlag = 0;
                // 更新GPS数据
                Get_GPS();
                // 更新跌倒检测数据
                FallDetection();
                // 更新距离数据
                Get_Distance();
                // GPS未锁定且基站定位无效时，周期性重试基站查询
                if(gpsInitFlag == 0 && lbs_valid == 0) {
                    static u16 lbs_retry_counter = 0;
                    if(lbs_retry_counter++ >= 1500) { // 约30秒(1500×20ms)
                        lbs_retry_counter = 0;
                        if(gsm_get_lbs(&lbs_longitude, &lbs_latitude)) {
                            lbs_valid = 1;
                        }
                    }
                }

                if(sendSmsFlag != 0) {
                    memset(SEND_BUF, 0, 400);
                    if(sendSmsFlag == 1) {
                        strcpy(SEND_BUF, "注意,检测到用户跌倒,经度:");
                    }
                    if(sendSmsFlag == 2) {
                        strcpy(SEND_BUF, "用户主动求救,需要紧急救援,经度:");
                    }
                    // 优先使用GPS坐标，未锁定时回退到基站坐标
                    if(gpsInitFlag) {
                        sprintf(BUF1, "%10.6f", GPS.longitude_Degree);
                        strcat(SEND_BUF, BUF1);
                        strcat(SEND_BUF, ",纬度:");
                        sprintf(BUF2, "%10.6f", GPS.latitude_Degree);
                        strcat(SEND_BUF, BUF2);
                    } else if(lbs_valid) {
                        sprintf(BUF1, "%10.6f", lbs_longitude);
                        strcat(SEND_BUF, BUF1);
                        strcat(SEND_BUF, "(基站),纬度:");
                        sprintf(BUF2, "%10.6f", lbs_latitude);
                        strcat(SEND_BUF, BUF2);
                        strcat(SEND_BUF, "(基站)");
                    } else {
                        strcat(SEND_BUF, "未知,纬度:未知");
                    }
                    sim800_send((unsigned char *)SEND_BUF);
                    memset(BUF1, 0, 50);
                    memset(BUF2, 0, 50);
                    sendSmsFlag = 0;
                }
            }
        }

    }
}

/**
 * 定时器2中断处理函数
 * 10ms中断一次，用于系统定时任务
 */
void TIM2_IRQHandler(void) {
    static u8 time_count1s = 0;
    static unsigned int timeCount = 0;
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        LED = GM;
        BeepUpdate();
        if(!alarmEnable) {
            StopBeep();
        }
        else if(WATER == 1) 
        {
            StartBeep(5);
        }

        if(timeCount++ >= 2)  // 改为20ms刷新一次（原来100ms）
        {
            timeCount = 0;
            refreshFlag = 1;
        }
        if(time_count1s++ >= 20) {
            time_count1s = 0;
            if(tiltDetected && fallTimer > 0) fallTimer--;
            if(secondCounter > 0) secondCounter--;
        }
    }
}

void USART2_IRQHandler(void)
{
    u8 com_data;
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        USART_ClearFlag(USART2, USART_FLAG_RXNE);
        com_data = USART2->DR;

        // 语音模块串口协议：0x31=触发求救短信，0x32=回传当前距离(cm)
        if(com_data == 0x31)
        {
            sendSmsFlag = 2;
        }

        if(com_data == 0x32)
        {
            char distStr[32];
            sprintf(distStr, "%d", (int)(currentDistance / 10));
            Usart2_SendString(distStr);
        }
    }
}
