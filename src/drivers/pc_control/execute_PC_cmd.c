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
#include "hal.h"
#include "usart.h"
#include "NRF_api.h"

BOOL isBroadcastRegularly;
BOOL testAckPending;
unsigned int last_broadcast_timer;

// 以下变量是发送测试用的
static unsigned short rx_group_num=0;
static unsigned int rx_bytes=0;
static unsigned int lost_rate_multi_10000,err_rate_multi_10000;
static unsigned int err_bytes=0;
static unsigned char valid_test_plen=LRWPAN_MAX_FRAME_SIZE-3; // 3 bytes header

// 以下buf是输出字符串到PC
static unsigned char tx_group_num_buf[6]={0};
static unsigned char rx_group_num_buf[6]={0};
static unsigned char err_rate_buf[7]={0};
static unsigned char lost_rate_buf[7]={0};

// stat
static unsigned char send_test_data[LRWPAN_MAX_FRAME_SIZE]={
	0x00,0xCD,0x8A,0x91,0xC6,0xD5,0xC4,0xC4,
	0x40,0x21,0x18,0x4E,0x55,0x86,0xF4,0xDC,
	0x8A,0x15,0xA7,0xEC,0x92,0xDF,0x93,0x53,
	0x30,0x18,0xCA,0x34,0xBF,0xA2,0xC7,0x59
};

void send_test(unsigned short num);
static void send_test_show_result(unsigned short current);

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

// 批量发送测试 统计信号质量,num为发送组数
void send_test(unsigned short num){
	unsigned short current=1;
	unsigned int last_timer_sendPacket;
	unsigned int last_timer_showPC=0;
	unsigned int timer;

	rx_group_num=rx_bytes=err_bytes=0;

	USART_INPUT_NVIC_disable();
	isOffline=TRUE;
	NRF_set_state(NRF_STATE_IDLE);
	send_test_data[0]=FRAME_TYPE_SHORT_SEND_TEST_SEND;

	while(current<=num){
		printf("%u/%u\r\n",current,num);
		halSendPacket(LRWPAN_MAX_FRAME_SIZE-2,send_test_data,TRUE);
		testAckPending=TRUE;

		last_timer_sendPacket=halGetMACTimer();
		while(1){
			timer=halMACTimerNowDelta(last_timer_sendPacket);
			if(testAckPending==FALSE){
				rx_bytes+=valid_test_plen; // recv a seq bytes
				rx_group_num++;
				break;
			}

			if(timer>1000)// wait for 100ms
				break;
		}

		// show to PC
		if(halMACTimerNowDelta(last_timer_showPC)>500){ // every 500ms
			last_timer_showPC=halGetMACTimer();
			send_test_show_result(current);
		}

		++current;
	}

	send_test_show_result(num); // last result

	USART_INPUT_NVIC_enable();
}

// 回调函数，被mac层函数调用
void send_test_replyACK(unsigned char *ptr){
	unsigned char i;
	static unsigned char rx_buf_reply[LRWPAN_MAX_FRAME_SIZE-2];
	rx_buf_reply[0]=FRAME_TYPE_SHORT_SEND_TEST_RECV;
	for(i=1;i<LRWPAN_MAX_FRAME_SIZE-2;++i){
		rx_buf_reply[i]=*(ptr+2+i);
	}
	halSendPacket(LRWPAN_MAX_FRAME_SIZE-2,rx_buf_reply,TRUE);
}

void send_test_checkData(unsigned char *ptr){
	unsigned char i;
	for(i=3;i<LRWPAN_MAX_FRAME_SIZE;++i){
		if(0 != (send_test_data[i-2] ^ (*(ptr+i))))
			++err_bytes;
	}
}

static void send_test_show_result(unsigned short current){ // 参数current是当前第几组
	// 已发送组数
	tx_group_num_buf[0]=(current/10000)%10+'0';
	tx_group_num_buf[1]=(current/1000)%10+'0';
	tx_group_num_buf[2]=(current/100)%10+'0';
	tx_group_num_buf[3]=(current/10)%10+'0';
	tx_group_num_buf[4]=(current)%10+'0';

	// 已接收组数
	rx_group_num_buf[0]=(rx_group_num/10000)%10+'0';
	rx_group_num_buf[1]=(rx_group_num/1000)%10+'0';
	rx_group_num_buf[2]=(rx_group_num/100)%10+'0';
	rx_group_num_buf[3]=(rx_group_num/10)%10+'0';
	rx_group_num_buf[4]=(rx_group_num)%10+'0';

	// 丢包率
	lost_rate_multi_10000=(((int)(current - rx_group_num))*1000/current);
//	lost_rate_multi_1000=(int)((current-rx_group_num)*1000/current);
	lost_rate_buf[0]=(lost_rate_multi_10000/1000)+'0';
	lost_rate_buf[1]=(lost_rate_multi_10000/100)%10+'0';
	lost_rate_buf[2]=(lost_rate_multi_10000/10)%10+'0';
	lost_rate_buf[3]='.';//小数点
	lost_rate_buf[4]=lost_rate_multi_10000%10+'0';
	lost_rate_buf[5]='%';

	// 错误率
	err_rate_multi_10000=((int)err_bytes*1000/rx_bytes);
	err_rate_buf[0]=(err_rate_multi_10000/1000)+'0';
	err_rate_buf[1]=(err_rate_multi_10000/100)%10+'0';
	err_rate_buf[2]=(err_rate_multi_10000/10)%10+'0';
	err_rate_buf[3]='.';
	err_rate_buf[4]=err_rate_multi_10000%10+'0';
	err_rate_buf[5]='%';

	// 发送到PC
	fprintf(stderr,"ZZTJ%s,%s,%s,%s@\r\n",
			tx_group_num_buf,rx_group_num_buf,lost_rate_buf,err_rate_buf);
}
