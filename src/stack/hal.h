/*
 * hal.h
 *
 *  Created on: 2016年8月23日
 *      Author: lixingcong
 *
 *      This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#ifndef SRC_STACK_HAL_H_
#define SRC_STACK_HAL_H_

typedef unsigned char BOOL;

extern unsigned int systick_count;

// A7190频道，设置不同的频道可以避免干扰，值范围：1~256
#ifndef LRWPAN_DEFAULT_START_CHANNEL
#define LRWPAN_DEFAULT_START_CHANNEL 62
#endif

// 单个packet最大长度，根据无线发射手册设置
#define LRWPAN_MAX_FRAME_SIZE 32

// 求出距离上次的毫秒数x之差，限制为0xffff即为65535ms
#define halMACTimerNowDelta(x) ((halGetMACTimer()-(x))& 0xffff)

// 修改为合适的网络地址，理论上值范围0~255，协调器默认地址为0
// 实际上该地址最大值不能超过ALL_NODES_NUM
#ifndef IEEE_ADDRESS_ARRAY_COORD
#define IEEE_ADDRESS_ARRAY_COORD   0x00
#endif
#ifndef IEEE_ADDRESS_ARRAY_ROUTER
#define IEEE_ADDRESS_ARRAY_ROUTER  0x01
#endif

#ifdef LRWPAN_COORDINATOR
#define IEEE_ADDRESS_ARRAY IEEE_ADDRESS_ARRAY_COORD
#endif
#ifdef LRWPAN_ROUTER
#define IEEE_ADDRESS_ARRAY IEEE_ADDRESS_ARRAY_ROUTER
#endif

//  节点编号
#define MY_NODE_NUM (IEEE_ADDRESS_ARRAY&0xff)

#ifndef _BOOL_TYPE_
#define _BOOL_TYPE_
typedef enum {
	FALSE = 0, TRUE = !FALSE
} bool;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

unsigned int halGetMACTimer(void);
void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode);
unsigned short halGetRandomShortByte(void);
void check_if_exceed_max_node_range();

#endif /* SRC_STACK_HAL_H_ */
