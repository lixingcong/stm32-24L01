/*
 * route_table.c
 *
 *  Created on: 2016年8月4日
 *      Author: lixingcong
 *     	Core functions for this stack.
 *
 *     	This is part of LixingcongPAN stack, release on Aug 26th, 2016
 *      Version 20160826
 *
 *      Copyright 2016 xingcong-li@foxmail.com
 */

#include "route_table.h"
#include "route_ping.h"
#include "route_AP_level.h"
#include "execute_PC_cmd.h"
#include "hal.h"
#include "stdio.h"
#include "delay.h"

//#define ROUTE_TABLE_OUTPUT_DEBUG

extern void aplRxCustomCallBack(void);
unsigned char all_nodes[ALL_NODES_NUM];  // 存放实时更新路由表，用于转发数据包等操作
unsigned char all_nodes_cache[ALL_NODES_NUM];  // 路由表缓存

unsigned char route_response[FRAME_LENGTH_ROUTE_CHANGE_RESPONSE];  // 缓冲区专门存放待发送的增量路由表，前面有3个字节的帧头
unsigned char route_response_offset;  // 增量路由表偏移量

static unsigned char payload_custom[LRWPAN_MAX_FRAME_SIZE];

BOOL isOffline;

// 我的角色：协调器 or 路由器，还有角色对应的状态机函数
MY_ROLE_ENUM my_role;
void (*mainFSM)();

unsigned int last_timer_beacon_sent;
unsigned int last_timer_children_checked;

unsigned char my_children_number;

void init_all_nodes() {
	unsigned char i;
	for (i = 0; i < ALL_NODES_NUM; ++i) {
		all_nodes[i] = all_nodes_cache[i] = 0xff;
	}
	route_response_offset = 3;
	my_children_number = 0;
}

// 寻找下一跳，基于all_nodes路由表
unsigned char get_next_hop(unsigned char this_hop, unsigned char dst) {
	unsigned char next_hop, next_hop_last;

	// check the range if valid
	if (dst >= ALL_NODES_NUM)
		return 0xff;

	// loopback
	if (this_hop == dst)
		return 0xff;

	next_hop = all_nodes[dst];

	// judge if dst is my child
	if (next_hop == this_hop)
		return dst;

	// determine route look-up direction, for a tree network
	if (this_hop == 0 || next_hop != 0xff) {  // look-up direction: down
		while (next_hop != this_hop) {
			next_hop_last = next_hop;
			next_hop = all_nodes[next_hop];
			if (next_hop == 0xff)
				break;
		}
		if (next_hop != 0xff)
			return next_hop_last;
	}

	// default return value is parent
	return all_nodes[this_hop];
}

// 使用ping检测孩子是否在线，不在线的将被删除
void check_my_children_online(BOOL isPingAllGrandsons) {
	unsigned char children_counter;
	unsigned char i, ping_result;
	children_counter = 0;
	for (i = 0; i < ALL_NODES_NUM; ++i) {
		if (all_nodes[i] == 0xff)
			continue;

		if (all_nodes[i] == (MY_NODE_NUM)) {  // my child
			ping_result=macTxCustomPing(i, PING_DIRECTION_TO_CHILDREN, 2, 300);
			if (0xff == ping_result) {
				all_nodes[i] = 0xff;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("check_my_children_online(): deleted child #%u\r\n",i);
#endif
			} else
				++children_counter;
		} else if (isPingAllGrandsons == TRUE) {  // not my child, and ping all grandson, ping direction is to other
			ping_result=macTxCustomPing(i, PING_DIRECTION_TO_OTHERS, 1, 300);
			if (0xff == ping_result)
				all_nodes[i] = 0xff;
		}

		all_nodes_ping[i]=ping_result; // update ping result, in order to upload to PC

	}
	my_children_number = children_counter;
}

