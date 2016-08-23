/*
 * test_USB_lxc.c
 * 2016-8-17
 * Author: li
 * base on test_route_table_broadcast.c and test_USB_ldd.c
 * Features:
 * * test p2p USB send (COORD-->ROUTER-->ROUTER-->ROUTER routine is ok)
 * * interact with PC ok
 * * auto refresh my route table for children and parent
 * * Set Freq using SPI2(LMX2581)
 */

#include "msstate_lrwpan.h"
#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "execute_PC_cmd.h"
#include "route_table.h"
#include "route_ping.h"
#include "apl_custom_function.h"
#include "timer2.h"
// --------USB--------------
#include "usb_lib.h"
#include "hw_config.h"
#include "usbio.h"
#include "app_cfg.h"
#include "usb_pwr.h"
// -------LMX2581------------
#include "ctl_lmx2581.h"
#include "stm32f10x_rcc.h"

#define PING_DELAY  10         // large number for a heartbeat
#define PING_RETRY_INTERVAL  3         //retry every second
#define PING_RETRY_TIMES  3             //retry for 3 times

UINT32 ping_cnt;
UINT32 my_timer;
UINT32 last_timer;
UINT16 numTimeouts;

// USB data length definitions for Ludongdong
#define MAX_QUEUE_LENGTH 40
#define USB_FROM_PHONE_MAX_LEN 162
#define A7190_MAX_LEN_LDD 180
// USB for Ludongdong
uint8_t ep2_send_ok,ep2_rev_ok;
BYTE ep2_data[USB_FROM_PHONE_MAX_LEN];

typedef enum _USB_APP_STATE_ENUM {
	USB_APP_STATE_SELF_CHECK,
	USB_APP_STATE_WAIT_FOR_USER_INPUT,
	USB_APP_STATE_SEND_DATA
}USB_APP_STATE_ENUM;

USB_APP_STATE_ENUM my_stage;

void TestPing(void) {
	unsigned char ping_result, ping_retry_counter;
	unsigned char my_stage,*ptr,j;
	my_stage=USB_APP_STATE_SELF_CHECK;
	// 必须设置为在线，这个标志位涉及到中断接收ACK/Ping/Packet的允许与否
	isOffline = FALSE;
	// 开启定期刷路由表
	TIM2_NVIC_enable();
	while (1) {
#ifdef LRWPAN_COORDINATOR
		if (dynamic_freq_mode != 0xff){			// 跳频模式
			work_under_dynamic_mode();
		}
#endif
		apsFSM();
		// Now ping my parent
		if (halMACTimerNowDelta(last_timer) > MSECS_TO_MACTICKS(PING_DELAY *1000)) {
			// only children have to ping,
#ifndef LRWPAN_COORDINATOR
			ping_result = aplSendCustomPing((mac_pib.macCoordExtendedAddress.bytes[0])&0xff,PING_RETRY_TIMES,PING_RETRY_INTERVAL*1000);
			if (ping_result == 0xff) {
				printf("#%u: ping timeout!\r\n", ping_cnt++);
				// 掉线后要及时设置自己标志位为已掉线
				isOffline=TRUE;
				deattach_from_my_parent();
				return;
			}else{
				printf("#%u: ping time=%ums\r\n", ping_cnt++, ping_result);
				all_nodes[MY_NODE_NUM+ALL_NODES_NUM]=ping_result;
				// you should keep in touch with my parent
				add_to_my_parent();
			}

#else
			printf("#%u\r\n", ping_cnt++);

#endif
			last_timer = halGetMACTimer();
		}

		// Now recv data from USB and send data to dst
		switch (my_stage) {
			case USB_APP_STATE_SELF_CHECK:
				ep2_rev_ok = USB_GetData(ENDP2, ep2_data, USB_FROM_PHONE_MAX_LEN);
				if (ep2_rev_ok) {
					if (ep2_data[6] == 1 && ep2_data[7] == 1) {      //组网检验
						ep2_data[7] = all_nodes[MY_NODE_NUM];
						USB_SendData(ENDP2, ep2_data, USB_FROM_PHONE_MAX_LEN);
					} else {
						ep2_data[7] = 0x20;
						USB_SendData(ENDP2, ep2_data, USB_FROM_PHONE_MAX_LEN);
					}
					for (j = 0; j < USB_FROM_PHONE_MAX_LEN; j++) {
						ep2_data[j] = 0;
					}
					ep2_rev_ok = 0;
					my_stage = USB_APP_STATE_WAIT_FOR_USER_INPUT;
				}
				break;
			// wait for recv
			case USB_APP_STATE_WAIT_FOR_USER_INPUT:
				ep2_rev_ok = USB_GetData(ENDP2, ep2_data, USB_FROM_PHONE_MAX_LEN);
				if (ep2_rev_ok) {
					printf("recv USB data !!!!!\r\n");
					// move pointer to msg offset
					ptr = &ep2_data[24];
					while (*ptr != 0)
						printf("%c", *(ptr++));
					printf("\r\n");

					// 判断第七伪
					ep2_rev_ok = 0;
					my_stage=USB_APP_STATE_SEND_DATA;
				}
				break;
			// after recv, go to send
			case USB_APP_STATE_SEND_DATA:
				// use my send function
				aplSendCustomMSG(MY_NODE_NUM, ep2_data[23], USB_FROM_PHONE_MAX_LEN, ep2_data);  // dst addr is ep3_data[17]
				// send route PATH to PC
				send_custom_routine_to_coord(ep2_data[23]);
				my_stage=USB_APP_STATE_WAIT_FOR_USER_INPUT;
				break;
		}

	}
}

