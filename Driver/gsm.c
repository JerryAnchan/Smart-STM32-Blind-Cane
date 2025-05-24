#include "gsm.h"
#include "usart1.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

char PhoneNumber[PHONE_NUMBER_LEN] = "18888888888";
char ConversionNum[CONVERSION_NUM_LEN] = {0};
uint8_t sendSmsFlag = 0;

void PhoneNumTranscoding(void)
{
    uint8_t i = 0;
    for(i = 0; i < 11; i++)
    {
        ConversionNum[i*4+0] = '0';
        ConversionNum[i*4+1] = '0';
        ConversionNum[i*4+2] = '3';
        ConversionNum[i*4+3] = PhoneNumber[i];
    }
}

void gsm_atcmd_send(char *at)
{
    unsigned short waittry;
    do
    {
        gsm_rev_start = 0;
        gsm_rev_okflag = 0;
        waittry = 0;
        uart1_send((unsigned char *)at, 0xFF);
        while(waittry++ < 3000)
        {
            if(gsm_rev_okflag == 1)
            {
                return;
            }
            delay_ms(1);
        }
    }
    while(gsm_rev_okflag == 0);
}

void gsm_init(void)
{
    gsm_atcmd_send("AT\r\n");
    delay_ms(1000);
    gsm_atcmd_send("AT+CSCS=\"UCS2\"\r\n");
    delay_ms(1000);
    gsm_atcmd_send("AT+CMGF=1\r\n");
    delay_ms(1000);
    gsm_atcmd_send("AT+CNMI=2,1\r\n");
    delay_ms(1000);
    gsm_atcmd_send("AT+CMGD=1,4\r\n");
    delay_ms(1000);
    gsm_atcmd_send("AT+CSMP=17,0,2,25\r\n");
    delay_ms(1000);
}

void gsm_send_msg(const char* number, char *content)
{
    uint8_t len;
    unsigned char gsm_at_txbuf[60];
    memset(gsm_at_txbuf, 0, 60);
    strncpy((char *)gsm_at_txbuf, "AT+CMGS=\"", 9);
    memcpy(gsm_at_txbuf + 9, number, 44);
    len = strlen((char *)gsm_at_txbuf);
    gsm_at_txbuf[len] = '"';
    gsm_at_txbuf[len + 1] = '\r';
    gsm_at_txbuf[len + 2] = '\n';
    uart1_send(gsm_at_txbuf, 0xFF);
    delay_ms(1000);
    uart1_send((unsigned char *)content, 0xFF);
    delay_ms(100);
    printf("%c", 0x1a);
    delay_ms(10);
}

void sim800_send(unsigned char *content)
{
    uint8_t send_error = 0;
    uint16_t send_count = 0;
    gsm_rev_okflag = 0;
    OLED_ShowStr(0, 6, "   Send Sms...  ", 2);
    gsm_send_msg(ConversionNum, (char *)content);
    delay_ms(2000);
    delay_ms(2000);
    delay_ms(2000);
    while(gsm_rev_okflag == 0)
    {
        if(send_count++ > 8000)
        {
            send_count = 0;
            send_error = 1;
            break;
        }
        delay_ms(1);
    }
    gsm_rev_okflag = 0;
    if(send_error == 1)
        OLED_ShowStr(0, 6, "   Send Fail!   ", 2);
    else
        OLED_ShowStr(0, 6, "   Send OK!     ", 2);
    // UsartRx1BufClear(); // 如有需要请在主程序调用
    delay_ms(2000);
    delay_ms(2000);
    delay_ms(2000);
    OLED_ShowStr(0, 6, "                ", 2);
}

void LongiAndLatiChangeUnicode(char *str1, char *str2)
{
    uint8_t i = 0, len;
    char *buf = str1;
    len = strlen(buf);
    for(i = 0; i < 3; i++)
    {
        if(buf[i] != ' ')
        {
            *str2++ = '0';
            *str2++ = '0';
            *str2++ = '3';
            *str2++ = buf[i];
        }
    }
    *str2++ = '0'; *str2++ = '0';
    *str2++ = '2'; *str2++ = 'E';
    i++;
    for(; i < len-1; i++)
    {
        *str2++ = '0';
        *str2++ = '0';
        *str2++ = '3';
        *str2++ = buf[i];
    }
    *str2 = '\0';
}
