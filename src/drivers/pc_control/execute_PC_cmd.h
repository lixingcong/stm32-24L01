/*
 * execute_PC_cmd.h
 *
 *  Created on: 2016年7月27日
 *      Author: lixingcong
 */

#ifndef _EXECUTE_PC_CMD_H_
#define _EXECUTE_PC_CMD_H_

#include "define_songlu.h"
#include "hal.h"

extern BOOL isBroadcastRegularly;
extern unsigned int last_broadcast_timer;

void upload_self_check_status();
void upload_route_table();
void send_test_msg_to_dst(unsigned char dst);

// after parsing, we need to execute commands sent from PC
void execute_PC_command();

// draw the path on PC
void upload_route_for_PC(unsigned char src, unsigned char dst);

#endif /* SRC_DRIVERS_PC_CONTROL_EXECUTE_PC_CMD_H_ */
