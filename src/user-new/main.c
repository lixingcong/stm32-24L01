/****************************************************************************
 * 文件名: main.c
 * 内容简述:
 *       演示 USART1 当接收到结束符是0x0d 0x0a时，将接收到的数据传送出去
 V6（V3板） V2（MINI板） LED1闪烁表示正在收发数据。
 *
 定义：
 TXD1----- PA9-US1-TX
 RXD1----- PA10-US1-RX
 速率：9600,n,8,1
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include "NRF24L01.h"

uint8_t TxBufferRF[40];
uint8_t RxBufferRF[40];
uint8_t rf_rec_flag, rf_tx_flag;

/* */
extern void SPI2_NRF24L01_Init(void);
extern void RX_Mode(void);
extern void TX_Mode(void);

/* Private variables ---------------------------------------------------------*/
uint8_t TxBufferUSART[40];
uint8_t RxBufferUSART[40];
uint8_t usart_rec_flag, usart_tx_flag, i;
__IO uint8_t TxCounter1 = 0x00;
__IO uint8_t RxCounter1 = 0x00;

uint32_t Rec_Len;

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);

void Delay(__IO uint32_t nCount);
void USART_Config(USART_TypeDef* USARTx);
void USART_SendChar(USART_TypeDef* USARTx, uint8_t data);  //发送一个char

//检测24L01是否存在
//返回值:0，成功;1，失败
unsigned char NRF24L01_Check(void) {
	unsigned char buf[5] = { 0XA4, 0XA4, 0XA4, 0XA4, 0XA4 };
	unsigned char i;
	SPI_Write_Buf(WRITE_REG1 + TX_ADDR, buf, 5);  //写入5个字节的地址.
	SPI_Read_Buf(TX_ADDR, buf, 5);  //读出写入的地址
	for (i = 0; i < 5; i++)
		if (buf[i] != 0XA4)
			break;
	if (i != 5)
		return 1;  //检测24L01错误
	return 0;		 //检测到24L01
}

int main(void) {
	uint8_t a = 0;  //LED高低电压控制
	/* System Clocks Configuration */
	RCC_Configuration();											  //系统时钟设置
	/*嵌套向量中断控制器
	 说明了USART1抢占优先级级别0（最多1位） ，和子优先级级别0（最多7位） */
	NVIC_Configuration();											  //中断源配置
	/*对控制LED指示灯的IO口进行了初始化，将端口配置为推挽上拉输出，口线速度为50Mhz。PA9,PA10端口复用为串口1的TX，RX。
	 在配置某个口线时，首先应对它所在的端口的时钟进行使能。否则无法配置成功，由于用到了端口B， 因此要对这个端口的时钟
	 进行使能，同时由于用到复用IO口功能用于配置串口。因此还要使能AFIO（复用功能IO）时钟。*/
	GPIO_Configuration();											  //端口初始化
	SPI2_NRF24L01_Init(); 										  //SPI2及NRF24L01接口初始化
	USART_Config(USART1);											  //串口1初始化

	// check
	while(NRF24L01_Check()==1){
		USART_SendChar(USART1, '?');
	}

	/*NRF24L01设置为接收模式*/
	RX_Mode();

	USART_SendChar(USART1, 'x');

	while (1) {
		if (usart_rec_flag == 1)  //判断是否收到一帧有效数据
		{
			usart_rec_flag = 0;
			NRF_Send_Data(TxBufferRF, sizeof(TxBufferRF));
			if (a == 0) {
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
				a = 1;
			}          //LED1  明暗闪烁
			else {
				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
				a = 0;
			}
		}
		if (rf_rec_flag == 1) {
			rf_rec_flag = 0;
			for (i = 0; i < 32; i++)          //发送字符串
					{
				USART_SendChar(USART1, TxBufferUSART[i]);
				//	Delay(0x0000ff00);
			}
		}
	}
}
//--------------------------------------------------------------------------------------
void USART_SendChar(USART_TypeDef* USARTx, uint8_t data) {
	USART_SendData(USARTx, data);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
		;
}
//--------------------------------------------------------------------------------------
void USART_Config(USART_TypeDef* USARTx) {
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//数据位8位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//停止位1位
	USART_InitStructure.USART_Parity = USART_Parity_No;				//无校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //无硬件流控
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//收发模式

	/* Configure USART1 */
	USART_Init(USARTx, &USART_InitStructure);							//配置串口参数函数

	/* Enable USART1 Receive and Transmit interrupts */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                    //使能接收中断
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断

	/* Enable the USART1 */
	USART_Cmd(USART1, ENABLE);
}
//--------------------------------------------------------------------------------------
void Delay(__IO uint32_t nCount) {
	for (; nCount != 0; nCount--)
		;
}
/*--------------------------------------------------------------------------------------
 系统时钟配置为72MHZ+外设时钟配置*/
void RCC_Configuration(void) {
	SystemInit();
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
}
//-------------------------------------------------------------------------------------- 
void GPIO_Configuration(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;				     //LED1控制--PC13(C8绿色板子)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         		 //USART1 TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    		 //复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);		    		 //A端口

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	         	 //USART1 RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;   	 //复用开漏输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);		         	 //A端口
}
//--------------------------------------------------------------------------------------
void NVIC_Configuration(void) {
	/*  结构声明*/
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	/* 优先级组 1  */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	/* Enable the USART1 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			     	//设置串口1中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	     	//抢占优先级 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;				//子优先级为0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;					//NRF24L01 中断响应
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;		    //抢占优先级 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;				//子优先级为1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能
	NVIC_Init(&NVIC_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);	   //NRF24L01 IRQ

	EXTI_InitStructure.EXTI_Line = EXTI_Line8;					   //NRF24L01 IRQ
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			   //EXTI中断
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		   //下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;						   //使能
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(EXTI_Line10);
}
