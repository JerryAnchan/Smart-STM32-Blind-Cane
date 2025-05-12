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

#define FLASH_SAVE_ADDR  ((u32)0x0800F000)

#define STM32_RX3_BUF       Usart3RecBuf
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

#define GPS_STR_LEN 48
GPS_INFO GPS;

extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
u8 GPS_rx_flag = 0;
u8 gpsInitFlag = 0;

u16 SAFET_Distance = 30;
float Distance = 0;
u8 distanceFlag = 0;
u8 Twinkle = 0;
u8 InitFlag = 1;
u8 setn = 0;
u8 shuaxin = 0;
u8 play_time = 0;
unsigned char miao = 0;
unsigned char display[16];
u8 tiltFlag = 0;
u8 fall = 0;
u8 fallTime = 10;
u8 SendFlag = 0x00;
float adx, ady, adz;
float acc, acc2;
bool Emergency = 0;

void UsartRx1BufClear(void) {
    memset(Usart1RecBuf, 0, USART1_RXBUFF_SIZE);
    RxCounter = 0;
}

float ChangeFloatData(int dat) {
    return (float)(dat)/10;
}

void SprintfIntNum(u16 data,char *str) {
    sprintf((char *)str,"%dcm  ",data);
    if(data>9)sprintf((char *)str,"%dcm ",data);
    if(data>99)sprintf((char *)str,"%dcm",data);
}

void ShowHomePage(void) {
    char i;
    if(InitFlag==1) {
        InitFlag = 0;
        OLED_CLS();
        OLED_ShowStr(54,0,"SET:",2);
        SprintfIntNum(SAFET_Distance,(char *)display);
        OLED_ShowStr(87,0,display,2);
        for(i=0;i<2;i++) OLED_ShowCN(i*16,2,i+26,0);
        for(i=0;i<2;i++) OLED_ShowCN(i*16,4,i+28,0);
        OLED_ShowChar(32,2,':',2,0);
        OLED_ShowChar(32,4,':',2,0);
    }
}

void DisplaySetValue(void) {
    u8 add=2,i;
    if(setn>=2) {
        for(i=0;i<11;i++) {
            OLED_ShowChar((add++)*8,4,'*',2,0);
        }
    }
    if(setn==0) {
        SprintfIntNum(SAFET_Distance,(char *)display);
        OLED_ShowStr(87,0,display,2);
    }
    if(setn==1) {
        SprintfIntNum(SAFET_Distance,(char *)display);
        OLED_ShowStr(50,4,display,2);
    }
}

void KeySettings(void) {
    char i;
    if(KEY1==0) {
        delay_ms(20);
        if(KEY1==0) {
            while(KEY1==0);
            setn++;
            if(setn > 1) {
                setn = 0;
                STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1);
                InitFlag=1;
            }
            if(setn==1) {
                OLED_CLS();
                for(i=0;i<6;i++) OLED_ShowCN(i*16+16,0,i+30,0);
            }
            DisplaySetValue();
        }
    }
    if(KEY2==0) {
        delay_ms(50);
        if(KEY2==0 && setn==1) {
            if(SAFET_Distance<450) SAFET_Distance++;
            DisplaySetValue();
        }
    }
    if(KEY3==0) {
        delay_ms(50);
        if(KEY3==0 && setn==1) {
            if(SAFET_Distance>0) SAFET_Distance--;
            DisplaySetValue();
        }
    }
    if(KEY4==0) {
        delay_ms(20);
        if(KEY4==0 && setn==0) {
            while(KEY4==0);
            Emergency = 1;
            if(fall==0) play_time = 0;
        }
    }
    if(KEY5==0) {
        delay_ms(20);
        if(KEY5==0 && setn==0) {
            while(KEY5==0);
            Emergency = 0;
            OLED_ShowStr(54,0,"SET:",2);
            SprintfIntNum(SAFET_Distance,(char *)display);
            OLED_ShowStr(87,0,display,2);
        }
    }
}

void CheckNewMcu(void) {
    u8 comper_str[6];
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
    comper_str[5] = '\0';
    if(strstr((char *)comper_str,"FDYDZ") == NULL) {
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"FDYDZ",5);
        delay_ms(50);
        STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1);
    }
    STM32F10x_Read(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1);
    if(SAFET_Distance>400) SAFET_Distance=30;
    delay_ms(100);
}

