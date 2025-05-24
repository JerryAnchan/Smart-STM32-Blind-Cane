
#include "adxl345.h"
#include "iic.h"
#include "delay.h"
#include "usart1.h" // 用于调试输出

void adxl345_init()
{
    u8 id = adxl345_read_reg(DEVICE_ID);
    if (id != 0xE5) {
        //printf("ADXL345 初�?化失败，ID = 0x%02X\r\n", id);
        return;
    }

    adxl345_write_reg(0X31, 0X0B); // 13位模�?±16g
    adxl345_write_reg(0x2C, 0x0B); // 100Hz
    adxl345_write_reg(0x2D, 0x08); // 进入测量模式
    adxl345_write_reg(0X2E, 0x00); // 禁用�?��
    adxl345_write_reg(0X1E, 0x00);
    adxl345_write_reg(0X1F, 0x00);
    adxl345_write_reg(0X20, 0x05);
}

void adxl345_write_reg(u8 addr, u8 val) 
{
    IIC_start();
    IIC_send_byte(slaveaddress);
    if (IIC_wait_ack()) goto stop;
    IIC_send_byte(addr);
    if (IIC_wait_ack()) goto stop;
    IIC_send_byte(val);
    if (IIC_wait_ack()) goto stop;
stop:
    IIC_stop();
}

u8 adxl345_read_reg(u8 addr)
{
    u8 temp = 0;
    IIC_start();
    IIC_send_byte(slaveaddress);
    if (IIC_wait_ack()) goto stop;
    IIC_send_byte(addr);
    if (IIC_wait_ack()) goto stop;
    IIC_start();
    IIC_send_byte(regaddress);
    if (IIC_wait_ack()) goto stop;
    temp = IIC_read_byte(0);
stop:
    IIC_stop();
    return temp;
}

void adxl345_read_data(short *x, short *y, short *z)
{
    u8 buf[6];
    u8 i;
    *x = *y = *z = 0;

    IIC_start();
    IIC_send_byte(slaveaddress);
    if (IIC_wait_ack()) goto error;
    IIC_send_byte(0x32);
    if (IIC_wait_ack()) goto error;

    IIC_start();
    IIC_send_byte(regaddress);
    if (IIC_wait_ack()) goto error;

    for(i = 0; i < 6; i++) {
        buf[i] = IIC_read_byte(i != 5);
    }
    IIC_stop();

    *x = (short)(((u16)buf[1] << 8) | buf[0]);
    *y = (short)(((u16)buf[3] << 8) | buf[2]);
    *z = (short)(((u16)buf[5] << 8) | buf[4]);
    return;

error:
    IIC_stop();
    *x = *y = *z = 0;
}

void adxl345_read_average(float *x, float *y, float *z, u8 times)
{
    u8 i, err = 0;
    short tx, ty, tz;
    *x = *y = *z = 0;

    if (times == 0) return;

    for (i = 0; i < times; i++) {
        adxl345_read_data(&tx, &ty, &tz);
        if (tx == 0 && ty == 0 && tz == 0) {
            err++;
            continue;
        }
        *x += tx;
        *y += ty;
        *z += tz;
        delay_ms(2);
    }

    if (err == times || times == err) {
        *x = *y = *z = 0;
        return;
    }

    *x /= (float)(times - err);
    *y /= (float)(times - err);
    *z /= (float)(times - err);
}
