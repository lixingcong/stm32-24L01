/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */


#include "hal.h"
#include "A7190.h"


unsigned char hal_payload[LRWPAN_MAX_FRAME_SIZE];

unsigned char halGetMACTimer(void) {
	return systick_count;
}

unsigned short halGetRandomShortByte(void) {
	return (systick_count & 0xffff);
}

void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode){
	if(flen>=LRWPAN_MAX_FRAME_SIZE-2){
		printf("halSendPacket: packet too long, drop\r\n");
		return;
	}
	if(isShortDataLengthMode==TRUE && flen<=256){
		WriteFIFO1(0x00);
	}else{
		WriteFIFO1(0xf0|((flen>>8)&0x01));
	}
	WriteFIFO1(flen&0xff);
	// TODO: WriteFIFO() para flen should be UINT16
	WriteFIFO(ptr,flen);
}
