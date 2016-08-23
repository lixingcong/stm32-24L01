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
	ROUTER_STATE_CHECK_PARENT,  // 检查父亲 目的是rejoin
	ROUTER_STATE_CHECK_CHILDREN,  // 检查孩子 目的是更新路由
	ROUTER_STATE_UPGRADE_TO_COORD // 升级为协调器
}ROUTER_STATE_ENUM;

extern ROUTER_STATE_ENUM router_FSM_state;

void router_FSM();
void router_send_join_request();

#endif /* SRC_STACK_ROUTER_FSM_H_ */
