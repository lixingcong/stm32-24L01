/*
 * test_new_stack.c
 *
 *  Created on: 2016年8月23日
 *     Author: lixingcong
 */

// --------hardware--------------
#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "A7190.h"
#include "misc.h"
// --------stack--------------
#include "FSM_coord.h"
#include "FSM_router.h"
#include "hal.h"
#include "route_table.h"
#include "route_AP_level.h"
// --------USB-----------------
#include "usb_1.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usbio.h"
#include "app_cfg.h"
#include "usb_pwr.h"
// --------LMX2581--------------
#include "ctl_lmx2581.h"
#include "stm32f10x_rcc.h"
// --------pc_control-----------
#include "execute_PC_cmd.h"

int main(){
	// TODO: remove 临时变量 2016年8月25日 上午11:22:24
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

	// init lmx2581
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	ctl_lmx2581_init();
	DelayMs(600);
	ctl_lmx2581_init();
	ctl_frequency_set(400);
	printf("LMX2581 init done\r\n");

	// init USB
	init_USB_GPIO();
	USB_Interrupts_Config();
	Set_USBClock(); 	//时钟设置并使能，USB时钟为48MHz
	USB_Init();
	printf("USB init done\r\n");

	dynamic_freq_mode=0xff; // 这个必须要设置0xff，否则无法接收东西

#ifdef LRWPAN_COORDINATOR
	my_role=ROLE_COORDINATOR;
	coord_FSM_state=COORD_STATE_INITAILIZE_ALL_NODES;
	mainFSM=coord_FSM;
#else
	my_role=ROLE_ROUTER;
	router_FSM_state=ROUTER_STATE_INITAILIZE_ALL_NODES;
	mainFSM=router_FSM;
#endif

	payload[0]='h';
	payload[1]='e';
	payload[2]='l';
	payload[3]='l';
	payload[4]='o';

	while(1){
		do{
			mainFSM();// 组网、入网状态机
		}while(isOffline==TRUE);

#ifdef LRWPAN_COORDINATOR
		if(dynamic_freq_mode!=0xff)
			work_under_dynamic_mode();
#endif
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
