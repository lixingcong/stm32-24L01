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
#include "FSM_coord.h"
#include "FSM_router.h"

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

	// configuration flags for pc_control
	isBroadcastRegularly = FALSE;  // 默认禁用上位机广播

	#ifdef LRWPAN_COORDINATOR
	my_role = ROLE_COORDINATOR;
	coord_FSM_state = COORD_STATE_INITAILIZE_ALL_NODES;
	mainFSM = coord_FSM;
#else
	my_role = ROLE_ROUTER;
	router_FSM_state = ROUTER_STATE_INITAILIZE_ALL_NODES;
	mainFSM = router_FSM;
#endif

	while (1) {
		do {
			mainFSM();  // 组网、入网状态机
		} while (isOffline == TRUE);
	}
}


void aplRxCustomCallBack() {
	unsigned short len, i;
	unsigned char *ptr;
	unsigned char isBoardcast=FALSE;
	printf("aplRxCustomCallBack(): recv a packet, type=");

	switch (aplGetRxMsgType()) {
		case FRAME_TYPE_LONG_BROADCAST:
			isBoardcast=TRUE;
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


#if 1

#define DISPLAY_ON_SERIAL

#ifndef DISPLAY_ON_SERIAL
	for (i = 0; i < len; ++i)
		printf("%c", *(ptr + i));
	puts("");
#endif

	show_msg_to_PC(aplGetRxMsgSrcAddr(), ptr, len, isBoardcast);

#else
	// move pointer to USB msg offset 24
	ptr += 24;
	while (*ptr != 0)
	printf("%c", *(ptr++));
	printf("\r\n");
#endif


}
