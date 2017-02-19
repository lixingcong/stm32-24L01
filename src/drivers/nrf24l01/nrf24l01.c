#include <nrf24l01.h>
#include "delay.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "stm32f10x_exti.h"
#include "SPI2.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//NRF24L01驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/13
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

//NRF24L01寄存器操作命令
#define NRF_READ_REG    0x00  //读配置寄存器,低5位为寄存器地址
#define NRF_WRITE_REG   0x20  //写配置寄存器,低5位为寄存器地址
#define RD_RX_PLOAD     0x61  //读RX有效数据,1~32字节
#define WR_TX_PLOAD     0xA0  //写TX有效数据,1~32字节
#define FLUSH_TX        0xE1  //清除TX FIFO寄存器.发射模式下用
#define FLUSH_RX        0xE2  //清除RX FIFO寄存器.接收模式下用
#define REUSE_TX_PL     0xE3  //重新使用上一包数据,CE为高,数据包被不断发送.
#define NOP             0xFF  //空操作,可以用来读状态寄存器
//SPI(NRF24L01)寄存器地址
#define CONFIG          0x00  //配置寄存器地址;bit0:1接收模式,0发射模式;bit1:电选择;bit2:CRC模式;bit3:CRC使能;
//bit4:中断MAX_RT(达到最大重发次数中断)使能;bit5:中断TX_DS使能;bit6:中断RX_DR使能
#define EN_AA           0x01  //使能自动应答功能  bit0~5,对应通道0~5
#define EN_RXADDR       0x02  //接收地址允许,bit0~5,对应通道0~5
#define SETUP_AW        0x03  //设置地址宽度(所有数据通道):bit1,0:00,3字节;01,4字节;02,5字节;
#define SETUP_RETR      0x04  //建立自动重发;bit3:0,自动重发计数器;bit7:4,自动重发延时 250*x+86us
#define RF_CH           0x05  //RF通道,bit6:0,工作通道频率;
#define RF_SETUP        0x06  //RF寄存器;bit3:传输速率(0:1Mbps,1:2Mbps);bit2:1,发射功率;bit0:低噪声放大器增益
#define STATUS          0x07  //状态寄存器;bit0:TX FIFO满标志;bit3:1,接收数据通道号(最大:6);bit4,达到最多次重发
//bit5:数据发送完成中断;bit6:接收数据中断;
#define MAX_TX  		0x10  //达到最大发送次数中断
#define TX_OK   		0x20  //TX发送完成中断
#define RX_OK   		0x40  //接收到数据中断

#define OBSERVE_TX      0x08  //发送检测寄存器,bit7:4,数据包丢失计数器;bit3:0,重发计数器
#define CD              0x09  //载波检测寄存器,bit0,载波检测;
#define RX_ADDR_P0      0x0A  //数据通道0接收地址,最大长度5个字节,低字节在前
#define RX_ADDR_P1      0x0B  //数据通道1接收地址,最大长度5个字节,低字节在前
#define RX_ADDR_P2      0x0C  //数据通道2接收地址,最低字节可设置,高字节,必须同RX_ADDR_P1[39:8]相等;
#define RX_ADDR_P3      0x0D  //数据通道3接收地址,最低字节可设置,高字节,必须同RX_ADDR_P1[39:8]相等;
#define RX_ADDR_P4      0x0E  //数据通道4接收地址,最低字节可设置,高字节,必须同RX_ADDR_P1[39:8]相等;
#define RX_ADDR_P5      0x0F  //数据通道5接收地址,最低字节可设置,高字节,必须同RX_ADDR_P1[39:8]相等;
#define TX_ADDR         0x10  //发送地址(低字节在前),ShockBurstTM模式下,RX_ADDR_P0与此地址相等
#define RX_PW_P0        0x11  //接收数据通道0有效数据宽度(1~32字节),设置为0则非法
#define RX_PW_P1        0x12  //接收数据通道1有效数据宽度(1~32字节),设置为0则非法
#define RX_PW_P2        0x13  //接收数据通道2有效数据宽度(1~32字节),设置为0则非法
#define RX_PW_P3        0x14  //接收数据通道3有效数据宽度(1~32字节),设置为0则非法
#define RX_PW_P4        0x15  //接收数据通道4有效数据宽度(1~32字节),设置为0则非法
#define RX_PW_P5        0x16  //接收数据通道5有效数据宽度(1~32字节),设置为0则非法
#define NRF_FIFO_STATUS 0x17  //FIFO状态寄存器;bit0,RX FIFO寄存器空标志;bit1,RX FIFO满标志;bit2,3,保留
//bit4,TX FIFO空标志;bit5,TX FIFO满标志;bit6,1,循环发送上一数据包.0,不循环;
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//24L01操作线
#define NRF24L01_CE   PBout(11) //24L01 RX/TX
#define NRF24L01_CSN  PBout(12) //SPI片选信号
#define NRF24L01_IRQ  PBin(10)  //IRQ主机数据输入
//24L01发送接收数据宽度定义
#define TX_ADR_WIDTH    5   	//5字节的地址宽度
#define RX_ADR_WIDTH    5   	//5字节的地址宽度
#define TX_PLOAD_WIDTH  32  	//32字节的用户数据宽度
#define RX_PLOAD_WIDTH  32  	//32字节的用户数据宽度

