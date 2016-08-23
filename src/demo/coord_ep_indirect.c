/*
  V0.1 Initial Release   10/July/2006  RBR

*/



/*
This tests indirect packets with just two nodes.
This tests the special case of an endpoint on
the coordinator sending an indirect packet, which
must be injected into the stack as if received
remotely.

Expects coordinator, one RFD.
The topology to test should be

Coordinator ->  RFD1

Start the coordinator first, then the RFD.


For this to work, the bindings in staticbind.h must
be correct. This test assumes that the LSBs of the
long addresses of the coordinator and RFD are non-zero, and
are not equal to each other, and uses these as the
endpoints for the indirect messages.

Also, if using Win32, you need to be running the virtual board
interface for the coordinator and the RFD, as you must
press/release SW1 to send a packet.

*/

#include "msstate_lrwpan.h"
#include "delay.h"


BYTE myLongAddress[8];

BYTE test_number;



LADDR_UNION dstADDR;
UINT32 my_timer;

BYTE payload[32];  //buffer for payload

static void print_test(void);
void getpayload(void);

void print_test(void){
	conPrintROMString("Test "); conPrintUINT8((UINT8) test_number);
	conPrintROMString(", Sending msg: ");
	conPrintString((char *)&payload[0]);
	conPCRLF();
}

#define NUM_MESSAGES 8
//convoluted mess because of different ways compilers treat const char strings
void getpayload(void) {
	BYTE *dst;
	ROMCHAR *src;
	BYTE msgnum;

	static ROMCHAR _x0_[] = "Hello!";
	static ROMCHAR _x1_[] = "Goodbye?";
	static ROMCHAR _x2_[] = "WTH?";
	static ROMCHAR _x3_[] = "Ouchies!";
	static ROMCHAR _x4_[] = "Shazbot!";
	static ROMCHAR _x5_[] = "FerSur!";
	static ROMCHAR _x6_[] = "NoSir!";
	static ROMCHAR _x7_[] = "Wazzup?";

	msgnum = halGetRandomByte();
	msgnum = msgnum % NUM_MESSAGES;

	switch (msgnum) {
	 case 0 :  src = &_x0_[0]; break;
	 case 1 :  src = &_x1_[0]; break;
	 case 2 :  src = &_x2_[0]; break;
	 case 3 :  src = &_x3_[0]; break;
	 case 4 :  src = &_x4_[0]; break;
	 case 5 :  src = &_x5_[0]; break;
	 case 6 :  src = &_x6_[0]; break;
	 default :  src = &_x7_[0]; break;
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
		  getpayload();
		  print_test();
		  conPrintROMString("Indirect MSG, no ack requested, SrcEP:  ");
		  conPrintUINT8(myLongAddress[0]);
		  conPrintROMString("Cluster: ");
		  conPrintUINT8(LRWPAN_APP_CLUSTER);
		  conPCRLF();
		  conPrintROMString("Press SW1 to send packet\n");
		  DelayMs(1000);

		  //comStrlen defined in compiler.h, this is compiler dependent
		  aplSendMSG (APS_DSTMODE_NONE,
			  NULL,
			  0,
			  LRWPAN_APP_CLUSTER,
			  myLongAddress[0],
			  payload,
			  strlen((char *)payload)+1,
			  apsGenTSN(),
			  FALSE);  //No APS ack requested
		  test_number++;
		  break;
	  case 1:
		  getpayload();
		  print_test();
		  conPrintROMString("Indirect MSG, APS ack requested, SrcEP:  ");
		  conPrintUINT8(myLongAddress[0]);
		  conPrintROMString("Cluster: ");
		  conPrintUINT8(LRWPAN_APP_CLUSTER);
		  conPCRLF();
		  conPrintROMString("Press SW1 to send packet\n");
		  DelayMs(1000);

		  dstADDR.saddr = 0; //Coordinator has address 0
		  aplSendMSG (APS_DSTMODE_NONE,
			  NULL, //no destination address for indirect messages
			  0, //dst EP
			  LRWPAN_APP_CLUSTER, //cluster is ignored for direct message
			  myLongAddress[0], //src EP
			  payload,
			  strlen((char *)payload)+1,
			  apsGenTSN(),
			  TRUE);  //APS ack requested
		  test_number = 0;
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
		conPrintROMString("MSG send FAILED!\n");
	}


}

void main (void){


	//this initialization set our SADDR to 0xFFFF,
	//PANID to the default PANID

	halInit();
	aplInit();  //init the stack
	conPrintConfig();
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts
	test_number = 0;

	//debug_level = 10;

	//get this for reference, will use the LSB as srcEP for indirect message
	halGetProcessorIEEEAddress(&myLongAddress[0]);

#ifdef LRWPAN_COORDINATOR

	aplFormNetwork();
	while(apsBusy()) {apsFSM();} //wait for finish
	conPrintROMString("Nwk formed\n");


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
#endif


#ifdef LRWPAN_RFD
	//announce ourselves to the coordinator so that we can test indirect messaging
	//this is only necessary if there are routers between us and the coordinator,
	//but since don't know the network topology, do it always if RFD.
	do {
		aplSendEndDeviceAnnounce(0);  //send to coordinator as it resolves bindings
		while(apsBusy()) {apsFSM();} //wait for finish
		if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
			conPrintROMString("End Device Announce succeeded!\n");
			break;
		}
		else {
			conPrintROMString("End Device Announce FAILED! Waiting, then trying again\n");
			my_timer= halGetMACTimer();
			//wait for 2 seconds
			while ((halMACTimerNowDelta(my_timer))< MSECS_TO_MACTICKS(2*1000));
		}
	}while(1);
#endif

#if defined(LRWPAN_RFD) || defined(LRWPAN_COORDINATOR)
	//now send packets
	while (1) {
		packet_test();
		while(apsBusy()) {apsFSM();} //wait for finish
	}
#endif


#ifdef LRWPAN_ROUTER
	//router does nothing, just routes
	conPrintROMString("Router, doing its thing.!\n");
	while(1) {apsFSM();}
#endif


}

//########## Callbacks ##########

//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
LRWPAN_STATUS_ENUM usrZepRxCallback(void){

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
	conPCRLF();
	return LRWPAN_STATUS_SUCCESS;
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo){

	return TRUE;

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

}

#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void ) {}
#endif


