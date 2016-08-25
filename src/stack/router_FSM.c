/*
 * router_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#include "common_func.h"
#include "router_FSM.h"
#include "hal.h"
#include "route_table.h"
#include "route_ping.h"
#include "delay.h"
#include "stdio.h"

ROUTER_STATE_ENUM router_FSM_state;

unsigned char router_fsm_payload[10];
unsigned int last_timer_parent_checked_me;

void router_FSM(){
	switch(router_FSM_state){
		case ROUTER_STATE_INITAILIZE_ALL_NODES:
			if(MY_NODE_NUM>=ALL_NODES_NUM){ // 超过节点。
				printf("ERROR: max allow nodes num is %u, but my addr is %u\r\n",ALL_NODES_NUM,MY_NODE_NUM);
				while(1);
			}
			init_all_nodes();
			router_FSM_state=ROUTER_STATE_JOIN_NETWORK;
			last_timer_children_checked=halGetMACTimer();
			printf("Router, my addr is %u\r\n",MY_NODE_NUM);
			isOffline=TRUE;
			break;
		case ROUTER_STATE_JOIN_NETWORK:
			if(isOffline==TRUE){ // offline
				if(halMACTimerNowDelta(last_timer_children_checked)>=MSECS_TO_MACTICKS((2000+(MY_NODE_NUM>>2)))){
					printf("trying to join network\r\n");
					router_send_join_request();
					last_timer_children_checked=halGetMACTimer();
				}
			}else{ // online
				printf("join succuessfully, parent is #%u\r\n",my_parent);
				router_FSM_state=ROUTER_STATE_CHECK_PARENT;
				last_timer_parent_checked_me=halGetMACTimer();
			}
			break;
		case ROUTER_STATE_CHECK_PARENT:
			if(halMACTimerNowDelta(last_timer_parent_checked_me)>=MSECS_TO_MACTICKS(INTERVAL_OF_MY_PARENT_CHECK_ME*1000)){
				printf("no parent's ping for a while, check it\r\n");
				if(0xff==macTxCustomPing(my_parent, PING_DIRECTION_TO_PARENT, 2, 200)){
					isOffline=TRUE;
					router_FSM_state=ROUTER_STATE_JOIN_NETWORK;
					printf("Start to rejoin...\r\n");
					break;
				}
				last_timer_parent_checked_me=halGetMACTimer();
			}
			router_FSM_state=ROUTER_STATE_CHECK_CHILDREN;
			break;
		case ROUTER_STATE_CHECK_CHILDREN:
			if(halMACTimerNowDelta(last_timer_children_checked)>=MSECS_TO_MACTICKS(INTERVAL_OF_CHECKING_CHILDREN*1000)){
				check_my_children_online();
				if(route_response_offset>3)
					send_route_increasing_change_to_parent();
				display_all_nodes();
				last_timer_children_checked=halGetMACTimer();
			}
			router_FSM_state=ROUTER_STATE_CHECK_PARENT;
			break;
		case ROUTER_STATE_UPGRADE_TO_COORD:
			// TODO: 升级为协调器 2016年8月25日 上午10:15:39
			break;
		default:
			break;
	}
}

void router_send_join_request(){
	router_fsm_payload[0]=FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
	router_fsm_payload[1]=0xff;
	router_fsm_payload[2]=MY_NODE_NUM;
	router_fsm_payload[3]=FRAME_FLAG_JOIN_REQUEST;
	halSendPacket(4, router_fsm_payload, TRUE);
}

// TODO: send_PAATH_to_PC， 依赖多跳实现 2016年8月24日 下午1:51:45