const u8 TX_ADDRESS[TX_ADR_WIDTH] = { 0x34, 0x43, 0x10, 0x10, 0x01 };  //发送地址
const u8 RX_ADDRESS[RX_ADR_WIDTH] = { 0x34, 0x43, 0x10, 0x10, 0x01 };
u8 tmp_buf[33], nrf_flag;	//tmp_buf为数据存储区域，nrf_flag为nrf24L01状态寄存器的情况

//初始化24L01的IO口
void NRF24L01_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  //使能复用
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;	//PB12 11推挽
	GPIO_Init(GPIOB, &GPIO_InitStructure);	//初始化指定IO
	GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_11);	//PB12 11下拉

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  //PB10 中断信号 浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//nRF24L01 NVIC配置
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//将GPIO管脚与外部中断线连接
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
	//nRF24L01 EXIT配置
	EXTI_InitStructure.EXTI_Line = EXTI_Line10;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(EXTI_Line10);

	SPI2_Init();    		//初始化SPI

	SPI_Cmd(SPI2, DISABLE);  // SPI外设不使能

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//SPI主机
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		//时钟悬空低
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;	//数据捕获于第1个时钟沿
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS信号由软件控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;		//定义波特率预分频的值:波特率预分频值为16
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRC值计算的多项式
	SPI_Init(SPI2, &SPI_InitStructure);  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

	SPI_Cmd(SPI2, ENABLE);  //使能SPI外设

	NRF24L01_CE = 0; 			//使能24L01
	NRF24L01_CSN = 1;			//SPI片选取消

}
//检测24L01是否存在
//返回值:0，成功;1，失败	
u8 NRF24L01_Check(void) {
	u8 buf[5] = { 0XA5, 0XA5, 0XA5, 0XA5, 0XA5 };
	u8 i;
	SPI2_SetSpeed(SPI_BaudRatePrescaler_4);  //spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）
	NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, buf, 5);  //写入5个字节的地址.
	NRF24L01_Read_Buf(TX_ADDR, buf, 5);  //读出写入的地址
	for (i = 0; i < 5; i++)
		if (buf[i] != 0XA5)
			break;
	if (i != 5)
		return 1;  //检测24L01错误
	return 0;		 //检测到24L01
}
//SPI写寄存器
//reg:指定寄存器地址
//value:写入的值
u8 NRF24L01_Write_Reg(u8 reg, u8 value) {
	u8 status;
	NRF24L01_CSN = 0;                 //使能SPI传输
	status = SPI2_ReadWriteByte(reg);                 //发送寄存器号
	SPI2_ReadWriteByte(value);      //写入寄存器的值
	NRF24L01_CSN = 1;                 //禁止SPI传输
	return (status);       			//返回状态值
}
//读取SPI寄存器值
//reg:要读的寄存器
u8 NRF24L01_Read_Reg(u8 reg) {
	u8 reg_val;
	NRF24L01_CSN = 0;          //使能SPI传输
	SPI2_ReadWriteByte(reg);   //发送寄存器号
	reg_val = SPI2_ReadWriteByte(0XFF);   //读取寄存器内容
	NRF24L01_CSN = 1;          //禁止SPI传输
	return (reg_val);           //返回状态值
}
//在指定位置读出指定长度的数据
//reg:寄存器(位置)
//*pBuf:数据指针
//len:数据长度
//返回值,此次读到的状态寄存器值 
u8 NRF24L01_Read_Buf(u8 reg, u8 *pBuf, u8 len) {
	u8 status, u8_ctr;
	NRF24L01_CSN = 0;           //使能SPI传输
	status = SPI2_ReadWriteByte(reg);           //发送寄存器值(位置),并读取状态值
	for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
		pBuf[u8_ctr] = SPI2_ReadWriteByte(0XFF);           //读出数据
	NRF24L01_CSN = 1;       //关闭SPI传输
	return status;        //返回读到的状态值
}
//在指定位置写指定长度的数据
//reg:寄存器(位置)
//*pBuf:数据指针
//len:数据长度
//返回值,此次读到的状态寄存器值
u8 NRF24L01_Write_Buf(u8 reg, u8 *pBuf, u8 len) {
	u8 status, u8_ctr;
	NRF24L01_CSN = 0;          //使能SPI传输
	status = SPI2_ReadWriteByte(reg);          //发送寄存器值(位置),并读取状态值
	for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
		SPI2_ReadWriteByte(*pBuf++);  //写入数据
	NRF24L01_CSN = 1;       //关闭SPI传输
	return status;          //返回读到的状态值
}
//启动NRF24L01发送一次数据
//txbuf:待发送数据首地址
//返回值:发送完成状况
u8 NRF24L01_TxPacket(u8 *txbuf) {

	SPI2_SetSpeed(SPI_BaudRatePrescaler_8);          //spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);          //写数据到TX BUF  32个字节
	NRF24L01_CE = 1;          //启动发送
	while (nrf_flag > 0)
		;          //等待发送完成
	if (nrf_flag & MAX_TX)          //达到最大重发次数
	{
		NRF24L01_Write_Reg(FLUSH_TX, 0xff);          //清除TX FIFO寄存器
		nrf_flag = 0;						//清除标记
		return MAX_TX;
	}
	if (nrf_flag & TX_OK)						//发送完成
	{
		nrf_flag = 0;			//清除标记
		return TX_OK;
	}
	nrf_flag = 0;					//清除标记
	return 0xff;					//其他原因发送失败
}

