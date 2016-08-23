/*
 V0.1 Initial Release   10/July/2006
 *2006/08/16 WXL 2.0
 */

/*
 This is a two node test, requires a Coordinator
 and an RFD. The coordinator and node simply
 ping-pong a packet back and forth, and print
 out the RSSI byte.  The RFD waits before
 bouncing it back, while the coordinator responds
 immediately.

 两个节点测试样例，需要一对用户，它们进行简单的pingpong互相握手，显示出信号强度
 RFD一直等待回复，coord则马上回复

 Expects coordinator, and one RFD.
 The topology to test should be:

 Coordinator ->  RFD1
 测试方法：coord发送给rfd1


 Start the coordinator first, then
 RFD1. If a RFD1 fails to join the network, try
 again. The RFD1 will prompt the user to hit
 a key to start the ping-pong.
 现在先启动coord，然后轮到rfd，如果rfd1无法加入网路，尝试一次，我们需要按键让其进行pingpong

 You can connect multiple RFDs if desired.

 You can also ping-pong through a router; see
 the note in usrJoinVerifyCallback(). The topology
 for a router would be:
 使用usrJoinVerifyCallback进行路由pingpong，拓朴结构如下

 coord -> router -> RFD1
 -> RFD2
 -> ..RFDn


 This  requires Virtual Boards to be running,
 since a switch press is needed to start the pinging.
 这需要一块仿真板进行测试，需要按键

 */

#include "msstate_lrwpan.h"
#include "delay.h"
#include "SPI1.h"
#include "lib_headers_stm32.h"
#include "my_queue.h"
#include "address_and_filter.h"
#ifndef LRWPAN_COORDINATOR
#define PING_DELAY   2  //wait before bouncing back
#else
#define PING_DELAY   0 //coordinator does not wait
#endif

#define RX_PING_TIMEOUT     5    //seconds

//BYTE ping_cnt = 0;
UINT32 my_timer;

UINT32 last_tx_start;
LADDR_UNION dstADDR;
slave_mode_t SLAVE_MODE;
spi_mode_t SPI_MODE;

/*
 ping pong 状态机：空闲状态、发送状态、等待发送状态
 */
typedef enum _PP_STATE_ENUM {
	PP_STATE_IDLE, PP_STATE_SEND, PP_STATE_WAIT_FOR_TX
} PP_STATE_ENUM;

PP_STATE_ENUM ppState;

int j_cnt = 20;
int init_done = 0;

void PingPong(void);

// 这个pingpong函数的目标是在哪里指定的
void PingPong(void) {
	int i;
	BYTE *a;
	UINT32 pulse_timer;  //脉冲定时器
	apsFSM();

	if (!A7190DataIsEmpty()) {
		if (SPI_MODE == SPI_DONE) {  //若SPI发送数据完毕
			if ((halMACTimerNowDelta(spi_timer)) > MSECS_TO_MACTICKS(1)) {  // 若定时器已经过去1个MAC-tick-tock的时间单位（毫秒）
				SLAVE_MODE = SLAVE_TX;
				SPI_MODE = SPI_SENDING;
				A7190Data = (BYTE*) A7190DataDequeue();				//出列后的数据
				SPI2->DR = *A7190Data;				//送到SPI2数据寄存器
				if (!init_done) {
					RCC->APB2ENR |= 1 << 5;       //PORTD时钟使能
					GPIOD->CRL &= 0XFFFFFF0F;
					GPIOD->CRL |= 0X00000030;       //PD1推挽输出
					GPIOD->ODR |= 1 << 1;    //PD1输出1

					init_done = 1;
					pulse_timer = halGetMACTimer();
					while (halMACTimerNowDelta(pulse_timer) < 8)
						//等待8毫秒
						;
					//  DelayUs(100);
				}
				GPIOD->ODR &= ~(1 << 1);    //PD1输出0

				pulse_timer = halGetMACTimer();
				while (halMACTimerNowDelta(pulse_timer) < 8)
					//等待8毫秒
					;
				GPIOD->ODR |= 1 << 1;    //PD1输出1
			}
		}

	}
	switch (ppState) {    //ping-pong state状态机

		case PP_STATE_IDLE:
			if (phoneDataIsEmpty())
				break;
			ppState = PP_STATE_SEND;
			break;
			// 发送Ping pong
		case PP_STATE_SEND:
			//	if ((halMACTimerNowDelta(my_timer))> MSECS_TO_MACTICKS(100)){
			//		MemDump();
			phoneData = phoneDataDequeue();			//出列数据成为phoneData
			for (i = 0; i < 8; i++)
				dstADDR.laddr.bytes[i] = phoneData->destaddr[i];

			my_timer = halGetMACTimer();  ///changed by chenfei

			// 函数原型：aplFmtSendMSG(dstMode, dstADDR, dstEP,   cluster, srcEP, pload, plen,   tsn,  reqack );
			//                        长短模式  地址     目的终端  簇       源终端 数据包 数据包长 序列号 是否ACK
			aplSendMSG(APS_DSTMODE_LONG, &dstADDR, 2,  //dst EP
					0,//cluster is ignored for direct message
					1,//src EP
					(BYTE *)phoneData, 145,  //msg & length
					apsGenTSN(),// 生成序列号
					FALSE)
			//No APS ack requested
			;

			// 调试用的打印出测试数据
			a = (BYTE *) phoneData;
			//	printf("\nsend to\n");
			for (i = 0; i < 145; i++)
				printf("%c", a[i]);
			printf("\n");
			ppState = PP_STATE_WAIT_FOR_TX;
			//	}
			break;
		case PP_STATE_WAIT_FOR_TX:
			if (apsBusy())
				break;  //status not ready yet if busy.
			if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
				ppState = PP_STATE_IDLE;
				//compute the latency of this TX send operation
				//aplGetLastTxTime gets the time that the LAST tx operation finished.
				//this will be the latency of the TX stack operation only if no mac retries were required
				// 计算出延迟，前提是重传次数为0，否则不准确，用这次发送时刻减去上一次的发送时刻得到延迟
				// 至于这个微秒是怎么从mac-tick转换过的，还真的莫明其妙
				last_tx_start = aplMacTicksToUs(aplGetLastTxTime() - last_tx_start);
				//		conPrintROMString("TX Stack latency(us): "); //测发送数据延时时间
				//		conPrintUINT32(last_tx_start);       //输出上一次的延迟
				//		conPCRLF();							//newline
			} else {
				conPrintROMString("Ping Send failed! Restarting timer to try again\r\n");
				my_timer = halGetMACTimer();	 // 初始化定时器
				ppState = PP_STATE_IDLE;
			}
			break;
	}
}

