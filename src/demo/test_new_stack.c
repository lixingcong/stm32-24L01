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

#define USB_FROM_PHONE_MAX_LEN 162
unsigned char usb_recv_buffer[USB_FROM_PHONE_MAX_LEN];

typedef enum _USB_APP_STATE_ENUM {
	USB_APP_STATE_SELF_CHECK,
	USB_APP_STATE_WAIT_FOR_USER_INPUT,
	USB_APP_STATE_SEND_DATA
}USB_APP_STATE_ENUM;

int main(){
	unsigned char *usb_recv_ptr,j;
	unsigned char ep2_rev_ok; // received ok flag

	USB_APP_STATE_ENUM my_usb_stage;
	my_usb_stage=USB_APP_STATE_WAIT_FOR_USER_INPUT;

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

	while(1){
		do{
			mainFSM();// 组网、入网状态机
		}while(isOffline==TRUE);

		// usb loop
		switch (my_usb_stage) {
			case USB_APP_STATE_SELF_CHECK:
				ep2_rev_ok = USB_GetData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
				if (ep2_rev_ok) {
					if (usb_recv_buffer[6] == 1 && usb_recv_buffer[7] == 1) {      //组网检验
						usb_recv_buffer[7] = all_nodes[MY_NODE_NUM];
						USB_SendData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
					} else {
						usb_recv_buffer[7] = 0x20;
						USB_SendData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
					}
					for (j = 0; j < USB_FROM_PHONE_MAX_LEN; j++) {
						usb_recv_buffer[j] = 0;
					}
					ep2_rev_ok = 0;
					my_usb_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT;
				}
				break;
				// wait for recv
			case USB_APP_STATE_WAIT_FOR_USER_INPUT:
				ep2_rev_ok = USB_GetData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
				if (ep2_rev_ok) {
					printf("recv USB data !!!!!\r\n");
					// move pointer to msg offset
					usb_recv_ptr = &usb_recv_buffer[24];
					while (*usb_recv_ptr != 0)
						printf("%c", *(usb_recv_ptr++));
					printf("\r\n");

					ep2_rev_ok = 0;
					my_usb_stage = USB_APP_STATE_SEND_DATA;
				}
				break;
				// after recv, go to send
			case USB_APP_STATE_SEND_DATA:

				aplSendCustomMSG(usb_recv_buffer[23],USB_FROM_PHONE_MAX_LEN,usb_recv_buffer);

				//send_custom_routine_to_coord(usb_recv_buffer[23]);
				my_usb_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT;
				break;
		}

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

	// move pointer to msg offset
	ptr+=24;
	while(*ptr!=0)
		printf("%c",*(ptr++));
	printf("\r\n");

#if 0
	for(i=0;i<len;++i)
		putchar(*(ptr+i));
	printf("\r\n");
#endif
}
