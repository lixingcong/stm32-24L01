/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: lixingcong
 *      HAL(Hardware Abstract Layer) functions. send via A7190, etc.
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#include "hal.h"
#include "NRF_api.h"
#include "NRF24L01.h"
#include "timer2.h"
#include "sys.h"
#include "route_table.h"

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

	static unsigned char tx_buf[NRF_PLOAD_LENGTH];
	unsigned char index;

	flen_real = flen + 2;

	if (flen_real > LRWPAN_MAX_FRAME_SIZE) {
		printf("halSendPacket: packet too long, drop\r\n");
		return;
	}

	if(NRF_read_state() != NRF_STATE_IDLE){
		printf("halSendPacket: busy when sending\r\n");
		return;
	}

	NRF_set_state(NRF_STATE_WAIT_TX);

	if (isShortDataLengthMode == TRUE) {
		tx_buf[0]=0x00;
		tx_buf[1]=flen_real;
	} else {
		tx_buf[0]=(0xf0 | ((flen_real >> 8) & 0x01));
		tx_buf[1]=(flen_real & 0xff);
	}

	for(index=0;index<flen;++index){
		tx_buf[index+2]=*(ptr+index);  // has 2 bytes offset
	}

	NRF_set_state(NRF_STATE_BUSY_TX);
	NRF_Send_Data(tx_buf, flen_real);
}

// all_nodes[]数组的下标不能被越界，否则stm32会死机，因此做启动检查地址是否合法
void check_if_exceed_max_node_range() {
	if (MY_NODE_NUM >= ALL_NODES_NUM) {  // 超过节点。
		printf("FATAL ERROR: max allow nodes num is %u, but my addr is %u\r\nProgram was forced to stop\r\n", ALL_NODES_NUM, MY_NODE_NUM);
		INTX_DISABLE();  // disable all interrupts
		while (1)
			;  // stop here infinitely
	}
}
