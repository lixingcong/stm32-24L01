#include "NRF24L01.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "stdio.h" // printf

#include "NRF_api.h"
#include "hal.h"

#include "route_table.h"

// registers
#define NRF_READ_REG        0x00  // Define read command to register
#define NRF_WRITE_REG       0x20  // Define write command to register
#define NRF_RD_RX_PLOAD     0x61   // Define RX payload register address
#define NRF_WR_TX_PLOAD     0xA0   // Define TX payload register address
#define NRF_FLUSH_TX        0xE1   // Define flush TX register command
#define NRF_FLUSH_RX        0xE2   // Define flush RX register command
#define NRF_REUSE_TX_PL     0xE3   // Define reuse TX payload register command
#define NRF_NOP             0xFF   // Define No Operation, might be used to read status register

//***************************************************//
// SPI(nRF24L01) registers(addresses)
#define NRF_CONFIG          0x00   // 'Config' register address
#define NRF_EN_AA           0x01   // 'Enable Auto Acknowledgment' register address
#define NRF_EN_RXADDR       0x02   // 'Enabled RX addresses' register address
#define NRF_SETUP_AW        0x03   // 'Setup address width' register address
#define NRF_SETUP_RETR      0x04   // 'Setup Auto. Retrans' register address
#define NRF_RF_CH           0x05   // 'RF channel' register address
#define NRF_RF_SETUP        0x06   // 'RF setup' register address
#define NRF_STATUS          0x07  // 'Status' register address
#define NRF_OBSERVE_TX      0x08  // 'Observe TX' register address
#define NRF_CD              0x09  // 'Carrier Detect' register address
#define NRF_RX_ADDR_P0      0x0A  // 'RX address pipe0' register address
#define NRF_RX_ADDR_P1      0x0B  // 'RX address pipe1' register address
#define NRF_RX_ADDR_P2      0x0C  // 'RX address pipe2' register address
#define NRF_RX_ADDR_P3      0x0D  // 'RX address pipe3' register address
#define NRF_RX_ADDR_P4      0x0E  // 'RX address pipe4' register address
#define NRF_RX_ADDR_P5      0x0F  // 'RX address pipe5' register address
#define NRF_TX_ADDR         0x10  // 'TX address' register address
#define NRF_RX_PW_P0        0x11  // 'RX payload width, pipe0' register address
#define NRF_RX_PW_P1        0x12  // 'RX payload width, pipe1' register address
#define NRF_RX_PW_P2        0x13  // 'RX payload width, pipe2' register address
#define NRF_RX_PW_P3        0x14  // 'RX payload width, pipe3' register address
#define NRF_RX_PW_P4        0x15  // 'RX payload width, pipe4' register address
#define NRF_RX_PW_P5        0x16  // 'RX payload width, pipe5' register address
#define NRF_FIFO_STATUS     0x17  // 'FIFO Status Register' register address

// set by user, address width and payload len
#define NRF_ADDR_WIDTH    4   //  TX(RX) address width

#define Select_NRF()     GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define NotSelect_NRF()    GPIO_SetBits(GPIOB, GPIO_Pin_12)

// 最好设定地址宽度为4字节 刚好能32位整数存下来
static unsigned char TX_ADDRESS_LOCAL[NRF_ADDR_WIDTH] = { 0x12, 0x34, 0x56, 0x78 };
static unsigned char TX_ADDRESS_DUMMY[NRF_ADDR_WIDTH] = { 0xee, 0xee, 0xff, 0xff };

static unsigned char NRF_SPI_SendByte(unsigned char byte);
static unsigned char NRF_SPI_Read(BYTE reg);
static unsigned char NRF_SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);
static unsigned char NRF_SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);
static unsigned char NRF_SPI_RW_Reg(BYTE data1, BYTE data2);
static void delay_ms(unsigned int x);
static unsigned char NRF_check_if_exist(void);
static void NRF_RX_Mode(void);
static void NRF_TX_Mode(void);

static unsigned char rx_buf[NRF_PLOAD_LENGTH];

/****************************************************************************
 * 名    称：void MODE_CE(unsigned char a)
 * 功    能：NRF24L01 收/发模式有效选择
 * 入口参数：a:  1：NRF24L01 收/发有效   0：关
 * 出口参数：无
 * 说    明：
 * 调用方法：MODE_CE(1);
 ****************************************************************************/
