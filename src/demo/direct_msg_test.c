/*
  V0.1 Initial Release   10/July/2006  RBR

*/



/*
This is a router test, using direct messages from
RFD to coordinator through router.

Expects coordinator, one router, at least one RFD.
The topology to test should be:

Coordinator ->  router -> RFD1
-> RFD2 (optional)


Start the coordinator first, then the router, then
RFD1. If a RFD1 fails to join the network, try
again. The usrJoinVerifyCallback() is written such
that the Coordinator will reject the RFDs, so that
the RFDs should join the router. Can add more than
one RFD.

YOU MUST EDIT the coord[] long address and put
in the long address of your coordinator so the
test that uses long addressing will work correctly.

The usrJoinVerify function at the coordinator level
rejects any node that is not a router, so this forces
routers to join the coordinator, and RFDs to join the
router.

This does not require any Virtual Boards to be running,
everything done through the console.

For WIN32, the Router project also has LRWPAN_RSSI defined
as a stronger value than the default (see util_get_rssi()
in halStack.c), so that packets
from the router appear to be closer in physical distance
than from other nodes, since RFDs use the RSSI value to
choose a node to respond to when they get multiple responses
from a beacon request.


In WIN32, start the coord first, then the router so that it
joins the coord, then RFDs so that they join the router.

*/

#include "msstate_lrwpan.h"
#include "hal.h"

BYTE payload[32];  //buffer for payload
BYTE test_number;
LADDR_UNION dstADDR;
UINT32 my_timer;

void getpayload(int msgNum);
void set_coord_LADDR();

void print_test(void){
    conPrintROMString("Test ");
    conPrintUINT8((UINT8) test_number);
     conPrintROMString(", Sending msg: ");
     conPrintString((char *)&payload[0]);
	conPCRLF();
}


//this is assumed to be the long address of our coordinator, in little endian order
//used to test LONG ADDRESSING back to coordinator
static BYTE coord[8];

static ROMCHAR _x0_[] = "TEST_1";
static ROMCHAR _x1_[] = "test_2";
static ROMCHAR _x2_[] = "TTTEEESSSTTT_3";

//convoluted mess because of different ways compilers treat const char strings
void getpayload(int msgNum) {
   BYTE *dst;
   ROMCHAR *src;
	
   switch (msgNum) {
     case 0 :  src = &_x0_[0]; break;
     case 1 :  src = &_x1_[0]; break;
     case 2 :  src = &_x2_[0]; break;
     default:  src = &_x0_[0]; break;
   }
    dst = &payload[0];
    while (*src) {
        *dst = *src;
         dst++;src++;
    }
   *dst = *src;
}




void packet_test(void) {

	switch(test_number) {
	  case 0:
		  getpayload(0);
		  conPrintROMString("Direct MSG to Coord, SHORT addr, dstEP: 1, no ack requested\n");
		  dstADDR.saddr = 0; //Coordinator has address 0
		  aplSendMSG (APS_DSTMODE_SHORT,
			  &dstADDR,
			  2, //dst EP
			  0, //cluster is ignored for direct message
			  1, //src EP
			  payload,
			  strlen((char *)payload)+1,
			  apsGenTSN(),
			  FALSE);  //No APS ack requested
		  conPrintROMString("wait for a while\n");
		  DelayMs(2500);
		  test_number++;
		  break;
	  case 1:
		  getpayload(1);
		  conPrintROMString("Direct MSG to Coord, SHORT addr,  dstEP: 1, APS ack requested\n");
		  dstADDR.saddr = 0; //Coordinator has address 0
		  aplSendMSG (APS_DSTMODE_SHORT,
			  &dstADDR,
			  2, //dst EP
			  0, //cluster is ignored direct message
			  1, //src EP
			  payload,
			  strlen((char *)payload)+1,
			  apsGenTSN(),
			  TRUE);    //request an ACK
		  test_number++;
		  conPrintROMString("wait for a while\n");
		  DelayMs(2500);
		  break;
	  case 2:
		  getpayload(2);
		  conPrintROMString("Direct MSG to Coord, LONG address (bypasses routing), dstEP: 1 \n");
		  //copy the long address
		  halUtilMemCopy(&dstADDR.laddr.bytes[0], &coord[0], 8);
		  aplSendMSG (APS_DSTMODE_LONG,
			  &dstADDR,
			  2, //dst EP
			  0, //cluster is ignored direct message
			  1, //src EP
			  payload,
			  strlen((char *)payload)+1,
			  apsGenTSN(),
			  FALSE); //ack request ignore for long addressing
		  conPrintROMString("wait for a while\n");
		  DelayMs(2500);
		  test_number++;
		  break;
	  default:
		  test_number = 0;
		  break;
	}


	//block, and see if message sent successfully
	while(apsBusy()) {apsFSM();}
	if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
		conPrintROMString("MSG send succeeded!\n");
	}else {
		printf("MSG send FAILED! error code:%u\n",aplGetStatus());
	}


}

