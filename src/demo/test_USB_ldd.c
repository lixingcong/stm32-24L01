/******************************************************************************************************************************************
 *
 *  File Name     : 主函数wxldemo.c
 *  Author   	   : 卢东东
 *  Create Date   : 2016.07.22
 *  Version   	   : 6.0第六版
 *  Description   : 将A7190与自组网协议联系起来，（*2）并且将A7190的数据传递到stm32，再发给手机
 （*1）将手机的数据传输给stm32，再通过A7190发送出去
 * ImportantPoints: 1. 需要定义协调器和发射器。
 *                  2: 路径为coord -> router -> RFD1-> RFD2-> ..RFDn
 *                  3: 必须先开协调器，再开发送器。发送器未加入，再次尝试。
 *                  4：数据传输是用pingpong函数来进行的，pingpong被多次调用
 *                  5：可加入多个发射器，相关内容在usrJoinVerifyCallback()函数中。
 *********************************************************************************************************************************************/

#include "usb_lib.h"
#include "msstate_lrwpan.h"
#include "delay.h"
#include "SPI2.h"
#include "usart.h"
#include "stdint.h"
#include "hal.h"
#include "aps.h"
#include "halStack.h"

#include "hw_config.h"
#include "usbio.h"
#include "delay.h"
#include "app_cfg.h"
#include "usb_pwr.h"
#include "wtp.h"
#include "my_queue.h"
#include "apl_custom_function.h"
#include "execute_PC_cmd.h"

#ifndef LRWPAN_COORDINATOR
#define PING_DELAY   2                                   //wait before bouncing back
#else
#define PING_DELAY   0                                   //coordinator does not wait
#endif
#define RX_PING_TIMEOUT     5                            //seconds
//this is assumed to be the long address of our coordinator, in little endian order
//used to test LONG ADDRESSING back to coordinator
BYTE IEEE_ADDR[8];
UINT16 PAN_ID;
UINT16 SHORTADDR;
BYTE address_filter[3];
BYTE ping_cnt = 0;
UINT32 my_timer;
UINT32 spi_timer;
UINT32 last_tx_start;
UINT8 LcdPage = 2;
BYTE test_data[145] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
LADDR_UNION dstADDR;

void PingPong(void);                                      //pingpong函数的声明
BYTE ep2_data[USB_FROM_PHONE_MAX_LEN];
BYTE ep1_data[USB_FROM_PHONE_MAX_LEN];
BYTE load[40];
uint16_t j = 0;                                             //循环次数
uint8_t ep2_rev_ok, ep1_rev_ok;                            //端点接收完成标志
uint8_t ep2_send_ok, ep1_send_ok;

/**************************************三种切换状态：空闲状态、发送状态、等待发送状态********************************************************/
typedef enum _PP_STATE_ENUM {
	PP_STATE_IDLE, PP_STATE_SEND, PP_STATE_WAIT_FOR_TX
} PP_STATE_ENUM;

int init_done = 0;
PP_STATE_ENUM ppState;                                     //结构体名赋值给一个ppstate表示状态的量
BYTE rxFlag;                                               //set from within usrRxPacketCallback，RX为接收数据，rxFlag是接收标志位
BYTE payload[USB_FROM_PHONE_MAX_LEN];                                           //收发数据
UINT16 numTimeouts;	                                       //延时
BOOL first_packet;                                         //第一个数据包
phone_data_t *phoneData;                                   //手机的DATA信息
BYTE *A7190Data;                                           //A7190信息
phoneDataQueue_t phoneDataQueue;                           //信号堆栈
phoneDataQueue_t A7190DataQueue;                           //A7190信号堆栈

extern uint8_t ep3_data[A7190_MAX_LEN_LDD];                              //!!!!!!!!!定义全局变量ep3，用的时候用estern，wtp.c用到了！！！！！

/***********************************************乒乓函数**************************************************************************/

