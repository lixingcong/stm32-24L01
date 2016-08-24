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

unsigned char route_response[3*ALL_NODES_NUM+3];// 缓冲区专门存放待发送的增量路由表，前面有3个帧头
unsigned char route_response_offset;// 增量路由表偏移量

// only for router update, used by update_route_table_info()
#ifdef LRWPAN_ROUTER
unsigned int last_route_updated_timer;
#endif

static unsigned char payload_custom[LRWPAN_MAX_FRAME_SIZE];
BOOL isOffline;

unsigned char my_children_number;

void init_all_nodes(){
	unsigned char i;
	for(i=0;i<ALL_NODES_NUM;++i){
		all_nodes[i]=all_nodes[i+ALL_NODES_NUM]=0xff;
	}
	route_response_offset=3;
	my_children_number=0;
	my_parent=0xff;
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
		if (all_nodes[i] == (MY_NODE_NUM)) {  // my child
			if(0xff==macTxCustomPing(i, PING_DIRECTION_TO_CHILDREN, 2, 300)){
				// TODO: 误删孩子情况偶尔出现 2016年8月18日 上午10:33:11
				// if not online, del node in cache
				update_route_response_content(FALSE, i, MY_NODE_NUM);
				all_nodes[i]=0xff;
				--my_children_number;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("Delete child #%u\r\n",i);
#endif
			}
		}
	}

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
	for(i=0;i<(*(ptr+1)-5);i+=3){
		switch(*(my_ptr++)){
			if(*(my_ptr)>=ALL_NODES_NUM)
				break;
			case FRAME_FLAG_UPDATE_ROUTE_ADD:
				all_nodes[*(my_ptr)]=*(my_ptr+1);
				printf("updated");
				break;
			case FRAME_FLAG_UPDATE_ROUTE_REMOVE:
				if(all_nodes[*(my_ptr)]==*(my_ptr+1))
					all_nodes[*(my_ptr)]=0xff;
				printf("deleted");
				break;
			default:
				break;
		}
		printf(": node #%u 's parent is #%u\r\n",*(my_ptr),*(my_ptr+1));
		my_ptr+=2;
	}

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
}
// 向父亲上传自己的路由表，
void send_route_increasing_change_to_parent(){
	unsigned char i;
	route_response[0]=FRAME_TYPE_SHORT_ROUTE_UPDATE;
	route_response[1]=my_parent;
	route_response[2]=MY_NODE_NUM;
	printf("in send increasing: ");
	for(i=0;i<route_response_offset;++i)
		printf("%x ",route_response[i]);
	printf("\r\n");
	halSendPacket(route_response_offset, &route_response[0], TRUE);

	DelayMs(2);
	route_response_offset=3;
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

void display_all_nodes(){
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM;++i)
		if(all_nodes[i]!=0xff)
			printf("Node #%u 's parent is #%u, ping=%ums\r\n",i,all_nodes[i],all_nodes[i+ALL_NODES_NUM]);
}

void update_route_table_cache(){
#if 0
	unsigned char i;
	for(i=1;i<ALL_NODES_NUM;++i){
		if(all_nodes[i]!=all_nodes_cache[i] ){
			if(all_nodes_cache[i]==0xff)
				update_route_response_content(TRUE, i, all_nodes[i]); // add a child
			if(all_nodes[i]==0xff)
				update_route_response_content(FALSE, i, all_nodes_cache[i]); // delete a child
		}
		all_nodes_cache[i]=all_nodes[i];
	}
#endif
}

void update_route_response_content(BOOL isAdd, unsigned char child, unsigned char parent){
	if(route_response_offset>=ALL_NODES_NUM*3+3) // excceed max len
		return;
	if(isAdd==TRUE)
		route_response[route_response_offset++]=FRAME_FLAG_UPDATE_ROUTE_ADD; // 增量添加
	else
		route_response[route_response_offset++]=FRAME_FLAG_UPDATE_ROUTE_REMOVE; // 增量删除
	route_response[route_response_offset++]=child;
	route_response[route_response_offset++]=parent;
}

void macRxCustomPacketCallback(unsigned char *ptr){
	unsigned short flen;
	if((*(ptr)&0xf0)==0xf0){
		// long msg(should judge outside)
		flen=((*ptr)&0x01)<<8;
		flen+=*(ptr+1);
	}else{
		// short msg
		flen=*(ptr+1);
		switch(*(ptr+2)){ // switch frame type
			case FRAME_TYPE_SHORT_BEACON:
				// TODO: beacon 2016年8月23日 下午11:53:12
				break;
			case FRAME_TYPE_SHORT_PING:
				macRxPingCallback(ptr);
				break;
			case FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL:
				if(*(ptr+5)==FRAME_FLAG_JOIN_REQUEST){ // join req
					if(all_nodes[MY_NODE_NUM]==*(ptr+4))// sender is my parent, not allow to join(loopback)
						break;
					if(all_nodes[*(ptr+4)]==MY_NODE_NUM){
						send_join_network_response(*(ptr+4));
						break;
					}
					if(my_children_number<MAX_CHILDREN_NUM){
						DelayMs(1);
						send_join_network_response(*(ptr+4));
					}

				}

				else if(*(ptr+5)==FRAME_FLAG_JOIN_RESPONSE){ // join response
					if (isOffline == TRUE) {
						isOffline = FALSE;
						my_parent = *(ptr + 4);
						all_nodes[MY_NODE_NUM]=my_parent;
						send_join_network_response_ack(*(ptr+4));
					}
				}else{ // a join ACK
					printf("recv a join ack\r\n");
					all_nodes[*(ptr+4)]=*(ptr+3);
					if(*(ptr+3)==MY_NODE_NUM){ // new child
						printf("Node #%u joined\r\n",*(ptr+4));
						update_route_response_content(TRUE, *(ptr+4), MY_NODE_NUM);
						++my_children_number;
					}

				}
				break;
			case FRAME_TYPE_SHORT_ROUTE_UPDATE:
				if(*(ptr+3)==MY_NODE_NUM || *(ptr+4)==my_parent)
					merge_grandsons(ptr);
				break;
			default:
				break;
		}
	}
}


void send_join_network_response(unsigned char dst){
	if(my_children_number<MAX_CHILDREN_NUM){
		payload_custom[0]=FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
		payload_custom[1]=dst;
		payload_custom[2]=MY_NODE_NUM;
		payload_custom[3]=FRAME_FLAG_JOIN_RESPONSE;
		halSendPacket(4, payload_custom, TRUE);
	}
}

void send_join_network_response_ack(unsigned char dst){
	payload_custom[0]=FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
	payload_custom[1]=dst;
	payload_custom[2]=MY_NODE_NUM;
	payload_custom[3]=FRAME_FLAG_JOIN_RESPONSE_ACK;
	halSendPacket(4, payload_custom, TRUE);
}


