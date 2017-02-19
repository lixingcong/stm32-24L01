#ifndef __24L01_H
#define __24L01_H	 		  
#include "sys.h"   
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板V3
//NRF24L01驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/17
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////
extern u8 tmp_buf[33];		//全局变量，中断中需要读取数据需要放入全局变量中 								   	   
extern u8 nrf_flag;			//状态标记

void NRF24L01_Init(void);						//初始化
void NRF24L01_RX_Mode(void);					//配置为接收模式
void NRF24L01_TX_Mode(void);					//配置为发送模式
u8 NRF24L01_Write_Buf(u8 reg, u8 *pBuf, u8 u8s);					//写数据区
u8 NRF24L01_Read_Buf(u8 reg, u8 *pBuf, u8 u8s);  //读数据区
u8 NRF24L01_Read_Reg(u8 reg);					//读寄存器
u8 NRF24L01_Write_Reg(u8 reg, u8 value);		//写寄存器
u8 NRF24L01_Check(void);						//检查24L01是否存在
u8 NRF24L01_TxPacket(u8 *txbuf);				//发送一个包的数据
//u8 NRF24L01_RxPacket(u8 *rxbuf);				//接收一个包的数据
#endif