int main(void) {
	//this initialization set our SADDR to 0xFFFF,
	//PANID to the default PANID
	//HalInit, evbInit will have to be called by the user
	halInit();	//init the hal
	phoneDataInit();
	aplInit();  //init the stack，最高层的初始化，递归调用其下层的初始化
	A7190DataInit();  //这个队列与phoneData队列是共用定义的
	conPrintConfig();  //调试串口初始化
	spi_timer = halGetMACTimer();  //当前的时刻
	ENABLE_GLOBAL_INTERRUPT();  //enable interrupts
	//debug_level = 10;

#ifdef LRWPAN_COORDINATOR

	aplFormNetwork();  //协调器新建一个网络LRWPAN，可以使得路由器或者RFD加入
	while (apsBusy()) {
		apsFSM();
	}  //wait for finish
	conPrintROMString("Network formed, waiting for RX\n");
	//  Print6(7,3,"Network formed!",1);
	//EVB_LED1_ON();
	ppState = PP_STATE_IDLE;

#else
	do {
		aplJoinNetwork();  //路由器加入由协调器创建的网络
		while (apsBusy()) {  //直到加入成功
			apsFSM();
		}
		if (aplGetStatus() == LRWPAN_STATUS_SUCCESS) {
#ifdef LRWPAN_ROUTER
			conPrintROMString("--ROUTER--");
#endif
#ifdef LRWPAN_RFD
			conPrintROMString("--RFD--");
#endif
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
		} else {
			conPrintROMString("Network Join FAILED! Waiting, then trying again\r\n");
			halWait(100);
			my_timer = halGetMACTimer();
			//wait for 2 seconds
			while ((halMACTimerNowDelta(my_timer)) < MSECS_TO_MACTICKS(2 * 1000))
				;
		}
	} while (1);

#endif
#ifdef LRWPAN_RFD
	//now send packets
	dstADDR.saddr = 0;//RFD sends to the coordinator
	conPrintROMString("Hit UP or DOWN switch to start!\n");
#endif

#if (defined(LRWPAN_RFD) || defined(LRWPAN_COORDINATOR))
	//WARNING - this is only for latency testing, max MAC retries is normally
	//set to aMaxFrameRetries (value=3) as defined in mac.h. Setting this to 0 means
	//that there will be no automatic retransmissions of frames if we do not get a MAC ACK back.
	//only do this in your normal code if you want to disable automatic retries
	aplSetMacMaxFrameRetries(0);//帧不重传，像udp数据包那样。目的是测试延迟
	while (1) {
		PingPong();	//开始打乒乓球了哦，你打一球我接球然后打回去！这个是协调器执行的函数
	}
#endif

#ifdef LRWPAN_ROUTER
	//router does nothing, just routes
	// 输出邻居
	DEBUG_PRINTNEIGHBORS(DBG_INFO);
	conPrintROMString("Router, doing its thing.!\n");
	ppState = PP_STATE_IDLE;
	my_timer = 0;
	while (1) {
		PingPong();
	}
#endif

	return 0;
}

//########## Callbacks ##########
// 注意Callback函数都在这里定义，下面的回调函数被mac.c文件中所调用，因此实际开发中注意使用宏定义屏蔽本demo的main()

//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
// 回调函数，处理来自RFD的数据
LRWPAN_STATUS_ENUM usrZepRxCallback(void) {

#ifdef LRWPAN_COORDINATOR
	// 若簇名字（树名）是Announce，则输出邻居表
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
// Application Support Sublayer（应用支持子层）
// 从APS接收到数据包后的回调函数
// 在返回给上一层（例如应用层）之前，用户必须先处理好这个数据包
LRWPAN_STATUS_ENUM usrRxPacketCallback(void) {

	BYTE len, *ptr;
	int i = 0;
	A7190DataEnqueue((phone_data_t *) aplGetRxMsgData());
	//use this source address as the next destination address
	dstADDR.saddr = aplGetRxSrcSADDR();
	conPCRLF();
	return LRWPAN_STATUS_SUCCESS;
}

#ifdef LRWPAN_FFD
//Callback to user level to see if OK for this node
//to join - implement Access Control Lists here based
//upon IEEE address if desired
// 是否只允许router节点加入
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

// 当有邻居加入时的回调函数：打印邻居表
BOOL usrJoinNotifyCallback(LADDR *ptr) {
	//allow anybody to join
	conPrintROMString("Node joined: ");
	conPrintLADDR(ptr);
#ifdef LRWPAN_COORDINATOR
	conPrintROMString("Coordinator, ");
#endif
#ifdef LRWPAN_ROUTER
	conPrintROMString("Router, ");
#endif
#ifdef LRWPAN_RFD
	conPrintROMString("RFD, ");
#endif
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

//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void) {
}
