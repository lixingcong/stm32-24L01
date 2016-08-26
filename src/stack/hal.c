/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: lixingcong
 *      HAL(Hardware Abstract Layer) functions. send via A7190, etc.
 */

#include "hal.h"
#include "A7190.h"
#include "timer2.h"

extern unsigned int systick_count;

// 返回当前的毫秒数
unsigned int halGetMACTimer(void) {
	return system_msecond;
}

unsigned short halGetRandomShortByte(void) {
	return (systick_count & 0xffff);
}

void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode) {
	unsigned short flen_real;
	unsigned char last_a7190_state;
	flen_real = flen + 2;

	if (flen_real + 1 > LRWPAN_MAX_FRAME_SIZE) {
		printf("halSendPacket: packet too long, drop\r\n");
		return;
	}

	last_a7190_state = A7190_read_state();
	A7190_set_state(WAIT_TX);

	StrobeCmd(CMD_STBY);
	Set_FIFO_len(((flen_real + 1) & 0xff), (((flen_real + 1) >> 8) & 0x01));  // 经过测试，要flen_real+1才能成功
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
