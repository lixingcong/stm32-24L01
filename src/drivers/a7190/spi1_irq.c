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
	//used by spp_rf_IRQ
	unsigned char flen;
	unsigned char *ptr, *rx_frame;
	// BYTE crc;
	//define alternate names for readability in this function
#define  fcflsb  ack_bytes[0]
#define  fcfmsb  ack_bytes[1]
#define  dstmode ack_bytes[2]
#define  srcmode ack_bytes[3]
	// complete frame has arrived
	if (A7190_read_state() == IDLE) {
#if 0
#ifdef LRWPAN_COORDINATOR
		if(dynamic_freq_mode!=0xff)//跳频模式忽略一切收到的数据包
			goto do_rxflush;
#endif
#endif
		//if last packet has not been processed, we just
		//read it but ignore it.
		A7190_set_state(BUSY_RX);
		ptr = NULL;  //temporary pointer
		flen = ReadFIFO1(1);  //read the length
#ifdef MAC_OUTPUT_DEBUG
		printf("recv len=%u\r\n",flen);
#endif

		do_rx:
		if (flen == LRWPAN_ACKFRAME_LENGTH) {
#if 0
			if(isOffline==TRUE)
				goto do_rxflush;
#ifdef MAC_OUTPUT_DEBUG
			printf("TTTHHHIISS is a ACK frame\r\n");
#endif
			//this should be an ACK.
			//read the packet, do not allocate space for it
			DEBUG_CHAR(DBG_ITRACE, DBG_CHAR_ACKPKT);
			ack_bytes[0] = flen;
			ReadFIFO(&ack_bytes[1], LRWPAN_ACKFRAME_LENGTH);
			// crc = A7190_ReadReg(MODE_REG);
			// crc = RFD;
			// TODO: check CRC
			if (1) {      //CRC check, should be implemented well later
				// CRC ok, perform callback if this is an ACK
				macRxCallback(ack_bytes, A7190_ReadRSSI());
			}
#endif

		}else if(flen==LRWPAN_PINGFRAME_LENGTH){
#if 0
			if(isOffline==TRUE)
				goto do_rxflush;
#ifdef MAC_OUTPUT_DEBUG
			printf("THISSSSS PING  FRAMe!!!\r\n");
#endif
			ReadFIFO(&ack_bytes[0],LRWPAN_PINGFRAME_LENGTH);
			A7190_set_state(IDLE);
			macRxPingCallback(ack_bytes);
#endif
		}else if(flen==1){ // my custom packet format
#if 0
			if(isOffline==TRUE)
				goto do_rxflush;
#ifdef MAC_OUTPUT_DEBUG
			printf("custom flen=1  FFFRRRAAAMMMEEE!\r\n");
#endif
			ack_bytes[0] = ReadFIFO1(1); // the real data flen
			ReadFIFO(&ack_bytes[1], ack_bytes[0]);
//			for(flen=0;flen<ack_bytes[0];flen++)
//				printf("%x ",ack_bytes[flen]);
//			puts("");
			macRxCustomPacketCallback(ack_bytes);
#endif
		}else {
#if 0
			//not an ack packet, lets do some more early rejection
			// that the CC2430 seems to not do that we want to do.
			//read the fcflsb, fcfmsb
			ReadFIFO(ack_bytes, 2);
			if(flen==0 && ack_bytes[0]==0 && ack_bytes[1]==0) // dummy blocks for dynamic freq test, drop
				goto do_rxflush;
			if (!local_radio_flags.bits.listen_mode) {
				//only reject if not in listen mode
				//get the src, dst addressing modes
				srcmode= LRWPAN_GET_SRC_ADDR(fcfmsb);
				dstmode = LRWPAN_GET_DST_ADDR(fcfmsb);
				if ((srcmode == LRWPAN_ADDRMODE_NOADDR) && (dstmode == LRWPAN_ADDRMODE_NOADDR)) {
					//reject this packet, no addressing info
					goto do_rxflush;
				}
			}

			if (!macRxBuffFull()) {
				//MAC TX buffer has room
				//allocate new memory space
				//read the length
				rx_frame = MemAlloc(flen + 1);
				ptr = rx_frame;
			} else {
				//MAC RX buffer is full
				DEBUG_CHAR(DBG_ITRACE, DBG_CHAR_MACFULL);
			}

			// at this point, if ptr is null, then either
			// the MAC RX buffer is full or there is  no
			// free memory for the new frame, or the packet is
			// going to be rejected because of addressing info.
			// In these cases, we need to
			// throw the RX packet away
			if (ptr == NULL) {
				//just flush the bytes
				goto do_rxflush;
			} else {
				//save packet, including the length
				*ptr = flen;
				ptr++;
				//save the fcflsb, fcfmsb bytes
				*ptr = fcflsb;
				ptr++;
				flen--;
				*ptr = fcfmsb;
				ptr++;
				flen--;
				//get the rest of the bytes
				ReadFIFO(ptr, flen);
				//do RX callback
				//check the CRC
				// conPrintStringLength((rx_frame+1), *rx_frame);
				// crc = A7190_ReadReg(MODE_REG);
				if (1) {  //CRC?
					//CRC good
					//change the RSSI byte from 2's complement to unsigned number
					phyRxCallback();
					macRxCallback(rx_frame, A7190_ReadRSSI());
				} else {
					// CRC bad. Free the packet
					MemFree(rx_frame);
				}
			}
#endif
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
		A7190_WriteReg( FIFO1_REG, 0xff);
		StrobeCmd( CMD_RX);
		StrobeCmd(CMD_RFR);
	}
	if (A7190_read_state() == WAIT_TX) {
		//
	}

#undef  fcflsb
#undef  fcfmsb
#undef  dstmode
#undef  srcmode
}

