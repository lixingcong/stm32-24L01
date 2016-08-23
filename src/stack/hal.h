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

#define LRWPAN_DEFAULT_START_CHANNEL 120

#define LRWPAN_PINGFRAME_LENGTH 5
#define LRWPAN_ACKFRAME_LENGTH 3
#define LRWPAN_MAX_FRAME_SIZE 256

#define LRWPAN_SYMBOLS_PER_SECOND   62500
#define MSECS_TO_MACTICKS(x)   (x*(LRWPAN_SYMBOLS_PER_SECOND/1000))
#define MACTICKS_TO_MSECS(x)   (x/(LRWPAN_SYMBOLS_PER_SECOND/1000))
#define halMACTimerDelta(x,y) ((x-(y))& MACTIMER_MAX_VALUE)


#ifndef _BOOL_TYPE_
#define _BOOL_TYPE_
typedef enum
{
  FALSE = 0, TRUE  = !FALSE
}
bool;
#endif

typedef enum _MY_ROLE_ENUM {
	ROLE_COORDINATOR,
	ROLE_ROUTER
}MY_ROLE_ENUM;

extern MY_ROLE_ENUM my_role;

#ifndef NULL
#define NULL ((void *)0)
#endif

unsigned char halGetMACTimer(void);

#endif /* SRC_STACK_HAL_H_ */
