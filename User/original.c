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

#define FLASH_SAVE_ADDR  ((u32)0x0800F000) 				//设置FLASH 保存地址(必须为偶数)

#define STM32_RX1_BUF       Usart1RecBuf 
#define STM32_Rx1Counter    RxCounter
#define STM32_RX1BUFF_SIZE  USART1_RXBUFF_SIZE

#define STM32_RX3_BUF       Usart3RecBuf 
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

#define GPS_STR_LEN 48
GPS_INFO   GPS;  //GPS信息结构体

extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
u8 GPS_rx_flag = 0;
u8 gpsInitFlag = 0;

//STM32超声波测距
u16  SAFET_Distance  = 30;         //安全距离
float  Distance = 0;//距离
u8  distanceFlag=0;
u8  Twinkle=0;             //取反闪烁标志
u8  InitFlag=1;						//初始化标志
u8  setn=0;//设置键按下次数记录
u8  sendFlag = 0;//发短信标志
u8 shuaxin=0;
u8 play_time = 0;
unsigned char miao=0;
char   PhoneNumber[11];//手机号码
char ConversionNum[44];//手机号码转码后存放数组
char callNumber[12];
unsigned char display[16];
u8 tiltFlag=0;
u8 fall=0;
u8 fallTime=10;
u8 sendSmsFlag = 0;        //发送短信标志
u8 SendFlag=0x00;
float adx,ady,adz;
float acc,acc2;
bool Emergency=0;

void UsartRx1BufClear(void)
{
		memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//清除缓存
		STM32_Rx1Counter = 0;
}

float ChangeFloatData(int dat)//转换为浮点数
{
		return (float)(dat)/10;
}

void SprintfIntNum(u16 data,char *str)//输出一个3位整型的数据 
{
		sprintf((char *)str,"%dcm  ",data);
		if(data>9)sprintf((char *)str,"%dcm ",data);
		if(data>99)sprintf((char *)str,"%dcm",data);
}
 
void PhoneNumTranscoding(void)//手机号转码
{
	  u8 i=0;
		for(i=0;i<11;i++)//发送中文短信手机号必须转码，前面需要加上003
		{
				ConversionNum[i*4+0] = '0';
			  ConversionNum[i*4+1] = '0';
			  ConversionNum[i*4+2] = '3';
			  ConversionNum[i*4+3] = PhoneNumber[i];
		}
}

void ShowHomePage(void)//显示主页面
{
		char i;
	  if(InitFlag==1)
		{
				InitFlag = 0;
			  OLED_CLS();//清屏
			  OLED_ShowStr(54,0,"SET:",2);
			  SprintfIntNum(SAFET_Distance,(char *)display);
				OLED_ShowStr(87,0,display,2);
			  for(i=0;i<2;i++)OLED_ShowCN(i*16,2,i+26,0);//测试显示中文：经度
				for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+28,0);//测试显示中文：纬度
				OLED_ShowChar(32,2,':',2,0);
				OLED_ShowChar(32,4,':',2,0);
		}
}

void gsm_atcmd_send(char *at)//GSM AT指令发送函数
{
    unsigned short waittry;//延时变量
    do
    {
        gsm_rev_start = 0;//接收开始标志清零
        gsm_rev_okflag = 0;//接收完成标志清零
        waittry = 0;//延时变量清零
        uart1_send((unsigned char *)at,0xFF);//串口发送内容
        while(waittry ++ < 3000)//进入while延时
        {
            if (gsm_rev_okflag == 1)//等待GSM返回OK
            {
                return;//返回出去
            }
            delay_ms(1);
        }
    }
    while(gsm_rev_okflag == 0);
}

void gsm_init(void)//gsm初始化
{
    gsm_atcmd_send("AT\r\n");//测试用的
    delay_ms(1000);
    gsm_atcmd_send("AT+CSCS=\"UCS2\"\r\n");//设置为unicode编码
    delay_ms(1000);
    gsm_atcmd_send("AT+CMGF=1\r\n");//设置为文本模式
    delay_ms(1000);
    gsm_atcmd_send("AT+CNMI=2,1\r\n");//来短信时提示，并存储到模块内存
	  delay_ms(1000);
	  gsm_atcmd_send("AT+CMGD=1,4\r\n");//清除短信
	  delay_ms(1000);
	  gsm_atcmd_send("AT+CSMP=17,0,2,25\r\n");//设置短信保留为5分钟，发送中文
	  delay_ms(1000);
}

