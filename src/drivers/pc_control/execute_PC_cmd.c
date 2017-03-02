/*
 * execute_PC_cmd.c
 *
 *  Created on: 2016年7月27日
 *      Author: lixingcong
 */

#include <exti8_irq.h>
#include "execute_PC_cmd.h"
#include "define_songlu.h"
#include "route_table.h"
#include "route_ping.h"
#include "A7190.h"

BOOL isBroadcastRegularly;
unsigned int last_broadcast_timer;

void set_payload_len(unsigned char i) {
	/*
	 PAYLOAD_SET_LEN=PAYLOAD_SET_LEN_LIST[i];
	 // fprintf(stdout,"payload has been set to %d\r\n",PAYLOAD_MAX_LEN);
	 */
}

void set_datarate(unsigned char i) {
	/*
	 RATE_DELAY_MS=(unsigned int)RATE_DELAY_MS_LIST[i];
	 */
}

void send_test_msg_to_dst(unsigned char dst) {

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
void execute_PC_command(control_from_pc_t *in) {
	printf("\r\nwork mode settings:");
	printf("\r\nmode:\t");
	switch (in->mode) {
		case 0:
			puts("static freq");
			break;
		case 1:
			puts("dynamic freq");
			break;
		default:
			puts("not set yet");
			return;
	}

	printf("\r\nfreq:\t");
	if (in->freq > 100) {
		printf("%u\r\n", in->freq);
	} else {
		switch (in->freq) {
			case MODE_DYNAMIC_FREQ_32CH:
				puts("32 channels");
				break;
			case MODE_DYNAMIC_FREQ_64CH:
				puts("64 channels");
				break;
			case MODE_DYNAMIC_FREQ_128CH:
				puts("128 channels");
				break;
			default:
				puts("not set yet");
				return;
		}
	}

	printf("\r\nto:\t");
	if (in->dst == 0xffff){
		puts("boardcast");
		isBroadcastRegularly=TRUE;
		last_broadcast_timer=halGetMACTimer();
	}

	else if ((in->dst) & 0x100){
		printf("#%d\r\n", (in->dst) & 0xff);
		isBroadcastRegularly=FALSE;
	}

	else {
		puts("not set yet");
		return;
	}

	printf("\r\nrate:\t");
	switch (in->rate) {
		case 0:
			puts("2Mbps");
			break;
		case 1:
			puts("512Kbps");
			break;
		case 2:
			puts("256Kbps");
			break;
		case 3:
			puts("128Kbps");
			break;
		default:
			puts("not set yet");
			return;
	}
	set_datarate(in->rate);

	printf("\r\ndlen:\t");
	switch (in->data_len) {
		case 0:
			puts("short 32Byte");
			break;
		case 1:
			puts("middle 128Byte");
			break;
		case 2:
			puts("long 512Byte");
			break;
		default:
			puts("not set yet");
			return;
	}
	set_payload_len(in->data_len);

	printf("\r\n--------------\r\n");
}

void upload_route_for_PC(unsigned char src, unsigned char dst) {
	fprintf(stderr, "ZZRT%u,%u@\r\n", src, dst);
	printf("--PC: Node #%u -> Node#%u@\r\n", src, dst);
}

