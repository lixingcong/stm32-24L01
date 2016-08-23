/*
 * router_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#include "common_func.h"
#include "router_FSM.h"
#include "hal.h"

ROUTER_STATE_ENUM router_FSM_state;

void route_FSM(){
	switch(router_FSM_state){
		case ROUTER_STATE_INITAILIZE_ALL_NODES:

			break;
		case ROUTER_STATE_JOIN_NETWORK:
			break;
		case ROUTER_STATE_CHECK_CHILDREN:
			break;
		case ROUTER_STATE_UPGRADE_TO_COORD:
			break;
		default:
			break;
	}
}