/*
*number 为对方手机号
*/
void gsm_send_msg(const char*number,char * content)
{
    u8 len;
    unsigned char  gsm_at_txbuf[60];//GSM AT命令缓存区
	
    memset(gsm_at_txbuf, 0, 60);//缓存清零
    strncpy((char *)gsm_at_txbuf,"AT+CMGS=\"",9);//将AT+CMGS=\"，复制到gsm_at_txbuf
    memcpy(gsm_at_txbuf + 9, number, 44);//将手机号码复制到AT+CMGS=\"之后
    len = strlen((char *)gsm_at_txbuf);//获取gsm_at_txbuf字符串长度
    gsm_at_txbuf[len] = '"'; // AT+CMGS=\"12345678901\"
    gsm_at_txbuf[len + 1] = '\r';
    gsm_at_txbuf[len + 2] = '\n';//gsm_at_txbuf最终的格式"AT+CMGS=\"手机号码\"\r\n"

    uart1_send(gsm_at_txbuf,0xFF);//发送需要接受短信的手机号码
    delay_ms(1000);

    uart1_send((unsigned char *)content,0xFF);  //发短信内容
    delay_ms(100);

    printf("%c",0x1a);     //发送结束符号
    delay_ms(10);
}

void sim800_send(unsigned char *content)
{
	  u8 send_error = 0;
	  u16 send_count = 0;
	
	  gsm_rev_okflag = 0;
		OLED_ShowStr(0,6,"   Send Sms...  ",2);
		gsm_send_msg(ConversionNum,(char *)content);//发送短信
		delay_ms(2000);//延时2秒
		delay_ms(2000);//延时2秒
		delay_ms(2000);//延时2秒
	  while(gsm_rev_okflag == 0)//等待返回OK指令
		{
			  if(send_count++ > 8000)
				{
						send_count = 0;
					  send_error = 1;
					  break;
				}
				delay_ms(1);
		};
	  gsm_rev_okflag = 0;
		if(send_error == 1)
		OLED_ShowStr(0,6,"   Send Fail!   ",2);//显示发送超时
		else
		OLED_ShowStr(0,6,"   Send OK!     ",2);
		UsartRx1BufClear();
		delay_ms(2000);//延时2秒
		delay_ms(2000);//延时2秒
		delay_ms(2000);//延时2秒
		OLED_ShowStr(0,6,"                ",2);
		if(Emergency  == 0)
		{
				OLED_ShowStr(54,0,"SET:",2);
				SprintfIntNum(SAFET_Distance,(char *)display);
				OLED_ShowStr(87,0,display,2);
		}
}


void DisplaySetValue(void) //显示设置值
{
		u8 add=2,i;

		if(setn>=2)
		{
			for(i=0;i<11;i++)
			{
					OLED_ShowChar((add++)*8,4,PhoneNumber[i],2,(setn+1)-(2+i));//显示手机号码
			}
		}
		if(setn==0)
		{
				SprintfIntNum(SAFET_Distance,(char *)display);
				OLED_ShowStr(87,0,display,2);
		}	
		if(setn==1)
		{
				SprintfIntNum(SAFET_Distance,(char *)display);
				OLED_ShowStr(50,4,display,2);
		}	
}

