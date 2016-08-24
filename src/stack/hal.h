/*
 * hal.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_HAL_H_
#define SRC_STACK_HAL_H_

typedef unsigned char BOOL;

extern unsigned int systick_count;

// A7190频道
#define LRWPAN_DEFAULT_START_CHANNEL 120

#define LRWPAN_PINGFRAME_LENGTH 4
#define LRWPAN_MAX_FRAME_SIZE 512

#define LRWPAN_SYMBOLS_PER_SECOND   62500
#define MSECS_TO_MACTICKS(x)   (x*(LRWPAN_SYMBOLS_PER_SECOND/1000))
#define MACTICKS_TO_MSECS(x)   (x/(LRWPAN_SYMBOLS_PER_SECOND/1000))
#define halMACTimerNowDelta(x) ((halGetMACTimer()-(x))& 0xfffff)


//修改为合适的网络地址，也可以使用make传递宏定义
#ifndef IEEE_ADDRESS_ARRAY_COORD
#define IEEE_ADDRESS_ARRAY_COORD   0x00
#endif
#ifndef IEEE_ADDRESS_ARRAY_ROUTER
#define IEEE_ADDRESS_ARRAY_ROUTER  0x01
#endif
#ifndef IEEE_ADDRESS_ARRAY_RFD
#define IEEE_ADDRESS_ARRAY_RFD     0x02
#endif

#ifdef LRWPAN_COORDINATOR
#define IEEE_ADDRESS_ARRAY IEEE_ADDRESS_ARRAY_COORD
#endif
#ifdef LRWPAN_ROUTER
#define IEEE_ADDRESS_ARRAY IEEE_ADDRESS_ARRAY_ROUTER
#endif
#ifdef LRWPAN_RFD
#define IEEE_ADDRESS_ARRAY IEEE_ADDRESS_ARRAY_RFD
#endif

//  节点编号
#define MY_NODE_NUM (IEEE_ADDRESS_ARRAY&0xff)


#ifndef _BOOL_TYPE_
#define _BOOL_TYPE_
typedef enum
{
  FALSE = 0, TRUE  = !FALSE
}
bool;
#endif


#ifndef NULL
#define NULL ((void *)0)
#endif

unsigned int halGetMACTimer(void);
void halSendPacket(unsigned short flen, unsigned char *ptr, BOOL isShortDataLengthMode);
unsigned short halGetRandomShortByte(void);

#endif /* SRC_STACK_HAL_H_ */
