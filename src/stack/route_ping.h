/*
 * route_ping.h
 *
 *  Created on: 2016年8月16日
 *      Author: lixingcong
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#ifndef SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_
#define SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_

#include "hal.h"
#include "route_table.h"

extern unsigned char all_nodes_ping[ALL_NODES_NUM];

#define PING_DIRECTION_TO_CHILDREN 0x01
#define PING_DIRECTION_TO_PARENT 0x02
#define PING_DIRECTION_TO_OTHERS 0x04

unsigned char macTxPing(unsigned char dst, BOOL isRequest, unsigned char direction);
void macRxPingCallback(unsigned char *ptr,BOOL isLongPing);
// ping
unsigned char macTxCustomPing(unsigned char dst, unsigned char direction, unsigned char retry_times, unsigned short retry_interval);
unsigned char macTxPingLongDistance(unsigned char dst,BOOL isRequest);

#endif /* SRC_DRIVERS_ROUTE_TABLE_ROUTE_PING_H_ */