void printJoinInfo(void) {
	conPrintROMString("My ShortAddress is: ");
	conPrintUINT16(aplGetMyShortAddress());
	conPCRLF();
	conPrintROMString("Parent LADDR: ");
	conPrintLADDR(aplGetParentLongAddress());
	conPrintROMString(", Parent SADDR: ");
	conPrintUINT16(aplGetParentShortAddress());
	conPCRLF();
}

typedef enum _JOIN_STATE_ENUM {
	JOIN_STATE_START_JOIN,
	JOIN_STATE_WAIT_FOR_JOIN,
	JOIN_STATE_RUN_APP1,
	JOIN_STATE_RUN_APP2,
	JOIN_STATE_RUN_APP3,
	JOIN_STATE_START_REJOIN,
	JOIN_STATE_WAIT_FOR_REJOIN,
} JOIN_STATE_ENUM;

JOIN_STATE_ENUM joinState;

#define MAX_REJOIN_FAILURES 3

void main(void) {
	UINT8 failures;
	Set_System();
	numTimeouts = 0;
	my_timer = 0;
	// 全局标志位
	is_self_check_ok=0xff; // 自检标志：初始化过程中置为0xff
	isOffline=TRUE;// 是否离线？不在网。
#ifdef LRWPAN_COORDINATOR
	dynamic_freq_mode=0xff; // 这个必须要设置0xff，否则无法接收东西
#endif

	init_all_nodes();// 删除所有路由表

	halInit();
	aplInit();  //init the stack
	TIM2_Init(); // 初始化定时器，但是暂时不允许中断
	conPrintConfig();

	// ENABLE_GLOBAL_INTERRUPT();  //enable interrupts
	joinState = JOIN_STATE_START_JOIN;  //main while(1) initial state
	ping_cnt = 0;
	// debug_level = 10;

	// USB Init for Ludongdong
	USB_Interrupts_Config();
	Set_USBClock(); 	//时钟设置并使能，USB时钟为48MHz
	USB_Init();
	printf("usbclock is OK \r\n");

	// Initialize for LMX2581
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	ctl_lmx2581_init();
	DelayMs(600);
	ctl_lmx2581_init();
	ctl_frequency_set(400);

	//This version of TestPing has Network Rejoin logic in case of excessive failures
	//by the TestPing() function during transmit.
	while (1) {
		apsFSM();
		switch (joinState) {
			case JOIN_STATE_START_JOIN:
				is_self_check_ok=FALSE;
#ifdef LRWPAN_COORDINATOR
				aplFormNetwork();
#else
				aplJoinNetwork()
				;
#endif
				joinState = JOIN_STATE_WAIT_FOR_JOIN;
				break;
			case JOIN_STATE_WAIT_FOR_JOIN:
				if (apsBusy())
					break;   //if stack is busy, continue
#ifdef LRWPAN_COORDINATOR
					if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
						conPrintROMString("Network formed, waiting for RX\n");
						joinState = JOIN_STATE_RUN_APP1;
					} else {
						//only reason for this to fail is some sort of hardware failure
						conPrintROMString("Network formation failed, waiting, trying again\n");
						my_timer= halGetMACTimer();
						//wait for 2 seconds
						while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
						joinState = JOIN_STATE_START_JOIN;
					}
#else
				if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
					conPrintROMString("Network Join succeeded!\r\n");
					printJoinInfo();
					// todo: 重新入网后更新父亲节点信息
					add_to_my_parent();
					joinState = JOIN_STATE_RUN_APP1;
				} else {
					conPrintROMString("Network Join FAILED! Waiting, then trying again\r\n");
					my_timer = halGetMACTimer();
					//wait for 2 seconds
					while ((halMACTimerNowDelta(my_timer)) < MSECS_TO_MACTICKS(2 * 1000))
						;
					joinState = JOIN_STATE_START_JOIN;
				}
#endif
				break;

			case JOIN_STATE_RUN_APP1:
				last_timer = halGetMACTimer();
#ifdef LRWPAN_COORDINATOR
				//WARNING - this is only for latency testing, this variable is normally
				//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
				//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
				//only do this in your normal code if you want to disable automatic retries
				aplSetMacMaxFrameRetries(0);
				is_self_check_ok=TRUE;
				TestPing();//only exits on if excessive misses
				is_self_check_ok=FALSE;
				// 关闭定期刷路由表
				TIM2_NVIC_disable();

				if(dynamic_freq_mode!=0xff)
					work_under_dynamic_mode();

				//reset first_packet to flag to true, this causes Coord to just
				//wait for a packet, and not try any retransmit on misses.
#else

				conPrintROMString("Hit any switch to start!\r\n")
				;
				joinState = JOIN_STATE_RUN_APP2;
#endif
				break;
			case JOIN_STATE_RUN_APP2:
				DelayMs(1);
			case JOIN_STATE_RUN_APP3:
				//WARNING - this is only for latency testing, this variable is normally
				//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
				//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
				//only do this in your normal code if you want to disable automatic retries
				aplSetMacMaxFrameRetries(0);
				//switch is pressed, run app
				my_timer = halGetMACTimer();  //timer must be initialized before entering PP_SEND_STATE
				is_self_check_ok=TRUE;
				TestPing();  //only exits on if excessive misses
				is_self_check_ok=FALSE; // mark as uploding status FALSE
				// 关闭定期刷路由表
				TIM2_NVIC_disable();
				//try rejoining network
				failures = 0;
				joinState = JOIN_STATE_START_REJOIN;
				break;
				//rejoin states only executed by RFD
#ifndef LRWPAN_COORDINATOR
			case JOIN_STATE_START_REJOIN:
				conPrintROMString("Trying to rejoin network!\r\n")
				;
				aplRejoinNetwork()
				;
				joinState = JOIN_STATE_WAIT_FOR_REJOIN;
				break;

			case JOIN_STATE_WAIT_FOR_REJOIN:
				if (apsBusy())
					break;   //if stack is busy, continue
				if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
					failures = 0;
					conPrintROMString("Network Rejoin succeeded!\r\n");
					// todo: 重新入网后更新父亲节点信息
					add_to_my_parent();
					printJoinInfo();
					joinState = JOIN_STATE_RUN_APP3;  //don't wait for button press
				} else {
					failures++;
					if (failures == MAX_REJOIN_FAILURES) {
						//this starts everything over
						conPrintROMString("Max Rejoins failed, trying to join.\r\n");
						joinState = JOIN_STATE_START_JOIN;
					} else {
						//else, wait to try again
						conPrintROMString("Network Rejoin FAILED! Waiting, then trying again\n");
						my_timer = halGetMACTimer();
						//wait for 2 seconds
						while ((halMACTimerNowDelta(my_timer)) < MSECS_TO_MACTICKS(2 * 1000))
							;
						joinState = JOIN_STATE_START_REJOIN;
					}
				}
				break;