// 合并孙子，默认覆盖。
void merge_grandsons(unsigned char *ptr) {
	unsigned char i, *my_ptr;
	my_ptr = ptr + 5;
	for (i = 0; i < (*(ptr + 1) - 5); i += 3) {
		switch (*(my_ptr++)) {
			if (*(my_ptr) >= ALL_NODES_NUM || *(my_ptr + 1) >= ALL_NODES_NUM)
				break;
		case FRAME_FLAG_UPDATE_ROUTE_ADD:
			all_nodes[*(my_ptr)] = *(my_ptr + 1);
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("updated: node #%u 's parent is #%u\r\n", *(my_ptr), *(my_ptr + 1));
#endif
			break;
		case FRAME_FLAG_UPDATE_ROUTE_REMOVE:
			if (all_nodes[*(my_ptr)] == *(my_ptr + 1)) {
				all_nodes[*(my_ptr)] = 0xff;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("deleted: node #%u 's last parent is #%u\r\n", *(my_ptr), *(my_ptr + 1));
#endif
			}
			break;
		default:
			break;
		}
		my_ptr += 2;
	}

}

/*
 * - - - -- - - - -- - - - -- - - - -- - - -- -  -- - - - - - - -
 * |                  custom frame by lixingcong                 |
 * - - -- - - - - - - -- - - -- - - - - - - -- - - - - - - - -- --
 * | name  | flen | frm_type |nexthop | dest | src | TTL | data  |
 * | Bytes |  2   |     1    |   1    |   1  |  1  |  1  |   *   |
 *-- - - - - - - -- - - -- - - - - - - -- - - - - - - - - - - - -
 */

// 注意dst为0xff为广播，谨慎使用
void send_custom_packet(unsigned char src, unsigned char dst, unsigned short flen, unsigned char *frm, unsigned char frm_type, unsigned char TTL) {
	unsigned short total_len, i;
	unsigned char nexthop;

	if (flen > (LRWPAN_MAX_FRAME_SIZE - 2 - FRAME_LENGTH_HEADER)) {  // total len = 2Bytes of len + header len + payload len
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("send_custom_packet(): packet too big, send fail\r\n");
#endif
		return;
	}

	total_len = FRAME_LENGTH_HEADER + flen;

	if (dst != 0xff) {  // broadcast packet
		nexthop = dst;
		if (0xff == macTxPing(nexthop, TRUE, PING_DIRECTION_TO_OTHERS))  // first try to send it as next hop
			nexthop = get_next_hop(MY_NODE_NUM, dst);  // if ping time out, choose next hop in normal way
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("send_custom_packet: nexthop=#%u, dst=#%u\r\n", nexthop, dst);
#endif
	} else
		nexthop = 0xff;

	payload_custom[0] = frm_type;
	payload_custom[1] = nexthop;
	payload_custom[2] = dst;
	payload_custom[3] = src;
	payload_custom[4] = TTL;  // Time to Live

	for (i = 0; i < flen; ++i)
		payload_custom[i + FRAME_LENGTH_HEADER] = *(frm + i);

	halSendPacket(total_len, payload_custom, FALSE);
}

// 向父亲上传自己的路由表，增量更新
void send_route_increasing_change_to_parent() {
	route_response[0] = FRAME_TYPE_SHORT_ROUTE_UPDATE;
	route_response[1] = my_parent;
	route_response[2] = MY_NODE_NUM;
	halSendPacket(route_response_offset, &route_response[0], TRUE);
	route_response_offset = 3;
}

// 把dst作为Payload,发送到协调器，然后由协调器上传给PC绘制路径图，该路径表示发送者到dst的路径
void send_custom_routine_to_coord(unsigned char dst) {
	static unsigned char routine_payload[FRAME_LENGTH_SEND_TO_PC];
	if (my_role == ROLE_COORDINATOR) {  // src=0, directly upload to PC
		upload_route_for_PC(0, dst);
		return;
	}
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	printf("in send routine path to coord\r\n");
#endif
	routine_payload[0] = FRAME_TYPE_SHORT_SEND_PATH_TO_PC;
	routine_payload[1] = 0;  // send to coord
	routine_payload[2] = MY_NODE_NUM;
	routine_payload[3] = dst;
	halSendPacket(FRAME_LENGTH_SEND_TO_PC, routine_payload, TRUE);
}

