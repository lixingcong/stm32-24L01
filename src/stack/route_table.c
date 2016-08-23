/*
 * route_table.c
 *
 *  Created on: 2016年8月4日
 *      Author: li
 *     	实现路由表更新，基于路由表
 */


#include "route_table.h"
#include "route_ping.h"
#include "apl_custom_function.h"
#include "A7190.h"
#include "execute_PC_cmd.h"
#include "hal.h"
#include "common_func.h"

//#define ROUTE_TABLE_OUTPUT_DEBUG

extern void aplRxCustomCallBack(void);
unsigned char all_nodes[ALL_NODES_NUM*2];// 存放实时更新路由表，用于转发数据包等操作
unsigned char all_nodes_cache[ALL_NODES_NUM]; // 存放上次的路由表，对比产生一个增量的路由表

unsigned char route_response[3*ALL_NODES_NUM];// 缓冲区专门存放待发送的增量路由表
unsigned char route_response_offset;// 增量路由表偏移量

// only for router update, used by update_route_table_info()
#ifdef LRWPAN_ROUTER
unsigned int last_route_updated_timer;
#endif

static unsigned char payload_custom[LRWPAN_MAX_FRAME_SIZE];
BOOL isOffline;

void init_all_nodes(){
	unsigned char i;
	for(i=0;i<ALL_NODES_NUM;++i){
		all_nodes_cache[i]=all_nodes[i]=all_nodes[i+ALL_NODES_NUM]=0xff;
	}
	route_response_offset=0;
}

// 寻找下一跳，基于all_nodes路由表
unsigned char get_next_hop(unsigned char this_hop, unsigned char dst) {
	unsigned char next_hop, next_hop_last;

	// check the range if valid
	if(dst >= ALL_NODES_NUM)
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
void check_my_children_online() {
	unsigned char i;
	for (i = 1; i < ALL_NODES_NUM; ++i) {
		if (all_nodes_cache[i] == (MY_NODE_NUM)) {  // my child
			if(0xff==macTxCustomPing(i, PING_DIRECTION_TO_CHILDREN, 2, 300)){
				// TODO: 误删孩子情况偶尔出现 2016年8月18日 上午10:33:11
				// if not online, del node in cache
				all_nodes[i]=0xff;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("Delete child #%u\r\n",i);
#endif
			}
		}
	}

}

void add_to_my_parent(){
	all_nodes[MY_NODE_NUM]=my_parent;
}

void deattach_from_my_parent(){
	all_nodes[MY_NODE_NUM]=0xff;
}

// 增加孩子
void add_to_my_child(unsigned char addr){
	all_nodes[addr]=(MY_NODE_NUM);
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	printf("add #%u as my child\r\n",addr);
#endif
}

// 合并孙子，默认覆盖。
void merge_grandsons(unsigned char *ptr){
	unsigned char i,*my_ptr;
	my_ptr=ptr+5;
	for(i=0;i<(*ptr-5);i+=3){
		if(((*(my_ptr+i))&0x80)==0x80){ // 增量添加
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("merged: ");
#endif
			all_nodes[(*(my_ptr+i))&0x3f]=*(my_ptr+i+1); // parent
			all_nodes[((*(my_ptr+i))&0x3f)+ALL_NODES_NUM]=*(my_ptr+i+2); // RSSI
		}else{ // 增量删除
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("deleted: ");
#endif
			if(all_nodes[(*(my_ptr+i))&0x3f]==*(my_ptr+i+1)){ // really delete this node
				all_nodes[(*(my_ptr+i))&0x3f]=0xff;
			}
		}
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
		printf("node #%u 's parent is #%u, RSSI=%u\r\n",*(my_ptr+i)&0x3f,*(my_ptr+i+1),*(my_ptr+i+2));
#endif
	}
}

// 清除孙子和信号强度
void clear_all_nodes(){
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM*2;++i)
		all_nodes[i]=0xff;
}

BOOL check_if_children_empty(){
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM;++i)
		if(all_nodes[i]==(MY_NODE_NUM))return FALSE;
	return TRUE;
}

/*
 * - - - -- - - - -- - - - -- - - - -- - - -- -
 * |         custom frame by lixingcong        |
 * - - -- - - - - - - -- - - -- - - - - - - -- -
 * | flen | nexthop | dest | src | prop | data |
 * |   1  |    1    |   1  |  1  |   1  |  *   |
 *-- - - - - - - -- - - -- - - - - - - -- - - -
 */

