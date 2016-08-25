/*
 * coord_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#include "FSM_coord.h"
#include "hal.h"
#include "route_table.h"
#include "stdio.h"

COORD_STATE_ENUM coord_FSM_state;

unsigned char coord_nodes_list[MAX_COORD_NUM];

void coordFSM(){
	unsigned char i;
	switch(coord_FSM_state){
		case COORD_STATE_INITAILIZE_ALL_NODES:
			init_all_nodes();
			coord_FSM_state=COORD_STATE_FORM_NETWORK;
			break;
		case COORD_STATE_FORM_NETWORK:
			printf("Coordinator, my addr is %u, Network formed\r\n",MY_NODE_NUM);
			coord_FSM_state=COORD_STATE_SEND_BEACON;
			last_timer_children_checked=last_timer_beacon_sent=halGetMACTimer();
			isOffline=FALSE;
			break;
		case COORD_STATE_SEND_BEACON:
			if(halMACTimerNowDelta(last_timer_beacon_sent)>=MSECS_TO_MACTICKS(INTERVAL_OF_SENDING_BEACON*1000)){
				// TODO: clear all coord nodes
				//coord_send_beacon();
				last_timer_beacon_sent=halGetMACTimer();
			}
			coord_FSM_state=COORD_STATE_CHECK_CHILDREN;
			break;
		case COORD_STATE_CHECK_CHILDREN:
			if(halMACTimerNowDelta(last_timer_children_checked)>=MSECS_TO_MACTICKS(INTERVAL_OF_CHECKING_CHILDREN*1000)){
				check_my_children_online();
				display_all_nodes();
				last_timer_children_checked=halGetMACTimer();
			}
			coord_FSM_state=COORD_STATE_SEND_BEACON;
			break;
		case COORD_STATE_DOWNGRADE_TO_ROUTER:
			// TODO: 降级为路由器 2016年8月23日 下午11:31:38
			return;
			break;
		default:
			coord_FSM_state=COORD_STATE_INITAILIZE_ALL_NODES;
			break;
	}
}

void coord_send_beacon(){
	static unsigned char coord_beacon_payload[FRAME_LENGTH_BEACON];
	coord_beacon_payload[0]=FRAME_TYPE_SHORT_BEACON;
	coord_beacon_payload[1]=0xff;
	coord_beacon_payload[2]=MY_NODE_NUM;
	halSendPacket(FRAME_LENGTH_BEACON, coord_beacon_payload, TRUE);
}
