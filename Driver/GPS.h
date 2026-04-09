/**
 * @file GPS.h
 * @brief GPS数据结构与NMEA解析接口
 */
#ifndef __GPS_H_
#define __GPS_H_

#define uchar unsigned char
#define uint  unsigned int

typedef struct{
	int year;  
	int month; 
	int  day;
	int hour;
	int minute;
	int second;
}DATE_TIME;

typedef  struct{
	double  latitude;  //纬度(NMEA ddmm.mmmm格式)
	double  longitude; //经度(NMEA dddmm.mmmm格式)
	double  latitude_Degree;	//纬度(度格式 dd.dddddd)
	double  longitude_Degree;	//经度(度格式 ddd.dddddd)
	float 	speed;      //速度
	float 	direction;  //航向
	float 	height_ground;    //水平面高度
	float 	height_sea;       //海拔高度
	int     satellite;
	uchar 	NS;
	uchar 	EW;
	DATE_TIME D;
}GPS_INFO;

//void GPS_Init(void);
int GPS_RMC_Parse(char *line,GPS_INFO *GPS);
int GPS_GGA_Parse(char *line,GPS_INFO *GPS);
int GPS_GSV_Parse(char *line,GPS_INFO *GPS);

void Int_To_Str(int x,char *Str);

#endif  //__GPS_H_

