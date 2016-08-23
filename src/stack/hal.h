/*
 * hal.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_HAL_H_
#define SRC_STACK_HAL_H_


typedef unsigned char       BOOL;

#define LRWPAN_DEFAULT_START_CHANNEL 24
#define LRWPAN_PINGFRAME_LENGTH 5
#define LRWPAN_ACKFRAME_LENGTH 3
#define LRWPAN_MAX_FRAME_SIZE 256

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

#endif /* SRC_STACK_HAL_H_ */