void KeySettings(void)//按键设置函数
{
	  char i;
	
		if(KEY1==0)//设置
		{
			  delay_ms(20);//消抖
			  if(KEY1==0)
				{
					  while(KEY1==0);

						setn++;
						if(setn > 12)
						{
							setn=0;
							STMFLASH_Write(FLASH_SAVE_ADDR + 0x40,(u16*)PhoneNumber,11);//退出设置前，先把手机号存储一遍
							PhoneNumTranscoding();
							STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1); //存储设置的距离
							InitFlag=1;
						}
						if(setn==1)
						{
							OLED_CLS();//清屏
							for(i=0;i<6;i++)OLED_ShowCN(i*16+16,0,i+30,0);//测试显示中文：设置提醒距离
						}
						if(setn==2)
						{
							for(i=0;i<8;i++)OLED_ShowCN(i*16,0,i+11,0);//测试显示中文：设置接收短信号码
						}
						DisplaySetValue();
				}
		}
		if(KEY2==0)//加
		{
			  if(setn!=0)delay_ms(80);
			  else       delay_ms(50);
			  if(KEY2==0)
				{
						if(setn==1)
						{
							  if(SAFET_Distance<450)SAFET_Distance++;
							  DisplaySetValue();
						}
						if(setn>=2)
						{
								PhoneNumber[setn-2]++;
								if(PhoneNumber[setn-2]>'9')PhoneNumber[setn-2]='0';
								
								DisplaySetValue();
						}
				}
		}
		if(KEY3==0)//减
		{
			  if(setn!=0)delay_ms(80);
			  else       delay_ms(50);
			  if(KEY3==0)
				{
						if(setn==1)
						{
								if(SAFET_Distance>0)SAFET_Distance--;
							  DisplaySetValue();
						}
						if(setn>=2)
						{
								PhoneNumber[setn-2]--;
								if(PhoneNumber[setn-2]<'0')PhoneNumber[setn-2]='9';
								DisplaySetValue();
						}
				}
		}
		if(KEY4==0)//一键求助
		{
			  delay_ms(20);
			  if(KEY4==0)
				{
					  while(KEY4==0);
						 if(setn==0)
						{
							  if(Emergency == 0)
							  {
										if(!(SendFlag&0x02))
										{
												SendFlag|=0x02;
												sendSmsFlag = 2;      //发送短信标志
										}
										if(fall==0)play_time = 0;
										Emergency = 1;     //紧急求助
							  }
						}
				}
		}
		if(KEY5==0)//取消求助
		{
			  delay_ms(20);
			  if(KEY5==0)
				{
					  while(KEY5==0);
						if(setn==0)
						{
							  if(Emergency==1)
								{
										Emergency=0;
										SendFlag&=0xFD;
										OLED_ShowStr(54,0,"SET:",2);
										SprintfIntNum(SAFET_Distance,(char *)display);
										OLED_ShowStr(87,0,display,2);
								}
						}
				}
		}
}

void CheckNewMcu(void)  // 检查是否是新的单片机，是的话清空存储区，否则保留
{
	  u8 comper_str[6],i;
		
	  STM32F10x_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
	  comper_str[5] = '\0';
	  if(strstr((char *)comper_str,"FDYDZ") == NULL)  //新的单片机
		{
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"FDYDZ",5); //写入“FDYDZ”，方便下次校验
			 delay_ms(50);
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x40,(u16*)"12345678910",11);//存入初始手机号
			 delay_ms(50);
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1); //存储设置的距离
	  }
		STM32F10x_Read(FLASH_SAVE_ADDR + 0x40,(u16*)PhoneNumber,11); //读出手机号
		PhoneNumTranscoding();
		STM32F10x_Read(FLASH_SAVE_ADDR + 0x60,&SAFET_Distance,1); //读出设置的距离
		if(SAFET_Distance>400)SAFET_Distance=30;
		for(i = 0; i < 11 ; i++)
		{
				if(PhoneNumber[i]<'0' || PhoneNumber[i]>'9')
				{
						break;
				}
		}
		if(i != 11)
		{
				memset(PhoneNumber, 0 , 11);    //清除缓存
			  sprintf(PhoneNumber,"12345678910");
		}
		delay_ms(100);
}

void FallDetection(void)//倾斜检测
{
	  u8 i;
	
	  adxl345_read_average(&adx,&ady,&adz,10);//获取数据
		acc=ady;
	  acc2=adx;
		if(acc<0)acc=-acc;
    if(acc2<0)acc2=-acc2;
		if(((u16)acc)>=190 || ((u16)acc2)>=190)//检测到倾斜
		{
			  tiltFlag=1;
		}
		else
		{
				tiltFlag=0;
			  fallTime=10;
		}
		
		if(fallTime==0)
		{
			  if(fall==0)
				{
					  OLED_ShowStr(40,0,"           ",2);
					  for(i=0;i<3;i++)OLED_ShowCN(i*16+70,0,i+8,0);//测试显示中文：摔倒！
					  play_time = 0;
					  fall=1;
				}
		}
		else 
		{
			  if(fall==1)
				{
						fall=0;
					 if(Emergency  == 1)
					 {
							for(i=0;i<4;i++)OLED_ShowCN(i*16+54,0,i+36,0);//测试显示中文：紧急求助
					 }
					 else
					 {
					
								OLED_ShowStr(54,0,"SET:",2);
								SprintfIntNum(SAFET_Distance,(char *)display);
								OLED_ShowStr(87,0,display,2);
					 }
				}
		}
		
		if(fall == 1)
		{
				if(!(SendFlag&0x01))
				{
						SendFlag|=0x01;
						sendSmsFlag = 1;      //发送短信标志
				}
		}
		else
		{
				SendFlag&=0xFE;
		}
}

