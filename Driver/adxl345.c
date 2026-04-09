/**
 * @file adxl345.c
 * @brief ADXL345三轴加速度计驱动（软件I2C）
 *
 * I2C地址: 0xA6(W) / 0xA7(R)（ALT ADDRESS接地）
 * 量程: ±16g，13位分辨率（4mg/LSB）
 * 采样率: 100Hz
 */
#include "adxl345.h"
#include "iic.h"
#include "delay.h"

/**
 * @brief  初始化ADXL345
 */
void adxl345_init()
{
    u8 id = adxl345_read_reg(DEVICE_ID);
    if (id != 0xE5) {
        return;
    }

    adxl345_write_reg(0X31, 0X0B); // DATA_FORMAT: 13位模式 ±16g
    adxl345_write_reg(0x2C, 0x0B); // BW_RATE: 100Hz采样率
    adxl345_write_reg(0x2D, 0x08); // POWER_CTL: 进入测量模式
    adxl345_write_reg(0X2E, 0x00); // INT_ENABLE: 禁用中断
    adxl345_write_reg(0X1E, 0x00); // OFSX: X轴偏移=0
    adxl345_write_reg(0X1F, 0x00); // OFSY: Y轴偏移=0
    adxl345_write_reg(0X20, 0x05); // OFSZ: Z轴偏移补偿
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

    IIC_start();
    IIC_send_byte(slaveaddress);
    if (IIC_wait_ack()) { goto error; }

    IIC_send_byte(0x32);
    if (IIC_wait_ack()) { goto error; }

    IIC_start();
    IIC_send_byte(slaveaddress | 0x01);
    if (IIC_wait_ack()) { goto error; }

    for (i = 0; i < 6; i++) {
        buf[i] = IIC_read_byte(i == 5 ? 0 : 1);
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
