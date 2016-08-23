//仅供测试

#include "A7190.h"
#include "delay.h"
#include "SPI1.h"
#include "lib_headers_stm32.h"
#include "halStack.h"

UINT32 spi_timer;
BYTE *A7190Data;
UINT16 SHORTADDR;
BYTE address_filter[3];

int TxDataViaA7190(BYTE *frm,BYTE len) {
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
int RxDataViaA7190(BYTE *buf, int len) {
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
	halInit();
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

//called when the slow timer interrupt occurs
#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void) {
}
#endif

//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void) {
}
