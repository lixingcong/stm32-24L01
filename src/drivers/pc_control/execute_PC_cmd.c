/*
 * execute_PC_cmd.c
 *
 *  Created on: 2016年7月27日
 *      Author: lixingcong
 */

#include "stdio.h"
#include "parse_PC_cmd.h"
#include "execute_PC_cmd.h"
#include "define_songlu.h"
#include "route_table.h"
#include "route_ping.h"
#include "route_AP_level.h"

BOOL isBroadcastRegularly;
unsigned int last_broadcast_timer;

void show_msg_to_PC(unsigned char sender, unsigned char *msg, unsigned char len, BOOL isBoardcast) {
	unsigned char i;
	fprintf(stderr, "ZZRC%u,%u,", isBoardcast, sender);
	for (i = 0; i < len; ++i) {
		fprintf(stderr, "%0x", *(msg + i));
	}
	fprintf(stderr, "@\r\n");
}

// 成功上报1，失败上报0
void upload_self_check_status() {
	if (isOffline == FALSE) {
#ifdef LRWPAN_COORDINATOR
		fprintf(stderr, "ZZCK1,C,%u@\r\n", MY_NODE_NUM);
#else
		fprintf(stderr,"ZZCK1,R,%u@\r\n",MY_NODE_NUM);
#endif
	} else {
		fprintf(stderr, "ZZCK0@\r\n");
	}
}

// 上传路由表
void upload_route_table() {
	unsigned char i;
	fprintf(stderr, "ZZST");
	fprintf(stderr, "$,R0,ADDR0, ,&0,#");
	for (i = 0; i < ALL_NODES_NUM; ++i) {
		if (all_nodes[i] < ALL_NODES_NUM)
			fprintf(stderr, "$,R%u,ADDR%u,%u,&%u,#", i, i, all_nodes_ping[i], all_nodes[i]);
	}
	fprintf(stderr, "@\r\n");
}

// 执行这个函数前,必须保证parse_command是正确返回值0的！！
void execute_PC_command() {
	printf("\r\nto:");
	if (cmd_send_msg.dest == 0xffff){
		puts("boardcast");
		isBroadcastRegularly=TRUE;
		last_broadcast_timer=halGetMACTimer();
	}

	else if ((cmd_send_msg.dest) & 0x100){
		printf("#%u\r\n", (cmd_send_msg.dest) & 0xff);
		isBroadcastRegularly=FALSE;
	}

	else {
		puts("not set yet");
		return;
	}
	printf("msg: %s\r\n",cmd_send_msg.msg);
	printf("\r\n--------------\r\n");

	// 将cmd_send_msg中的内容发送出去
	aplSendMSG(cmd_send_msg.dest & 0xff, cmd_send_msg.len & 0xff, cmd_send_msg.msg);
}

void upload_route_for_PC(unsigned char src, unsigned char dst) {
	fprintf(stderr, "ZZRT%u,%u@\r\n", src, dst);
	printf("--PC: Node #%u -> Node#%u@\r\n", src, dst);
}