void PingPong(void)                                      //乒乓函数应该是数据收发相关
{
	int j, k;
	BYTE *a;
	apsFSM();                                          //check RX，检查接收状态

#ifdef LRWPAN_COORDINATOR

#endif

	if (!A7190DataIsEmpty())   //a7190接收到数据
	{

		A7190Data = (BYTE*) A7190DataDequeue();
		printf("\n this is A7190Data\n");
		for (j = 0; j < 82; j++)                            //将payload里面的数据依次打印出来
			printf("%c", A7190Data[j]);
		printf("\n");
		//A7190队列出队列进入
	}

	A7190DataInit();                //对A7190进行清空队列操作，再次初始化

	switch (ppState)   //状态选择功能跳转switch函数
	{

		/******************************PP_STATE_IDLE********************************/
		case PP_STATE_IDLE: 			//case空闲状态！！！！！！
			A7190DataInit();
			phoneDataInit();
			//	if( phoneDataIsEmpty() )
//{
			//	printf("data is empty\n");

			ep2_rev_ok = USB_GetData(ENDP2, ep2_data, USB_FROM_PHONE_MAX_LEN);
			if (ep2_rev_ok){
				puts("in ping pong: ok");

				Phonedata_int((char*) ep2_data);
				puts("in ping pong 333333: ok");

				phoneDataEnqueue((phone_data_t *) ep3_data);
				printf("rev ok\n");

				for (j = 0; j < A7190_MAX_LEN_LDD; j++) {
					printf("%u: %x\r\n", j,ep3_data[j]);											//打印ep3
					//printf("\n");
				}
				printf("now it is EP2:\r\n");
				for (j = 0; j < USB_FROM_PHONE_MAX_LEN; j++) {
					printf("%u: %x\r\n", j,ep2_data[j]);											//打印ep3
					//printf("\n");
				}
				
				ep2_rev_ok = 0;
				ppState = PP_STATE_SEND;
			} else {
				ppState = PP_STATE_IDLE;
			}
			break;
//}			

//	if( phoneDataIsEmpty() )	break;                    //堆栈为空跳出
			//     ppState = PP_STATE_SEND;                        //否则，也就是堆栈不为空，改为                 发送状态!!!1
			//		break;
			
			/******************************PP_STATE_IDLE********************************/
		case PP_STATE_SEND:                                  //case发送状态！！！！！！
															 //	if ((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(100)){
															 //MemDump();
			phoneData = phoneDataDequeue();	                   //条件满足，进入发送状态后，队列消息phonedatadequeue赋值给phonedata
//		
//								 	for(j=0;j<8;j++)
//       {
//        	dstADDR.laddr.bytes[j]=aplGetParentLongAddress()->bytes[j];  //发送地址为自己父节点
//       }

			for (j = 0; j < 8; j++) {                            //字节以及目标地址，8位一位一位发送
				dstADDR.laddr.bytes[j] = phoneData->destaddr[j];
			}

			//		dstADDR.laddr.bytes[i] = 0;
			//increment ping counter
			//	ping_cnt++;
			//this was value received by this node
			//received packet, ping it back
			//format the packet
			//	payload[0] = (BYTE) ping_cnt;
			//	payload[1] =  (BYTE) (ping_cnt>>8);
			//	ppState = PP_STATE_WAIT_FOR_TX;
			my_timer = halGetMACTimer();                    //changed by chenfei
#if 0
			aplSendMSG(APS_DSTMODE_LONG, &dstADDR, 2,  //dst EP
					0,//cluster is ignored for direct message
					1,//src EP
					(BYTE *)phoneData, 82,  //msg length信息长度
					apsGenTSN(), FALSE)
			;  //No APS ack requested
#else
			aplSendCustomMSG(IEEE_ADDRESS_ARRAY&0xff,
					dstADDR.laddr.bytes[0],
					82,
					(BYTE *)phoneData);
#endif
			a = (BYTE *) phoneData;
			printf("\nTHIS IS phoneData\n");
			for (j = 0; j < A7190_MAX_LEN_LDD; j++)                            //将payload里面的数据依次打印出来
				printf("%u: %x\r\n",j, a[j]);;
			//		printf("\n%d\n",*(BYTE *)phoneData);

			phoneDataInit();                     //数据发送完成后将队列初始化清空操作！！！！！
			
//				while (!phoneDataIsEmpty()) 
//				{
//					pop();
//				}					//		printf("\n%d\n",phoneData->data_payload[126]);	 发送数据的串口显示
			ppState = PP_STATE_WAIT_FOR_TX;                 //发送和串口打印结束后，由发送状态转换为等待发送状态，后跳出
			//	}
			break;

			/***************************PP_STATE_WAIT_FOR_TX********************************/
		case PP_STATE_WAIT_FOR_TX:                          //case等待发送状态！！！！！！
		
			if (apsBusy())
				break;                             //status not ready yet if busy.
			if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
				printf("\n    send ok    \n");
				ppState = PP_STATE_IDLE;

				last_tx_start = aplMacTicksToUs(aplGetLastTxTime() - last_tx_start);
				//huan hang
			} else {
				conPrintROMString("Ping Send failed! Restarting timer to try again\n");
				my_timer = halGetMACTimer();	                    //chongxin gouzao dingshi qi
				ppState = PP_STATE_IDLE;
			}
			break;
	}
}
/*********************************************以上全为pingpong函数内容***************************************************************/

