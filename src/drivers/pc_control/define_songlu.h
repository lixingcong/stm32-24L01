/*
 * define_songlu.h
 *
 *  Created on: 2016年7月20日
 *      Author: lixingcong
 *      根据songlu的指令预处理 固定变量
 */

#ifndef _DEFINE_SONGLU_H_
#define _DEFINE_SONGLU_H_

#define MODE_OPTION_FLAG 0x41
#define MODE_STATIC_FREQ 0x31    // '1'
#define MODE_DYNAMIC_FREQ 0x32  // '2'
#define MODE_DYNAMIC_FREQ_32CH 0x31
#define MODE_DYNAMIC_FREQ_64CH 0x32
#define MODE_DYNAMIC_FREQ_128CH 0x33
static unsigned char DIVISION_OF_DYNAMIC_FREQ[3] = { 32, 64, 128 };

#define SEND_DIRECTION_OPTION_FLAG 0x42 // 'B'
#define SEND_DIRECTION_P2P 0x31  // '1'
#define SEND_DIRECTION_BOARDCAST 0x32  // '2'

#define RATE_OPTION_FLAG 0x43  // 'C'
#define RATE_2M 0x34 // '4'
#define RATE_512K 0x33 // '3'
#define RATE_256K 0x32 // '2'
#define RATE_128K 0x31  // '1'
static unsigned char RATE_DELAY_MS_LIST[4] = { 0, 3, 7, 15 };  //经过逻辑分析仪测试得出的延时值

#define PAYLOAD_SET_LEN_OPTION_FLAG 0x44 // 'D'
#define PAYLOAD_SET_LEN_SHORT 0x31
#define PAYLOAD_SET_LEN_MIDDLE 0x32
#define PAYLOAD_SET_LEN_LONG 0x33
static unsigned char PAYLOAD_SET_LEN_LIST[3] = { 32, 128, 219 };

typedef struct control_from_pc {
	unsigned char data_len, mode, rate;
	;
	unsigned short freq;
	unsigned short dst;
} control_from_pc_t;

extern control_from_pc_t my_control_from_pc;

#endif