// 注意dst为0xff为广播，谨慎使用
void send_custom_packet(unsigned char src, unsigned char dst,unsigned char flen,unsigned char *frm, unsigned char frm_type){
	unsigned char total_len,i;
	unsigned char *ptr;
	unsigned short delayms;
	if(flen>MAX_CUSTOM_FRAME_LENGTH){
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
		printf("send_custom_packet(): packet too big, send fail\r\n");
#endif
		return;
	}

	total_len=5+flen;
	ptr=payload_custom;
	*(ptr++)=total_len;
	if(dst==0xff){
		*(ptr++)=0xff;
		*(ptr++)=0xff;
	} else {
		*(ptr++) = get_next_hop((MY_NODE_NUM), dst);
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
		printf("in send custom packet: nexthop=%u\r\n",*(ptr-1));
#endif
		if (*(ptr - 1) == 0xff) {
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("relay dst not arrived, frm_type=%u\r\n",frm_type);
#endif
			return;
		}
		*(ptr++) = dst;
	}
	*(ptr++)=src;
	*(ptr++)=frm_type;
	for(i=0;i<flen;++i)
		*(ptr++)=*(frm+i);

#if 0
//#ifndef LRWPAN_COORDINATOR
	// TODO: sendcustom函数中随机延时长度的设定 2016年8月18日 上午11:23:10
	// 可以在rxcustom中加一个标志位禁止接收其它东西专心收发数据
	delayms=halGetRandomShortByte();
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	printf("delay #%u ms\r\n",delayms);
#endif
	DelayMs(delayms);// to avoid broadcast storm
#endif
	A7190_set_state(IDLE);
	halSendPacket(1, payload_custom , TRUE);
//	printf("in send custom: ");
//	for(i=0;i<5;++i)
//		printf("%x ",payload_custom[i]);
//	printf("\r\n");
	DelayMs(1);

}

// 给孩子和孙子们发递归广播
void send_custom_broadcast(unsigned char flen,unsigned char *frm){
	// TODO: use DSN to avoid last broadcast 2016年8月10日 上午8:23:12
	send_custom_packet(MY_NODE_NUM, 0xff,flen,frm,CUSTOM_FRAME_TYPE_BROADCAST);
}

// 向孩子广播一个路由更新请求
void send_custom_upload_route_request(){
	send_custom_packet(MY_NODE_NUM, 0xff,0,NULL,CUSTOM_FRAME_TYPE_ROUTE_UPDATE_REQUEST);
#ifdef LRWPAN_COORDINATOR
	//clear_all_nodes();
#else
	if(check_if_children_empty()==TRUE){
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
		printf("my children is empty, uploading route table\r\n");
#endif
		send_custom_upload_route_response();
	}else{
		clear_all_nodes();
	}
#endif
}
// 向父亲上传自己的路由表，
void send_custom_upload_route_response(){
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	printf("in send upload response: dst=%u\r\n",dst);
#endif
	add_to_my_parent();

	if(route_response_offset!=0){
		send_custom_packet(MY_NODE_NUM, my_parent,route_response_offset,&route_response[0],CUSTOM_FRAME_TYPE_ROUTE_UPDATE_RESPONSE);
		route_response_offset=0;
	}
}

// 把dst作为Payload,发送到协调器，然后由协调器上传给PC绘制路径图，该路径表示发送者到dst的路径
void send_custom_routine_to_coord(unsigned char dst){
	static unsigned char routine_payload;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	printf("in send routine path to coord\r\n");
#endif
	routine_payload=dst;
	if(MY_NODE_NUM==0) // src=0, directly upload to PC
		upload_route_for_PC(0, dst);
	else
		send_custom_packet(MY_NODE_NUM, 0, 1, &routine_payload, CUSTOM_FRAME_TYPE_UPLOADROUTEPATH_TO_PC);
}

