/*
 * NRF_api.h
 *
 *  Created on: 2017年3月2日
 *      Author: li
 */

#ifndef SRC_DRIVERS_NRF24L01_NRF_API_H_
#define SRC_DRIVERS_NRF24L01_NRF_API_H_

#include "NRF24L01.h"

typedef enum {
	NRF_STATE_IDLE,
	NRF_STATE_BUSY_TX,
	NRF_STATE_BUSY_RX
} rtx_state_t;


void initRF(void);
void NRF_set_state(rtx_state_t state);
rtx_state_t NRF_read_state(void);

#endif /* SRC_DRIVERS_NRF24L01_NRF_API_H_ */
