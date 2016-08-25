#include "usart.h"
//#include "stdio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

void USART2_init(void) {
	GPIO_InitTypeDef usart_gpio_initstructure;
	USART_InitTypeDef usart2_usart_initstructure;
	//enable gpio clock
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE);

	//connect PA.2 to usart2's tx
	//connect PA.3 to usart2's rx

	//Configure USART Tx as alternate function push-pull
	usart_gpio_initstructure.GPIO_Pin = GPIO_Pin_2;
	usart_gpio_initstructure.GPIO_Mode = GPIO_Mode_AF_PP;
	usart_gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &usart_gpio_initstructure);

	//Configure USART Rx as alternate function push-pull
	usart_gpio_initstructure.GPIO_Pin = GPIO_Pin_3;
	usart_gpio_initstructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	GPIO_Init(GPIOA, &usart_gpio_initstructure);

	// usart init
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	usart2_usart_initstructure.USART_BaudRate = 115200;
	usart2_usart_initstructure.USART_WordLength = USART_WordLength_8b;
	usart2_usart_initstructure.USART_StopBits = USART_StopBits_1;
	usart2_usart_initstructure.USART_Parity = USART_Parity_No;
	usart2_usart_initstructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;


#ifndef USE_USART1_AS_OUTPUT_DEBUG
	USART_WakeUpConfig(USART2, USART_WakeUp_IdleLine);
	USART_ITConfig(USART2, USART_IT_PE, DISABLE);  //禁止中断总开关
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);  //禁止发送缓冲区中断
	USART_ITConfig(USART2, USART_IT_TC, DISABLE);  //禁止发送完成中断
	USART_ITConfig(USART2, USART_IT_IDLE, DISABLE);  //禁止IDLE中断
	USART2->CR1 &= ~(1 << 1);	  //USART处于正常模式，而非静默模式
	USART2->CR1 &= ~(1 << 0);	  //禁止发送断开帧
	usart2_usart_initstructure.USART_Mode = USART_Mode_Tx;
#else
	usart2_usart_initstructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
#endif

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  //允许缓冲区非空中断
	//configuration
	USART_Init(USART2, &usart2_usart_initstructure);
	USART_Cmd(USART2, ENABLE);  //使能串口
}

void USART1_init(void) {
	GPIO_InitTypeDef usart_gpio_initstructure;
	USART_InitTypeDef usart1_usart_initstructure;
	//enable gpio clock
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE);
	
	//connect PA.9 to usart1's tx
	//connect PA.10 to usart1's rx
	
	//Configure USART Tx as alternate function push-pull
	usart_gpio_initstructure.GPIO_Pin = GPIO_Pin_9;
	usart_gpio_initstructure.GPIO_Mode = GPIO_Mode_AF_PP;
	usart_gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &usart_gpio_initstructure);

	//Configure USART Rx as alternate function push-pull
	usart_gpio_initstructure.GPIO_Pin = GPIO_Pin_10;
	usart_gpio_initstructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//???

	GPIO_Init(GPIOA, &usart_gpio_initstructure);

	// usart init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	usart1_usart_initstructure.USART_BaudRate = 115200;
	usart1_usart_initstructure.USART_WordLength = USART_WordLength_8b;
	usart1_usart_initstructure.USART_StopBits = USART_StopBits_1;
	usart1_usart_initstructure.USART_Parity = USART_Parity_No;
	usart1_usart_initstructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart1_usart_initstructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

#ifdef USE_USART1_AS_OUTPUT_DEBUG
	USART_WakeUpConfig(USART1, USART_WakeUp_IdleLine );
	USART_ITConfig(USART1, USART_IT_PE, DISABLE);  //禁止中断总开关
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);  //禁止发送缓冲区中断
	USART_ITConfig(USART1, USART_IT_TC, DISABLE);  //禁止发送完成中断
	USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);  //禁止IDLE中断
	USART1->CR1 &= ~(1 << 1);	  //USART处于正常模式，而非静默模式
	USART1->CR1 &= ~(1 << 0);	  //禁止发送断开帧
	usart1_usart_initstructure.USART_Mode = USART_Mode_Tx;
#else
	usart1_usart_initstructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
#endif

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //允许接收缓冲区非空中断

	//configuration
	USART_Init(USART1, &usart1_usart_initstructure);//
	USART_Cmd(USART1, ENABLE);  //使能串口
}

//usart config
void USART_scanf_config_EXT(void) {
	NVIC_InitTypeDef NVIC_InitStructure;
	//初始化中断向量
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
#ifndef USE_USART1_AS_OUTPUT_DEBUG
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
#else
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
#endif
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
