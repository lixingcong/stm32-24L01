/*
 * parse_PC_cmd.h
 *
 *  Created on: 2016年7月20日
 *      Author: lixingcong
 */

#ifndef _PARSE_CONTROL_CMD_H_
#define _PARSE_CONTROL_CMD_H_

#include "define_songlu.h"

typedef struct CMD_SEND_MSG_T_{
	unsigned short dest;
	unsigned short len;
	unsigned char msg[30];
}CMD_SEND_MSG_T;

extern CMD_SEND_MSG_T cmd_send_msg;
extern unsigned short send_test_group_num;
extern unsigned short send_test_group_fail_delay;

char parse_command(char *in1);
//void str2case(char *in, char *out);

#endif /* SRC_DRIVERS_PC_CONTROL_PARSE_CONTROL_CMD_H_ */
