/*
	2016-7-78
	base on ping_pong_rejoin.c
	Author: li
	test upload for Dr.Huang check
	interact with PC
*/

#include "msstate_lrwpan.h"
#include "SPI1.h"
#include "delay.h"
#include "usart.h"
#include "parse_control_cmd.h"
#include "send_control_cmd.h"
#include "route_table.h"

#ifndef LRWPAN_COORDINATOR
#define PING_DELAY   1  //wait before bouncing back
#else
#define PING_DELAY   0 //coordinator does not wait
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
void printJoinInfo(void);

UINT8 pingRxTimeouts;   //successive RX timeouts
UINT8 pingTxFailures;   //successive TX failures

#define MAX_RX_TIMEOUTS 5
#define MAX_TX_FAILURES 5


void PingPong (void ) {
	static unsigned int phy_last_tx_time;
	unsigned char node_num;
	//exits on excessive RX or TX failures
	while(1) {
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
					if (!rxFlag) {
						//increment total timeouts
						numTimeouts++;     //got tired of waiting for a response, send again
						pingRxTimeouts++;
						if (pingRxTimeouts > MAX_RX_TIMEOUTS) {
							//too many successive timeouts, exit for rejoin
							return;
						}
					} else {
						pingRxTimeouts = 0;  //reset successive timeouts
					}
					rxFlag = 0; //clear flag
					//start timer
					my_timer= halGetMACTimer();
					pingTxFailures = 0;
					ppState = PP_STATE_SEND;

				}
				break;

			case PP_STATE_SEND:
				if ((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(PING_DELAY*1000)){
					//MemDump();
					ppState = PP_STATE_WAIT_FOR_TX;

					// todo: payload
#ifdef LRWPAN_ROUTER
					for(node_num=0;node_num<=ALL_NODES_NUM;node_num++)
						all_nodes[node_num]=0xff;
					all_nodes[IEEE_ADDRESS_ARRAY&0xff]=aplGetParentLongAddress()->bytes[0];

					last_tx_start = halGetMACTimer();
					aplSendMSG (APS_DSTMODE_SHORT,
						&dstADDR,
						2, //dst EP
						0, //cluster is ignored for direct message
						1, //src EP
						&all_nodes[0],
						ALL_NODES_NUM+1,  //msg length
						apsGenTSN(),
						FALSE);  //No APS ack requested
#endif
				}
				break;

			case PP_STATE_WAIT_FOR_TX:
				if (apsBusy()) break; //status not ready yet if busy.
				if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
					ppState = PP_STATE_START_RX;
				}else {
					pingTxFailures++;
					if (pingTxFailures > MAX_TX_FAILURES) {
						//the only reason this should happen is because of our Radio failure
						//or our parent is not returning MAC acks.
						return;
					}
					printf("Ping Send failed! Restarting timer to try again, code:%d\r\n",aplGetStatus());
					my_timer= halGetMACTimer();
					ppState = PP_STATE_SEND;
				}
				break;
		}
	}
}

void printJoinInfo(void){

	conPrintROMString("My ShortAddress is: ");
	conPrintUINT16(aplGetMyShortAddress());
	conPCRLF();
	conPrintROMString("Parent LADDR: ");
	conPrintLADDR(aplGetParentLongAddress());
	conPrintROMString(", Parent SADDR: ");
	conPrintUINT16(aplGetParentShortAddress());
	conPCRLF();
}

typedef enum _JOIN_STATE_ENUM {
	JOIN_STATE_START_JOIN,
	JOIN_STATE_WAIT_FOR_JOIN,
	JOIN_STATE_RUN_APP1,
	JOIN_STATE_RUN_APP2,
	JOIN_STATE_RUN_APP3,
	JOIN_STATE_START_REJOIN,
	JOIN_STATE_WAIT_FOR_REJOIN,
}JOIN_STATE_ENUM;

JOIN_STATE_ENUM joinState;

#define MAX_REJOIN_FAILURES 3

