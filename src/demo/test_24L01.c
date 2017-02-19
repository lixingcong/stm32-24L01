/*
 * test_24L01.c
 *
 *  Created on: 2017年2月19日
 *      Author: li
 */

#include "usart.h"
#include "misc.h"
#include "delay.h"
#include "nrf24l01.h"
#include "stdio.h"

int main(){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_delay();
	USART1_init();
	NRF24L01_Init();
//	USART_scanf_config_EXT();
	printf("hello\n");
	while(NRF24L01_Check())
	{
		printf("error\n");
 		DelayMs(200);
	}
	printf("nrf24l01 ok\n");

	while(1);
}
