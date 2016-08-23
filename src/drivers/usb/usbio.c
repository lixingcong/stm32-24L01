/********************************************************************************
*
*  File Name     : usbio.c
*  Author   	 : 王保礼
*  Create Date   : 2016.06.01
*  Version   	 : 1.0
*  Description   : USB端点数据读写函数
*  History       : 1. Data:
*                     Author:
*                     Modification:
*
********************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_endp.h"
#include "usbio.h"
#include "stdio.h"
#include "delay.h"



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/
uint16_t EP2_RevNum=0;  //EP2接收数据长度
uint8_t EP2_RevFlag=1;  //EP2端点数据接收完成标志
uint8_t EP1_RevFlag=1;  //EP1端点数据接收完成标志


/* Private function prototypes -----------------------------------------------*/  
/* Private functions ---------------------------------------------------------*/




/********************************************************************************
*
* Description : USB发送端点数据函数，将数据通过USB发送出去
* Arguments   : bEpNum：ENDP1 or ENDP2
*               data：待发送数据首地址
*               dataNum：需要发送的字节数，ENDP1最大64字节，ENDP2没有限制
* Returns     : 1：发送成功，0：发送失败或未完成发送
* Notes       : 
*
********************************************************************************/

uint32_t USB_SendData(uint8_t bEpNum,uint8_t *data,uint32_t dataNum)
{
    uint8_t send_time=0;
    uint8_t num=0;
    uint8_t i=0;

	if(bEpNum == ENDP1) //端点1
    {
		UserToPMABufferCopy(data, ENDP1_TXADDR, dataNum);
        SetEPTxCount(bEpNum, dataNum);
        SetEPTxValid(bEpNum);
	}
    else    //端点2
    {   
        send_time = dataNum / 64;
        if(dataNum > 64)   //大于64字节
        {
            for(i=0;i<send_time;i++)
            {
                UserToPMABufferCopy(data, ENDP2_TXADDR, 64);
                SetEPTxCount(bEpNum, 64);
                SetEPTxValid(bEpNum);
                data += 64;
                DelayMs(1);//delay 1ms ,间隔应该可以更小，但没有测试
            }
            num = dataNum % 64; //计算剩余字节数
            UserToPMABufferCopy(data, ENDP2_TXADDR, num);
            SetEPTxCount(bEpNum, num);
            SetEPTxValid(bEpNum);            
            
        }
        else    //小于等于64字节
        {
            UserToPMABufferCopy(data, ENDP2_TXADDR, dataNum);
            SetEPTxCount(bEpNum, dataNum);
            SetEPTxValid(bEpNum); 
        }

	}    
    
	return dataNum;  //没什么意义的返回值，以后会优化
}


/********************************************************************************
*
* Description : USB读取端点数据函数
* Arguments   : bEpNum：ENDP1 or ENDP2
*               data：数据存储首地址
*               dataNum：需要读取的字节数，ENDP1最大64字节，ENDP2没有限制
* Returns     : 1：接收成功，0：接收失败或未完成接收
* Notes       : 
*
********************************************************************************/

uint8_t USB_GetData(uint8_t bEpNum,uint8_t *data,uint32_t dataNum)
{
    uint16_t len=0;
    static uint32_t tmp=1;
    
    if(bEpNum == ENDP2)
    {

        if(tmp) //确保只执行一次
        {
            EP2_RevNum = dataNum;
            tmp=0;
        }
        
        
        if(EP2_RevFlag == 0)    //指定长度数据接收完成
        {
             for(len=0;len<dataNum;len++)
             {
                *data=USB_EP2_Receive_Buffer[len];  //拷贝数据
                data++;
             }
            EP2_RevFlag = 1;
            tmp=1;
            return 1;   //完成接收
        }
        
        return 0;   //未完成接收
    }
    
    else if(bEpNum == ENDP1)
    {
        if(EP1_RevFlag == 0)    //指定长度数据接收完成
        {
             for(len=0;len<dataNum;len++)
             {
                *data=USB_EP1_Receive_Buffer[len];  //拷贝数据
                data++;
             }
            EP1_RevFlag = 1;
            return 1;   //完成接收
        }
        
        return 0;   //未完成接收        
    }
    
    else
        printf("usb endpoint err\n\r");

    return 0;   //未完成接收或错误
}


/*********************************END OF FILE**********************************/

