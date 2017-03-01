/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_exti.h"
//#include "usart_scanf_irq.h"
//#include "SPI1.h"
//#include "usb_istr.h"
#include "stdio.h"

#include "stm32f10x_it.h"
#include "stm32f10x_gpio.h"
#include "NRF24L01.h"
//#include "timer2.h"

unsigned int systick_count=0;

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{

}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	++systick_count;
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

#ifndef USE_USART1_AS_OUTPUT_DEBUG
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		// WARNING: spaces were ignore by scanf()
//		scanf("%s", usart_scanf_data);
//		usart_irq_scanf_callback();
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}

}
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET)	{
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
	}
}
#else
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		// WARNING: spaces were ignore by scanf()
//		scanf("%s", usart_scanf_data);
//		usart_irq_scanf_callback();
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}

}
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET)	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	}
}
#endif

void EXTI9_5_IRQHandler(void) {
	u8 i = 0;
	u8 status;
	// TODO 2017年2月27日上午11:44:55 重新封装SPI_RW_Reg之类的函数，增强可读性
	if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8) == 0) {  //判断是否是PA0线变低
			status = SPI_Read(READ_REG1 + STATUS);			// 读取状态寄存其来判断数据接收状况
			if (status & 0x40){				    		// 判断是否接收到数据
				SPI_Read_Buf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH);  //从接收缓冲区里读出数据
				printf("%s", rx_buf);
			} else if ((status & 0x10) > 0) {					 //发射达到最大复发次数
				SPI_RW_Reg(0xe1, 0);					 	 //清除发送缓冲区
				RX_Mode();								 //进入接收模式
			} else if ((status & 0x20) > 0) {					 //发射后收到应答
				// TODO 2017年2月27日上午11:21:25 PB5对应原作者的哪个管脚？为什么应答？
				GPIO_SetBits(GPIOB, GPIO_Pin_5);
				SPI_RW_Reg(0xe1, 0);					     //清除发送缓冲区
				RX_Mode();								 //进入接收模式
			}
			SPI_RW_Reg(WRITE_REG1 + STATUS, status);	     //清除07寄存器标志
		}
		EXTI_ClearITPendingBit(EXTI_Line8);			 //清除EXTI0上的中断标志
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
//	USB_Istr();
}

// 定时器2用于更新system_msecond
void TIM2_IRQHandler(void){
//	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
//		++system_msecond;
//		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
//	}
}