/**********************************************以下全为主函数main内容****************************************************************/
int main(void) {
	Set_System();
	/*********************************Usart OK*****************************/
	numTimeouts = 0;
	my_timer = 0;
	first_packet = TRUE;
	halInit();                                              //init the hal

	dynamic_freq_mode=0xff;
															//evbInit();// init the board
	//SPI2_Init();
	phoneDataInit();                                        //令队列的头和尾都是零
	aplInit();                                              //init the stack，同aps初始化
	A7190DataInit();                                        //A7190的初始化
	Log("Ad-Hoc Network TEST V1.6 \r\n");                   //串口显示自组网测试1.6版本
	/***************************Ad-Hoc Network***************************/
	conPrintConfig();                                       // chuankoupeizhi，串口配置初始化
	spi_timer = halGetMACTimer();                           //SPI传输的时间等于硬件层获取MAC地址的时间
	ping_cnt = 0;                                           //乒乓函数参数
	rxFlag = 0;                                             //接收标志位初始化置0
	printf("Configation is OK \r\n");
	/****************************Configation OK****************************/USB_Interrupts_Config();                                //配置使能中断
	Set_USBClock(); 	//时钟设置并使能，USB时钟为48MHz
	//delay_init();
	printf("usbclock is OK \r\n");
	USB_Init();                                            //在USB init.c文件中，获取设备当前状态及主机正式请求等信息
	printf("usb is ok\r\n");
	/*********************************USB OK*****************************/

	/*********************************协调器一直这个循环中*****************************/
#ifdef LRWPAN_COORDINATOR                                 //先打开协调器，不然会出现问题

	aplFormNetwork();                                       //初始化一个新网络
	while(apsBusy()) {apsFSM();}                            //等待加入一个新的网络
	conPrintROMString("Network formed, waiting for RX\n");//网络已经组成，等待接收数据
	// conPrintROMString("COORDINATOR is OK \r\n");                          //  Print6(7,3,"Network formed!",1);

	ppState = PP_STATE_IDLE;//此时的状态为空闲等待状态
	
#else
	do {
		aplJoinNetwork();                                     //jia ru yi jing cun zai de wangluo，加入一个已经存在的网络
		while (apsBusy()) {
			apsFSM();
		}                          //wait for finish dengdai jiaru wancheng等待完成等待加入程序
		if (aplGetStatus() == LRWPAN_STATUS_SUCCESS)          //cheng gong jiaru wangluo  
				//	EVB_LED1_ON();                                    //如果加入成功的话，选择性的执行以下程序
				{	                                                    //     ClearScreen();
#ifdef LRWPAN_ROUTER                                      //如果是路径的话，进行显示，以下为各种显示的语言
			conPrintROMString("--ROUTER--");
			//      Print8(0,25,"--ROUTER-- ",1);
#endif
#ifdef LRWPAN_RFD
			//      Print8(0,45,"--RFD--  ",1);
#endif
			conPrintROMString("Network Join succeeded!\n");
			//      Print6(2,3,"Network Join succeed!",1);
			//   Print6(3,3,"eded!",1);
			conPrintROMString("My ShortAddress is: ");
			conPrintUINT16(aplGetMyShortAddress());	             //shuchu wangluo dizhi
			//     Print6(3,3,"My SADDR: ",1);
			//      LcdPrintUINT16(aplGetMyShortAddress(),3,63);
			conPCRLF();
			conPrintROMString("Parent LADDR: ")
			conPrintLADDR(aplGetParentLongAddress());
			//     Print6(4,3,"Parent LADDR: ",1);
			//     LcdPrintLADDR(aplGetParentLongAddress(),4,87);
			conPrintROMString(", Parent SADDR: ");
			conPrintUINT16(aplGetParentShortAddress());
			conPCRLF();
			break;
		} else {  //jiaru wngluo shibai                          //如果加入网络失败的话，执行下面的语句，串口显示网络加入失败，重新尝试
			conPrintROMString("Network Join FAILED! Waiting, then trying again\n");
			//     ClearScreen();
#ifdef LRWPAN_ROUTER
			//    Print8(0,25,"--ROUTER-- ",1);
#endif
#ifdef LRWPAN_RFD
			//     Print8(0,40,"--RFD--  ",1);
#endif
			halWait(100);
			//     Print6(2,3,"Network Join FAILED! ",1);
			//     Print6(3,3,"Waiting, then trying",1);
			//     Print6(4,3,"again!",1);
			my_timer = halGetMACTimer();
			//wait for 2 seconds
			while ((halMACTimerNowDelta(my_timer)) < MSECS_TO_MACTICKS(2 * 1000))
				;
		}
	} while (1);

#endif
	
#if ( defined(LRWPAN_COORDINATOR))
	//WARNING - this is only for latency testing, max MAC retries is normally
	//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
	//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
	//only do this in your normal code if you want to disable automatic retries
	aplSetMacMaxFrameRetries(0);//she ding chongchuan cishu wei 0 biaoshi bu chongchuan
	//		my_timer= halGetMACTimer();
	while (1) {
		//	if((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(10))
		//	{
		//		A7190DataEnqueue((phone_data_t *)test_data);
		//		my_timer= halGetMACTimer();
		//	}
		PingPong();//diaoyong pingpong buting fasong jieshou ,jinxing zhuangtai zhuanhuan
		
	}
#endif
	
	/*********************************协调器一直这个循环中*****************************/

	/*********************************路由器一直这个循环中*****************************/
#ifdef LRWPAN_ROUTER
	//router does nothing, just routes
	DEBUG_PRINTNEIGHBORS(DBG_INFO);//dayin linju xinxi答应邻居的信息ACK
	conPrintROMString("Router, doing its thing.!\n");
	//  Print6(7,3,"Router is running!",1);
	ppState = PP_STATE_IDLE;                                  //进入空闲状态
	my_timer = 0;
	while (1) {
//		
//		ep1_rev_ok=USB_GetData(ENDP1,ep1_data,145);
//		  if(ep1_rev_ok)
//				{
//while(j_cnt>0)		
// 	if((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(1)&&j_cnt > 0)
//		{
// 		   phoneDataEnqueue((phone_data_t *)ep1_data);
// 		   my_timer = halGetMACTimer();
// 	j_cnt--;
// }
//	ep1_rev_ok=0;
//		}
//		
		PingPong();
	}
#endif
	/*********************************路由器一直这个循环中*****************************/
//return 0;
}

