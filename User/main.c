/**
 * @file main.c
 * @brief 系统主控逻辑（测距 + 跌倒检测 + GPS/LBS定位 + GSM短信告警）
 *
 * 目标平台: STM32F103 + VL53L1X + ADXL345 + GPS + SIM800C
 * 外设概览:
 *   VL53L1X  — 激光测距 (I2C, PB4/PB5)
 *   ADXL345  — 三轴加速度计/跌倒检测 (I2C, PC14/PC15)
 *   GPS      — 定位 (USART3, 9600bps)
 *   SIM800C  — GSM短信+基站定位 (USART1, 9600bps)
 *   OLED     — 128x64显示屏 (I2C, PB6/PB7)
 *   蜂鸣器  — 报警音 (PC13)
 *   按键   — 5键操作 (PB12~15, PA8)
 */
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
#include "app_ui.h"
#include "app_sensor.h"
#include "app_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
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
u8 sendFlag = 0x00;           // 发送标志位域: bit1(0x02)=手动求救已发
float accelX, accelY, accelZ; // 加速度计数据
float accelMagnitude, accelMagnitude2; // 加速度幅值
bool emergencyMode = 0;       // 紧急模式标志
u8 alarmEnable = 0;           // 报警使能(0=初始化静音,1=允许蜂鸣)

// 基站定位回退坐标（GPS未锁定时使用）
double lbs_longitude = 0;  // 基站经度(WGS84度)
double lbs_latitude = 0;   // 基站纬度(WGS84度)
u8 lbs_valid = 0;              // 基站坐标是否有效(0=未获取, 1=有效)

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
    
    /*
     * ===== 上电自检：I2C通信 =====
     * I2C总线测试 → 设备ID读取 → 启动状态检查
     * 任一步骤失败则锁死(while(1))并在OLED展示接线提示
     */
    OLED_CLS();
    OLED_ShowStr(0, 0, "I2C Test...", 2);
    delay_ms(100);
    
    {
        uint8_t test_result;
        unsigned char test_buf[20];
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
            
            while(1) delay_ms(100);
        }
        
        // I2C测试通过，读取实际ID
        OLED_ShowStr(0, 0, "I2C Test OK!", 2);
        delay_ms(100);
        
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
        
        delay_ms(150);
    }
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "VL53L1X Test", 2);
    delay_ms(100);
    
    /*
     * VL53L1X初始化：最多3次重试，每次间隔500ms
     * 失败则锁死并显示错误码 + 接线提示(SDA→PB4, SCL→PB5)
     */
    {
        uint8_t init_result = 1;
        uint8_t try_count;
        unsigned char error_buf[20];
        
        OLED_CLS();
        OLED_ShowStr(0, 0, "Initializing", 2);
        OLED_ShowStr(0, 2, "Please wait...", 1);
        delay_ms(100);
        
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
            
            while(1) delay_ms(100); // 停止运行
        }
    }
    
    OLED_CLS();
    OLED_ShowStr(0, 0, "VL53L1X OK!", 2);
    OLED_ShowStr(0, 2, "Start Range..", 1);
    delay_ms(300);
    
    /*
     * 启动测距：最多5次重试 + 状态寄存器(0x06/0x31)诊断
     * 启动失败为非致命错误，继续运行（可能后续自恢复）
     */
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
    // TIM2: (499+1)*(7199+1)/72MHz = 50ms中断周期
    TIM2_Init(500-1, 7199);
    TIM3_Init(7199, 0);      // TIM3: 72MHz无分频，用于超声波回波计时
    
    // GSM初始化
    OLED_ShowStr(0,2,"   GSM Init...  ",2);
    gsm_init();
    gsm_rev_okflag = 1;
    // 等待GSM模块初始化完成
    wait_count = 0;
    gsm_rev_okflag = 0;
    while(gsm_rev_okflag == 0 && wait_count++ < 5000) { // 5000×1ms=5秒超时上限
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

                /*
                 * 组装并发送求救短信
                 * 坐标来源三级回退: GPS坐标 → 基站坐标(标记"基站") → "未知"
                 */
                if(sendSmsFlag != 0) {
                    memset(SEND_BUF, 0, 400);
                    if(sendSmsFlag == 1) {
                        strcpy(SEND_BUF, "注意,检测到用户跌倒,经度:");
                    }
                    if(sendSmsFlag == 2) {
                        strcpy(SEND_BUF, "用户主动求救,需要紧急救援,经度:");
                    }
                    Serial_SendByte(0x35); // ASR Pro: "正在发送求救信号，请耐心等待"
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
 * TIM2中断处理函数（10ms节拍）
 *
 * 职责:
 * - LED状态同步光敏传感器
 * - 蜂鸣器状态机更新(BeepUpdate)
 * - 水位报警检测
 * - 20ms(2×10ms)置位refreshFlag触发主循环任务
 * - 200ms(20×10ms)跌倒倒计时递减
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
        else if(WATER == 0) 
        {
            StartBeep(5);
        }

        if(timeCount++ >= 2)  // 2×10ms=20ms刷新周期
        {
            timeCount = 0;
            refreshFlag = 1;
        }
        if(time_count1s++ >= 20) { // 20×10ms=200ms，跌倒倒计时精度
            time_count1s = 0;
            if(tiltDetected && fallTimer > 0) fallTimer--;
            if(secondCounter > 0) secondCounter--;
        }
    }
}

/**
 * USART2中断处理（语音模块串口协议）
 *
 * 协议: 单字节指令
 *   0x31 = 触发求救短信
 *   0x32 = 请求回传当前距离(cm)
 */
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