#endif

			default:
				joinState = JOIN_STATE_START_JOIN;

		}

	}
}

//########## Callbacks ##########

//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
LRWPAN_STATUS_ENUM usrZepRxCallback(void) {

#ifdef LRWPAN_COORDINATOR
	if (aplGetRxCluster() == ZEP_END_DEVICE_ANNOUNCE) {
		//a new end device has announced itself, print out the
		//the neightbor table and address map
		dbgPrintNeighborTable();
	}
#endif
	return LRWPAN_STATUS_SUCCESS;
}

//callback from APS when packet is received
//user must do something with data as it is freed
//within the stack upon return.

LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {
	// 这里不需处理
}

// 使用这个函数代替usrRxPacketCallback
void aplRxCustomCallBack(){
	unsigned char *ptr,len,i;
	printf("recv a custom packet, type=");

	switch(aplGetCustomRxMsgType()){
		case CUSTOM_FRAME_TYPE_BROADCAST:
			printf("broadcast: \r\n");
			break;
		case CUSTOM_FRAME_TYPE_DATA:
			printf("msg: \r\n");
			break;
		default:
			printf("unsupport msg in RxCallback\r\n");
			return;
	}
	ptr=aplGetCustomRxMsgData();
	len=aplGetCustomRxMsgLen();

	USB_SendData(ENDP2, ptr, len);

	// move pointer to msg offset
	ptr+=24;

	while(*ptr!=0)
		printf("%c",*(ptr++));
	printf("\r\n");

#if 0
	printf("another phone received: ");
	for (i = 0; i < len; i++) {
		printf("%u: %x\r\n", i, *(ptr+i));
	}
	printf("\r\n");
#endif
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo) {

#if 0      //set this to '1' if you want to test through a router
//only accept routers.
//only let routers join us if we are coord
#ifdef LRWPAN_COORDINATOR
	if (LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) {
		//this is a router, let it join
		conPrintROMString("Accepting router\n");
		return TRUE;
	} else {
		conPrintROMString("Rejecting non-router\n");
		return FALSE;
	}
#else
	return TRUE;
#endif

#else

	return TRUE;

#endif

}

BOOL usrJoinNotifyCallback(LADDR *ptr) {
	//allow anybody to join
	conPrintROMString("Node joined: ");
	conPrintLADDR(ptr);
	conPCRLF();
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	add_to_my_child(ptr->bytes[0]);
	return TRUE;
}
#endif

//called when the slow timer interrupt occurs
#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void) {
}
#endif

//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void) {
}
