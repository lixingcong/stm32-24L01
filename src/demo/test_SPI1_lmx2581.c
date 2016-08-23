/*
 * test_SPI1_lmx2581.c
 *
 *  Created on: 2016年8月11日
 *      Author: li
 *      一个简单测试SPI1写入程序
 */

#include "SPI2.h"
#include "ctl_lmx2581.h"
#include "stm32f10x.h"
#include "usart.h"
#include "lrwpan_common_types.h"
#include "halStack.h"
#include "delay.h"
#include "A7190.h"
#include "hal.h"
#include "halStack.h"

void main(void) {
	unsigned short freq;
	unsigned int last_timer;
	unsigned char payload[400];
	halInit();
	printf("ok\r\n");
	freq=400;
	A7190_set_state(IDLE);
	last_timer=halGetMACTimer();
	ctl_frequency_set(500);
	while(1){
		/*
		if (halMACTimerNowDelta(last_timer) > MSECS_TO_MACTICKS(1000)){ // 每1秒改变VCO频率
			ctl_frequency_set(freq);
			freq=(freq<700?(freq+10):400);
			last_timer=halGetMACTimer();
		}
		*/
		halSendPacket(256, payload); //死里发包
		DelayMs(3);
	}
}


LRWPAN_STATUS_ENUM usrZepRxCallback(void) {
}

LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {
}

#ifdef LRWPAN_FFD
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo) {
}

BOOL usrJoinNotifyCallback(LADDR *ptr) {
}
#endif

#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void) {
}
#endif

//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void) {
}
