/*
 * test_new_stack.c
 *
 *  Created on: 2016年8月23日
 *     Author: lixingcong
 *
 *     This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *     Version 20160826
 *
 *     Copyright 2016 xingcong-li@foxmail.com
 */

// --------hardware--------------
#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "A7190.h"
#include "misc.h"
#include "timer2.h"
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

unsigned char usb_recv_buffer[USB_FROM_PHONE_MAX_LEN];  // symbol USB_FROM_PHONE_MAX_LEN was defined in usb_1.h

typedef enum _USB_APP_STATE_ENUM {
	USB_APP_STATE_SELF_CHECK, USB_APP_STATE_WAIT_FOR_USER_INPUT, USB_APP_STATE_SEND_DATA
} USB_APP_STATE_ENUM;

int main() {
	unsigned char *usb_recv_ptr;
	unsigned short j;
	unsigned char usb_recv_ok_flag;  // received ok flag
	unsigned char broadcast_buffer_for_pc_control[8] = { 0xff, 0xff, 'h', 'e', 'l', 'l', 'o', 0 };

	USB_APP_STATE_ENUM my_usb_stage;
	my_usb_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT; // 跳过自检
//	my_usb_stage = USB_APP_STATE_SELF_CHECK;  // 自检通过后才能进行收发

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	// init delay timer
	init_delay();

	// init timer2(system_msecond)
	TIM2_Init();

	// init USART
	USART1_init();
	USART2_init();
	USART_scanf_config_EXT();

	// init SPI1+A7190
	SPI1_Init();
	EXTI_config_for_A7190();
	initRF();
	printf("A7190 init done, PLL channel=%u\r\n", LRWPAN_DEFAULT_START_CHANNEL);

#if 0
	// init SPI2+lmx2581
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	ctl_lmx2581_init();
	DelayMs(600);
	ctl_lmx2581_init();
	ctl_frequency_set(400);
	printf("LMX2581 init done\r\n");
#endif

	// init USB
	init_USB_GPIO();
	USB_Interrupts_Config();
	Set_USBClock(); 	//时钟设置并使能，USB时钟为48MHz
	USB_Init();
	printf("USB init done\r\n");

	// configuration flags for pc_control
	dynamic_freq_mode = 0xff;  // 这个必须要设置0xff，否则A7190强行工作跳频模式无法接收东西
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

		// usb loop
		switch (my_usb_stage) {
			case USB_APP_STATE_SELF_CHECK:
				usb_recv_ok_flag = USB_GetData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
				if (usb_recv_ok_flag) {
					if (usb_recv_buffer[7] == 1) {      //组网检验
						usb_recv_buffer[7] = all_nodes[MY_NODE_NUM];
						USB_SendData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
					} else {
						usb_recv_buffer[7] = 0x20;
						USB_SendData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
					}
					for (j = 0; j < USB_FROM_PHONE_MAX_LEN; j++) {
						usb_recv_buffer[j] = 0;
					}
					usb_recv_ok_flag = 0;
					my_usb_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT;
				}
				break;

				// wait for recv
			case USB_APP_STATE_WAIT_FOR_USER_INPUT:
				usb_recv_ok_flag = USB_GetData(ENDP2, usb_recv_buffer, USB_FROM_PHONE_MAX_LEN);
				if (usb_recv_ok_flag) {
					printf("recv USB data !!!!!\r\n");
					// move pointer to msg offset
					usb_recv_ptr = &usb_recv_buffer[10];
					while (*usb_recv_ptr != 0)
						printf("%c", *(usb_recv_ptr++));
					printf("\r\n");

					usb_recv_ok_flag = 0;
					my_usb_stage = USB_APP_STATE_SEND_DATA;
				}
				break;
				// after recv, go to send23
			case USB_APP_STATE_SEND_DATA:

				aplSendMSG(usb_recv_buffer[9], USB_FROM_PHONE_MAX_LEN, usb_recv_buffer);

				if (usb_recv_buffer[6] == 0x01)  // 上传路径标志位
					send_custom_routine_to_coord(usb_recv_buffer[9]);

				my_usb_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT;
				break;

		}


#ifdef LRWPAN_COORDINATOR
		if (dynamic_freq_mode != 0xff)
		work_under_dynamic_freq_mode();

		if(isBroadcastRegularly == TRUE) {  // 周期性发送广播
			if(halMACTimerNowDelta(last_broadcast_timer) >= (3000)) {
				fprintf(stderr,"ZZIFsending broadcast...@\r\n");
				aplSendBROADCAST(8,broadcast_buffer_for_pc_control);
				last_broadcast_timer=halGetMACTimer();
			}
		}
#endif
	}
	return 0;
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
	
	USB_SendData(ENDP2, ptr, len);

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
