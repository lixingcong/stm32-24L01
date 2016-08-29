/*
 * router_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: lixingcong
 *      This is a FSM(Finite State Machine) for router
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#include "FSM_router.h"
#include "FSM_coord.h"
#include "hal.h"
#include "route_table.h"
#include "route_ping.h"
#include "delay.h"
#include "stdio.h"

ROUTER_STATE_ENUM router_FSM_state;

unsigned int last_timer_parent_checked_me;
void update_route_cache_and_find_difference();

void router_FSM() {
	switch (router_FSM_state) {
		case ROUTER_STATE_INITAILIZE_ALL_NODES:
			check_if_exceed_max_node_range();
			init_all_nodes();
			router_FSM_state = ROUTER_STATE_JOIN_NETWORK;
			last_timer_children_checked = halGetMACTimer();
			printf("Router, my addr is %u\r\n", MY_NODE_NUM);
			isOffline = TRUE;
			break;

		case ROUTER_STATE_JOIN_NETWORK:
			if (isOffline == TRUE) {  // offline
				if (halMACTimerNowDelta(last_timer_children_checked) >= ((2000 + (MY_NODE_NUM << 1)))) {
					printf("trying to join network\r\n");
					router_send_join_request();
					last_timer_children_checked = halGetMACTimer();
				}
			} else {  // online
				printf("join successfully, parent is #%u\r\n", my_parent);
				router_FSM_state = ROUTER_STATE_CHECK_PARENT;
				last_timer_parent_checked_me = halGetMACTimer();
			}
			break;

		case ROUTER_STATE_CHECK_PARENT:
			if (halMACTimerNowDelta(last_timer_parent_checked_me) >= (INTERVAL_OF_MY_PARENT_CHECK_ME * 1000)) {
				printf("no parent's ping for a while, check it\r\n");
				if (0xff == macTxCustomPing(my_parent, PING_DIRECTION_TO_PARENT, 2, 200)) {  // ping my parent timeout, mark myself as offline
					isOffline = TRUE;
					router_FSM_state = ROUTER_STATE_JOIN_NETWORK;
					printf("Start to rejoin...\r\n");
					break;
				}
				last_timer_parent_checked_me = halGetMACTimer();
			}
			router_FSM_state = ROUTER_STATE_CHECK_CHILDREN;
			break;

		case ROUTER_STATE_CHECK_CHILDREN:
			if (halMACTimerNowDelta(last_timer_children_checked) >= (INTERVAL_OF_CHECKING_CHILDREN * 1000)) {
				check_my_children_online();
				update_route_cache_and_find_difference();
				if (route_response_offset > 3)  // if differences exists
					send_route_increasing_change_to_parent();
				display_all_nodes();
				last_timer_children_checked = halGetMACTimer();
			}
			router_FSM_state = ROUTER_STATE_CHECK_PARENT;
			break;

		case ROUTER_STATE_UPGRADE_TO_COORD:
			// TODO: 升级为协调器，在什么条件下触发 2016年8月25日 上午10:15:39
			mainFSM = coord_FSM;
			coord_FSM_state = COORD_STATE_FORM_NETWORK;
			break;

		default:
			router_FSM_state = ROUTER_STATE_INITAILIZE_ALL_NODES;
			break;
	}
}

void router_send_join_request() {
	static unsigned char router_join_payload[FRAME_LENGTH_JOIN_INFO];
	router_join_payload[0] = FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
	router_join_payload[1] = 0xff;
	router_join_payload[2] = MY_NODE_NUM;
	router_join_payload[3] = FRAME_FLAG_JOIN_REQUEST;
	halSendPacket(FRAME_LENGTH_JOIN_INFO, router_join_payload, TRUE);
}

// 查找路由表差异，以便于传输增量路由表
void update_route_cache_and_find_difference() {
	unsigned char i;
	for (i = 0; i < ALL_NODES_NUM; ++i) {
		if (all_nodes[i] != all_nodes_cache[i]) {
			if (all_nodes_cache[i] == 0xff)
				update_route_response_content(TRUE, i, all_nodes[i]);  // add
			else
				update_route_response_content(FALSE, i, all_nodes_cache[i]);  // delete

		}
		all_nodes_cache[i] = all_nodes[i];
	}
}

