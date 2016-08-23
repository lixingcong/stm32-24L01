/*
 * hal.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_HAL_H_
#define SRC_STACK_HAL_H_


typedef unsigned char       BOOL;


#ifndef _BOOL_TYPE_
#define _BOOL_TYPE_
typedef enum
{
  FALSE = 0, TRUE  = !FALSE
}
bool;
#endif

#endif /* SRC_STACK_HAL_H_ */
