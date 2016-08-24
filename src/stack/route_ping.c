/*
 * route_ping.c
 *
 *  Created on: 2016年8月16日
 *      Author: li
 *      Ping时延
 */

#include "common_func.h"
#include "route_ping.h"
#include "route_table.h"
#include "delay.h"
#include "hal.h"
#include "stdio.h"
#include "router_FSM.h"

//used for ping
typedef struct _ROUTE_PING_DATA {
	unsigned int last_tx_timer;
	unsigned char ackPending;
	unsigned char dst;
}ROUTE_PING_DATA;

ROUTE_PING_DATA route_ping_data;

// ping时延

unsigned char macTxPing(unsigned char dst, BOOL isRequest, unsigned char direction){
	unsigned int timer;
	unsigned char ping[LRWPAN_PINGFRAME_LENGTH];
	ping[0]=FRAME_TYPE_SHORT_PING;
	ping[1]=dst;
	ping[2]=MY_NODE_NUM;
	if(isRequest==TRUE)
		ping[3]=0x10; // ping request
	else
		ping[3]=0x00; // ping response
	ping[3]|=direction;

	halSendPacket(LRWPAN_PINGFRAME_LENGTH, ping, TRUE);

#ifdef MAC_OUTPUT_DEBUG_PING
	printf("macTxPing(): sent ping packet\r\n");
#endif

	if (isRequest == TRUE) {
		route_ping_data.ackPending=TRUE;
		route_ping_data.last_tx_timer=halGetMACTimer();
		route_ping_data.dst=dst;
		while (1) {
			timer = halMACTimerNowDelta(route_ping_data.last_tx_timer);

			if(route_ping_data.ackPending == FALSE)
				break;
			// ping timeout=30ms
			if (timer > MSECS_TO_MACTICKS(30)){
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

}

/*
 * 自定义的ping函数
 * retry_times: ping 重试次数
 * retry_interval: ping 重试间隔，单位毫秒
 */
unsigned char macTxCustomPing(unsigned char dst, unsigned char direction, unsigned char retry_times, unsigned short retry_interval){
	unsigned char ping_cnt,result;
	unsigned int last_ping_timer;
	ping_cnt=retry_times;
	result=macTxPing(dst, TRUE, direction);
	if(result==0xff){
		last_ping_timer=halGetMACTimer();
		while(1){
			if(halMACTimerNowDelta(last_ping_timer) > MSECS_TO_MACTICKS(retry_interval)){
				printf(" ping to #%u: retry for %u times\r\n",dst,ping_cnt);
				result=macTxPing(dst, TRUE, direction);
				if(result==0xff)
					--ping_cnt;
				else
					return result;
				if(ping_cnt==0)
					break;
				last_ping_timer=halGetMACTimer();
			}
		}
	}
	return result;
}


// TODO: send direction 改变路由表 2016年8月23日 下午11:20:13
void macRxPingCallback(unsigned char *ptr) {
	if (*(ptr+3) == (MY_NODE_NUM)) {
		if(isOffline==TRUE) // 离线状态不响应ping
			return;
		if ((*(ptr + 5) & 0x10 ) == 0x10) {  // receive a ping request, make a response to him
			// todo: 这里出现一种情况：父亲的路由表没有孩子，却给孩子回复ping包，需要修正
			switch((*(ptr+5))&0x0f){
				case PING_DIRECTION_TO_CHILDREN:
					macTxPing(*(ptr+4), FALSE, PING_DIRECTION_TO_PARENT);
					last_timer_parent_checked_me=halGetMACTimer();// 更新定时器：父亲刚刚给我检查了！
					break;
				case PING_DIRECTION_TO_PARENT:
					all_nodes[*(ptr+4)]=MY_NODE_NUM;  // 孩子发给我的东西，通通接收，并改变路由表
					macTxPing(*(ptr+4), FALSE, PING_DIRECTION_TO_CHILDREN);
					break;
				default:
					macTxPing(*(ptr+4), FALSE, PING_DIRECTION_TO_OTHERS);
					break;
			}

		} else {  // receive a ping response, mark mac_ping_data.ackpending as FALSE
			if (*(ptr + 4) == route_ping_data.dst) {
#ifdef MAC_OUTPUT_DEBUG_PING
				printf("macRxPingCallback(): ok dsn in macPingcallback()\r\n");
#endif
				route_ping_data.ackPending = FALSE;
			}
		}
	}else{ // ping dst is not me
		if((*(ptr+5)&0x0f)==PING_DIRECTION_TO_CHILDREN){
			all_nodes[*(ptr+3)]=*(ptr+4); // 对于偷听到的Ping包，可以实现邻居更新
		}
	}
}
