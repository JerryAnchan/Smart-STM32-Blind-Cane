#include "gsm.h"
#include "usart1.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

char PhoneNumber[PHONE_NUMBER_LEN] = "18543448120";
uint8_t sendSmsFlag = 0;

// UTF-8字符串转16进制字符串
void utf8_to_hexstr(const char* utf8, char* hexstr)
{
    while (*utf8) {
        sprintf(hexstr, "%02X", (unsigned char)*utf8);
        hexstr += 2;
        utf8++;
    }
    *hexstr = '\0';
}

// 初始化，仅发送一次smson命令
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

// 发送短信，内容需转16进制字符串
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

// 兼容主程序调用
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
