/*
 * NRF_api.c
 *
 * 供上层协议栈使用的API
 *
 *  Created on: 2017年3月2日
 *      Author: li
 */

#include "NRF_api.h"
#include "NRF24L01.h"

static  rtx_state_t rtx_state;
unsigned char recv_buffer_a7190[NRF_PLOAD_LENGTH];

void initRF(void){
	NRF24L01_Init();
	NRF_set_state(IDLE);
}

rtx_state_t NRF_read_state() {
	return rtx_state;
}

void NRF_set_state(rtx_state_t state) {
	rtx_state = state;
}