void main (void){

	UINT8 failures,i;

	//this initialization set our SADDR to 0xFFFF,
	//PANID to the default PANID

	//HalInit, evbInit will have to be called by the user

	numTimeouts = 0;
	my_timer = 0;
	first_packet = TRUE;
	halInit();
	aplInit();  //init the stack
	conPrintConfig();
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts

	joinState = JOIN_STATE_START_JOIN;  //main while(1) initial state

	ping_cnt = 0;
	rxFlag = 0;    //set to '1' when a packet is received.
	// debug_level = 10;

	for(i=0;i<ALL_NODES_NUM+1;i++){
		all_nodes[i]=0xff;
	}
	//This version of PingPong has Network Rejoin logic in case of excessive failures
	//by the PingPong() function during transmit.
	while (1) {
		apsFSM();
		switch (joinState) {

		  case JOIN_STATE_START_JOIN:
#ifdef LRWPAN_COORDINATOR
			  aplFormNetwork();
#else
			  aplJoinNetwork();
#endif
			  joinState = JOIN_STATE_WAIT_FOR_JOIN;
			  break;
		  case JOIN_STATE_WAIT_FOR_JOIN:
			  if (apsBusy()) break;   //if stack is busy, continue
#ifdef LRWPAN_COORDINATOR
			  if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
				  conPrintROMString("Network formed, waiting for RX\n");
				  joinState = JOIN_STATE_RUN_APP1;
				  upload_self_check_status(TRUE);
			  } else {
				  //only reason for this to fail is some sort of hardware failure
				  conPrintROMString("Network formation failed, waiting, trying again\n");
				  upload_self_check_status(FALSE);
				  my_timer= halGetMACTimer();
				  //wait for 2 seconds
				  while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
				  joinState = JOIN_STATE_START_JOIN;
			  }
#else
			  if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
				  conPrintROMString("Network Join succeeded!\r\n");
				  printJoinInfo();
				  joinState = JOIN_STATE_RUN_APP1;
			  } else {
				  conPrintROMString("Network Join FAILED! Waiting, then trying again\r\n");
				  my_timer= halGetMACTimer();
				  //wait for 2 seconds
				  while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
				  joinState = JOIN_STATE_START_JOIN;
			  }
#endif

			  break;

		  case JOIN_STATE_RUN_APP1:
#ifdef LRWPAN_COORDINATOR
			  //WARNING - this is only for latency testing, this variable is normally
			  //set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
			  //that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
			  //only do this in your normal code if you want to disable automatic retries
			  aplSetMacMaxFrameRetries(0);
			  //coordinator waits for RX first
			  ppState = PP_STATE_START_RX;
			  pingRxTimeouts = 0;          //used by PingPong to track RX successive failures
			  pingTxFailures = 0;          //used by PingPont to track TX successive failures.
			  first_packet = TRUE;
			  PingPong(); //only exits on if excessive misses
			  //reset first_packet to flag to true, this causes Coord to just
			  //wait for a packet, and not try any retransmit on misses.
#else

			  conPrintROMString("Hit any switch to start!\r\n");
			  joinState = JOIN_STATE_RUN_APP2;
#endif
			  break;
		  case JOIN_STATE_RUN_APP2:
			  DelayMs(500);
		  case JOIN_STATE_RUN_APP3:
			  //WARNING - this is only for latency testing, this variable is normally
			  //set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
			  //that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
			  //only do this in your normal code if you want to disable automatic retries
			  aplSetMacMaxFrameRetries(0);
			  //switch is pressed, run app
			  dstADDR.saddr = 0; //RFD sends to the coordinator
			  ppState = PP_STATE_SEND;
			  pingRxTimeouts = 0;          //used by PingPong to track RX successive failures
			  pingTxFailures = 0;          //used by PingPont to track TX successive failures.
			  my_timer= halGetMACTimer();  //timer must be initialized before entering PP_SEND_STATE
			  PingPong();//only exits on if excessive misses
			  //try rejoining network
			  failures = 0;
			  joinState = JOIN_STATE_START_REJOIN;
			  break;
			  //rejoin states only executed by RFD
#ifndef LRWPAN_COORDINATOR
		  case JOIN_STATE_START_REJOIN:
			  conPrintROMString("Trying to rejoin network!\r\n");
			  aplRejoinNetwork();
			  joinState = JOIN_STATE_WAIT_FOR_REJOIN;
			  break;

		  case JOIN_STATE_WAIT_FOR_REJOIN:
			  if (apsBusy()) break;   //if stack is busy, continue
			  if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
				  failures = 0;
				  conPrintROMString("Network Rejoin succeeded!\r\n");
				  printJoinInfo();
				  joinState = JOIN_STATE_RUN_APP3;  //don't wait for button press
			  } else {
				  failures++;
				  if (failures == MAX_REJOIN_FAILURES) {
					  //this starts everything over
					  conPrintROMString("Max Rejoins failed, trying to join.\r\n");
					  joinState = JOIN_STATE_START_JOIN;
				  } else {
					  //else, wait to try again
					  conPrintROMString("Network Rejoin FAILED! Waiting, then trying again\n");
					  my_timer= halGetMACTimer();
					  //wait for 2 seconds
					  while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
					  joinState = JOIN_STATE_START_REJOIN;
				  }
			  }
			  break;
#endif

		  default:
			  joinState = JOIN_STATE_START_JOIN;

		}

	}
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

LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {
	BYTE *ptr,i;
	printf("data recv\r\n");
#ifdef LRWPAN_COORDINATOR
	ptr=aplGetRxMsgData();
	for(i=0;i<ALL_NODES_NUM+1;i++){
		if((*ptr)!=0xff){
			all_nodes[i]=*ptr;
			printf("updated: node #%u 's parent is #%u\r\n",i,(*ptr));
		}
		ptr++;
	}
	printf("now all nodes:\r\n");
	for(i=0;i<ALL_NODES_NUM+1;i++){
		printf("node #%u 's parent is #%u\r\n",i,all_nodes[i]);
	}
#endif
	rxFlag=1;
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
	conPrintROMString("Accepting router\n");
	return TRUE;
}else {
	conPrintROMString("Rejecting non-router\n");
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
