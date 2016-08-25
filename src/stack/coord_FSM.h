/*
 * coord_FSM.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_COORD_FSM_H_
#define SRC_STACK_COORD_FSM_H_

typedef enum _COORD_STATE_ENUM {
	COORD_STATE_INITAILIZE_ALL_NODES, // 初始化
	COORD_STATE_FORM_NETWORK, // 形成网络
	COORD_STATE_SEND_BEACON, // 周期性发送协调器在线帧（有利于两个协调器融合）
	COORD_STATE_CHECK_CHILDREN,  // 检查孩子
	COORD_STATE_DOWNGRADE_TO_ROUTER // 退化成路由器，网络拓朴不变
}COORD_STATE_ENUM;

extern COORD_STATE_ENUM coord_FSM_state;

void coord_send_beacon();

void coordFSM();

#endif /* SRC_STACK_COORD_FSM_H_ */
