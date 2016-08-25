/*
 * route_table.h
 *
 *  Created on: 2016年7月28日
 *      Author: li
 */

#ifndef _ROUTE_TABLE_H_
#define _ROUTE_TABLE_H_
#include "hal.h"

// only for router update, used by update_route_table_info()
#ifdef LRWPAN_ROUTER
extern unsigned int last_route_updated_timer;
#endif

// 是否掉线
extern BOOL isOffline;

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

void display_all_nodes();
void update_route_response_content(BOOL isAdd, unsigned char child, unsigned char parent);


// callback for spi1_irq()
void macRxCustomPacketCallback(unsigned char *ptr,BOOL isShortMSG, unsigned short flen);

// 入网回复
void send_join_network_response(unsigned char dst, BOOL isACK);


// 自定义的mac层转发常量
#define CUSTOM_FRAME_TYPE_BROADCAST 0xff
#define CUSTOM_FRAME_TYPE_ROUTE_UPDATE_REQUEST  0x01
#define CUSTOM_FRAME_TYPE_ROUTE_UPDATE_RESPONSE 0x02
#define CUSTOM_FRAME_TYPE_DATA 0x03
#define CUSTOM_FRAME_TYPE_UPLOADROUTEPATH_TO_PC 0x04

void send_route_increasing_change_to_parent();


#endif /* _DEFINE_ROUTE_TABLE_H_ */