/***********************************************以上全为主函数main内容****************************************************************/

//########## Callbacks ##########
//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.	//huidiao hanshu zai zhongduan shebei cuoyoucaozuo wancheng zhihou zhixing
LRWPAN_STATUS_ENUM usrZepRxCallback(void) {	  //zhongduanshebei huidiao hanshu

#ifdef LRWPAN_COORDINATOR
	if (aplGetRxCluster() == ZEP_END_DEVICE_ANNOUNCE) {
		//a new end device has announced itself, print out the
		//the neightbor table and address map
		dbgPrintNeighborTable();//dayin chu linjin liebiao
	}
#endif
	return LRWPAN_STATUS_SUCCESS;
}

//callback from APS when packet is received
//user must do something with data as it is freed
//within the stack upon return.	//zai jieshou dao shujubao hou zhixing huiidoa hanshu ,yonghu bixu zhixing caozuo zai zhan kongjian huishou zhiqian dui shuju jinxing chuli  

LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {  //huidiao hanshu shuchu shuju xinxi

	BYTE len, *ptr;
	int i = 0;
// 	//just print out this data
//         if(LcdPage == 2)
//         {
//       //    ClearScreen();
//         }
// #ifdef LRWPAN_COORDINATOR
//   conPrintROMString("Coordinator, ");
//  // Print8(0,30,"--COORD--       ",1);
// #endif
// #ifdef LRWPAN_ROUTER
//   conPrintROMString("Router, ");
//  // Print8(0,25,"--ROUTER-- ",1);
// #endif
// #ifdef LRWPAN_RFD
//   conPrintROMString("RFD, ");
//  // Print8(0,40,"--RFD--  ",1);
// #endif
//	conPrintROMString("User Data Packet Received: \n");
// 	conPrintROMString("SrcSADDR: ");
// 	conPrintUINT16(aplGetRxSrcSADDR());
//     //    Print6(LcdPage,3,"SrcSADDR: ",1);
//     //    LcdPrintUINT16(aplGetRxSrcSADDR(),LcdPage++,63);
// 	conPrintROMString(", DstEp: ");
// 	conPrintUINT8(aplGetRxDstEp());
// 	conPrintROMString(", Cluster: ");
// 	conPrintUINT8(aplGetRxCluster());
// 	conPrintROMString(", MsgLen: ");
// 	len = aplGetRxMsgLen();
// 	conPrintUINT8(len);
// 	conPrintROMString(",RSSI: ");
// 	conPrintUINT8(aplGetRxRSSI());
//     //    Print6(LcdPage,3,"RSSI: ",1);
//      //   LcdPrintUINT8(aplGetRxRSSI(),LcdPage++,39);
//	conPCRLF();
//printf("\ndddddd\n");
//////// 	ptr = aplGetRxMsgData();
////////	 	printf("\n ZSFASDCSA \n");
//////// 	for(i = 0;i<145;i++)
//////// 	printf("%x",ptr[i]);
//////// 	printf("\n");
//conPrintROMString("PingCnt: ");
	A7190DataEnqueue((phone_data_t *) aplGetRxMsgData());
	//printf();
	//ping_cnt = *ptr;
//	ptr++;
//	ping_cnt += ((UINT16)*ptr)<<8;
//	conPrintUINT16(ping_cnt);
	//   Print6(LcdPage,3,"PingCnt: ",1);
	//    LcdPrintUINT16(ping_cnt,LcdPage++,57);
	//     Print6(LcdPage,100," ",1);
//        LcdPage++;
	//    if(LcdPage >= 8)
	//    {
	//      LcdPage = 2;
	//     }
	//conPrintROMString(", RxTimeouts: ");
//	conPrintUINT16(numTimeouts);
	rxFlag = 1;	//signal that we got a packet
	//use this source address as the next destination address
	dstADDR.saddr = aplGetRxSrcSADDR();
//	conPCRLF();
	return LRWPAN_STATUS_SUCCESS;
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
BOOL usrJoinVerifyCallback(LADDR *ptr, BYTE capinfo) {

#if 0      //set this to '1' if you want to test through a router
//only accept routers.
//only let routers join us if we are coord
#ifdef LRWPAN_COORDINATOR
	if (LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) {
		//this is a router, let it join
		conPrintROMString("Accepting router\n");
		return TRUE;
	} else {
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

BOOL usrJoinNotifyCallback(LADDR *ptr) {

	//allow anybody to join

	conPrintROMString("Node joined: ");
	conPrintLADDR(ptr);
	//  ClearScreen();
#ifdef LRWPAN_COORDINATOR
	conPrintROMString("Coordinator, ");
	// Print8(0,30,"--COORD--    ",1);
#endif
#ifdef LRWPAN_ROUTER
	conPrintROMString("Router, ");
//  Print8(0,25,"--ROUTER-- ",1);
#endif
#ifdef LRWPAN_RFD
	conPrintROMString("RFD, ");
//  Print8(0,40,"--RFD--  ",1);
#endif
	//   Print6(2,3,"Node joined: ",1);
	//    LcdPrintLADDR(ptr,2,81);
	conPCRLF();
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	return TRUE;
}
#endif

//called when the slow timer interrupt occurs
#ifdef LRWPAN_ENABLE_SLOW_TIMER
void usrSlowTimerInt(void) {
}
#endif

//基本的中断回调函数，是否进行取决于硬件层的各个状态
//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void) {
}

void aplRxCustomCallBack(){
	BYTE len, *ptr;
	int i = 0;
	A7190DataEnqueue((phone_data_t *) aplGetRxMsgData());
	rxFlag = 1;
}
