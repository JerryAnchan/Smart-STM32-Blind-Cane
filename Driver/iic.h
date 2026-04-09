/**
 * @file iic.h
 * @brief ????I2C?????PC14=SCL, PC15=SDA?
 */
#ifndef __IIC_H
#define __IIC_H
#include "sys.h"

#define IIC_SCL    PCout(14) // SCL??
#define IIC_SDA    PCout(15) // SDA??
#define READ_SDA   PCin(15)  // ??SDA??

void I2C_SDA_OUT(void);
void I2C_SDA_IN(void);
void IIC_init(void);
void IIC_start(void);
void IIC_stop(void);
void IIC_ack(void);
void IIC_noack(void);
u8 IIC_wait_ack(void);
void IIC_send_byte(u8 txd);
u8 IIC_read_byte(u8 ack);
#endif