void main (void){
	halInit();
	aplInit();  //init the stack
	conPrintConfig();
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts
	test_number = 0;

	set_coord_LADDR();

	//debug_level = 10;


#ifdef LRWPAN_COORDINATOR

	aplFormNetwork();
	while(apsBusy()) {apsFSM();} //wait for finish

	conPrintROMString("Nwk formed, waiting for join and reception\n");
	while(1) {apsFSM();}

#else
	do {
		aplJoinNetwork();
		while(apsBusy()) {apsFSM();} //wait for finish
		if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
			conPrintROMString("Network Join succeeded!\n");
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
			conPrintROMString("Network Join FAILED! Waiting, then trying again\n");
                        my_timer= halGetMACTimer();
                       //wait for 2 seconds
                      while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
		       }
	} while(1);


#ifdef LRWPAN_RFD
	//now send packets
	while (1) {
		packet_test();
		print_test();
		while(apsBusy()) {apsFSM();} //wait for finish
	}
#endif
#ifdef LRWPAN_ROUTER
	//router does nothing, just routes
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	conPrintROMString("Router, doing its thing.!\n");
	while(1) {apsFSM();}
#endif

#endif


}

//########## Callbacks ##########

//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
LRWPAN_STATUS_ENUM usrZepRxCallback(void){

#if 1     //change this to '0' if you want to test without a router
#ifdef LRWPAN_COORDINATOR
	if (aplGetRxCluster() == ZEP_END_DEVICE_ANNOUNCE) {
		//a new end device has announced itself, print out the
		//the neightbor table and address map
		dbgPrintNeighborTable();
	}
#endif
#endif
  return LRWPAN_STATUS_SUCCESS;
}

//callback from APS when packet is received
//user must do something with data as it is freed
//within the stack upon return.

LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {

	BYTE len, *ptr;

	//just print out this data

	conPrintROMString("User Data Packet Received: \n");
	conPrintROMString("SrcSADDR: ");
	conPrintUINT16(aplGetRxSrcSADDR());
	conPrintROMString(", DstEp: ");
	conPrintUINT8(aplGetRxDstEp());
	conPrintROMString(", Cluster: ");
	conPrintUINT8(aplGetRxCluster());
	conPrintROMString(", Msg Length: ");
	len = aplGetRxMsgLen();
	conPrintUINT8(len);
	conPCRLF();
	conPrintROMString("Msg: ");
	ptr = aplGetRxMsgData();
	while(len){
		halPutch(*ptr);
		ptr++; len--;
	}
        return LRWPAN_STATUS_SUCCESS;
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo){

#ifdef LRWPAN_COORDINATOR
	//only let routers join us
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


void usrIntCallback(void){
#ifdef LRWPAN_ROUTER
	static unsigned int cnt_router_recv=0;
	printf("recv a packet. #%d\r\n",cnt_router_recv++);
#endif
}

//called when the slow timer interrupt occurs
#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void ) {}
#endif

// set coordinator's LADDE in hal.h
void set_coord_LADDR(){
	int i=7;
	while(i>=0){
		coord[i]=(BYTE)(IEEE_ADDRESS_ARRAY_COORD>>(i*8)&0xff);
		--i;
	}
}
