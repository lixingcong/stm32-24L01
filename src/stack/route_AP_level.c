/*
 * route_AP_level.c
 *
 *  Created on: 2016年8月12日
 *      Author: lixingcong
 *      Working on Application Layer, providing API for user: sendCustomMsg, etc.
 */

#include "route_AP_level.h"
#include "route_table.h"
#include "route_ping.h"
#include "delay.h"
#include "hal.h"
#include "stdio.h"

APS_CUSTOM_FRAME my_custom_frame;

void update_AP_msg(unsigned char *ptr,unsigned short flen){
	unsigned char i;
	my_custom_frame.data=ptr+6;
	my_custom_frame.flen=flen-6;
	my_custom_frame.src_addr=*(ptr+5);

	// frame type
	my_custom_frame.frame_type=*(ptr+2);
}


