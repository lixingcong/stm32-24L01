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
	printf("coord\r\n");
#else
	printf("router\r\n");
#endif

	payload[0]=3;
	payload[1]='h';
	payload[2]='i';

	while(1){
#ifdef LRWPAN_COORDINATOR
		if (A7190_read_state() == IDLE) {
			A7190_set_state(WAIT_TX);
			StrobeCmd(CMD_STBY);
			Set_FIFO_len(3 &0xff,0x00);
			StrobeCmd(CMD_TFR);
			WriteFIFO(payload, 3);
			A7190_set_state(BUSY_TX);
			StrobeCmd(CMD_TX);
		}else
			printf("busy when TX\r\n");
		DelayMs(1000);
#endif

	}
	return 0;
}