void FallDetection(void) {
    u8 i;
    adxl345_read_average(&adx, &ady, &adz, 10); // 读取加速度计数据
    acc = ady;
    acc2 = adx;
    if (acc < 0) acc = -acc;
    if (acc2 < 0) acc2 = -acc2;
    if (((u16)acc) >= 190 || ((u16)acc2) >= 190) tiltFlag = 1;
    else {
        tiltFlag = 0;
        fallTime = 10;
    }

    // 在 OLED 上显示加速度计数据
    sprintf((char *)display, "X:%5.1f", adx);
    OLED_ShowStr(64, 6, display, 1);
    sprintf((char *)display, "Y:%5.1f", ady);
    OLED_ShowStr(64, 7, display, 1);

    if (fallTime == 0) {
        if (fall == 0) {
            OLED_ShowStr(40, 0, "           ", 2);
            for (i = 0; i < 3; i++) OLED_ShowCN(i * 16 + 70, 0, i + 8, 0);
            play_time = 0;
            fall = 1;
        }
    } else {
        if (fall == 1) {
            fall = 0;
            if (Emergency == 1) {
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0);
            } else {
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(SAFET_Distance, (char *)display);
                OLED_ShowStr(87, 0, display, 2);
            }
        }
    }
}

void Get_Distance(void) {
    u8 i;
    Distance = (Get_SR04_Distance() * 331) * 1.0 / 1000; // 获取距离并转换为毫米
    if (Distance == 0xFFFF) { // 检测到错误值
        OLED_ShowStr(0, 6, "SR04 Error", 1); // 显示错误信息
        delay_ms(500); // 延时避免频繁刷新
        return; // 跳过后续逻辑
    }
    if (Distance >= 4500) Distance = 4500; // 限制最大距离
    SprintfIntNum((u16)Distance / 10, (char *)display); // 格式化距离值
    OLED_ShowStr(0, 0, display, 2); // 显示距离

    if (Emergency == 0) { // 非紧急状态
        if (Distance / 10 <= SAFET_Distance) { // 距离小于安全距离
            if (distanceFlag == 0) {
                distanceFlag = 1;
                if (fall == 0) play_time = 0;
                for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 2, 0); // 显示警告
                delay_ms(2000); // 延时
                OLED_ShowStr(54, 0, "SET:", 2);
                SprintfIntNum(SAFET_Distance, (char *)display);
                OLED_ShowStr(87, 0, display, 2);
            }
        } else {
            distanceFlag = 0; // 重置标志
        }
    } else { // 紧急状态
        for (i = 0; i < 4; i++) OLED_ShowCN(i * 16 + 54, 0, i + 36, 0); // 显示紧急状态
    }
}

void Get_GPS(void) {
    static u8 errorNum=0;
    static u8 timeCount=0;
    if (rev_stop == 1 && timeCount++>=5) {
        if (GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) {
            errorNum = 0;
            gps_flag = 0;
            rev_stop  = 0;
            gpsInitFlag=1;
        } else {
            if (errorNum++ >= 30) {
                errorNum = 30;
                gpsInitFlag = 0;
                OLED_ShowStr(50, 3, "GPS ERR", 2);
            }
            gps_flag = 0;
            rev_stop  = 0;
        }
        timeCount=0;
    }
    sprintf((char *)display,"%10.6f ",GPS.longitude_Degree);
    OLED_ShowStr(40, 2, (u8*)display, 2);
    sprintf((char *)display,"%10.6f ",GPS.latitude_Degree);
    OLED_ShowStr(40, 4, (u8*)display, 2);
}

int main(void) {
    delay_init();
    NVIC_Configuration();
    delay_ms(200);
    I2C_Configuration();
    GPS_rx_flag = 0;
    CheckNewMcu();
    BEEP_GPIO_Init();
    KEY_GPIO_Init();
    IIC_init();
    adxl345_init();
    HC_SR04_IO_Init();
    LED_GPIO_Init();
    WT588D_GPIO_INIT();
    OLED_Init();
    OLED_CLS();
    uart1_Init(9600);
    USART3_Init(9600);
    OLED_CLS();
    UsartRx1BufClear();
    GPS_rx_flag = 1;
    TIM2_Init(500-1,7199);
    TIM3_Init(7199,0);

    while (1) {
        KeySettings();
        ShowHomePage();
        if (setn == 0) {
            if (shuaxin == 1) {
                shuaxin = 0;
                Get_GPS();
                Get_Distance();
                FallDetection();
            }
        }
        delay_ms(5);
    }
}

void TIM2_IRQHandler(void) {
    static u8 time_count1s=0;
    static u8 play_flag = 0;
    static unsigned int timeCount=0;
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        LED=GM;

        if(timeCount++>=10) {
            timeCount = 0;
            shuaxin=1;
        }
        if(time_count1s++ >= 20) {
            time_count1s = 0;
            if(tiltFlag && fallTime>0) fallTime--;
            if(miao > 0) miao--;
            play_flag = 0;
            if(distanceFlag==1) play_flag = 3;
            if(Emergency==1) play_flag = 2;
            if(fall==1) play_flag = 1;
            if(play_flag!=0) {
                if(play_time==0) Line_1A(play_flag);
                if(play_time++>=5) play_time=0;
            } else {
                play_time=0;
            }
        }
    }
}
