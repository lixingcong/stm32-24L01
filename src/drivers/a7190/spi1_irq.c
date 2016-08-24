#include "hal.h"
#include "spi1_irq.h"
#include "A7190.h"
#include "A7190reg.h"
#include "SPI1.h"
#include "route_table.h"
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

		ack_bytes[0]=flen;
		do_rx:
		if ((flen&0xf0)==0xf0) { // long
			// TODO: 处理超长包（干扰包），限制flen为512 2016年8月24日 上午10:03:49
		}else if((flen&0xf0)==0x00){ // short
			ack_bytes[1]=ReadFIFO1(1);

			ReadFIFO(&ack_bytes[2],ack_bytes[1]);
			printf("%x %x ",ack_bytes[0],ack_bytes[1]);
			for(i=2;i<ack_bytes[1];++i)
				printf("%x ",ack_bytes[i]);
			printf("\r\n");
			macRxCustomPacketCallback(ack_bytes);
		}else{
			goto do_rxflush; // drop invalid packet
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
		Set_FIFO_len(0xff, 0x01); // 这句话很重要，复位fifo指针
		StrobeCmd( CMD_RX);
		StrobeCmd(CMD_RFR);
	}
	if (A7190_read_state() == WAIT_TX) {
		//
	}

}

