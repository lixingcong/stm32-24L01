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
#include "A7190.h"
#include "ctl_lmx2581.h"

unsigned char dynamic_freq_mode;
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

#ifdef LRWPAN_COORDINATOR
// 跳频模式，暂时失去协调器的组网功能（在spi1_irq.c中屏蔽接收处理过程，收到任何东西都将丢弃）
void work_under_dynamic_freq_mode() {
	unsigned short freq, freq_step_in;
	unsigned char steps;
	unsigned int last_timer;
	steps = DIVISION_OF_DYNAMIC_FREQ[dynamic_freq_mode];
#define LMX2581_MAX_FREQ 700
#define LMX2581_MIN_FREQ 400
	freq = LMX2581_MIN_FREQ;
	freq_step_in = (LMX2581_MAX_FREQ - LMX2581_MIN_FREQ) / steps;
	last_timer = halGetMACTimer();
	// reset first three bytes to avoid received by other nodes incidentally
	recv_buffer_a7190[0] = recv_buffer_a7190[1] = recv_buffer_a7190[2] = 0;
	while (1) {
		if (dynamic_freq_mode == 0xff)  // 定频模式：跳出
			break;

		halSendPacket(((unsigned short) LRWPAN_MAX_FRAME_SIZE), recv_buffer_a7190, FALSE);  // 往死里发包
		// TODO: 跳频模式下发送间隔，频谱仪上显示不稳定 2016年8月18日 下午1:01:51
		if (halMACTimerNowDelta(last_timer) > 10) {  // 每10ms改变VCO频率
			freq = (freq < (LMX2581_MAX_FREQ - freq_step_in) ? (freq + freq_step_in) : LMX2581_MIN_FREQ);
			ctl_frequency_set(freq);
			last_timer = halGetMACTimer();
		}
	}
	ctl_frequency_set(my_control_from_pc.freq);
#undef LMX2581_MAX_FREQ
#undef LMX2581_MIN_FREQ
}
#endif

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
	for (i = 1; i <= ALL_NODES_NUM; ++i) {
		if (all_nodes[i] < ALL_NODES_NUM)  // todo: no ping info (ALL
			fprintf(stderr, "$,R%u,ADDR%u,%u,&%u,#", i, i, 0xff, all_nodes[i]);
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
		dynamic_freq_mode = 0xff;  // 取消跳频模式
		printf("%u\r\n", in->freq);
		// to 彭朋：这里设置定频工作
		// ctl_frequency_set(in->freq);
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
		dynamic_freq_mode = ((in->freq) - '1');
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

