/***********************
加速度传感器ADXL345数据读取程序
IIC总线接口
SDA：PD5
SCL：PD6
************************/
#include "adxl345.h"
#include "iic.h"
#include "delay.h"
//传感器初始化
void adxl345_init()
{
		adxl345_write_reg(0X31,0X0B);		//低电平中断输出,13位全分辨率,输出数据右对齐,16g量程 
		adxl345_write_reg(0x2C,0x0B);		//数据输出速度为100Hz
		adxl345_write_reg(0x2D,0x08);	   	//链接使能,测量模式,省电特性
		adxl345_write_reg(0X2E,0x80);		//不使用中断		 
	 	adxl345_write_reg(0X1E,0x00);
		adxl345_write_reg(0X1F,0x00);
		adxl345_write_reg(0X20,0x05);	
}
//写寄存器函数
void adxl345_write_reg(u8 addr,u8 val) 
{
	IIC_start();  				 
	IIC_send_byte(slaveaddress);     	//发送写器件指令	 
	IIC_wait_ack();	   
    IIC_send_byte(addr);   			//发送寄存器地址
	IIC_wait_ack(); 	 										  		   
	IIC_send_byte(val);     		//发送值					   
	IIC_wait_ack();  		    	   
    IIC_stop();						//产生一个停止条件 	   
}
//读寄存器函数
u8 adxl345_read_reg(u8 addr)
{
	u8 temp=0;		 
	IIC_start();  				 
	IIC_send_byte(slaveaddress);	//发送写器件指令	 
	temp=IIC_wait_ack();	   
    IIC_send_byte(addr);   		//发送寄存器地址
	temp=IIC_wait_ack(); 	 										  		   
	IIC_start();  	 	   		//重新启动
	IIC_send_byte(regaddress);	//发送读器件指令	 
	temp=IIC_wait_ack();	   
    temp=IIC_read_byte(0);		//读取一个字节,不继续再读,发送NAK 	    	   
    IIC_stop();					//产生一个停止条件 	    
	return temp;
}
//读取数据函数
void adxl345_read_data(short *x, short *y, short *z)
{
    u8 buf[6];
    u8 i;
    IIC_start();
    IIC_send_byte(slaveaddress);    //发送写器件指令
    if(IIC_wait_ack()) goto error;
    IIC_send_byte(0x32);            //发送寄存器地址
    if(IIC_wait_ack()) goto error;

    IIC_start();
    IIC_send_byte(regaddress);      //发送读器件指令
    if(IIC_wait_ack()) goto error;
    for(i=0; i<6; i++)
    {
        if(i==5) buf[i]=IIC_read_byte(0);
        else buf[i]=IIC_read_byte(1);
    }
    IIC_stop();
    *x=(short)(((u16)buf[1]<<8)+buf[0]);
    *y=(short)(((u16)buf[3]<<8)+buf[2]);
    *z=(short)(((u16)buf[5]<<8)+buf[4]);
    return;
error:
    IIC_stop();
    *x = *y = *z = 0; // 采集失败时返回0
}
//连读读取几次取平均值函数
//times 取平均值的次数
void adxl345_read_average(float *x, float *y, float *z, u8 times)
{
    u8 i, err = 0;
    short tx, ty, tz;
    *x = 0; *y = 0; *z = 0;
    if(times)
    {
        for(i=0; i<times; i++)
        {
            adxl345_read_data(&tx, &ty, &tz);
            if(tx==0 && ty==0 && tz==0) { err++; continue; }
            *x += tx; *y += ty; *z += tz;
            delay_ms(5);
        }
        if(err == times) { *x = *y = *z = 0; return; }
        *x /= (times-err);
        *y /= (times-err);
        *z /= (times-err);
    }
}
//void get_angle(float x_angle,float y_angle,float z_angle)
//{
//	short ax,ay,az;
//	adxl345_read_average(&ax,&ay,&az,10);
//	x_angle=atan(ax/sqrt((az*az+ay*ay)));
//	y_angle=atan(ay/sqrt((ax*ax+az*az)));
//	z_angle=atan(sqrt((ax*ax+ay*ay)/az));
//}
