#include "SPI1.h"

#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "misc.h"


void SPI1_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	// spi 配置结构体
	SPI_InitTypeDef SPI_InitStructure;

	/*enable gpioa clock*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	/*configure sck miso mosi*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	/*configure cs*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	/*configure wtr*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_InitStructure);	
	/*enable spi clock*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);
	/*configure spi1 parameters*/
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	// TODO: SPI速率 恢复8分频 2016年8月2日 上午10:49:35
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //降低速率便于逻辑分析仪采集，原来是8分频的
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1,&SPI_InitStructure);
	/*enable spi1*/
	SPI_Cmd(SPI1, ENABLE);//使能spi外设
}


void EXTI_config_for_A7190(){
	// 外部中断结构体
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	/*extend interupt configuration*/	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource8);
	/* Configure EXTI Line9 to generate an interrupt on falling edge */  
	// PA8中断: A7190的GIO2管脚，当有RX活动时将会低电平
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling ;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	/* Enable the EXTI9_5 Interrupt */
	// TODO: 关注无线接收和串口中断的优先级顺序，我觉得应该是无线优先级高于串口 2016年7月27日 下午3:37:05
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
}


unsigned char SPI1_ReadWriteByte(unsigned char TxData) {
	unsigned char retry = 0;
	while ((SPI1->SR & 1 << 1) == 0)  //等待发送区空脮
	{
		retry++;
		if (retry > 200)
			return 0;
	}
	SPI1->DR = TxData;	 	  //发送一个字节
	retry = 0;
	while ((SPI1->SR & 1 << 0) == 0)  //等待接收完一个byte
	{
		retry++;
		if (retry > 200)
			return 0;
	}
	return SPI1->DR;          //返回收到的数据
}

unsigned char SPI1_ReadByte(unsigned char TxData) {
	unsigned char retry = 0;
	while ((SPI1->SR & 1 << 0) == 0)
	{
		retry++;
		if (retry > 200)
			return 0;
	}
	return SPI1->DR;
}

