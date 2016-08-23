/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */


#include "hal.h"

MY_ROLE_ENUM my_role;
unsigned char my_parent;//父亲节点

unsigned char halGetMACTimer(void) {
	return systick_count;
}

unsigned short halGetRandomShortByte(void) {
	return (systick_count & 0xffff);
}

void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode){
	if(isShortDataLengthMode==TRUE){

	}else{

	}
}
