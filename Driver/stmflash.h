/**
 * @file stmflash.h
 * @brief STM32内部Flash读写驱动接口
 *
 * Flash容量: 256KB, 起始地址: 0x08000000
 * 扇区大小: 2048字节（对256KB及以上型号）
 */
#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "sys.h"  

#define STM32_FLASH_SIZE 256 	 		// Flash容量(KB)
#define STM32_FLASH_WREN 1              // 使能Flash写入(0=禁止, 1=使能)

#define STM32_FLASH_BASE 0x08000000 	// STM32 Flash起始地址
 
 

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据

//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif

















