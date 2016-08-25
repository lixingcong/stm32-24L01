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
#include "A7190.h"
#include "misc.h"
#include "common_func.h"
#include "coord_FSM.h"
#include "router_FSM.h"
#include "hal.h"
#include "route_table.h"
#include "apl_custom_function.h"

int main(){
	unsigned char payload[512];
	unsigned int my_timer,i;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	// init delay timer
	init_delay();
	// init USART
	USART1_init();
	USART2_init();
	USART_scanf_config_EXT();

	// init SP1+A7190
	SPI1_Init();

	EXTI_config_for_A7190();

	initRF();
	printf("A7190 init done, PLL channel=%u\r\n",LRWPAN_DEFAULT_START_CHANNEL);

#ifdef LRWPAN_COORDINATOR
	my_role=ROLE_COORDINATOR;
	coord_FSM_state=COORD_STATE_INITAILIZE_ALL_NODES;
#else
	my_role=ROLE_ROUTER;
	router_FSM_state=ROUTER_STATE_INITAILIZE_ALL_NODES;
#endif

	payload[0]='h';
	payload[1]='e';
	payload[2]='l';
	payload[3]='l';
	payload[4]='o';

	while(1){
		if(my_role==ROLE_COORDINATOR){
			while(1){
				coordFSM();
			}
		}

		else{
			while(1){
				do{
					router_FSM();
				}while(isOffline==TRUE);
				aplSendCustomMSG(0,5,payload);
				DelayMs(2000);
			}
		}

	}
	return 0;
}

void aplRxCustomCallBack(){
	unsigned short len,i;
	unsigned char *ptr;
	printf("recv a custom packet, type=");

	switch(aplGetCustomRxMsgType()){
		case FRAME_TYPE_LONG_BROADCAST:
			printf("broadcast: \r\n");
			break;
		case FRAME_TYPE_LONG_MSG:
			printf("msg: \r\n");
			break;
		default:
			printf("unsupport msg in RxCallback\r\n");
			return;
	}
	ptr=aplGetCustomRxMsgData();
	len=aplGetCustomRxMsgLen();

	for(i=0;i<len;++i)
		putchar(*(ptr+i));
	printf("\r\n");
}