//启动NRF24L01发送一次数据
//txbuf:待发送数据首地址
//返回值:0，接收完成；其他，错误代码
//u8 NRF24L01_RxPacket(u8 *rxbuf)
//{
//	u8 sta;		    							   
//	SPI2_SetSpeed(SPI_BaudRatePrescaler_8); //spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）   
//	sta=NRF24L01_Read_Reg(STATUS);  //读取状态寄存器的值    	 
//	NRF24L01_Write_Reg(NRF_WRITE_REG+STATUS,sta); //清除TX_DS或MAX_RT中断标志
//	if(sta&RX_OK)//接收到数据
//	{
//		NRF24L01_Read_Buf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//读取数据
//		NRF24L01_Write_Reg(FLUSH_RX,0xff);//清除RX FIFO寄存器 
//		return 0; 
//	}	   
//	return 1;//没收到任何数据
//}					    
//该函数初始化NRF24L01到RX模式
//设置RX地址,写RX数据宽度,选择RF频道,波特率和LNA HCURR
//当CE变高后,即进入RX模式,并可以接收数据了		   
void NRF24L01_RX_Mode(void) {
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (u8*) RX_ADDRESS, RX_ADR_WIDTH);					//写RX节点地址

	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);    //使能通道0的自动应答
	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);    //使能通道0的接收地址
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);	     //设置RF通信频率
	NRF24L01_Write_Reg(NRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);	     //选择通道0的有效数据宽度
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0f);	     //设置TX发射参数,0db增益,2Mbps,低噪声增益开启
	NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0f);	     //配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式
	SPI2_SetSpeed(SPI_BaudRatePrescaler_8);  //spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）
	NRF24L01_Write_Reg(FLUSH_RX, 0xff);  //清除RX FIFO寄存器
	NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, 0xff);	//***一定要清空状态寄存器，否则会出错。**
	NRF24L01_CE = 1;  //CE为高,进入接收模式
}
//该函数初始化NRF24L01到TX模式
//设置TX地址,写TX数据宽度,设置RX自动应答的地址,填充TX发送数据,选择RF频道,波特率和LNA HCURR
//PWR_UP,CRC使能
//当CE变高后,即进入RX模式,并可以接收数据了		   
//CE为高大于10us,则启动发送.	 
void NRF24L01_TX_Mode(void) {
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, (u8*) TX_ADDRESS, TX_ADR_WIDTH);  //写TX节点地址
	NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (u8*) RX_ADDRESS, RX_ADR_WIDTH);  //设置TX节点地址,主要为了使能ACK

	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);     //使能通道0的自动应答
	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);  //使能通道0的接收地址
	NRF24L01_Write_Reg(NRF_WRITE_REG + SETUP_RETR, 0x1a);  //设置自动重发间隔时间:500us + 86us;最大自动重发次数:10次
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);       //设置RF通道为40
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0f);  //设置TX发射参数,0db增益,2Mbps,低噪声增益开启
	NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0e);    //配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式,开启所有中断
	NRF24L01_Write_Reg(FLUSH_TX, 0xff);    //清除TX FIFO寄存器
	NRF24L01_Write_Reg(FLUSH_RX, 0xff);    //清除RX FIFO寄存器
	NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, 0xff);	//清除状态寄存器
	NRF24L01_CE = 1;	//CE为高,10us后启动发送
}
//nRF24L01中断服务程序
void EXTI9_5_IRQHandler(void) {
	u8 istatus;
	//判断是否是线路6引起的中断
	if (EXTI_GetITStatus(EXTI_Line10) != RESET) {
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == 0) {
			istatus = NRF24L01_Read_Reg(STATUS);            // 读取状态寄存其来判断数据接收状况
			nrf_flag = istatus;
			if (istatus & 0x40)            //bit6:数据接收中断
			{
				NRF24L01_Read_Buf(RD_RX_PLOAD, tmp_buf, RX_PLOAD_WIDTH);            //读取数据
				NRF24L01_Write_Reg(FLUSH_RX, 0xff);            //清除RX FIFO寄存器
			} else if ((istatus & 0x10) > 0)            //达到最大发送次数中断
					{
				NRF24L01_Write_Reg(FLUSH_TX, 0xff);            //清除TX FIFO寄存器
				//NRF24L01_RX_Mode();			//发送结束，转为接收状态。
				//（可以在中断中转为接收状态，也可在NRF24L01_TxPacket之后转为接收状态）。如果不处理达到最大发送次数的情况下。中断中转换更好一点。
			} else if ((istatus & 0x20) > 0)				//TX发送完成中断
					{
				NRF24L01_Write_Reg(FLUSH_TX, 0xff);				//清除TX FIFO寄存器
				//NRF24L01_RX_Mode();			//发送结束，转为接收状态。
			}
			NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, istatus);				//清除状态寄存器
		}
		EXTI_ClearITPendingBit(EXTI_Line6);  //清除标志
//		printf("you!");
	} else {
//		printf("fuck");
	}
}