void Get_Distance(void)
{
	 u8 i;
	
	 Distance = (Get_SR04_Distance() * 331) * 1.0/1000;   //Get_SR04_Distance()返回单程声波传输时间 us,转换为秒=时间*10^(-6);331m/s等于331000mm/s，
	 //最终换算为Distance =Get_SR04_Distance()*10^(-6)*331000=(Get_SR04_Distance() * 331) * 1.0/1000;
	 if(Distance>=4500)Distance=4500;
	
	  SprintfIntNum((u16)Distance/10,(char *)display);
	  OLED_ShowStr(0,0,display,2);
	
	  if(Emergency  == 0)
		{
				if(Distance/10<=SAFET_Distance)
				{
					if(distanceFlag==0)
					{
						distanceFlag=1;
						if(fall==0)play_time = 0;
						for(i=0;i<4;i++)OLED_ShowCN(i*16+54,0,i+2,0);//测试显示中文：距离过近
						delay_ms(1000);
						delay_ms(1000);
						OLED_ShowStr(54,0,"SET:",2);
						SprintfIntNum(SAFET_Distance,(char *)display);
						OLED_ShowStr(87,0,display,2);
					}
				}else 
				{
					distanceFlag=0;//低于安全距离
				}
		}
		else
		{
				for(i=0;i<4;i++)OLED_ShowCN(i*16+54,0,i+36,0);//测试显示中文：紧急求助
		}
}

void Get_GPS(void)//获取GPS数据
{
	  static u8 errorNum=0;
	  static u8 timeCount=0;
	
		if (rev_stop == 1 && timeCount++>=5)   //如果接收完一行
		{
				if (GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) //解析GPRMC
				{
						errorNum = 0;
						gps_flag = 0;
						rev_stop  = 0;
						gpsInitFlag=1;
				}
				else
				{
						if (errorNum++ >= 30) //如果数据无效超过30次
						{
								errorNum = 30;
								gpsInitFlag = 0;
						}
						gps_flag = 0;
						rev_stop  = 0;
				}
				timeCount=0;
		}
		sprintf((char *)display,"%10.6f ",GPS.longitude_Degree);
		OLED_ShowStr(40, 2, (u8*)display, 2);//显示经度

		sprintf((char *)display,"%10.6f ",GPS.latitude_Degree);
		OLED_ShowStr(40, 4, (u8*)display, 2);//显示纬度
}

void LongiAndLatiChangeUnicode(char *str1,char *str2)//经纬度坐标转Unicode码
{
	  u8 i=0,len;
	  char *buf = str1;
	  len = strlen(buf);//获取字符串长度
	
		for(i=0; i < 3; i++)//小数点前3位
		{
				if(buf[i] != ' ')
				{    
            *str2++ = '0';
					  *str2++ = '0';
					  *str2++ = '3';
					  *str2++ = buf[i];
				}
		}
		
		*str2++ = '0';*str2++ = '0';//小数点
		*str2++ = '2';*str2++ = 'E';

		i++;
		
		for(;i < len-1; i++)//小数点后6位
		{
				*str2++ = '0';
				*str2++ = '0';
				*str2++ = '3';
				*str2++ = buf[i];
		}
		*str2 = '\0';
}