// 标准包的转发 多跳 每经过一跳 TTL减一
void send_custom_packet_relay(unsigned char src, unsigned char dst, unsigned short flen, unsigned char *frm, unsigned char frm_type, unsigned char TTL) {
	unsigned char i, *ptr;

	if (src != MY_NODE_NUM)
	printf("Routing ");
	else
	printf("Sending ");
	printf("packet...frm_type=0x%x, dst=%u, TTL=%u\r\n", frm_type, dst, TTL);

	if (TTL == 0) {
		printf("relay FAIL: TTL is 0, drop packet\r\n");
		return;
	}

#if 0 // print the msg
	for (i = 0; i < flen; ++i)
	printf("%c", *(frm + i));
	printf("\r\n");
#else // print the USB msg, offset is 24
	ptr = frm + 24;
	while (*ptr != 0)
		printf("%c", *(ptr++));
	printf("\r\n");
#endif

	send_custom_packet(src, dst, flen, frm, frm_type, (TTL - 1));
}

void display_all_nodes() {
	unsigned char i;
	for (i = 0; i < ALL_NODES_NUM; ++i)
		if (all_nodes[i] != 0xff)
			printf("- Node #%u 's parent is #%u\r\n", i, all_nodes[i]);
	printf("-\r\n");
}

void update_route_response_content(BOOL isAdd, unsigned char child, unsigned char parent) {
	if (route_response_offset >= FRAME_LENGTH_ROUTE_CHANGE_RESPONSE)  // excceed max len
		return;
	if (isAdd == TRUE)
		route_response[route_response_offset++] = FRAME_FLAG_UPDATE_ROUTE_ADD;  // 增量添加
	else
		route_response[route_response_offset++] = FRAME_FLAG_UPDATE_ROUTE_REMOVE;  // 增量删除
	route_response[route_response_offset++] = child;
	route_response[route_response_offset++] = parent;
}

