/*
  V0.1 Initial Release   10/July/2006  RBR

*/



/*
This is a two node test, requires a Coordinator
and an RFD. The coordinator and node simply
ping-pong a packet back and forth, and print
out the RSSI byte.  The RFD waits before
bouncing it back, while the coordinator responds
immediately.

Expects coordinator, and one RFD.
The topology to test should be:

Coordinator ->  RFD1


Start the coordinator first, then
RFD1. If a RFD1 fails to join the network, try
again. The RFD1 will prompt the user to hit
a key to start the ping-pong.

You can connect multiple RFDs if desired.

You can also ping-pong through a router; see
the note in usrJoinVerifyCallback(). The topology
for a router would be:

coord -> router -> RFD1
-> RFD2
-> ..RFDn


This  requires Virtual Boards to be running,
since a switch press is needed to start the pinging.


*/

#include "msstate_lrwpan.h"
#include "SPI1.h"

#ifndef LRWPAN_COORDINATOR
#define PING_DELAY   2  //wait before bouncing back
#else
#define PING_DELAY   0 // RFD does not wait
#endif

#define RX_PING_TIMEOUT     5    //seconds
//this is assumed to be the long address of our coordinator, in little endian order
//used to test LONG ADDRESSING back to coordinator

UINT16 ping_cnt;
UINT32 my_timer;
UINT32  last_tx_start;


LADDR_UNION dstADDR;

typedef enum _PP_STATE_ENUM {
	PP_STATE_START_RX,
	PP_STATE_WAIT_FOR_RX,
	PP_STATE_SEND,
	PP_STATE_WAIT_FOR_TX
}PP_STATE_ENUM;

PP_STATE_ENUM ppState;
BYTE rxFlag;              //set from within usrRxPacketCallback
BYTE payload[2];
UINT16 numTimeouts;
BOOL first_packet;

void  PingPong(void);

void PingPong (void ) {

	apsFSM();

	switch (ppState) {

			case  PP_STATE_START_RX:
				if (!first_packet) {
					my_timer= halGetMACTimer();
					ppState = PP_STATE_WAIT_FOR_RX;
				}else if (rxFlag) {
					//on first packet, do not start timer, just wait for a packet.
					ppState = PP_STATE_WAIT_FOR_RX;
					first_packet = FALSE;
				}
				break;

			case PP_STATE_WAIT_FOR_RX:
				//rxFlag is set from within usrRxPacketCallback
				if (rxFlag || halMACTimerNowDelta(my_timer) > MSECS_TO_MACTICKS( RX_PING_TIMEOUT *1000 )) {
					if (!rxFlag) numTimeouts++;     //got tired of waiting for a response, send again
					rxFlag = 0; //clear flag
					//start timer
					my_timer= halGetMACTimer();
					ppState = PP_STATE_SEND;

				}
				break;    

			case PP_STATE_SEND:
				if ((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(PING_DELAY*1000)){
					MemDump();

					//increment ping counter
					ping_cnt++; //this was value received by this node
					//received packet, ping it back
					//format the packet
					payload[0] = (BYTE) ping_cnt;
					payload[1] =  (BYTE) (ping_cnt>>8);
					ppState = PP_STATE_WAIT_FOR_TX;
					last_tx_start = halGetMACTimer();

					aplSendMSG (APS_DSTMODE_SHORT,
						&dstADDR,
						2, //dst EP
						0, //cluster is ignored for direct message
						1, //src EP
						&payload[0],
						2,  //msg length
						apsGenTSN(),
						FALSE);  //No APS ack requested
					ppState = PP_STATE_WAIT_FOR_TX;
				}
				break;

			case PP_STATE_WAIT_FOR_TX:
				if (apsBusy()) break; //status not ready yet if busy.
				if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
					ppState = PP_STATE_START_RX;
					//compute the latency of this TX send operation
					//aplGetLastTxTime gets the time that the LAST tx operation finished.
					//this will be the latency of the TX stack operation only if no mac retries were required
					/*
					last_tx_start = aplMacTicksToUs(aplGetLastTxTime() - last_tx_start);
					conPrintROMString("TX Stack latency(us): ");
					conPrintUINT32(last_tx_start);
					conPCRLF();
					*/
					printf("success!\r\n\r\n\r\n");
				}else {
					conPrintROMString("Ping Send failed! Restarting timer to try again\r\n");
					printf("ping send error code is %d\r\n",aplGetStatus());
					my_timer= halGetMACTimer();
					ppState = PP_STATE_SEND;
				}
				break;
	}
}



