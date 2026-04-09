/**
 * @file gsm.h
 * @brief GSM模块接口（短信发送 / 基站定位）
 */
#ifndef __GSM_H
#define __GSM_H

#include "sys.h"
#include "OLED_I2C.h"
#include <stdint.h>

#define PHONE_NUMBER_LEN 12
#define CONVERSION_NUM_LEN 44

extern char PhoneNumber[PHONE_NUMBER_LEN];      // 手机号
extern char ConversionNum[CONVERSION_NUM_LEN];  // 转码后手机号
extern uint8_t sendSmsFlag;                     // 发送短信标志
extern uint8_t gsm_rev_start;
extern uint8_t gsm_rev_okflag;

void PhoneNumTranscoding(void);
void gsm_init(void);
void gsm_send_msg(const char* number, const char* content);
void sim800_send(unsigned char *content);
void gsm_lbs_init(void);
uint8_t gsm_get_lbs(double *lon, double *lat);

#endif
