#define NRF_GLOBALS

#include "NRF24L01.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "globals.h"

void SPI2_NRF24L01_Init(void);
void RX_Mode(void);
void TX_Mode(void);

/****************************************************************************
 * 名    称：void MODE_CE(unsigned char a)
 * 功    能：NRF24L01 收/发模式有效选择
 * 入口参数：a:  1：NRF24L01 收/发有效   0：关
 * 出口参数：无
 * 说    明：
 * 调用方法：MODE_CE(1);
 ****************************************************************************/
void MODE_CE(unsigned char a) {			            //NRF24L01 MODE-CE
	if (a == 1)
		GPIO_SetBits(GPIOB, GPIO_Pin_11);	    //On
	else
		GPIO_ResetBits(GPIOB, GPIO_Pin_11);			//Off
}

/****************************************************************************
 * 名    称：void SPI2_NRF24L01_Init(void)
 * 功    能：NRF24L01 SPI2接口初始化
 * 入口参数：无
 * 出口参数：无
 * 说    明：
 * 调用方法：SPI2_NRF24L01_Init();
 ****************************************************************************/
void SPI2_NRF24L01_Init(void) {
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);	   //使能SPI2外设时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);    //使能GPIOB 时钟

	/* 配置 SPI2 引脚: SCK, MISO and MOSI（PB13, PB14, PB15) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          //复用功能（推挽）输出  SPI2
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 配置SPI2 NRF24L01+片选  PB12 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		   //输出模式最大速度50MHz
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		   //通用推挽输出模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 配置NRF24L01+ 模式选择  PB11 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;                //NRF24L01  MODE-CE
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		   //输出模式最大速度50MHz
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		   //通用推挽输出模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_11);	    //On

	/* 配置NRF24L01+ 中断信号产生连接到  PB8 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;		 		  //NRF24L01 IRQ
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			  //上拉输入模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//禁止SPI2 NRF24L01+的片选。
	NotSelect_NRF();

	/* SPI2 配置 */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   //全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;						   //主模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;					   //8位
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;						   //时钟极性 空闲状态时，SCK保持低电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;						   //时钟相位 数据采样从第一个时钟边沿开始
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;							   //软件产生NSS
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;  //波特率控制 SYSCLK/16
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;				   //数据高位在前
	SPI_InitStructure.SPI_CRCPolynomial = 7;							   //CRC多项式寄存器初始值为7
	SPI_Init(SPI2, &SPI_InitStructure);

	/* 使能SPI2  */
	SPI_Cmd(SPI2, ENABLE);
}

/****************************************************************************
 * 名    称：unsigned char SPI2_NRF_SendByte(unsigned char byte)
 * 功    能：通过SPI2 发送一个字节的数据。
 * 入口参数：byte： 	发送的数据
 * 出口参数：接收到的字节
 * 说    明：
 * 调用方法：SPI2_NRF_SendByte(data1);
 ****************************************************************************/
unsigned char SPI2_NRF_SendByte(unsigned char byte) {
	/* 循环检测发送缓冲区是否是空 */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
		;

	/* 通过SPI2外设发出数据 */
	SPI_I2S_SendData(SPI2, byte);

	/* 等待接收数据，循环检查接收数据缓冲区 */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
		;

	/* 返回读出的数据 */
	return SPI_I2S_ReceiveData(SPI2);
}

/****************************************************************************
 * 名    称：unsigned char SPI_RW_Reg(unsigned char data1,unsigned char data2)
 * 功    能：通过SPI2 将单字节写入到NRF24L01+指定的寄存器里。
 * 入口参数：data1： 	NRF24L01寄存器
 data2: 	    单字节数据
 * 出口参数：接收到的字节
 * 说    明：
 * 调用方法：SPI_RW_Reg(WRITE_REG1 + EN_AA, 0x3f);
 ****************************************************************************/
unsigned char SPI_RW_Reg(unsigned char data1, unsigned char data2) {
	unsigned int Data = 0;
	Select_NRF();			    			 //选择NRF24L01片选
	Data = SPI2_NRF_SendByte(data1);		 //指定NRF24L01寄存器
	SPI2_NRF_SendByte(data2);				 //写入数据
	NotSelect_NRF(); 						 //禁止NRF24L01片选
	return (Data);							 //返回NRF24L01 写寄存器的状态信息

}

/****************************************************************************
 * 名    称：unsigned char SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes)
 * 功    能：通过SPI2 将数组里的数据写入到NRF24L01+指定的寄存器里。
 * 入口参数：reg： 	NRF24L01寄存器
 pBuf: 	数组
 bytes：	写入的字节数
 * 出口参数：接收到的字节
 * 说    明：
 * 调用方法：SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P0, TX_ADDRESS0, TX_ADR_WIDTH);
 ****************************************************************************/
