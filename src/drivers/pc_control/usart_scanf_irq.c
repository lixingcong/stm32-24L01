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
#include "delay.h"

// 设置存储
control_from_pc_t my_control_from_pc;
control_from_pc_t *my_control_from_pc_ptr=&my_control_from_pc;

unsigned char usart_scanf_data[MAX_USART1_BUFFER_LEN];

// Hint: input test hex string:  0F535A4131353637423239433144310D0A   定频567M
//                               0F535A4132353631423239433144310D0A   跳频32频道
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
	res=parse_command(usart_scanf_data, my_control_from_pc_ptr);
	switch(res){
		case 0: // 设置工作模式
			execute_PC_command(my_control_from_pc_ptr);
			fprintf(stderr, "ZZIFSET OK!@\r\n");
			break;
		case 1:  // 上报路由
			upload_route_table();
			break;
		case 2:  //上报状态
			upload_self_check_status();
			break;
		default: // 未识别命令
			fprintf(stderr, "ZZIFInvalid Command@\r\n");
			break;
	}

}
