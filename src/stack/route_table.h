/*
 * route_table.h
 *
 *  Created on: 2016年7月28日
 *      Author: lixingcong
 */

#ifndef _ROUTE_TABLE_H_
#define _ROUTE_TABLE_H_
#include "hal.h"

typedef enum _MY_ROLE_ENUM {
	ROLE_COORDINATOR,
	ROLE_ROUTER
}MY_ROLE_ENUM;

extern MY_ROLE_ENUM my_role;
extern void (*mainFSM)();

#define my_parent (all_nodes[MY_NODE_NUM])

extern unsigned int last_timer_beacon_sent;
extern unsigned int last_timer_children_checked;

// 是否掉线
extern BOOL isOffline;

// 最多允许存放的协调器数目
#define MAX_COORD_NUM 3
// 最大孩子数目
#define MAX_CHILDREN_NUM 1

// 短帧的长度，不含2个头字节flen，因为头字节是由halSendPacket()函数修改的
#define FRAME_LENGTH_BEACON 3
#define FRAME_LENGTH_PING 4
#define FRAME_LENGTH_JOIN_INFO 4
#define FRAME_LENGTH_SEND_TO_PC 4
#define FRAME_LENGTH_ROUTE_CHANGE_RESPONSE (ALL_NODES_NUM*3+3)

// 短帧类型（帧长小于256）
#define FRAME_TYPE_SHORT_PING 0x00
#define FRAME_TYPE_SHORT_SEND_PATH_TO_PC  0x01
#define FRAME_TYPE_SHORT_BEACON 0xf0
#define FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL 0xf2
#define FRAME_TYPE_SHORT_ROUTE_UPDATE 0xf4
#define FRAME_TYPE_SHORT_CHANGE_ROLE 0xf8

// 长帧类型（帧长较长，长度为256~512）
#define FRAME_TYPE_LONG_MSG 0x00
#define FRAME_TYPE_LONG_ACK 0x02
#define FRAME_TYPE_LONG_BROADCAST 0x04
#define FRAME_TYPE_LONG_MSG_WITH_ACK 0xf0


// 请求入网的标志
#define FRAME_FLAG_JOIN_REQUEST 0x01
#define FRAME_FLAG_JOIN_RESPONSE 0x02
#define FRAME_FLAG_JOIN_RESPONSE_ACK 0x04

// 定期检查间隔
#define INTERVAL_OF_SENDING_BEACON 10
#define INTERVAL_OF_CHECKING_CHILDREN 5
#define INTERVAL_OF_MY_PARENT_CHECK_ME (INTERVAL_OF_CHECKING_CHILDREN*2)

// 路由表逐级上传（增量更新）类型
#define FRAME_FLAG_UPDATE_ROUTE_ADD 0xf0
#define FRAME_FLAG_UPDATE_ROUTE_REMOVE 0x0f

// 所有节点，含有路由器
#define ALL_NODES_NUM 20

// 我的孩子数目
extern unsigned char my_children_number;

// 偏移值
extern unsigned char route_response_offset;


// 数组下标[1...ALL_NODES_NUM-1]是指向父亲
// all_nodes[0]无定义
extern unsigned char all_nodes[ALL_NODES_NUM];

void init_all_nodes();

// 获取下一跳的长地址
unsigned char get_next_hop(unsigned char this_hop,unsigned char dst);

void check_my_children_online();

void merge_grandsons(unsigned char *ptr);


// send function
void send_custom_broadcast(unsigned char flen,unsigned char *frm);
void send_custom_packet(unsigned char src, unsigned char dst,unsigned short flen,unsigned char *frm, unsigned char frm_type);
void send_custom_packet_relay(unsigned char src,unsigned char dst,unsigned char flen,unsigned char *frm,unsigned char frm_type);
void send_custom_routine_to_coord(unsigned char dst);
// 增量更新路由表
void send_route_increasing_change_to_parent();

void display_all_nodes();
void update_route_response_content(BOOL isAdd, unsigned char child, unsigned char parent);


// callback for spi1_irq()
void macRxCustomPacketCallback(unsigned char *ptr,BOOL isShortMSG, unsigned short flen);

// 入网回复
void send_join_network_response(unsigned char dst, BOOL isACK);





#endif /* _DEFINE_ROUTE_TABLE_H_ */