void NRF_MODE_CE(unsigned char a) {			            //NRF24L01 MODE-CE
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
void NRF24L01_Init(void) {
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	unsigned char i;
	unsigned char nrf_baud = 1;				//速率设置：0为2Mbps，1为1Mbps

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);	   //使能SPI2外设时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);    //使能GPIOB 时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    //使能端口复用时钟

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

	/* 配置NRF24L01+ 中断信号 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;		 		  //NRF24L01 IRQ
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			  //上拉输入模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;					//NRF24L01 中断响应
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		    //抢占优先级 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;				//子优先级为1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能
	NVIC_Init(&NVIC_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);	   //NRF24L01 IRQ

	EXTI_InitStructure.EXTI_Line = EXTI_Line7;					   //NRF24L01 IRQ
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			   //EXTI中断
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		   //下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;						   //使能
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(EXTI_Line7);

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

	// 以下是公用的24L01初始化
	NRF_MODE_CE(0);

	// 检查24L01是否存在
	while(NRF_check_if_exist()==0){
		printf("24L01 error\n");
		delay_ms(500);
	}

	// 复位24L01，避免stm32芯片复位后，无法发送接收数据
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_CONFIG,0x00); // use power down mode (PWR_UP = 0)
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_STATUS,0x7f); // clear data ready flag and data sent flag in status register
	NRF_SPI_RW_Reg(NRF_FLUSH_TX, 0); // flush tx buffer
	NRF_SPI_RW_Reg(NRF_FLUSH_RX, 0); // flush rx buffer

	// 设置地址宽度
#if NRF_ADDR_WIDTH == 3
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_SETUP_AW, 0x01);
#elif NRF_ADDR_WIDTH == 4
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_SETUP_AW, 0x02);
#else
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_SETUP_AW, 0x03);
#endif

	// 发送部分
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_SETUP_RETR, 0x00);  // 关闭自动重发

	// 接收部分
	// 数据通道0
	NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_RX_ADDR_P0, TX_ADDRESS_LOCAL, NRF_ADDR_WIDTH);  //数据通道0接收地址，最大5个字节， 此处接收地址和发送地址相同
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_RX_PW_P0, NRF_PLOAD_LENGTH);  // 接收数据通道0有效数据宽度32   范围1-32

	// 数据通道1-5
	for (i = 0; i < 5; i++) {
		if (i == 0) {
			//数据通道1接收地址 5字节
			NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_RX_ADDR_P1 + i, TX_ADDRESS_DUMMY, NRF_ADDR_WIDTH);
		} else {
			//数据通道i+1接收地址，只可以设置1个字节， 高字节与TX_ADDRESS_DUMMY[39:8]相同
			NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_RX_ADDR_P1 + i, TX_ADDRESS_DUMMY, 1);
		}
		// 接收数据通道i+1有效数据宽度32   范围1-32
		NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_RX_PW_P1 + i, NRF_PLOAD_LENGTH);
	}

	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_EN_AA, 0x00);      // 使能通道0-通道5接收关闭自动应答
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_EN_RXADDR, 0x01);  // 接收通道0使能，关闭其他通道
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_RF_CH, 0);         // 选择射频工作频道0   范围0-127

	if (nrf_baud == 0)
		NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_RF_SETUP, 0x0f);   // 0db, 2MPS   射频寄存器   无线速率bit5:bit3		   发射功率bit2-bit1
												   //                           00: 1M BPS	                 00:-18dB
												   //                           01: 2M BPS	                 01:-12dB
												   //                           10: 250K BPS	             10:-6dB
												   //                           11：保留                     11:0dB

	else
		NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_RF_SETUP, 0x07);   // 0db, 1MPS

	// nrf24l01 enter to recv mode
	NRF_RX_Mode();
}

/****************************************************************************
 * 名    称：unsigned char SPI2_NRF_SendByte(unsigned char byte)
 * 功    能：通过SPI2 发送一个字节的数据。
 * 入口参数：byte： 	发送的数据
 * 出口参数：接收到的字节
 * 说    明：
 * 调用方法：SPI2_NRF_SendByte(data1);
 ****************************************************************************/
static unsigned char NRF_SPI_SendByte(unsigned char byte) {
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
static unsigned char NRF_SPI_RW_Reg(unsigned char data1, unsigned char data2) {
	unsigned int Data = 0;
	Select_NRF();			    			 //选择NRF24L01片选
	Data = NRF_SPI_SendByte(data1);		 //指定NRF24L01寄存器
	NRF_SPI_SendByte(data2);				 //写入数据
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
static unsigned char NRF_SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes) {
	unsigned char status, byte_ctr;

	Select_NRF();                     //选择NRF24L01片选
	status = NRF_SPI_SendByte(reg);	  //指定NRF24L01寄存器

	for (byte_ctr = 0; byte_ctr < bytes; byte_ctr++)    //写入指定长度的数据
			{
		NRF_SPI_SendByte(*pBuf++);
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
static unsigned char NRF_SPI_Read(BYTE reg) {
	unsigned char Data;
	Select_NRF();						    //选择NRF24L01片选
	NRF_SPI_SendByte(reg);			    //指定NRF24L01寄存器
	Data = NRF_SPI_SendByte(0);			//读出数据
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
static unsigned char NRF_SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes) {
	unsigned char status, i;

	Select_NRF();              			//选择NRF24L01片选
	status = NRF_SPI_SendByte(reg);	   	//读出指定NRF24L01寄存器的状态信息
	for (i = 0; i < bytes; i++)              //读出指定长度的数据
			{
		pBuf[i] = NRF_SPI_SendByte(0);
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
static void delay_ms(unsigned int x) {
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

static void NRF_RX_Mode(void) {
	NRF_MODE_CE(0);

	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_STATUS, 0xff);	     //设置状态寄存器初始化
	NRF_SPI_RW_Reg(NRF_FLUSH_RX, 0);		//清除缓冲区
	NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_RX_ADDR_P0, TX_ADDRESS_LOCAL, NRF_ADDR_WIDTH);  // 将通道0的接收地址设置为 0通道的发射地址
	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_CONFIG, 0x0f);     // bit6 接收中断产生时，IRQ引脚产生低电平
											   // bit5 发送中断产生时，IRQ引脚产生低电平
											   // bit4 最大重复发送次数完成时 IRQ引脚产生低电平
											   // bit3 CRC校验允许
											   // bit2 16位CRC
											   // bit1 上电
											   // bit0 接收模式
	NRF_MODE_CE(1);								   // 使能接收模式
}

/****************************************************************************
 * 名    称：TX_Mode(void)
 * 功    能：设置NRF24L01+的发送模式
 * 入口参数：无
 * 出口参数：无
 * 说    明：设置了6个发射通道地址、射频频道0、16位CRC、收发中断、增益0dB等等
 * 调用方法：TX_Mode();
 ****************************************************************************/
static void NRF_TX_Mode(void) {
	NRF_MODE_CE(0);

	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_STATUS, 0xff);	     //设置状态寄存器初始化
	NRF_SPI_RW_Reg(NRF_FLUSH_TX, 0);		//清除发送缓冲区

	NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_TX_ADDR, TX_ADDRESS_LOCAL, NRF_ADDR_WIDTH);         //数据通道0发送地址，最大5个字节

	NRF_SPI_RW_Reg(NRF_WRITE_REG + NRF_CONFIG, 0x0e);     // bit6 接收中断产生时，IRQ引脚产生低电平
											   // bit5 发送中断产生时，IRQ引脚产生低电平
											   // bit4 最大重复发送次数完成时 IRQ引脚产生低电平
											   // bit3 CRC校验允许
											   // bit2 16位CRC
											   // bit1 上电
											   // bit0 发送模式

	NRF_MODE_CE(1);								   // 使能发送模式
	
}

/****************************************************************************
 * 名    称：NRF_Send_Data(BYTE* data_buffer, BYTE Nb_bytes)
 * 功    能：将保存在USB接收缓存区的32字节的数据通过NRF24L01+发送出去
 * 入口参数：data_buffer   待发送数据
 Nb_bytes	  待发送数据长度
 * 出口参数：无
 * 说    明：数据小于32，把有效数据外的空间用0填满
 * 调用方法：RX_Mode();
 ****************************************************************************/
void NRF_Send_Data(BYTE* data_buffer, unsigned short Nb_bytes) {
	unsigned char i = 0;
	static unsigned char nrf_tx_buf[NRF_PLOAD_LENGTH];

	NRF_MODE_CE(0);								 //NRF 模式控制

	NRF_TX_Mode();								 //设置为发送模式

	for (i = 0; i < NRF_PLOAD_LENGTH; ++i) {
		if (i < Nb_bytes)
			nrf_tx_buf[i] = *(data_buffer + i);
		else
			nrf_tx_buf[i] = 0;  //当接收到的USB虚拟串口数据小于32，把有效数据外的空间用0填满
	}
	NRF_MODE_CE(0);
	NRF_SPI_Write_Buf(NRF_WR_TX_PLOAD, nrf_tx_buf, NRF_PLOAD_LENGTH);        //发送32字节的缓存区数据到NRF24L01
	NRF_MODE_CE(1);														//保持10us以上，将数据发送出去
}

//检测24L01是否存在
//返回值:1: 存在 ;0 不存在
static unsigned char NRF_check_if_exist(void) {
	unsigned char buf[5] = { 0XA4, 0XA4, 0XA4, 0XA4, 0XA4 };
	unsigned char i;

	NRF_SPI_Write_Buf(NRF_WRITE_REG + NRF_TX_ADDR, buf, NRF_ADDR_WIDTH);  //写入地址.
	NRF_SPI_Read_Buf(NRF_TX_ADDR, buf, NRF_ADDR_WIDTH);  //读出写入的地址

	for (i = 0; i < NRF_ADDR_WIDTH; i++){
		if (buf[i] != 0XA4)
			return 0; //检测24L01错误
	}

	return 1;		 //检测到24L01
}

// 中断接收处理函数
void NRF_interupt_handler(void){
	unsigned char flen_h, flen_l, i;
	unsigned short total_flen;
	unsigned char status;

	status = NRF_SPI_Read(NRF_READ_REG + NRF_STATUS);				// 读取状态寄存其来判断数据接收状况
	if (0 != (status & 0x40)){		// 判断是否接收到数据
		if(NRF_STATE_IDLE != NRF_read_state()){
			goto flush_rx;
		}

		NRF_set_state(NRF_STATE_BUSY_RX);
		NRF_SPI_Read_Buf(NRF_RD_RX_PLOAD, rx_buf, NRF_PLOAD_LENGTH);  //从接收缓冲区里读出数据

		// printf("%s", rx_buf);
		flen_h = rx_buf[0];
		flen_l = rx_buf[1]; //read the length(LSB 8 bit)

		for(i=0;i<rx_buf[1];++i)
			 fprintf(stderr,"%x ",rx_buf[i]);
		 fprintf(stderr,"\r\n");

		if ((flen_h & 0xfe) == 0xf0) {  // long
			total_flen = ((flen_h & 0x01) << 8) | flen_l;
			macRxCustomPacketCallback(rx_buf, FALSE, total_flen);
		} else if ((flen_h & 0xff) == 0x00) {  // short
			macRxCustomPacketCallback(rx_buf, TRUE, flen_l);
		} else {
			printf("invalid packet, flush it\n"); // drop invalid packet
		}

		NRF_set_state(NRF_STATE_IDLE);

		flush_rx:
		NRF_SPI_RW_Reg(NRF_FLUSH_RX, 0);		//清除缓冲区
		NRF_SPI_RW_Reg(NRF_WRITE_REG+NRF_STATUS, status);	     //清除07寄存器标志
	} else if (status & 0x10) {				    //发射达到最大复发次数（在自动答复模式下）
		NRF_SPI_RW_Reg(NRF_FLUSH_TX, 0);		//清除发送缓冲区
		NRF_RX_Mode();							//进入接收模式
		NRF_set_state(NRF_STATE_IDLE);
	} else if (status & 0x20) {					//数据发送完毕
		NRF_SPI_RW_Reg(NRF_FLUSH_TX, 0);		//清除发送缓冲区
		NRF_RX_Mode();							//进入接收模式
		NRF_set_state(NRF_STATE_IDLE);
	}
}
