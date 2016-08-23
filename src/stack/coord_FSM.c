/*
 * coord_FSM.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#include "coord_FSM.h"

COORD_STATE_ENUM coord_FSM_state;

void coordFSM(){
	switch(coord_FSM_state){
		case COORD_STATE_INITAILIZE_ALL_NODES:
			break;
		case COORD_STATE_FORM_NETWORK:
			break;
		case COORD_STATE_SEND_BEACON:
			break;
		case COORD_STATE_CHECK_CHILDREN:
			break;
		case COORD_STATE_DESTROY_NETWORK:
			break;
		default:
			break;
	}
}
