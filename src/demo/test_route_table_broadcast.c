/*
 * test_route_table_packet.c
 * 2016-8-10
 * Author: li
 * base on ping_pong_rejoin.c
 * test broadcast and relaying, use route_table.c, which is written by me.
 * interact with PC
 * COORD-->ROUTER-->ROUTER-->ROUTER已经测试3跳可以实现
 */

#include "msstate_lrwpan.h"
#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "execute_PC_cmd.h"
#include "route_table.h"
#include "apl_custom_function.h"
#include "stm32f10x_rcc.h"
#include "ctl_lmx2581.h"

#define PING_DELAY  1
#define PING_RETRY_INTERVAL  2         //retry every second
#define PING_RETRY_TIMES  3             //retry for 3 times

UINT32 ping_cnt;
UINT32 my_timer;
UINT32 last_timer;
unsigned char i_pload;
LADDR_UNION dstADDR;
BYTE payload[200];
UINT16 numTimeouts;
// route upload
UINT32 last_route_update;
BOOL isNeedCheckChildren;

void TestPing(void);
void printJoinInfo(void);

void TestPing(void) {
	unsigned char ping_result, ping_retry_counter;
	// 必须设置为在线，这个标志位涉及到中断接收ACK/Ping/Packet的允许与否
	isOffline=FALSE;
	last_route_update = halGetMACTimer();
	isNeedCheckChildren = FALSE;
	while (1) {
#ifdef LRWPAN_COORDINATOR
		if(dynamic_freq_mode!=0xff)// 跳频模式需要跳出来
			break;
#endif
		apsFSM();
		// Now ping my parent
		if (halMACTimerNowDelta(last_timer) > MSECS_TO_MACTICKS(PING_DELAY *1000)) {
			// only children have to ping,
#ifndef LRWPAN_COORDINATOR
			ping_result = macTxPing(aplGetParentShortAddress(), halGetRandomByte(), TRUE);
			if (ping_result == 0xff) {
				ping_retry_counter = PING_RETRY_TIMES;
				last_timer = halGetMACTimer();

				while (ping_retry_counter != 0) {
					printf("ping fail, now try again, retry %u times\r\n", ping_retry_counter);
					while (halMACTimerNowDelta(last_timer) < MSECS_TO_MACTICKS(PING_RETRY_INTERVAL *1000))
						;
					ping_result = macTxPing(aplGetParentShortAddress(), halGetRandomByte(), TRUE);
					if (ping_result == 0xff)
						--ping_retry_counter;
					else
						break;
					last_timer = halGetMACTimer();
				}

				if (ping_retry_counter == 0){
					// 掉线后要及时设置自己标志位为已掉线
					isOffline=TRUE;
					return;
				}

			}
			printf("#%u: ping time=%ums\r\n", ping_cnt++, ping_result);
#else
			printf("#%u: waiting for pinging\r\n",ping_cnt++);

#endif

			// Now send Broadcast
#ifdef LRWPAN_COORDINATOR
			if (isNeedCheckChildren == FALSE) {
				// printf("ready to send dst relay\r\n");
				payload[0] = 'h';
				payload[1] = 'i';
				payload[2] = '\0';
				// aplSendCustomBROADCAST(3,payload); // 测试广播
				aplSendCustomMSG(IEEE_ADDRESS_ARRAY&0xff,1,3,payload);
				DelayMs(1);
			}
#endif

			last_timer = halGetMACTimer();
		}

		// Now check route table
		if (halMACTimerNowDelta(last_route_update) > MSECS_TO_MACTICKS(1000)) {
			last_route_update = halGetMACTimer();
			if (isNeedCheckChildren == FALSE) {
#ifdef LRWPAN_COORDINATOR
				send_custom_upload_route_request();
				DelayMs(1);
#endif
			} else {
				check_my_children_online();
			}

			display_all_nodes();
			isNeedCheckChildren = 1 - isNeedCheckChildren; // True or False convert from each other
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

	//this initialization set our SADDR to 0xFFFF,
	//PANID to the default PANID

	//HalInit, evbInit will have to be called by the user

	numTimeouts = 0;
	my_timer = 0;

	// 正在组网中，自检置为0xff
	is_self_check_ok=0xff;

	init_all_nodes();

	halInit();
	aplInit();  //init the stack
	conPrintConfig();
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts

	joinState = JOIN_STATE_START_JOIN;  //main while(1) initial state
	isOffline=TRUE;

	// Initialize for LMX2581
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	ctl_lmx2581_init();
	DelayMs(600);
	ctl_lmx2581_init();

	// 这个必须要设置0xff，否则无法接收东西
#ifdef LRWPAN_COORDINATOR
	dynamic_freq_mode=0xff;
#endif

	ping_cnt = 0;
	// debug_level = 10;

	//This version of TestPing has Network Rejoin logic in case of excessive failures
	//by the TestPing() function during transmit.
	while (1) {
		apsFSM();
		switch (joinState) {

			case JOIN_STATE_START_JOIN:
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
						is_self_check_ok=TRUE;
					} else {
						//only reason for this to fail is some sort of hardware failure
						conPrintROMString("Network formation failed, waiting, trying again\n");
						is_self_check_ok=FALSE;
						my_timer= halGetMACTimer();
						//wait for 2 seconds
						while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
						joinState = JOIN_STATE_START_JOIN;
					}
#else
				if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
					conPrintROMString("Network Join succeeded!\r\n");
					printJoinInfo();
					add_to_my_parent();
					joinState = JOIN_STATE_RUN_APP1;
					is_self_check_ok=TRUE;
				} else {
					conPrintROMString("Network Join FAILED! Waiting, then trying again\r\n");
					my_timer = halGetMACTimer();
					//wait for 2 seconds
					while ((halMACTimerNowDelta(my_timer)) < MSECS_TO_MACTICKS(2 * 1000))
						;
					joinState = JOIN_STATE_START_JOIN;
					is_self_check_ok=FALSE;
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
				TestPing();//only exits on if excessive misses
#ifdef LRWPAN_COORDINATOR
				if(dynamic_freq_mode!=0xff)
					work_under_dynamic_mode();
#endif
				//reset first_packet to flag to true, this causes Coord to just
				//wait for a packet, and not try any retransmit on misses.
#else

				conPrintROMString("Hit any switch to start!\r\n")
				;
				joinState = JOIN_STATE_RUN_APP2;
#endif
				break;
			case JOIN_STATE_RUN_APP2:
				DelayMs(200);
			case JOIN_STATE_RUN_APP3:
				//WARNING - this is only for latency testing, this variable is normally
				//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
				//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
				//only do this in your normal code if you want to disable automatic retries
				aplSetMacMaxFrameRetries(0);
				//switch is pressed, run app
				dstADDR.saddr = 0;  //RFD sends to the coordinator
				my_timer = halGetMACTimer();  //timer must be initialized before entering PP_SEND_STATE
				TestPing();  //only exits on if excessive misses
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

	BYTE *ptr;
	static unsigned int recv_conter = 0;
	//just print out this data
	printf("#%u:", recv_conter++);
	conPrintROMString("User Data Packet Received: ");

	ptr = aplGetRxMsgData();
	printf("%s\r\n", ptr);

	//use this source address as the next destination address
	dstADDR.saddr = aplGetRxSrcSADDR();
	return LRWPAN_STATUS_SUCCESS;

}

void aplRxCustomCallBack(){
	unsigned char *ptr,len,i;
	printf("recv a custom packet, type=");

	switch(aplGetCustomRxMsgType()){
		case CUSTOM_FRAME_TYPE_BROADCAST:
			printf("broadcast: ");
			break;
		case CUSTOM_FRAME_TYPE_DATA:
			printf("msg: ");
			break;
		default:
			printf("unsupport msg in RxCallback\r\n");
			return;
	}
	ptr=aplGetCustomRxMsgData();
	len=aplGetCustomRxMsgLen();
	for(i=0;i<len;++i)
		putchar(*(ptr+i));
	puts("");
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
