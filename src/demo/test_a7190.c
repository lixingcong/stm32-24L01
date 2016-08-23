//仅供测试

#include "A7190.h"
#include "delay.h"
#include "SPI1.h"
#include "lib_headers_stm32.h"


int TxDataViaA7190(unsigned char *frm,unsigned char len) {
	if (A7190_read_state() == IDLE) {  //is TX active?
		// Asserting the status flag and enabling ACK reception if expected.
		A7190_set_state(WAIT_TX);
		StrobeCmd(CMD_STBY);
		StrobeCmd(CMD_TFR);
		WriteFIFO(frm, len);
		A7190_set_state(BUSY_TX);
		StrobeCmd(CMD_TX);
		// while(check GIO1);;;;
	} else{
		printf("busy when Tx\r\n");
		return 1;
	}
	return 0;
}
int RxDataViaA7190(unsigned char *buf, int len) {
	// 应该加入一个检测电平GIO的，类似于中断的玩意
	if (A7190_read_state() == IDLE) {
		A7190_set_state(BUSY_RX);
		ReadFIFO(buf,len);
		A7190_set_state(IDLE);
	}else{
		printf("busy when Rx\r\n");
		return 1;
	}
	return 0;
}

int main(){
	int res;
#ifdef LRWPAN_COORDINATOR
	char test_chars[12]="hellocoord!";
#else
	char test_chars[12]="hellorouter";
#endif
	char buf[12];
	int cnt=0;

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	// init delay timer
	init_delay();
	// init USART
	USART1_init();
	USART2_init();
	USART_scanf_config_EXT();

	// init SP1+A7190
	SPI1_Init();

	//EXTI_config_for_A7190();


	initRF();
	printf("\r\ninit ok\r\n");
	while(1){
		res=1;
#ifdef LRWPAN_COORDINATOR
		res=TxDataViaA7190(test_chars,12);
		if(res==0)
			printf("sent\r\n");
#else
		res=RxDataViaA7190(buf, 12);
		if(res==0){
			for(int i=0;i<12;i++)putchar(buf[i]);
			printf("  %d\r\n",cnt++);
		}
#endif
		DelayMs(1000);
	}
}