// 标准包的转发 多跳
void send_custom_packet_relay(unsigned char src,unsigned char dst,unsigned char flen,unsigned char *frm,unsigned char frm_type){
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
	unsigned char i;
	if(src!=MY_NODE_NUM)
		printf("Routing ");
	else
		printf("Sending ");
	printf("packet...frm_type=%u, msg=",frm_type);
	for(i=0;i<flen;++i)
		printf("%c",*(frm+i));
	printf("\r\n");
#endif
	send_custom_packet(src,dst,flen,frm,frm_type);
}
// custom frame的接收处理函数
void macRxCustomPacketCallback(unsigned char *ptr){
	unsigned char i;
	switch (*(ptr + 4)) {
		case CUSTOM_FRAME_TYPE_BROADCAST:
			if (*(ptr + 1) == MY_NODE_NUM) {  // next hop node is me
				if (*(ptr + 2) != MY_NODE_NUM) { // dst is not me, relay it
					// don't show msg
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("relaying broadcast, src=#%u, dst=#%u\r\n",*(ptr+3),*(ptr+2));
#endif
					send_custom_packet_relay(*(ptr + 3), *(ptr + 2), (*ptr) - 5, ptr + 5,CUSTOM_FRAME_TYPE_BROADCAST);
				} else {
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("recv my dst broadcast: ");
#endif
					update_AP_msg(ptr);
					aplRxCustomCallBack();
					send_custom_broadcast(*(ptr) - 5, ptr + 5);  // send broadcast to my grandsons
				}
			} else if (*(ptr + 1) == 0xff) {  // all children's broadcast
				if (*(ptr + 3) == my_parent) {  // if src is my parent
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("recv a normal broadcast: ");
#endif
					update_AP_msg(ptr);
					aplRxCustomCallBack();
					send_custom_broadcast(*(ptr) - 5, ptr + 5);  // recursive send broadcast
				}
			} else {
				// illegal broadcast
			}
			break;
		case CUSTOM_FRAME_TYPE_ROUTE_UPDATE_REQUEST:
#ifdef LRWPAN_ROUTER
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("recv a custom route update request\r\n");
#endif
			if(*(ptr+3)==my_parent) {  // the src is my parent
				//clear_all_nodes(); // flush all nodes
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("route update request is from my parent\r\n");
#endif
				send_custom_upload_route_request();// recursive send broadcast
				last_route_updated_timer=halGetMACTimer(); // write down current time
			} else {
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("route update request is not from my parent\r\n");
#endif
			}
#endif
			break;
		case CUSTOM_FRAME_TYPE_ROUTE_UPDATE_RESPONSE:
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("recv a custom route response: src=#%u dst=#%u\r\n", *(ptr + 3), *(ptr + 2));
#endif
			if (*(ptr + 2) == MY_NODE_NUM) {  // dst is me
				merge_grandsons(ptr);
#ifndef LRWPAN_COORDINATOR
				send_custom_upload_route_response();
#endif
			}
			break;
		case CUSTOM_FRAME_TYPE_DATA:
			if (*(ptr + 1) == MY_NODE_NUM) {  // next hop is me
				if(*(ptr+2)!=MY_NODE_NUM){ // dst is not me, relay it
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("relaying msg packet, src=#%u, dst=#%u\r\n",*(ptr+3),*(ptr+2));
#endif
					send_custom_packet_relay(*(ptr + 3), *(ptr + 2), (*ptr) - 5, ptr + 5,CUSTOM_FRAME_TYPE_DATA);
				}else{ // dst is me
					update_AP_msg(ptr);
					aplRxCustomCallBack();
				}
			}
			break;
		case CUSTOM_FRAME_TYPE_UPLOADROUTEPATH_TO_PC:
			if (*(ptr + 1) == MY_NODE_NUM) {  // next hop is me
				if(*(ptr+2)!=MY_NODE_NUM){ // dst is not me, relay it
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
					printf("relaying route PC packet, src=#%u, dst=#%u\r\n",*(ptr+3),*(ptr+2));
#endif
					send_custom_packet_relay(*(ptr + 3), *(ptr + 2), 1, ptr + 5,CUSTOM_FRAME_TYPE_UPLOADROUTEPATH_TO_PC);
				}else{ // dst is me, uploading path to PC
					upload_route_for_PC(*(ptr+3), *(ptr+5));
				}
			}
			break;
		default:
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
			printf("unsupport custom frame type\r\n");
#endif
			break;
	}
}

void display_all_nodes(){
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM;++i)
		if(all_nodes[i]!=0xff)
			printf("Node #%u 's parent is #%u, ping=%ums\r\n",i,all_nodes[i],all_nodes[i+ALL_NODES_NUM]);
}

void update_route_table_cache(){
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM;++i){
		if(all_nodes[i]!=0xff)
			update_route_response_content(TRUE,i,all_nodes[i]); // update ping
		else if(all_nodes_cache[i]!=0xff)
			update_route_response_content(FALSE,i,all_nodes_cache[i]); // remove a node
		// new cache value
		all_nodes_cache[i]=all_nodes[i];
	}

}

void update_route_response_content(BOOL isAdd, unsigned char child, unsigned char parent){
	if(route_response_offset>=ALL_NODES_NUM*3) // excceed max len
		return;
	if(isAdd==TRUE)
		route_response[route_response_offset++]=child|0x80; // 增量添加
	else
		route_response[route_response_offset++]=child|0x00; // 增量删除
	route_response[route_response_offset++]=parent;
	route_response[route_response_offset++]=all_nodes[child+ALL_NODES_NUM];
}


