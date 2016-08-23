/*
 * hal.c
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */


#include "hal.h"

MY_ROLE_ENUM my_role;

unsigned char halGetMACTimer(void) {
	return systick_count;
}
