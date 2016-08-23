//Time-stamp: < 1.c 2016-07-19 13:40:13 >
/*功能：根据上位机发送的指令解析出命令参数
 */

#include "define_songlu.h"
#include "parse_PC_cmd.h"
#include "string.h"
#include "stdio.h"
#include "execute_PC_cmd.h"

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

// firstly execute it
char parse_command(char *in1, control_from_pc_t *in2) {
	unsigned int len, i;
	char *ptr = in1;
	/* valify */
	len = *(ptr++);
	if (strlen(in1) != len) {
		printf("parse_command error: invalid len, drop packet\r\n");
		printf("packet[0] len is %d, but in fact len is %d\r\n",len,strlen(in1));
		return -1;
	}
	if (*(ptr) == 'S' && *(ptr+1) == 'Z') {
		// skip 'SZ'
		ptr+=2;
		/* work mode */
		if (*(ptr++) == MODE_OPTION_FLAG) {
			switch (*(ptr++)) {
				case MODE_STATIC_FREQ:
					// input a 3-digit num
					in2->mode = 0;
					in2->freq =((*ptr++)-48)*100;
					in2->freq+=((*ptr++)-48)*10;
					in2->freq+=((*ptr++)-48);
					break;
				case MODE_DYNAMIC_FREQ:
					// input a M0x
					in2->mode = 1;
					in2->freq=*(ptr+2);
					ptr+=3;
					break;
				default:
					printf("error work mode, drop.\r\n");
					return -1;
			}
		} else {
			printf("no work mode flag, drop.\r\n");
			return -1;
		}

		/* dst mode */
		if (*(ptr++) == SEND_DIRECTION_OPTION_FLAG) {
			switch (*(ptr++)) {
				case SEND_DIRECTION_P2P:
					if ((*ptr) >= 'a' && (*ptr) <= 'z')
						in2->dst = (0x01 << 8 | (*ptr) - 'a') + 1;
					else {
						printf("error dst number, drop.\r\n");
						return -1;
					}
					break;
				case SEND_DIRECTION_BOARDCAST:
					in2->dst = 0xffff;
					break;
				default:
					printf("error dst number, drop.\r\n");
					return -1;
			}
			++ptr;
		} else {
			printf("no dst flag, drop.\r\n");
			return -1;
		}

		/* rate */
		if (*(ptr++) == RATE_OPTION_FLAG) {
			switch (*(ptr++)) {
				case RATE_2M:
					in2->rate = 0;
					break;
				case RATE_512K:
					in2->rate = 1;
					break;
				case RATE_256K:
					in2->rate = 2;
					break;
				case RATE_128K:
					in2->rate = 3;
					break;
				default:
					printf("error data rate, drop.\r\n");
					return -1;
			}
		} else {
			printf("no rate mode flag, drop.\r\n");
			return -1;
		}

		/* data frame len */
		if (*(ptr++) == PAYLOAD_SET_LEN_OPTION_FLAG) {
			switch (*(ptr++)) {
				case PAYLOAD_SET_LEN_SHORT:
					in2->data_len = 0;
					break;
				case PAYLOAD_SET_LEN_MIDDLE:
					in2->data_len = 1;
					break;
				case PAYLOAD_SET_LEN_LONG:
					in2->data_len = 2;
					break;
				default:
					printf("error data len, drop.\r\n");
					return -1;
			}
		} else {
			printf("no FIFO len mode flag, drop.\r\n");
			return -1;
		}

		return 0;
	}else if (*(ptr) == 'Z' && *(ptr+1) == 'T') {
		printf("uploading route table.\r\n");
		return 1;
	}else if( *(ptr) == 'S' && *(ptr+1) == 'C'){
		printf("uploading self check status\r\n");
		return 2;
	}
	else{
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
