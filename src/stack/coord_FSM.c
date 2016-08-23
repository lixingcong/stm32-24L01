/*
 * coord_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#include "common_func.h"
#include "coord_FSM.h"
#include "hal.h"
#include "route_table.h"

#define INTERVAL_OF_SENDING_BEACON 10
#define INTERVAL_OF_CHECKING_CHILDREN 5

COORD_STATE_ENUM coord_FSM_state;

unsigned int last_timer_beacon_sent;
unsigned int last_timer_children_checked;

unsigned char coord_fsm_payload[10];
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
			break;
		case COORD_STATE_SEND_BEACON:
			if(halMACTimerNowDelta(last_timer_beacon_sent)>=MSECS_TO_MACTICKS(INTERVAL_OF_SENDING_BEACON*1000)){
				// TODO: clear all coord nodes
				coord_send_beacon();
				last_timer_beacon_sent=halGetMACTimer();
			}
			coord_FSM_state=COORD_STATE_CHECK_CHILDREN;
			break;
		case COORD_STATE_CHECK_CHILDREN:
			if(halMACTimerNowDelta(last_timer_children_checked)>=MSECS_TO_MACTICKS(INTERVAL_OF_SENDING_BEACON*1000)){

				last_timer_children_checked=halGetMACTimer();
			}
			break;
		case COORD_STATE_DESTROY_NETWORK:
			break;
		default:
			break;
	}
}

void coord_send_beacon(){
	coord_fsm_payload[0]=FRAME_TYPE_SHORT_BEACON;
	coord_fsm_payload[1]=0xff;
	coord_fsm_payload[2]=MY_NODE_NUM;
	halSendPacket(3, coord_fsm_payload, TRUE);
}
