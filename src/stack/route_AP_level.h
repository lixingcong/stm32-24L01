/*
 * route_AP_level.c
 *
 *  Created on: 2016年8月12日
 *      Author: lixingcong
 */

#ifndef _APL_CUSTOM_FUNCTION_H_
#define _APL_CUSTOM_FUNCTION_H_

#include "route_table.h"
#include "route_ping.h"

typedef struct _APS_CUSTOM_FRAME {
	unsigned char src_addr;
	unsigned char *data;
	unsigned short flen;
	unsigned char frame_type;
} APS_CUSTOM_FRAME;

extern APS_CUSTOM_FRAME my_custom_frame;

// 获取消息，常用于aplRxCustomPacketCallback函数内
#define aplGetRxMsgData() (my_custom_frame.data)
#define aplGetRxMsgLen() (my_custom_frame.flen)
#define aplGetRxMsgSrcAddr() (my_custom_frame.src_addr)
#define aplGetRxMsgType() (my_custom_frame.frame_type)

// 发送函数帧长flen最大为256-5=251
// 定义宏 MAX_CUSTOM_FRAME_LENGTH数值251在route_table.h

// 发送消息，自动路由，支持多跳，点对点。
#define aplSendMSG(dst,flen,frm) \
		send_custom_packet_relay(MY_NODE_NUM,dst,flen,frm,FRAME_TYPE_LONG_MSG,LONG_MSG_DEFAULT_TTL)

// 发送给孩子的广播, 广播给所有孩子和孙子节点
#define aplSendBROADCAST(flen,frm) \
		send_custom_packet_relay(MY_NODE_NUM,0xff,flen,frm,FRAME_TYPE_LONG_BROADCAST,LONG_MSG_DEFAULT_TTL)

// 先发送到dst节点，dst节点收到后广播后，由dst给它的孩子和孙子进行广播
#define aplSendDstBROADCAST(dst,flen,frm) \
		send_custom_packet_relay(MY_NODE_NUM,dst,flen,frm,FRAME_TYPE_LONG_BROADCAST,LONG_MSG_DEFAULT_TTL)

// 更新AP层的接收信息，以便于对接aplRxCustomCallBack()
void update_AP_msg(unsigned char *ptr, unsigned short flen);

#define aplSendPing(dst,retry_times,retry_interval) \
		macTxCustomPing(dst, PING_DIRECTION_TO_OTHERS, retry_times,retry_interval)

#endif /* APL_CUSTOM_FUNCTION_H_ */
