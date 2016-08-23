/*
 * test_new_stack.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */


#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "timer2.h"
#include "misc.h"

int main(){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	// init USART
	USART1_init();
	USART2_init();
	USART_scanf_config_EXT();

	// init SP1+A7190
	return 0;
}
