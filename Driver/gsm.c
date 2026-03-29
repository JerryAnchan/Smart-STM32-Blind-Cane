#include "gsm.h"
#include "usart1.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char PhoneNumber[PHONE_NUMBER_LEN] = "18275967713";
uint8_t sendSmsFlag = 0;

/* 将UTF-8原始字节转为十六进制ASCII串，供DTU短信配置指令使用 */
void utf8_to_hexstr(const char* utf8, char* hexstr)
{
    while (*utf8) {
        sprintf(hexstr, "%02X", (unsigned char)*utf8);
        hexstr += 2;
        utf8++;
    }
    *hexstr = '\0';
}

/* 初始化短信功能：等待模块回包OK后再退出 */
void gsm_init(void)
{
    unsigned short waittry;
    gsm_rev_start = 0;
    gsm_rev_okflag = 0;
    do {
        waittry = 0;
        uart1_send((unsigned char *)"config,set,smson,1,0,0,0,0,1,1\r\n", 0xFF);
        while(waittry++ < 3000)
        {
            if(gsm_rev_okflag == 1)
            {
                break;
            }
            delay_ms(1);
        }
    } while(gsm_rev_okflag == 0);
}

static char hex_content[512] = {0};
static char cmd[600] = {0};

/* 发送短信：内容按模块要求使用16进制字符串 */
void gsm_send_msg(const char* number, const char* content)
{
    unsigned short waittry;
    utf8_to_hexstr(content, hex_content);
    snprintf(cmd, sizeof(cmd), "config,set,sms,%s,%s\r\n", PhoneNumber, hex_content);

    gsm_rev_start = 0;
    gsm_rev_okflag = 0;
    waittry = 0;
    uart1_send((unsigned char *)cmd, 0xFF);
    while(waittry++ < 3000)
    {
        if(gsm_rev_okflag == 1)
        {
            break;
        }
        delay_ms(1);
    }
}

/* 兼容主程序历史接口，内部带超时重试 */
void sim800_send(unsigned char *content)
{
    uint8_t send_error = 0;
    uint16_t send_count = 0;
    uint8_t retry = 0;
    const uint8_t max_retry = 3;

    do {
        send_error = 0;
        send_count = 0;
        gsm_rev_okflag = 0;
        OLED_ShowStr(0, 6, "   Send SMS...  ", 2);
        gsm_send_msg(PhoneNumber, (char *)content);
        delay_ms(500);
        while(gsm_rev_okflag == 0)
        {
            if(send_count++ > 8000)
            {
                send_error = 1;
                break;
            }
            delay_ms(1);
        }
        gsm_rev_okflag = 0;
        if(send_error == 1) {
            OLED_ShowStr(0, 6, " Send FAIL!Retry ", 2);
            delay_ms(500);
        }
        retry++;
    } while(send_error == 1 && retry < max_retry);

    if(send_error == 1)
        OLED_ShowStr(0, 6, "   Send FAIL!   ", 2);
    else
        OLED_ShowStr(0, 6, "   Send OK!     ", 2);
    delay_ms(500);
    OLED_ShowStr(0, 6, "                ", 2);
}

/* 开启基站定位功能，需在 gsm_init 之后调用 */
void gsm_lbs_init(void)
{
    unsigned short waittry;
    gsm_rev_okflag = 0;
    uart1_send((unsigned char *)"config,set,location,1,1,60,0,0,0,0\r\n", 0xFF);
    waittry = 0;
    while(waittry++ < 5000)
    {
        if(gsm_rev_okflag == 1) break;
        delay_ms(1);
    }
}

/**
 * 查询基站定位坐标
 * @param lon 经度输出(WGS84)
 * @param lat 纬度输出(WGS84)
 * @return 1=成功 0=失败或超时
 * @note 基站定位最长超时60秒，可能失败
 */
uint8_t gsm_get_lbs(double *lon, double *lat)
{
    unsigned short waittry;
    char *p;

    /* 清空接收缓冲区，确保能完整捕获本次响应 */
    memset(Usart1RecBuf, 0, USART1_RXBUFF_SIZE);
    RxCounter = 0;

    gsm_rev_okflag = 0;
    uart1_send((unsigned char *)"config,get,lbsloc\r\n", 0xFF);

    /* 等待模块回包(含"ok")，基站查询最长60秒 */
    waittry = 0;
    while(waittry++ < 60000)
    {
        if(gsm_rev_okflag == 1) break;
        delay_ms(1);
    }
    if(gsm_rev_okflag == 0) return 0;

    /* "ok"检测点在坐标数据之前，等待剩余字节接收完成 */
    delay_ms(300);

    /* 响应格式: config,lbsloc,ok,经度,纬度\r\n */
    p = strstr(Usart1RecBuf, "lbsloc,ok,");
    if(p == NULL) return 0;

    p += 10; /* 跳过 "lbsloc,ok," */
    *lon = atof(p);

    p = strchr(p, ',');
    if(p == NULL) return 0;
    p++;
    *lat = atof(p);

    if(*lon == 0.0 && *lat == 0.0) return 0;
    return 1;
}
