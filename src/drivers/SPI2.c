#include "SPI2.h"

#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "misc.h"

// Description:spi程序
// File Name: SPI2.c
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

/*
 * 函数名：SPI2_Config
 * 描述  ：SPI2初始化
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
void SPI2_Config(void)
{
	/*定义一个SPI_InitTypeDef类型的结构体*/
	SPI_InitTypeDef SPI_InitStructure;
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能外设时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	/* 配置时钟 SPI2_SCK ---PB13 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 配置mcu输入 SPI2_MISO---PB14 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 配置mcu输出 SPI2_MOSI---PB15 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 配置芯片片选 SPI2_NSS ---PB12 */
	GPIO_InitStructure.GPIO_Pin = SPI2_NSS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* SPI2 配置 */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;//SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;//主机模式
	//PI_InitStructure.SPI_Mode = SPI_Mode_Slave;//从机模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;//16位数据模式
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;//时钟悬空高
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;//捕获第2个时钟沿
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//自己控制片选
	//SPI2连接在低速外设时钟APB1上,最大频率为36MHz,
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;//时钟分频设置
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;//数据传输从MSB位(高位)开始
	SPI_InitStructure.SPI_CRCPolynomial = 7;//CRC校验配置,实际没有启用
	SPI_Init(SPI2, &SPI_InitStructure);

	/* SPI2中断配置  */
	//SPI2_IRQConfig();

	/* 使能 SPI2  */
	SPI_Cmd(SPI2, ENABLE);

	SPI2_CS_HIGH();
}
