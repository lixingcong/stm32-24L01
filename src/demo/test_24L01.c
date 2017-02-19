/*
 * test_24L01.c
 *
 *  Created on: 2017年2月19日
 *      Author: li
 */

#include "usart.h"
#include "misc.h"
#include "delay.h"


int main(){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_delay();
	USART1_init();
	USART_scanf_config_EXT();
	printf("hello\n");
	while(1);
}
