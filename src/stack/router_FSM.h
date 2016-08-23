/*
 * router_FSM.h
 *
 *  Created on: 2016年8月23日
 *      Author: li
 */

#ifndef SRC_STACK_ROUTER_FSM_H_
#define SRC_STACK_ROUTER_FSM_H_

typedef enum _ROUTER_STATE_ENUM {
	ROUTER_STATE_INITAILIZE_ALL_NODES, // 初始化
	ROUTER_STATE_JOIN_NETWORK, // 加入网络
	ROUTER_STATE_CHECK_CHILDREN,  // 检查孩子
	ROUTER_STATE_UPGRADE_TO_COORD // 升级为协调器
}ROUTER_STATE_ENUM;

extern ROUTER_STATE_ENUM router_FSM_state;

#endif /* SRC_STACK_ROUTER_FSM_H_ */
