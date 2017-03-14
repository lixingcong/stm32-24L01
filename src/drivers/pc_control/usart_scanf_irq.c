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

// Hint: input test hex string:  0F535A4131353637423239433144310D0A   定频567M 广播
//                               0F535A4132353631423239433144310D0A   跳频32频道
//                               0F535A4131353637423239433144310D0A   定频567 单播
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
			// 批量发送测试
			break;
		default:  // 未识别命令
			fprintf(stderr, "ZZIFInvalid Command@\r\n");
			break;
	}

}
