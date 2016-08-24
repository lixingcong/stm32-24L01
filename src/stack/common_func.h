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
extern unsigned int last_timer_beacon_sent;
extern unsigned int last_timer_children_checked;

// 最多允许存放的协调器数目
#define MAX_COORD_NUM 3
// 最大孩子数目
#define MAX_CHILDREN_NUM 3

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

// 请求入网的标志
#define FRAME_FLAG_JOIN_REQUEST 0x01
#define FRAME_FLAG_JOIN_RESPONSE 0x02
#define FRAME_FLAG_JOIN_RESPONSE_ACK 0x04

// 定期检查间隔
#define INTERVAL_OF_SENDING_BEACON 10
#define INTERVAL_OF_CHECKING_CHILDREN 5
#define INTERVAL_OF_MY_PARENT_CHECK_ME (INTERVAL_OF_CHECKING_CHILDREN*2)


#endif /* SRC_STACK_COMMON_FUNC_H_ */
