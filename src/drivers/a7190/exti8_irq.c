#include "hal.h"
#include "exti8_irq.h"
#include "A7190.h"
#include "A7190reg.h"
#include "SPI1.h"
#include "route_table.h"

extern unsigned char dynamic_freq_mode;
unsigned char recv_buffer_a7190[LRWPAN_MAX_FRAME_SIZE]; // max len

// 外部中断PA8服务函数，当a7190有发送（接收）活动时调用
//This interrupt used for both TX and RX
void EXTI8_irq_a7190(void) {
	unsigned char flen,i;
	unsigned short total_flen;
	if (A7190_read_state() == IDLE) {
#ifdef LRWPAN_COORDINATOR
		if(dynamic_freq_mode!=0xff)//跳频模式忽略一切收到的数据包
			goto do_rxflush;
#endif

		A7190_set_state(BUSY_RX);
		flen = ReadFIFO1(1);  //read the length

		recv_buffer_a7190[0]=flen;
		recv_buffer_a7190[1]=ReadFIFO1(1);

		do_rx:
		if ((flen&0xfe)==0xf0) { // long
			total_flen=((flen&0x01)<<8)|recv_buffer_a7190[1];
			ReadFIFO(&recv_buffer_a7190[2],total_flen-2);
			macRxCustomPacketCallback(recv_buffer_a7190,FALSE,total_flen);
		}else if((flen&0xff)==0x00){ // short
			ReadFIFO(&recv_buffer_a7190[2],recv_buffer_a7190[1]);
#if 0
			printf("%x %x ",recv_buffer_a7190[0],recv_buffer_a7190[1]);
			for(i=2;i<recv_buffer_a7190[1];++i)
				printf("%x ",recv_buffer_a7190[i]);
			printf("\r\n");
#endif
			macRxCustomPacketCallback(recv_buffer_a7190,TRUE,recv_buffer_a7190[1]);
		}else{
			goto do_rxflush; // drop invalid packet
		}

		//flush any remaining bytes
		do_rxflush:
		StrobeCmd(CMD_RX);
		StrobeCmd(CMD_RFR);
		Set_FIFO_len(0xff, 0x01); // 复位FIFO指针
		A7190_set_state(IDLE);
	}	//end receive interrupt (FIFOP)

	// Transmission of a packet is finished. Enabling reception of ACK if required.
	if (A7190_read_state() == BUSY_TX) {
		//Finished TX, do call back
		A7190_set_state(IDLE);
		Set_FIFO_len(0xff, 0x01); // 复位FIFO指针
		StrobeCmd( CMD_RX);
		StrobeCmd(CMD_RFR);
	}
	if (A7190_read_state() == WAIT_TX) {
		//
	}

}

