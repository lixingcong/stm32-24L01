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
#include "NRF_api.h"
#include "stdio.h"

// --------pc_control-----------
#include "execute_PC_cmd.h"

// stack
#include "route_table.h"
#include "route_AP_level.h"

int main(){
	unsigned char err;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_delay();

	// init USART
	USART1_init();
	USART2_init();
	USART_scanf_config_EXT();

	// init timer2(system_msecond)
	TIM2_Init();

	// nrf24l01
	initRF();
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


void aplRxCustomCallBack() {
	unsigned short len, i;
	unsigned char *ptr;
	printf("aplRxCustomCallBack(): recv a packet, type=");

	switch (aplGetRxMsgType()) {
		case FRAME_TYPE_LONG_BROADCAST:
			printf("broadcast: \r\n");
			break;
		case FRAME_TYPE_LONG_MSG:
			printf("msg: \r\n");
			break;
		default:
			printf("unsupported msg, ignore it\r\n");
			return;
	}

	ptr = aplGetRxMsgData();
	len = aplGetRxMsgLen();

	// 上位机发送的广播，打印到上位机，不往USB上面发送，return
	if ((aplGetRxMsgType() == FRAME_TYPE_LONG_BROADCAST) && *(ptr) == 0xff && *(ptr + 1) == 0xff) {
		fprintf(stderr, "ZZIFrecv a broadcast: %s@\r\n", ptr + 2);
		return;
	}

	// USB_SendData(ENDP2, ptr, len);

/*
#if 1
	for (i = 0; i < len; ++i)
		printf("%u: %x\r\n", i, *(ptr + i));
#else
	// move pointer to USB msg offset 24
	ptr += 24;
	while (*ptr != 0)
	printf("%c", *(ptr++));
	printf("\r\n");
#endif
*/

}