unsigned char SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes) {
	unsigned char status, byte_ctr;

	Select_NRF();                     //选择NRF24L01片选
	status = SPI2_NRF_SendByte(reg);	  //指定NRF24L01寄存器

	for (byte_ctr = 0; byte_ctr < bytes; byte_ctr++)    //写入指定长度的数据
			{
		SPI2_NRF_SendByte(*pBuf++);
	}
	NotSelect_NRF();                  //禁止NRF24L01片选
	return (status);          		  //返回NRF24L01 写寄存器的状态信息
}

/****************************************************************************
 * 名    称：unsigned char SPI_Read(BYTE reg)
 * 功    能：通过SPI2 将NRF24L01+指定的寄存器里读出一个字节。
 * 入口参数：reg： 	NRF24L01寄存器
 * 出口参数：指定NRF24L01寄存器的状态信息
 * 说    明：
 * 调用方法：status=SPI_Read(READ_REG1+STATUS);
 ****************************************************************************/
unsigned char SPI_Read(BYTE reg) {
	unsigned char Data;
	Select_NRF();						    //选择NRF24L01片选
	SPI2_NRF_SendByte(reg);			    //指定NRF24L01寄存器
	Data = SPI2_NRF_SendByte(0);			//读出数据
	NotSelect_NRF(); 					    //禁止NRF24L01片选
	return (Data);
}

/****************************************************************************
 * 名    称：unsigned char SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes)
 * 功    能：通过SPI2 将NRF24L01+指定的寄存器里的数据读出指定长度到指定的数组里。
 * 入口参数：reg： 	NRF24L01寄存器
 pBuf：  数组
 bytes： 长度
 * 出口参数：指定NRF24L01寄存器的状态信息
 * 说    明：
 * 调用方法：SPI_Read_Buf(RD_RX_PLOAD,rx_buf,TX_PLOAD_WIDTH);
 ****************************************************************************/
unsigned char SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes) {
	unsigned char status, i;

	Select_NRF();              			//选择NRF24L01片选
	status = SPI2_NRF_SendByte(reg);	   	//读出指定NRF24L01寄存器的状态信息
	for (i = 0; i < bytes; i++)              //读出指定长度的数据
			{
		pBuf[i] = SPI2_NRF_SendByte(0);
	}
	NotSelect_NRF();                    //禁止NRF24L01片选
	return (status);          		    //返回指定NRF24L01寄存器的状态信息
}

/****************************************************************************
 * 名    称：delay_ms(unsigned int x)
 * 功    能：延时基数为1毫秒程序
 * 入口参数：x   延时的毫秒数
 * 出口参数：无
 * 说    明：无
 * 调用方法：delay_ms(1);
 ****************************************************************************/
void delay_ms(unsigned int x) {
	unsigned int i, j;
	i = 0;
	for (i = 0; i < x; i++) {
		j = 108;
		while (j--)
			;
	}
}

/****************************************************************************
 * 名    称：RX_Mode(void)
 * 功    能：设置NRF24L01+的接收模式
 * 入口参数：无
 * 出口参数：无
 * 说    明：设置了6个接收通道地址，数据宽度32、接收自动应答、6个接收通道使能、
 *			射频频道0、16位CRC、收发中断、增益0dB等等
 * 调用方法：RX_Mode();
 ****************************************************************************/

void RX_Mode(void) {
	unsigned char i;

	nrf_baud = 0;							//默认速率2Mbps

	TX_ADDRESS_LOCAL[0] = 0x34;	            //通道0 发射地址
	TX_ADDRESS_LOCAL[1] = 0x43;
	TX_ADDRESS_LOCAL[2] = 0x10;
	TX_ADDRESS_LOCAL[3] = 0x10;
	TX_ADDRESS_LOCAL[4] = 0x01;

	TX_ADDRESS_DUMMY[0] = 0x01;				//通道1~5 发射地址（程序里面没有用到）
	TX_ADDRESS_DUMMY[1] = 0xE1;
	TX_ADDRESS_DUMMY[2] = 0xE2;
	TX_ADDRESS_DUMMY[3] = 0xE3;
	TX_ADDRESS_DUMMY[4] = 0x02;

	MODE_CE(0);

	// 数据通道0
	SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P0, TX_ADDRESS_LOCAL, TX_ADR_WIDTH);  //数据通道0接收地址，最大5个字节， 此处接收地址和发送地址相同
	SPI_RW_Reg(WRITE_REG1 + RX_PW_P0, TX_PLOAD_WIDTH);  // 接收数据通道0有效数据宽度32   范围1-32

	// 数据通道1-5
	for (i = 0; i < 5; i++) {
		if (i == 0) {
			//数据通道1接收地址 5字节
			SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P1 + i, TX_ADDRESS_DUMMY, TX_ADR_WIDTH);
		} else {
			//数据通道i+1接收地址，只可以设置1个字节， 高字节与TX_ADDRESS_DUMMY[39:8]相同
			SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P1 + i, TX_ADDRESS_DUMMY, 1);
		}
		// 接收数据通道i+1有效数据宽度32   范围1-32
		SPI_RW_Reg(WRITE_REG1 + RX_PW_P1 + i, TX_PLOAD_WIDTH);
	}

	SPI_RW_Reg(WRITE_REG1 + EN_AA, 0x00);      // 使能通道0-通道5接收关闭自动应答
	SPI_RW_Reg(WRITE_REG1 + EN_RXADDR, 0x01);  // 接收通道0使能，关闭其他通道
	SPI_RW_Reg(WRITE_REG1 + RF_CH, 0);         // 选择射频工作频道0   范围0-127

	if (nrf_baud == 0)
		SPI_RW_Reg(WRITE_REG1 + RF_SETUP, 0x0f);   // 0db, 2MPS   射频寄存器   无线速率bit5:bit3		   发射功率bit2-bit1
												   //                           00: 1M BPS	                 00:-18dB
												   //                           01: 2M BPS	                 01:-12dB
												   //                           10: 250K BPS	             10:-6dB
												   //                           11：保留                     11:0dB

	else
		SPI_RW_Reg(WRITE_REG1 + RF_SETUP, 0x07);   // 0db, 1MPS

	SPI_RW_Reg(WRITE_REG1 + CONFIG, 0x0f);     // bit6 接收中断产生时，IRQ引脚产生低电平
											   // bit5 发送中断产生时，IRQ引脚产生低电平
											   // bit4 最大重复发送次数完成时 IRQ引脚产生低电平
											   // bit3 CRC校验允许
											   // bit2 16位CRC
											   // bit1 上电
											   // bit0 接收模式
	MODE_CE(1);								   // 使能接收模式
}

