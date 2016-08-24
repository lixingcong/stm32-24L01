/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */


#include "hal.h"
#include "A7190.h"


unsigned char hal_payload[LRWPAN_MAX_FRAME_SIZE];

unsigned int halGetMACTimer(void) {
	return systick_count;
}

unsigned short halGetRandomShortByte(void) {
	return (systick_count & 0xffff);
}

void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode){
	unsigned int flen_real;
	unsigned char last_a7190_state;
	flen_real=flen+2;

	if (flen_real > LRWPAN_MAX_FRAME_SIZE) {
		printf("halSendPacket: packet too long, drop\r\n");
		return;
	}

	last_a7190_state=A7190_read_state();
	A7190_set_state(WAIT_TX);

	StrobeCmd(CMD_STBY);
	Set_FIFO_len((flen_real&0xff),((flen_real>>8)&0x01));
	StrobeCmd(CMD_TFR);

	if (isShortDataLengthMode == TRUE && flen_real <= 256) {
		WriteFIFO1(0x00);
		WriteFIFO1(flen_real);
	} else {
		WriteFIFO1(0xf0 | ((flen_real >> 8) & 0x01));
		WriteFIFO1(flen_real & 0xff);
	}
	WriteFIFO(ptr, flen);

	A7190_set_state(BUSY_TX);
	StrobeCmd(CMD_TX);
	DelayMs(1);
	A7190_set_state(last_a7190_state);
}