void macRxCustomPacketCallback(unsigned char *ptr, BOOL isShortMSG, unsigned short flen) {
	unsigned short i;
	if (isShortMSG == FALSE) {
		switch (*(ptr + 2)) {  // switch frame type [LONG]
			case FRAME_TYPE_LONG_MSG:
			case FRAME_TYPE_LONG_MSG_WITH_ACK:
				if (*(ptr + 3) == MY_NODE_NUM) {  // next hop is me
					if (*(ptr + 4) == MY_NODE_NUM) {  // dst is me, recv it
						if (*(ptr + 2) == FRAME_TYPE_LONG_MSG_WITH_ACK) {
							// TODO: reply an ACK 2016年8月25日 上午9:40:21
						}
						update_AP_msg(ptr, flen);
						aplRxCustomCallBack();
					} else {  // dst is not me, relay it
						send_custom_packet_relay(*(ptr + 5), *(ptr + 4), flen - 7, ptr + 7, *(ptr + 2), *(ptr + 6));
					}
				}
				break;
			case FRAME_TYPE_LONG_ACK:
				// recv a ACK respons, now compare dsn
				break;
			case FRAME_TYPE_LONG_BROADCAST:
				if (*(ptr + 3) == MY_NODE_NUM) {  // next hop is me
					if (*(ptr + 4) == MY_NODE_NUM) {  // dst is me, recv it as a broadcast
						update_AP_msg(ptr, flen);
						aplRxCustomCallBack();
						send_custom_packet_relay(MY_NODE_NUM, 0xff, flen - 7, ptr + 7, FRAME_TYPE_LONG_BROADCAST, *(ptr + 6));  // send broadcast to my grandsons
					} else {  // dst is not me, relay it
						send_custom_packet_relay(*(ptr + 5), *(ptr + 4), flen - 7, ptr + 7, *(ptr + 2), *(ptr + 6));
					}
				} else if (*(ptr + 5) == my_parent) {  // src is my parent
					if (*(ptr + 3) == 0xff) {  // next hop is 0xff: all children's broadcast
						update_AP_msg(ptr, flen);
						aplRxCustomCallBack();
						send_custom_packet_relay(MY_NODE_NUM, 0xff, flen - 7, ptr + 7, FRAME_TYPE_LONG_BROADCAST, *(ptr + 6));  // send broadcast to my grandsons
					}
				} else {
					// invalid broadcast(not from my parent)
				}
				break;

			case FRAME_TYPE_LONG_PING:
				if (*(ptr + 3) == MY_NODE_NUM) {  // next hop is me
					if (*(ptr + 4) == MY_NODE_NUM) {  // dst is me, reply ping
						macRxPingCallback(ptr,TRUE);
					} else {  // dst is not me, relay it
						send_custom_packet_relay(*(ptr + 5), *(ptr + 4), flen - 7, ptr + 7, *(ptr + 2), *(ptr + 6));
					}
				}
				break;

			default:
				break;
		}
	} else {
		switch (*(ptr + 2)) {  // switch frame type [SHORT]
			case FRAME_TYPE_SHORT_BEACON:
				// TODO: beacon callback 2016年8月23日 下午11:53:12
				break;
			case FRAME_TYPE_SHORT_PING:
				macRxPingCallback(ptr,FALSE);
				break;
			case FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL:
				if (*(ptr + 5) == FRAME_FLAG_JOIN_REQUEST) {  // join requesst
					if (my_parent == *(ptr + 4))  // sender is my parent, not allow to join(loopback)
						break;
					if (*(ptr + 4) == MY_NODE_NUM)  // conflict address, not allow(this situation should not appear in reality)
						break;
					if (all_nodes[*(ptr + 4)] == MY_NODE_NUM) {  // if dst was my son before, let him join again
						send_join_network_response(*(ptr + 4), FALSE);
						break;
					}
					if (my_children_number < MAX_CHILDREN_NUM) {
						DelayMs(1);  // wait for a while
						send_join_network_response(*(ptr + 4), FALSE);
					}

				} else if (*(ptr + 5) == FRAME_FLAG_JOIN_RESPONSE) {  // join response
					if (isOffline == TRUE) {
						isOffline = FALSE;
						my_parent = *(ptr + 4);
						send_join_network_response(*(ptr + 4), TRUE);
					}
				} else {  // join ACK
					all_nodes[*(ptr + 4)] = *(ptr + 3);
					if (*(ptr + 3) == MY_NODE_NUM) {  // it is my new child
						printf("Node #%u joined\r\n", *(ptr + 4));
					}

				}
				break;
			case FRAME_TYPE_SHORT_ROUTE_UPDATE:
				if (*(ptr + 3) == MY_NODE_NUM || *(ptr + 4) == my_parent) {
					merge_grandsons(ptr);
				}
				break;
			case FRAME_TYPE_SHORT_SEND_PATH_TO_PC:
				if (*(ptr + 3) == MY_NODE_NUM){
					printf("recv a path for PC: #%u -> #%u\r\n", *(ptr + 4), *(ptr + 5));
					upload_route_for_PC(*(ptr+4), *(ptr+5));
				}
				break;
			case FRAME_TYPE_SHORT_SEND_TEST_SEND:
				send_test_replyACK(ptr);
				fprintf(stderr,"recv a send test req\r\n");
				break;
			case FRAME_TYPE_SHORT_SEND_TEST_RECV:
				send_test_checkData(ptr);
				fprintf(stderr,"recv a send test reply\r\n");
				testAckPending=FALSE;
				break;
			default:
				fprintf(stderr,"not support short msg!\r\n");
				break;
		}
	}
}

void send_join_network_response(unsigned char dst, BOOL isACK) {
	static unsigned char join_response_payload[FRAME_LENGTH_JOIN_INFO];
	join_response_payload[0] = FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
	join_response_payload[1] = dst;
	join_response_payload[2] = MY_NODE_NUM;
	if (isACK == FALSE)
		join_response_payload[3] = FRAME_FLAG_JOIN_RESPONSE;
	else
		join_response_payload[3] = FRAME_FLAG_JOIN_RESPONSE_ACK;
	halSendPacket(FRAME_LENGTH_JOIN_INFO, join_response_payload, TRUE);
}
