/**
 * @file stmflash.c
 * @brief STM32内部Flash读写驱动
 *
 * 适用芯片: STM32F103 (256KB Flash, 扇区大小2048字节)
 * Flash地址空间: 0x08000000 ~ 0x0803FFFF
 * 写入策略: 先读整扇区→检查是否需擦除→合并新数据→回写
 */
#include "stmflash.h"
#include "delay.h"
 
/**
 * @brief 读取指定地址的半字(16位)
 * @param faddr 读取地址（必须2字节对齐）
 * @return 该地址存储的16位数据
 */
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}
#if STM32_FLASH_WREN	//如果使能了写   
/**
 * @brief 不检查直接写入（调用前必须确保目标区域已擦除，即全为0xFFFF）
 * @param WriteAddr 起始地址
 * @param pBuffer   数据指针
 * @param NumToWrite 写入的半字数量
 */
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i=0;i<NumToWrite;i++)
	{
		FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
	    WriteAddr+=2;//地址增加2.
	}  
} 
/**
 * @brief 从指定地址写入指定长度数据（带擦除保护）
 * @param WriteAddr  起始地址（必须2字节对齐）
 * @param pBuffer    数据指针
 * @param NumToWrite 写入的半字数量
 * @note  内部会自动处理跨扇区写入，每扇区2048字节(1024半字)
 */
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 // 小容量Flash扇区1KB
#else 
#define STM_SECTOR_SIZE	2048 // 256KB及以上Flash扇区2KB
#endif		 
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2]; // 扇区缓冲（用于擦除前保存原有数据）
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)	
{
	u32 secpos;	   //扇区地址
	u16 secoff;	   //扇区内偏移地址(16位字计算)
	u16 secremain; //扇区内剩余地址(16位字计算)	   
 	u16 i;    
	u32 offaddr;   //去掉0X08000000后的地址
	if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return; // 地址超出Flash范围
	FLASH_Unlock();

	/* ① 计算目标地址在Flash中的扇区位置和偏移 */
	offaddr=WriteAddr-STM32_FLASH_BASE;       // 相对0x08000000的偏移
	secpos=offaddr/STM_SECTOR_SIZE;            // 所在扇区编号
	secoff=(offaddr%STM_SECTOR_SIZE)/2;        // 扇区内偏移（以半字为单位）
	secremain=STM_SECTOR_SIZE/2-secoff;        // 本扇区剩余空间（半字）
	if(NumToWrite<=secremain)secremain=NumToWrite;//不大于该扇区范围
	while(1) 
	{
		/* ② 读出当前扇区全部内容到缓冲区 */
		STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);
		/* ③ 检查目标区域是否已擦除（0xFFFF表示已擦） */
		for(i=0;i<secremain;i++)
		{
			if(STMFLASH_BUF[secoff+i]!=0XFFFF)break; // 非0xFFFF则需擦除
		}
		if(i<secremain) // 需要擦除
		{
			/* ④ 擦除整个扇区，合并新数据后回写 */
			FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);
			for(i=0;i<secremain;i++)//复制
			{
				STMFLASH_BUF[i+secoff]=pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//写入整个扇区  
		}else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain); // 目标区域已擦除，直接写入
		if(NumToWrite==secremain)break; // 写入完成
		else // 未写完，进入下一个扇区继续
		{
			secpos++;				//扇区地址增1
			secoff=0;				//偏移位置为0 	 
		   	pBuffer+=secremain;  	//指针偏移
			WriteAddr+=secremain;	//写地址偏移	   
		   	NumToWrite-=secremain;	//字节(16位)数递减
			if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//下一个扇区还是写不完
			else secremain=NumToWrite;//下一个扇区可以写完了
		}	 
	};	
	FLASH_Lock();//上锁
}
#endif

/**
 * @brief 从指定地址读取指定长度数据
 * @param ReadAddr   起始地址
 * @param pBuffer    数据输出缓冲区
 * @param NumToRead  读取的半字数量
 */
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

/**
 * @brief 测试写入（写入单个半字的便捷接口）
 * @param WriteAddr  目标地址
 * @param WriteData  要写入的数据
 */
void Test_Write(u32 WriteAddr,u16 WriteData)   	
{
	STMFLASH_Write(WriteAddr,&WriteData,1);//写入一个字 
}
















