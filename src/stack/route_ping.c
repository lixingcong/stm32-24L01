/*
 * route_ping.c
 *
 *  Created on: 2016年8月16日
 *      Author: lixingcong
 *      Send a ping query to make sure destination node is online
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#include "route_ping.h"
#include "route_table.h"
#include "delay.h"
#include "hal.h"
#include "stdio.h"

extern unsigned int last_timer_parent_checked_me;  // defined in FSM_router.c
unsigned char all_nodes_ping[ALL_NODES_NUM];

//used for ping
typedef struct _ROUTE_PING_DATA {
	unsigned int last_tx_timer;
	unsigned char ackPending;
	unsigned char dst;
} ROUTE_PING_DATA;

ROUTE_PING_DATA route_ping_data;
ROUTE_PING_DATA route_longping_data;

// ping时延

unsigned char macTxPing(unsigned char dst, BOOL isRequest, unsigned char direction) {
	unsigned int timer;
	unsigned char ping[FRAME_LENGTH_PING];
	ping[0] = FRAME_TYPE_SHORT_PING;
	ping[1] = dst;
	ping[2] = MY_NODE_NUM;
	if (isRequest == TRUE)
		ping[3] = 0xf0;  // ping request
	else
		ping[3] = 0x00;  // ping response
	ping[3] |= direction;

	halSendPacket(FRAME_LENGTH_PING, ping, TRUE);

#ifdef MAC_OUTPUT_DEBUG_PING
	printf("macTxPing(): sent ping packet\r\n");
#endif

	if (isRequest == TRUE) {
		route_ping_data.ackPending = TRUE;
		route_ping_data.last_tx_timer = halGetMACTimer();
		route_ping_data.dst = dst;
		while (1) {
			timer = halMACTimerNowDelta(route_ping_data.last_tx_timer);

			if (route_ping_data.ackPending == FALSE)
				break;
			// TODO: ping timeout=5ms
			if (timer > 5) {
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macTxPing(): ping time out\r\n");
#endif
				break;
			}
		}
		if (route_ping_data.ackPending == FALSE) {  // Successfully got ping ACK, return latency
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macTxPing(): ping ack successfully\r\n");
#endif
			return (unsigned char) (timer);
		}

	} else
		return 0;

	return 0xff;  //timeout

}

/*
 * 自定义的ping函数
 * direction: 有三种方向：父亲->孩子、孩子->父亲、不分方向
 * retry_times: ping 重试次数
 * retry_interval: ping 重试间隔，单位毫秒
 */
unsigned char macTxCustomPing(unsigned char dst, unsigned char direction, unsigned char retry_times, unsigned short retry_interval) {
	unsigned char ping_cnt, result;
	unsigned int last_ping_timer;
	ping_cnt = retry_times;
	result = macTxPing(dst, TRUE, direction);
	if (result == 0xff) {
		last_ping_timer = halGetMACTimer();
		while (1) {
			if (halMACTimerNowDelta(last_ping_timer) > (retry_interval)) {
				printf(" ping to #%u: retry for %u times\r\n", dst, ping_cnt);
				result = macTxPing(dst, TRUE, direction);
				if (result == 0xff)
					--ping_cnt;
				else
					return result;
				if (ping_cnt == 0)
					break;
				last_ping_timer = halGetMACTimer();
			}
		}
	}
	return result;
}

void macRxPingCallback(unsigned char *ptr,BOOL isLongPing) {
	if (isLongPing == FALSE) { // msg is [SHORT PING]
		if (*(ptr + 3) == (MY_NODE_NUM)) {
			if ((*(ptr + 5) & 0xf0) == 0xf0) {  // receive a ping request, make a response to him
				switch ((*(ptr + 5)) & 0x0f) {
					case PING_DIRECTION_TO_CHILDREN:
						macTxPing(*(ptr + 4), FALSE, PING_DIRECTION_TO_PARENT);
						my_parent = *(ptr + 4);  // 父亲的给我发的，根据父亲的的ping方向改变路由
						last_timer_parent_checked_me = halGetMACTimer();  // 更新定时器：父亲刚刚给我检查了！
						isOffline = FALSE;
						break;
					case PING_DIRECTION_TO_PARENT:
						all_nodes[*(ptr + 4)] = MY_NODE_NUM;  // 孩子发给我的东西，根据孩子的ping方向改变路由表
						macTxPing(*(ptr + 4), FALSE, PING_DIRECTION_TO_CHILDREN);
						break;
					default:
						macTxPing(*(ptr + 4), FALSE, PING_DIRECTION_TO_OTHERS);
						break;
				}

			} else if ((*(ptr + 5) & 0xf0) == 0x00) {  // receive a ping response, mark mac_ping_data.ackpending as FALSE
				if (*(ptr + 4) == route_ping_data.dst) {
#ifdef MAC_OUTPUT_DEBUG_PING
					printf("macRxPingCallback(): dst replied ACK ok\r\n");
#endif
					route_ping_data.ackPending = FALSE;
				}
			} else {
				// invalid ping packet
			}
		} else {  // ping dst is not me
			if ((*(ptr + 5) & 0x0f) == PING_DIRECTION_TO_CHILDREN) {  // 对于目标不是自己的packet，旁人可以偷听到。借此可以实现邻居更新
				all_nodes[*(ptr + 3)] = *(ptr + 4);
			}
		}
	}else{ // msg is [LONG PING]
		if(*(ptr+7)==FRAME_FLAG_LONGPING_REQUEST){ // recv a long ping request, reply it
			macTxPingLongDistance(*(ptr+5), FALSE);
		}else{ // recv a long ping response
			if(*(ptr+5)==route_longping_data.dst){ // src is my desired node
				route_longping_data.ackPending=FALSE; // clear pending flag
			}
		}
	}

}

void ping_all_nodes(){
	unsigned char i;
	for(i=0;i<ALL_NODES_NUM;++i)
		if(all_nodes[i]!=0xff)
			all_nodes_ping[i]=macTxPing(i, TRUE, PING_DIRECTION_TO_OTHERS);
}

unsigned char macTxPingLongDistance(unsigned char dst,BOOL isRequest){
	unsigned char ping_long_flag;
	unsigned int timer;

	if(isRequest==TRUE)
		ping_long_flag=FRAME_FLAG_LONGPING_REQUEST; // payload
	else
		ping_long_flag=FRAME_FLAG_LONGPING_RESPONSE;

	// 调用长消息发送
	send_custom_packet(MY_NODE_NUM, dst, 1, &ping_long_flag, FRAME_TYPE_LONG_PING, LONG_MSG_DEFAULT_TTL);

	if (isRequest == TRUE) {
		route_longping_data.ackPending = TRUE;
		route_longping_data.last_tx_timer = halGetMACTimer();
		route_longping_data.dst = dst;
		while (1) {
			timer = halMACTimerNowDelta(route_longping_data.last_tx_timer);
			if (route_longping_data.ackPending == FALSE)
				break;
			// TODO: ping timeout=(20ms * default TTL)
			if (timer > LONG_MSG_DEFAULT_TTL*20)
				break;
		}
		if (route_longping_data.ackPending == FALSE)   // Successfully got ping ACK, return latency
			return (unsigned char) (timer);

	} else
		return 0;

	return 0xff;  //timeout

}
