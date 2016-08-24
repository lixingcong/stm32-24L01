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
unsigned char all_nodes[ALL_NODES_NUM];// 存放实时更新路由表，用于转发数据包等操作

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
		all_nodes[i]=0xff;
	}
	route_response_offset=3;
	my_children_number=0;
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
	unsigned char children_counter;
	unsigned char i;
	children_counter=0;
	for (i = 1; i < ALL_NODES_NUM; ++i) {
		if (all_nodes[i] == (MY_NODE_NUM)) {  // my child
			if(0xff==macTxCustomPing(i, PING_DIRECTION_TO_CHILDREN, 2, 300)){
				// TODO: 误删孩子情况偶尔出现 2016年8月18日 上午10:33:11
				// if not online, del node in cache
				update_route_response_content(FALSE, i, MY_NODE_NUM);
				all_nodes[i]=0xff;
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
				printf("Delete child #%u\r\n",i);
#endif
			}else
				++children_counter;
		}
	}
	my_children_number=children_counter;
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

/*
 * - - - -- - - - -- - - - -- - - - -- - - -- -  --
 * |         custom frame by lixingcong            |
 * - - -- - - - - - - -- - - -- - - - - - - -- - - -
 * | flen | frm_type |nexthop | dest | src | data  |
 * |   2  |     1    |   1    |   1  |  1  |  *    |
 *-- - - - - - - -- - - -- - - - - - - -- - - - - -
 */

// 注意dst为0xff为广播，谨慎使用
void send_custom_packet(unsigned char src, unsigned char dst,unsigned short flen,unsigned char *frm, unsigned char frm_type){
	unsigned short total_len,i;
	unsigned char nexthop;

	if(flen>LRWPAN_MAX_FRAME_SIZE-6){
#ifdef ROUTE_TABLE_OUTPUT_DEBUG
		printf("send_custom_packet(): packet too big, send fail\r\n");
#endif
		return;
	}

	total_len=4+flen;

	nexthop=dst;
	if(0xff==macTxPing(nexthop, TRUE, PING_DIRECTION_TO_OTHERS)) // first try to send it as next hop
		nexthop=get_next_hop(nexthop, MY_NODE_NUM);
	printf("send_custom_packet: nexthop=#%u, dst=#%u\r\n",nexthop,dst);

	payload_custom[0]=frm_type;
	payload_custom[1]=nexthop;
	payload_custom[2]=dst;
	payload_custom[3]=src;

	for(i=0;i<flen;++i)
		payload_custom[i+4]=*(frm+i);

	halSendPacket(total_len, payload_custom , FALSE);
//	printf("in send custom: ");
//	for(i=0;i<5;++i)
//		printf("%x ",payload_custom[i]);
//	printf("\r\n");

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
//	printf("in send increasing: ");
//	for(i=0;i<route_response_offset;++i)
//		printf("%x ",route_response[i]);
//	printf("\r\n");
	halSendPacket(route_response_offset, &route_response[0], TRUE);
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
			printf("- Node #%u 's parent is #%u\r\n",i,all_nodes[i]);
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

void macRxCustomPacketCallback(unsigned char *ptr, BOOL isShortMSG, unsigned short flen){
	unsigned short i;
	if(isShortMSG==FALSE){
		// long msg(should judge outside)
		printf("recv long msg:\r\n");
		for(i=0;i<flen;i++)
			printf("%u: %x\r\n",i,*(ptr+i));
	}else{
		switch(*(ptr+2)){ // switch frame type
			case FRAME_TYPE_SHORT_BEACON:
				// TODO: beacon 2016年8月23日 下午11:53:12
				break;
			case FRAME_TYPE_SHORT_PING:
				macRxPingCallback(ptr);
				break;
			case FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL:
				if(*(ptr+5)==FRAME_FLAG_JOIN_REQUEST){ // join req
					if(my_parent==*(ptr+4))// sender is my parent, not allow to join(loopback)
						break;
					if(*(ptr+4)==MY_NODE_NUM) // conflict address, not allow(this situation should not appear in reality)
						break;
					if(all_nodes[*(ptr+4)]==MY_NODE_NUM){ // if dst was my son before, let him join again
						send_join_network_response(*(ptr+4),FALSE);
						break;
					}
					if(my_children_number<MAX_CHILDREN_NUM){
						DelayMs(1); // slow down for a while
						send_join_network_response(*(ptr+4),FALSE);
					}

				}

				else if(*(ptr+5)==FRAME_FLAG_JOIN_RESPONSE){ // join response
					if (isOffline == TRUE) {
						isOffline = FALSE;
						my_parent = *(ptr + 4);
						send_join_network_response(*(ptr+4),TRUE);
					}
				}else{ // a join ACK
					printf("recv a join ack\r\n");
					all_nodes[*(ptr+4)]=*(ptr+3);
					if(*(ptr+3)==MY_NODE_NUM){ // new child
						printf("Node #%u joined\r\n",*(ptr+4));
						update_route_response_content(TRUE, *(ptr+4), MY_NODE_NUM);
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


void send_join_network_response(unsigned char dst, BOOL isACK){
	payload_custom[0]=FRAME_TYPE_SHORT_JOIN_NETWORK_SIGNAL;
	payload_custom[1]=dst;
	payload_custom[2]=MY_NODE_NUM;
	if(isACK==FALSE)
		payload_custom[3]=FRAME_FLAG_JOIN_RESPONSE;
	else
		payload_custom[3]=FRAME_FLAG_JOIN_RESPONSE_ACK;
	halSendPacket(4, payload_custom, TRUE);
}

