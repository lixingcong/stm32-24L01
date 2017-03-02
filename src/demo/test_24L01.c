/*
 * test_24L01.c
 *
 *  Created on: 2017年2月19日
 *      Author: li
 */

#include "usart.h"
#include "misc.h"
#include "delay.h"
#include "NRF24L01.h"
#include "stdio.h"

int main(){
	unsigned char err;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_delay();
	USART1_init();
	NRF24L01_Init();

//	USART_scanf_config_EXT();
	printf("nrf24l01 ok\n");

#ifdef SENDING
	printf("send mode\n");
#else
	printf("recv mode\n");
#endif
	while(1){
#ifdef SENDING
		NRF_Send_Data("hello", NRF_PLOAD_LENGTH);
		printf("delaying..\n");
		DelayMs(1000);
#endif
	}
}
