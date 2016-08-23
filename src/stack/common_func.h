/*
 * common_func.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_COMMON_FUNC_H_
#define SRC_STACK_COMMON_FUNC_H_

typedef enum _MY_ROLE_ENUM {
	ROLE_COORDINATOR,
	ROLE_ROUTER
}MY_ROLE_ENUM;

extern MY_ROLE_ENUM my_role;
extern unsigned char my_parent;

// 最多允许存放的协调器数目
#define MAX_COORD_NUM 3

// 短帧类型（帧长小于256）
#define FRAME_TYPE_SHORT_PING 0x00
#define FRAME_TYPE_SHORT_ACK  0x01
#define FRAME_TYPE_SHORT_BEACON 0xf0
#define FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL 0xf2
#define FRAME_TYPE_SHORT_ROUTE_UPDATE 0xf4
#define FRAME_TYPE_SHORT_ROUTE_SEND_TO_PC 0xf5
#define FRAME_TYPE_SHORT_CHANGE_ROLE 0xf8

// 长帧类型（帧长较长，小于500）
#define FRAME_TYPE_LONG_MSG 0x00
#define FRAME_TYPE_LONG_BROADCAST 0xff




#endif /* SRC_STACK_COMMON_FUNC_H_ */
