/*
 * route_ping.c
 *
 *  Created on: 2016年8月16日
 *      Author: li
 *      Ping时延
 */

#include "route_ping.h"
#include "route_table.h"
#include "delay.h"
#include "hal.h"


//used for ping
typedef struct _ROUTE_PING_DATA {
	unsigned int last_tx_timer;
	unsigned char ackPending;
	unsigned char dsn;
}ROUTE_PING_DATA;

ROUTE_PING_DATA route_ping_data;

// ping时延
unsigned char macTxPing(unsigned char dst, unsigned char dsn, BOOL isRequest) {
#if 0
	unsigned int timer;
	unsigned char ping[LRWPAN_PINGFRAME_LENGTH];

	// format ping
	ping[0]=LRWPAN_PINGFRAME_LENGTH;
	if(isRequest==TRUE)
		ping[1]=0x01; // ping request
	else
		ping[1]=0x02; // ping response
	ping[2]=MY_NODE_NUM;
	ping[3]=dst;
	ping[4]=dsn;

	// send ping
	halSendPacket(LRWPAN_PINGFRAME_LENGTH, &ping[0],TRUE);
	DelayMs(1);

#ifdef MAC_OUTPUT_DEBUG_PING
	printf("macTxPing(): sent ping packet\r\n");
#endif

	if (isRequest == TRUE) {
		route_ping_data.dsn=dsn;
		route_ping_data.ackPending=TRUE;
		route_ping_data.last_tx_timer=halGetMACTimer();
		while (1) {
			timer = halMACTimerNowDelta(route_ping_data.last_tx_timer);

			if(route_ping_data.ackPending == FALSE)
				break;
			// ping timeout=100ms
			if (timer > MSECS_TO_MACTICKS(100)){
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macTxPing(): ping time out\r\n");
#endif
				break;
			}
		}
		if (route_ping_data.ackPending == FALSE){
#ifdef MAC_OUTPUT_DEBUG_PING
			printf("macTxPing(): ping ack successfully\r\n");
#endif
			return (unsigned char) (MACTICKS_TO_MSECS(timer));  // Successfully ack
		}

	}else
		return 0;

	return 0xff; //timeout
#endif
}

void macRxPingCallback(unsigned char *ptr) {
	if (*(ptr+3) == (MY_NODE_NUM)) {
		if (*(ptr + 1) == 0x01) {  // receive a ping request, make a response to him
			// todo: 这里出现一种情况：父亲的路由表没有孩子，却给孩子回复ping包，需要修正
			if(all_nodes[*(ptr+2)]==*(ptr+3)||all_nodes[*(ptr+3)]==*(ptr+2))
			macTxPing(*(ptr+2), *(ptr + 4), FALSE);
		} else if (*(ptr + 1) == 0x02) {  // receive a ping response, mark mac_ping_data.ackpending as FALSE
			if (*(ptr + 4) == route_ping_data.dsn) {
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macRxPingCallback(): ok dsn in macPingcallback()\r\n");
#endif
				route_ping_data.ackPending = FALSE;
			} else{
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macRxPingCallback(): FAIL dsn in macPingcallback()\r\n");
#endif
			}
		} else{
#ifdef MAC_OUTPUT_DEBUG_PING
			printf("macRxPingCallback(): ping pkt is mine, but not a valid ping.\r\n");
#endif
		}
	}else{
#ifdef MAC_OUTPUT_DEBUG_PING
		printf("macRxPingCallback(): not my ping packet\r\n");
#endif
	}

}
