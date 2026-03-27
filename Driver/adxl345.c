#include "adxl345.h"
#include "iic.h"
#include "delay.h"
#include "usart1.h" // 用于调试输出
#include "OLED_I2C.h"
#include <stdio.h>

/**
 * @brief  初始化ADXL345
 */
void adxl345_init()
{
    u8 id = adxl345_read_reg(DEVICE_ID);
    if (id != 0xE5) {
        //printf("ADXL345 初始化失败，ID = 0x%02X\r\n", id);
        return;
    }

    adxl345_write_reg(0X31, 0X0B); // 13位模式 ±16g
    adxl345_write_reg(0x2C, 0x0B); // 100Hz
    adxl345_write_reg(0x2D, 0x08); // 进入测量模式
    adxl345_write_reg(0X2E, 0x00); // 禁用中断
    adxl345_write_reg(0X1E, 0x00);
    adxl345_write_reg(0X1F, 0x00);
    adxl345_write_reg(0X20, 0x05);
}

/**
 * @brief  向ADXL345写寄存器
 * @param  addr: 寄存器地址
 * @param  val:  写入的值
 */
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

/**
 * @brief  读取ADXL345寄存器
 * @param  addr: 寄存器地址
 * @retval 读取到的值
 */
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

/**
 * @brief  读取ADXL345三轴加速度数据
 * @param  x: X轴数据指针
 * @param  y: Y轴数据指针
 * @param  z: Z轴数据指针
 */
void adxl345_read_data(short *x, short *y, short *z)
{
    u8 buf[6];
    u8 i;
    //Uart1_SendStr("RD1\r\n"); // 开始
    IIC_start();
    IIC_send_byte(slaveaddress);
    if (IIC_wait_ack()) { /*Uart1_SendStr("ERR1\r\n");*/ goto error; }

    IIC_send_byte(0x32);
    if (IIC_wait_ack()) { /*Uart1_SendStr("ERR2\r\n");*/ goto error; }

    IIC_start();
    IIC_send_byte(slaveaddress | 0x01);
    if (IIC_wait_ack()) { /*Uart1_SendStr("ERR3\r\n");*/ goto error; }

    for (i = 0; i < 6; i++) {
        buf[i] = IIC_read_byte(i == 5 ? 0 : 1);
    }
    IIC_stop();

    *x = (short)(((u16)buf[1] << 8) | buf[0]);
    *y = (short)(((u16)buf[3] << 8) | buf[2]);
    *z = (short)(((u16)buf[5] << 8) | buf[4]);
    //Uart1_SendStr("OK\r\n");
    return;

error:
    IIC_stop();
    *x = *y = *z = 0;
    //Uart1_SendStr("RDE\r\n");
}

/**
 * @brief  多次采样并取平均值
 * @param  x: X轴平均值指针
 * @param  y: Y轴平均值指针
 * @param  z: Z轴平均值指针
 * @param  times: 采样次数
 */
void adxl345_read_average(float *x, float *y, float *z, u8 times)
{
    u8 i, err = 0;
    short tx, ty, tz;
    // char buf[32]; // 调试用，暂时注释
    *x = *y = *z = 0;

    if (times == 0) return;

    //sprintf(buf, "TIMES=%d\r\n", times);
    //Uart1_SendStr(buf);

    for (i = 0; i < times; i++) {
        //Uart1_SendStr("AVG_IN\r\n");
        adxl345_read_data(&tx, &ty, &tz);
        //Uart1_SendStr("AVG_OUT\r\n");
        if (tx == 0 && ty == 0 && tz == 0) {
            err++;
            continue;
        }
        *x += tx;
        *y += ty;
        *z += tz;
        delay_ms(2);
        //sprintf(buf, "i=%d\r\n", i);
        //Uart1_SendStr(buf);
    }
    //Uart1_SendStr("AVG_DONE\r\n");

    if (err == times || times == err) {
        *x = *y = *z = 0;
        return;
    }

    *x /= (float)(times - err);
    *y /= (float)(times - err);
    *z /= (float)(times - err);
}
