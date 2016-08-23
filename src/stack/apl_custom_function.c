/*
 * apl_custom_function.c
 *
 *  Created on: 2016年8月12日
 *      Author: li
 */

#include "apl_custom_function.h"
#include "route_table.h"
#include "route_ping.h"
#include "delay.h"
#include "timer2.h"
#include "hal.h"
#include "stdio.h"

APS_CUSTOM_FRAME my_custom_frame;

void update_AP_msg(unsigned char *ptr){
	unsigned char i;
	my_custom_frame.data=ptr+5;
	my_custom_frame.flen=*(ptr)-5;
	my_custom_frame.src_addr=*(ptr+3);

	// frame type
	my_custom_frame.frame_type=*(ptr+4);
}

void update_route_table_info() {
	static unsigned char isNeedCheckChildren;
	// TODO: 协调器掉线后如何让路由器自动更新路由 全局标志位 2016年8月17日 上午9:08:00
	if (isNeedCheckChildren == 1) {

#ifdef LRWPAN_COORDINATOR
		send_custom_upload_route_request();
		DelayMs(1);
#else
		// router should check his route table without receiving regular update request
		// interval is 4 seconds
		if(halMACTimerNowDelta(last_route_updated_timer) > MSECS_TO_MACTICKS(TIM2_UPDATE_TIME*3000)){
			printf("my coord is offline, now i want to update my route tbl\r\n");
			send_custom_upload_route_request();
			last_route_updated_timer=halGetMACTimer();
		}
#endif
	} else {
		check_my_children_online();
		update_route_table_cache();
		display_all_nodes();
	}
	isNeedCheckChildren = 1 - isNeedCheckChildren;  // True or False convert from each other
}

/*
 * 自定义的ping函数
 * retry_times: ping 重试次数
 * retry_interval: ping 重试间隔，单位毫秒
 */
unsigned char macTxCustomPing(unsigned char dst, BOOL isRequest, unsigned char retry_times, unsigned short retry_interval){
	unsigned char ping_cnt,result;
	unsigned int last_ping_timer;
	ping_cnt=retry_times;
	result=macTxPing(dst, halGetRandomShortByte()&0xff, TRUE);
	if(result==0xff){
		last_ping_timer=halGetMACTimer();
		while(1){
			if(halMACTimerNowDelta(last_ping_timer) > MSECS_TO_MACTICKS(retry_interval)){
				printf(" ping: retry for %u times\r\n",ping_cnt);
				result=macTxPing(dst, halGetRandomShortByte()&0xff, TRUE);
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
