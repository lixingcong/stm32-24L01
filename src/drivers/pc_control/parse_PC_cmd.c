/*
 * parse_PC_cmd.c
 *
 *  Created on: 2016年7月19日
 *      Author: lixingcong
 *      根据上位机发送的指令解析出命令参数
 */

#include "define_songlu.h"
#include "parse_PC_cmd.h"
#include "string.h"
#include "stdio.h"
#include "execute_PC_cmd.h"

// global
CMD_SEND_MSG_T cmd_send_msg={0};
unsigned short send_test_group_num;
unsigned short send_test_group_fail_delay;

#if 0
int hex2dec(char in) {
	if (in <= '9' && in >= '0')
	return in - 48;
	if (in <= 'f' && in >= 'a')
	return in - 87;
	return -1;
}

char dec2hex(int in) {
	if (in >= 0) {
		if (in < 10)
		return in + 48;
		if (in < 16)
		return in + 87;
	}
	return -1;
}

void str2case(char *in, char *out) {
	char *ptr = in, *ptr_out = out;
	while (*ptr) {
		if ((*ptr <= 'Z' && *ptr >= 'A') || (*ptr <= 'z' && *ptr >= 'a'))
		*ptr_out = (*ptr) + 32;
		else
		*ptr_out = *ptr;
		++ptr;
		++ptr_out;
	}
	*ptr_out = 0;
}

int hex2str(char *in, char *out, int len) {
	int i, sum;
	char *ptr = out;
	if (len % 2 == 1) {
		printf("hex2str error: len is even\r\n");
		return -1;
	}
	for (i = 0; i < len; i += 2) {
		sum = hex2dec(*(in + i + 1));
		sum += hex2dec(*(in + i)) << 4;
		*ptr = sum;
		++ptr;
	}
	*ptr = 0;
	return 0;
}
int str2hex(char *in, char *out, int len) {
	int i;
	char high, low;
	char *ptr = out;
	for (i = 0; i < len; ++i) {
		*ptr = dec2hex((int) (*(in + i)) >> 4);
		ptr++;
		*ptr = dec2hex((int) (*(in + i)) & 0x0f);
		ptr++;
	}
	*ptr = 0;
	return 0;
}
#endif

// 从字符串中取出数字 参数：str字符串，stopChar停止字符，len指针是取出数字跨越的字符个数
static unsigned short get_num_from_string(unsigned char *str, unsigned char stopChar, unsigned char *len){
	unsigned short val=0;
	unsigned char i=0;
	while(*(str+i)!=stopChar){
		val=val*10+(*(str+i)-'0');
		i++;
	}
	*len=i;
	return val;
}

// firstly execute it
char parse_command(char *in1) {
	unsigned int len, i;
	char *ptr = in1;
	unsigned char len_of_num;
	/* valify */
	len = *(ptr++) - 32;
	if (strlen(in1) != len) {
		//printf("parse_command error: invalid len, drop packet\r\n");
		printf("packet[0] len is %d, but in fact len is %d\r\n", len, strlen(in1));
		return -1;
	}
	if (*(ptr) == 'S' && *(ptr + 1) == 'Z') {
		// skip 'SZ'
		ptr += 2;
		/* dst mode */
		if (*(ptr++) == SEND_DIRECTION_OPTION_FLAG) {
			switch (*(ptr++)) {
				case SEND_DIRECTION_P2P:
					if ((*ptr) >= 'a' && (*ptr) <= 'z')
						cmd_send_msg.dest = (0x01 << 8 | (*ptr) - 'a');
					else {
						printf("error dst number, drop.\r\n");
						return -1;
					}
					break;
				case SEND_DIRECTION_BOARDCAST:
					cmd_send_msg.dest=0xff;
					break;
				default:
					printf("error dst number, drop.\r\n");
					return -1;
			}
			ptr += 2;  // skip '#'
			for (i = 0; i < len - 7; ++i) {
				cmd_send_msg.msg[i] = (*(ptr+i)=='\\')?(' '):(*(ptr+i)); // escape from space
			}
			cmd_send_msg.msg[i]=0; // add NULL at the end of string
			cmd_send_msg.len=len - 7;
		} else {
			printf("no dst flag, drop.\r\n");
			return -1;
		}
		return CMD_SEND_MSG;
	} else if (*(ptr) == 'Z' && *(ptr + 1) == 'T') {
		printf("uploading route table.\r\n");
		return CMD_REQUEST_ROUTETABLE;
	} else if (*(ptr) == 'S' && *(ptr + 1) == 'C') {
		printf("uploading self check status\r\n");
		return CMD_SELF_CHECK;
	} else if (*(ptr) == 'T' && *(ptr + 1) == 'N') {
		// skip 'Tn'
		ptr += 2;
		send_test_group_num=get_num_from_string(ptr, ',', &len_of_num);
		send_test_group_fail_delay = get_num_from_string(ptr + len_of_num + 1, 0, &len_of_num);
		//fprintf(stderr, "num=%u, delay=%u\r\n",send_test_group_num, send_test_group_fail_delay);
		return CMD_SEND_TEST;
	} else {
		printf("invalid command from PC, drop.\r\n");
		return -1;
	}
}

/*
 // 演示程序： 运行在PC机上，不是stm32上
 int  main()
 {
 int len;
 char buffer1[MAX_BUF_LEN];
 char buffer2[MAX_BUF_LEN];
 control_from_pc_t *my_control_from_pc_ptr=&my_control_from_pc;
 while(1){
 printf("input a hex\r\n");
 scanf("%s",buffer1);
 len=strlen(buffer1);
 str2case(buffer1,buffer2);
 hex2str(buffer2,buffer1,len);
 len>>=1;
 parse_command(buffer1,my_control_from_pc_ptr);
 print_command(my_control_from_pc_ptr);
 }
 return 0;
 }
 */
