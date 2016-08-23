/*
 * route_ping.h
 *
 *  Created on: 2016年8月16日
 *      Author: li
 */

#ifndef SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_
#define SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_

#include "hal.h"

unsigned char macTxPing(unsigned char dst, unsigned char dsn, BOOL isRequest);
void macRxPingCallback(unsigned char *ptr);

#endif /* SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_ */
