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

int main(){
	unsigned char payload[10];
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

	printf("init done\r\n");
#ifdef LRWPAN_COORDINATOR
	my_role=ROLE_COORDINATOR;
	coord_FSM_state=COORD_STATE_INITAILIZE_ALL_NODES;
#else
	my_role=ROLE_ROUTER;
	router_FSM_state=ROUTER_STATE_INITAILIZE_ALL_NODES;
#endif


	while(1){
		if(my_role==ROLE_COORDINATOR)
			while(1)coordFSM();
		else
			while(1)router_FSM();
	}
	return 0;
}
