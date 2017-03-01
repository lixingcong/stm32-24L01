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
	SPI2_NRF24L01_Init();
	// nrf24l01 enter to recv mode
	RX_Mode();

//	USART_scanf_config_EXT();
	printf("hello\n");
	while(NRF24L01_check_if_exist()==0)
	{
		printf("error\n");
 		DelayMs(200);
	}
	printf("nrf24l01 ok\n");

#ifdef SENDING
	printf("send mode\n");
#else
	printf("recv mode\n");
#endif
	while(1){
#ifdef SENDING
		NRF_Send_Data("hello", TX_PLOAD_WIDTH);
		printf("delaying..\n");
		DelayMs(1000);
#endif
	}
}