void main (void){


	//this initialization set our SADDR to 0xFFFF,
	//PANID to the default PANID

	//HalInit

	numTimeouts = 0;
	my_timer = 0;
	first_packet = TRUE;
	halInit();
	aplInit();  //init the stack
	conPrintConfig();
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts

	ping_cnt = 0;
	rxFlag = 0;
	debug_level = 10;


#ifdef LRWPAN_COORDINATOR

	aplFormNetwork();
	while(apsBusy()) {apsFSM();} //wait for finish
	conPrintROMString("Network formed, waiting for RX\r\n");
	ppState = PP_STATE_START_RX;
#else
	do {
		aplJoinNetwork();
		while(apsBusy()) {apsFSM();} //wait for finish
		if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
			conPrintROMString("Network Join succeeded!\r\n");
			conPrintROMString("My ShortAddress is: ");
			conPrintUINT16(aplGetMyShortAddress());
			conPCRLF();
			conPrintROMString("Parent LADDR: ")
				conPrintLADDR(aplGetParentLongAddress());
			conPrintROMString(", Parent SADDR: ");
			conPrintUINT16(aplGetParentShortAddress());
			conPCRLF();
			break;
		}else {
			conPrintROMString("Network Join FAILED! Waiting, then trying again\r\n");
			my_timer= halGetMACTimer();
			//wait for 2 seconds
			while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
		}
	} while(1);

#endif

#ifdef LRWPAN_RFD
	//now send packets
	dstADDR.saddr = 0; //RFD sends to the coordinator
	ppState = PP_STATE_SEND;
	conPrintROMString("Hit any switch to start!\n");
#endif

#if (defined(LRWPAN_RFD) || defined(LRWPAN_COORDINATOR))
	//WARNING - this is only for latency testing, max MAC retries is normally
	//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
	//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
	//only do this in your normal code if you want to disable automatic retries
	aplSetMacMaxFrameRetries(0);

	while (1) {
		PingPong();
	}
#endif


#ifdef LRWPAN_ROUTER
	//router does nothing, just routes
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	conPrintROMString("Router, doing its thing.!\r\n");
	while(1) {apsFSM();}
#endif


}

//########## Callbacks ##########

//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
LRWPAN_STATUS_ENUM  usrZepRxCallback(void){

#ifdef LRWPAN_COORDINATOR
	if (aplGetRxCluster() == ZEP_END_DEVICE_ANNOUNCE) {
		//a new end device has announced itself, print out the
		//the neightbor table and address map
		dbgPrintNeighborTable();
	}
#endif
 return LRWPAN_STATUS_SUCCESS;
}

//callback from APS when packet is received
//user must do something with data as it is freed
//within the stack upon return.

LRWPAN_STATUS_ENUM  usrRxPacketCallback(void) {

	BYTE len, *ptr;

	//just print out this data
	conPrintROMString("\r\nUser Data Packet Received: \r\n");
	/*
	conPrintROMString("SrcSADDR: ");
	conPrintUINT16(aplGetRxSrcSADDR());
	conPrintROMString(", DstEp: ");
	conPrintUINT8(aplGetRxDstEp());
	conPrintROMString(", Cluster: ");
	conPrintUINT8(aplGetRxCluster());
	conPrintROMString(", MsgLen: ");
	*/
	len = aplGetRxMsgLen();
	/*
	conPrintUINT8(len);
	conPrintROMString(",RSSI: ");
	conPrintUINT8(aplGetRxRSSI());
	conPCRLF();
	conPrintROMString("PingCnt: ");
	*/
	ptr = aplGetRxMsgData();
	ping_cnt = *ptr;
	ptr++;
	ping_cnt += ((UINT16)*ptr)<<8;
	/*
	conPrintUINT16(ping_cnt);
	conPrintROMString(", RxTimeouts: ");
	conPrintUINT16(numTimeouts);
	*/
	rxFlag = 1;//signal that we got a packet
	//use this source address as the next destination address
	dstADDR.saddr = aplGetRxSrcSADDR();   
	conPCRLF();
	return LRWPAN_STATUS_SUCCESS;
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo){\

#if 0      //set this to '1' if you want to test through a router
//only accept routers.
//only let routers join us if we are coord
#ifdef LRWPAN_COORDINATOR
if (LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) {
	//this is a router, let it join
	conPrintROMString("Accepting router\r\n");
	return TRUE;
}else {
	conPrintROMString("Rejecting non-router\r\n");
	return FALSE;
}
#else
return TRUE;
#endif

#else

return TRUE;

#endif

}

BOOL usrJoinNotifyCallback(LADDR *ptr){

	//allow anybody to join

	conPrintROMString("Node joined: ");
	conPrintLADDR(ptr);
	conPCRLF();
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	return TRUE;
}
#endif

//called when the slow timer interrupt occurs
#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void ) {}
#endif


//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void){}