int main(void)
{	
	  char SEND_BUF[400];//发送短信缓存
	  char BUF1[50],BUF2[50]; //BUF1为经纬度转换前缓存区，BUF2为转换后缓存区
	
		delay_init();	    //延时函数初始化	
    NVIC_Configuration();  //中断优先级
	  delay_ms(200);
	  I2C_Configuration(); //OLED IIC引脚初始化
	  GPS_rx_flag = 0;
	  CheckNewMcu();//校验单片机
	  BEEP_GPIO_Init(); //蜂鸣器引脚初始化
	  KEY_GPIO_Init();//按键初始化
	  IIC_init();//IIC初始化
		adxl345_init();//ADXL345初始化
	  HC_SR04_IO_Init();  //超声波模块GPIO初始化
	  LED_GPIO_Init();//LED初始化
	  WT588D_GPIO_INIT();
		OLED_Init(); //OLED初始化
	  OLED_CLS();//清屏
	  OLED_ShowStr(0,2,"   GSM Init...  ",2);
		uart1_Init(9600);
		gsm_init();//gsm初始化
	  USART3_Init(9600);
		OLED_CLS();//清屏
		UsartRx1BufClear();
		GPS_rx_flag = 1;
		TIM2_Init(500-1,7199);//计数到500为50ms   500*100=50000us=50ms
	  TIM3_Init(7199,0);    //定时100us
		//Tout = ((arr+1)*(psc+1))/Tclk ;  
	  //Tclk:定时器输入频率(单位MHZ)
	  //Tout:定时器溢出时间(单位us)
		while(1)
		{  
			  KeySettings();
				ShowHomePage();
			  if(setn == 0)//不在设置状态下，读取相关数据
				{
					  if(shuaxin == 1)
						{
							  shuaxin=0;
							  Get_GPS(); //获取经纬度
							  FallDetection();//检测有没有摔倒
								Get_Distance();//获取距离
							
								if(sendSmsFlag!=0)
								{
										/******************************************************************************************/
										/*******************以下为短信内容处理部分，发送中文短信必须转换为Unicode码****************/
										/******************************************************************************************/
										memset(SEND_BUF,0, 400);   			//清空缓冲区
									
									  if(sendSmsFlag==1)
									  {
												strncpy(SEND_BUF,"8BF76CE8610FFF0C68C06D4B523080014EBA64545012FF01",48);	//请注意，检测到老人摔倒！
										}
										if(sendSmsFlag==2)
									  {
												strncpy(SEND_BUF,"62119047523056F096BEFF0C970089815E2E52A9FF01",44);	//我遇到困难，需要帮助！
										}
										strcat(SEND_BUF,"7ECF5EA6");																							//经度
										memset(BUF1,0,50);      //清空缓冲区
										memset(BUF2,0,50);      //清空缓冲区
										sprintf((char *)BUF1,"%10.6f ",GPS.longitude_Degree);
										LongiAndLatiChangeUnicode(BUF1,BUF2);                                         //将经度转码
										strcat(SEND_BUF,BUF2);	
										
										strcat(SEND_BUF,"FF0C7EAC5EA6");																							//，纬度
										memset(BUF1,0,50);      //清空缓冲区
										memset(BUF2,0,50);      //清空缓冲区
										sprintf((char *)BUF1,"%10.6f ",GPS.latitude_Degree);
										LongiAndLatiChangeUnicode(BUF1,BUF2);                                         //将纬度转码
										strcat(SEND_BUF,BUF2);
										strcat(SEND_BUF,"3002");
										sim800_send((unsigned char *)SEND_BUF);//发送短信
										
										/***************************************** END ******************************************/
										sendSmsFlag = 0;
								}
						}
				}
				delay_ms(1);
		}
}

void TIM2_IRQHandler(void)//定时器2中断服务程序	 
{ 
	  static u8 time_count1s=0;
	  static u8 play_flag = 0;
	  static unsigned int timeCount=0;
		if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{ 
				TIM_ClearITPendingBit(TIM2, TIM_IT_Update); //清除中断标志位  
			  LED=GM;//光线暗，开灯
				if(timeCount++>=10)
				{
						timeCount = 0;
					  shuaxin=1;
				}
				if(time_count1s++ >= 20)//1秒时间到
				{
						time_count1s = 0;
            if(tiltFlag)
						{
								if(fallTime>0)fallTime--;
						}
						if(miao > 0)miao--;
						
            play_flag = 0;

						if(WATER==1)play_flag = 4;   //距离较近播报优先级第4
						if(distanceFlag==1)play_flag = 3;   //距离较近播报优先级第3
						if(Emergency==1)play_flag = 2;   //紧急求助播报优先级第2
						if(fall==1)play_flag = 1;   //跌倒优先级最高
						
						if(play_flag!=0)
						{
								if(play_time==0)Line_1A(play_flag-1);      //语音播报
								if(play_time++>=5)play_time=0;
						}
						else
						{
								play_time=0;
						}
				}
	  }
}

