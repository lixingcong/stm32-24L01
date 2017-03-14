/*
 * define_songlu.h
 *
 *  Created on: 2016年7月20日
 *      Author: lixingcong
 *      根据songlu的指令预处理 固定变量
 */

#ifndef _DEFINE_SONGLU_H_
#define _DEFINE_SONGLU_H_

enum{
	CMD_SEND_MSG,
	CMD_REQUEST_ROUTETABLE,
	CMD_SEND_TEST,
	CMD_SELF_CHECK
};

#define SEND_DIRECTION_OPTION_FLAG 0x42 // 'B'
#define SEND_DIRECTION_P2P 0x31  // '1'
#define SEND_DIRECTION_BOARDCAST 0x32  // '2'

#endif
