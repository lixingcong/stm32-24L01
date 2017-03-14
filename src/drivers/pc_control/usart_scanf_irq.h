/*
 * usart_scanf_irq.h
 *
 *  Created on: 2016年7月20日
 *      Author: lixingcong
 */

#ifndef _USART_SCANF_IRQ_H_
#define _USART_SCANF_IRQ_H_

#define MAX_USART1_BUFFER_LEN 60

// usart1 rx buffer
extern unsigned char usart_scanf_data[MAX_USART1_BUFFER_LEN];

void usart_irq_scanf_callback(void);

#endif
