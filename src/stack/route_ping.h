/*
 * route_ping.h
 *
 *  Created on: 2016年8月16日
 *      Author: lixingcong
 */

#ifndef SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_
#define SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_

#include "hal.h"

#define PING_DIRECTION_TO_CHILDREN 0x01
#define PING_DIRECTION_TO_PARENT 0x02
#define PING_DIRECTION_TO_OTHERS 0x04

unsigned char macTxPing(unsigned char dst, unsigned char dsn, BOOL isRequest);
void macRxPingCallback(unsigned char *ptr);
// ping
unsigned char macTxCustomPing(unsigned char dst, unsigned char direction, unsigned char retry_times, unsigned short retry_interval);

#endif /* SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_ */
