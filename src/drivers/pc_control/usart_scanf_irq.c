/*
 * usart_scanf_irq.c
 *
 *  Created on: 2016年7月20日
 *      Author: lixingcong
 *      串口中断服务回调函数，处理来自上位机的命令
 */

#include "usart_scanf_irq.h"
#include "parse_PC_cmd.h"
#include "execute_PC_cmd.h"
#include "stdio.h"
#include "string.h"
#include "route_table.h"
#include "route_ping.h"
#include "delay.h"


unsigned char usart_scanf_data[MAX_USART1_BUFFER_LEN];

// Hint: input test hex string:
//   37535a4231632368692c6d616e5c695c616d5c746f6e790d0a  send a msg to #2

void usart_irq_scanf_callback() {
	static char res;
#if 0           // for debug
	int len = strlen(usart_scanf_data);
	int i;
	fprintf(stderr, "in usart1 callback, len=%d, str:\r\n", strlen(usart_scanf_data));
	for (i = 0; i < len; i++)
	fprintf(stderr, "%x ", usart_scanf_data[i]);
	fprintf(stderr, "\r\n");
#endif

	// parse
	res = parse_command(usart_scanf_data);
	switch (res) {
		case CMD_SEND_MSG:
			execute_PC_command();
			fprintf(stderr, "ZZIFSET OK!@\r\n");
			break;
		case CMD_REQUEST_ROUTETABLE:  // 上报路由
			upload_route_table();
			break;
		case CMD_SELF_CHECK:  //上报状态
			upload_self_check_status();
			break;
		case CMD_SEND_TEST:
			send_test(send_test_group_num);
			break;
		default:  // 未识别命令
			fprintf(stderr, "ZZIFInvalid Command@\r\n");
			break;
	}

}