/****************************************************************************
 * 名    称：TX_Mode(void)
 * 功    能：设置NRF24L01+的发送模式
 * 入口参数：无
 * 出口参数：无
 * 说    明：设置了6个发射通道地址、射频频道0、16位CRC、收发中断、增益0dB等等
 * 调用方法：TX_Mode();
 ****************************************************************************/
void TX_Mode(void) {
	NotSelect_NRF();
	MODE_CE(0);

	// TODO: 这是指定六个通道互相发送的关键 2017年2月26日 下午5:33:38
	nrf_Pipe = 0;
	
	SPI_RW_Reg(WRITE_REG1 + SETUP_RETR, 0x00);  // 关闭自动重发

	switch (nrf_Pipe) {
		case 0:
			SPI_Write_Buf(WRITE_REG1 + TX_ADDR + nrf_Pipe, TX_ADDRESS_LOCAL, TX_ADR_WIDTH);         //数据通道0发送地址，最大5个字节
			SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P0 + nrf_Pipe, TX_ADDRESS_LOCAL, TX_ADR_WIDTH);  // 将通道0的接收地址设置为 0通道的发射地址
			break;
		default:
			SPI_Write_Buf(WRITE_REG1 + TX_ADDR + nrf_Pipe, TX_ADDRESS_DUMMY, TX_ADR_WIDTH);    //数据通道nrf_pipe发送地址，最大5个字节
			SPI_Write_Buf(WRITE_REG1 + RX_ADDR_P0 + nrf_Pipe, TX_ADDRESS_DUMMY, TX_ADR_WIDTH); // 将通道nrf_pipe的接收地址设置为dummy的发射地址
			break;
	}

	SPI_RW_Reg(WRITE_REG1 + CONFIG, 0x0e);     // bit6 接收中断产生时，IRQ引脚产生低电平
											   // bit5 发送中断产生时，IRQ引脚产生低电平
											   // bit4 最大重复发送次数完成时 IRQ引脚产生低电平
											   // bit3 CRC校验允许
											   // bit2 16位CRC
											   // bit1 上电
											   // bit0 发送模式

	MODE_CE(1);								   // 使能发送模式
	
}

/****************************************************************************
 * 名    称：NRF_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes)
 * 功    能：将保存在USB接收缓存区的32字节的数据通过NRF24L01+发送出去
 * 入口参数：data_buffer   待发送数据
 Nb_bytes	  待发送数据长度
 * 出口参数：无
 * 说    明：数据小于32，把有效数据外的空间用0填满
 * 调用方法：RX_Mode();
 ****************************************************************************/
void NRF_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes) {
	uchar i = 0;
	MODE_CE(0);								 //NRF 模式控制

	SPI_RW_Reg(WRITE_REG1 + STATUS, 0xff);	     //设置状态寄存器初始化
	SPI_RW_Reg(0xe1, 0);						 //清除TX FIFO寄存器
	SPI_RW_Reg(0xe2, 0);		    			 //清除RX FIFO寄存器
	TX_Mode();								 //设置为发送模式
	delay_ms(1);
	if (Nb_bytes < 32) {						 //当接收到的USB虚拟串口数据小于32，把有效数据外的空间用0填满
		for (i = Nb_bytes; i < 32; i++)
			data_buffer[i] = 0;
	}
	MODE_CE(0);
	SPI_Write_Buf(WR_TX_PLOAD, data_buffer, TX_PLOAD_WIDTH);        //发送32字节的缓存区数据到NRF24L01
	MODE_CE(1);														//保持10us以上，将数据发送出去
}

