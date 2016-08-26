/*
 * coord_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: lixingcong
 *      This is a FSM(Finite State Machine) for coordinator
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#include "FSM_coord.h"
#include "FSM_router.h"
#include "hal.h"
#include "route_table.h"
#include "stdio.h"

COORD_STATE_ENUM coord_FSM_state;

unsigned char coord_nodes_list[MAX_COORD_NUM];

void coord_FSM() {
	switch (coord_FSM_state) {
		case COORD_STATE_INITAILIZE_ALL_NODES:
			check_if_exceed_max_node_range();
			init_all_nodes();
			coord_FSM_state = COORD_STATE_FORM_NETWORK;
			break;

		case COORD_STATE_FORM_NETWORK:
			printf("Coordinator, my addr is %u, Network formed\r\n", MY_NODE_NUM);
			coord_FSM_state = COORD_STATE_SEND_BEACON;
			last_timer_children_checked = last_timer_beacon_sent = halGetMACTimer();
			isOffline = FALSE;
			break;

		case COORD_STATE_SEND_BEACON:
			if (halMACTimerNowDelta(last_timer_beacon_sent) >= (INTERVAL_OF_SENDING_BEACON * 1000)) {
				// TODO: clear all coord nodes
				//coord_send_beacon();
				last_timer_beacon_sent = halGetMACTimer();
			}
			coord_FSM_state = COORD_STATE_CHECK_CHILDREN;
			break;

		case COORD_STATE_CHECK_CHILDREN:
			if (halMACTimerNowDelta(last_timer_children_checked) >= (INTERVAL_OF_CHECKING_CHILDREN * 1000)) {
				check_my_children_online();
				display_all_nodes();
				last_timer_children_checked = halGetMACTimer();
			}
			coord_FSM_state = COORD_STATE_SEND_BEACON;
			break;

		case COORD_STATE_DOWNGRADE_TO_ROUTER:
			// TODO: 降级为路由器，在什么条件下触发 2016年8月23日 下午11:31:38
			mainFSM = router_FSM;
			isOffline = TRUE;
			router_FSM_state = ROUTER_STATE_JOIN_NETWORK;
			break;

		default:
			coord_FSM_state = COORD_STATE_INITAILIZE_ALL_NODES;
			break;
	}
}

void coord_send_beacon() {
	static unsigned char coord_beacon_payload[FRAME_LENGTH_BEACON];
	coord_beacon_payload[0] = FRAME_TYPE_SHORT_BEACON;
	coord_beacon_payload[1] = 0xff;
	coord_beacon_payload[2] = MY_NODE_NUM;
	halSendPacket(FRAME_LENGTH_BEACON, coord_beacon_payload, TRUE);
}
