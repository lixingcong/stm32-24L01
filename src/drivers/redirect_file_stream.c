/*
 * redirect_file_stream.c
 *
 *  Created on: 2016年8月2日
 *      Author: lixingcong
 * 
 * redirect stdout and stderr to USART1 and USART2 seperately
 * use Micro Lib
 * For Windows Keil only
 */
 
#include "stdio.h"
#include "stm32f10x_usart.h"


struct __FILE{
	int handle;
};

enum{
	STDOUT_HANDLE,
	STDERR_HANDLE
};

FILE __stdout={STDOUT_HANDLE};
FILE __stderr={STDERR_HANDLE};

int fputc(int ch,FILE *f)
{
	int ret = EOF ;
	switch( f->handle ){
#ifdef USE_USART1_AS_OUTPUT_DEBUG
		case STDERR_HANDLE:
#else
		case STDOUT_HANDLE:
#endif
			ret = ch ;
			USART_SendData(USART2,(uint8_t)ch);
			while((USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET));
			break ;
#ifdef USE_USART1_AS_OUTPUT_DEBUG
		case STDOUT_HANDLE:
#else
		case STDERR_HANDLE:
#endif
			ret = ch ;
			USART_SendData(USART1,(uint8_t)ch);
			while((USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET));
			break ;
		default:
			break ;
		}
		return ret;
}
int fgetc(FILE *f){
	unsigned short c;
#ifdef USE_USART1_AS_OUTPUT_DEBUG
	while ((USART2->SR & USART_FLAG_RXNE) == (uint16_t) RESET);
	c = (char) (USART2->DR & (uint16_t) 0x01FF);
#else
	while ((USART1->SR & USART_FLAG_RXNE) == (uint16_t) RESET);
	c = (char) (USART1->DR & (uint16_t) 0x01FF);
#endif
	return c;
}

