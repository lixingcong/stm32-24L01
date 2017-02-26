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
	unsigned char err;

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

//	while(1);

#ifdef SENDING
	NRF24L01_TX_Mode();
	printf("send mode\n");
#else
	NRF24L01_RX_Mode();
	printf("recv mode\n");
#endif
	while(1){
#ifdef SENDING
		if((err=NRF24L01_TxPacket("good"))==0x20){
			printf("sent\n");
		}else{
			printf("error:%x\n",err);
		}

		printf("delaying..\n");
		DelayMs(1000);
#endif
	}
}
