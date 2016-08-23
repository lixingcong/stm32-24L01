#include "hal.h"
#include "spi1_irq.h"
#include "A7190.h"
#include "A7190reg.h"
#include "SPI1.h"
//#include "route_table.h"
//#include "execute_PC_cmd.h"
//#include "route_ping.h"

unsigned char ack_bytes[LRWPAN_MAX_FRAME_SIZE]; // max len


//This interrupt used for both TX and RX
void spi1_irq_a7190(void) {
	unsigned char flen,i;

	if (A7190_read_state() == IDLE) {
#if 0
#ifdef LRWPAN_COORDINATOR
		if(dynamic_freq_mode!=0xff)//跳频模式忽略一切收到的数据包
			goto do_rxflush;
#endif
#ifdef LRWPAN_ROUTER
		if(isOffline==TRUE)
			goto do_rxflush;
#endif
#endif
		A7190_set_state(BUSY_RX);
		flen = ReadFIFO1(1);  //read the length


		do_rx:
		if (flen !=0) {
			ReadFIFO(ack_bytes, flen-1);
			printf("recv A7190: ");
			printf("%x ",flen);
			for(i=0;i<flen-1;++i)
				printf("%x ",ack_bytes[i]);
			printf("\r\n");
		}

		//flush any remaining bytes
		do_rxflush:
		StrobeCmd(CMD_RX);
		StrobeCmd(CMD_RFR);
		A7190_set_state(IDLE);
	}	//end receive interrupt (FIFOP)

	// Transmission of a packet is finished. Enabling reception of ACK if required.
	if (A7190_read_state() == BUSY_TX) {
		//Finished TX, do call back
		A7190_set_state(IDLE);
		StrobeCmd( CMD_RX);
		StrobeCmd(CMD_RFR);
	}
	if (A7190_read_state() == WAIT_TX) {
		//
	}

}

