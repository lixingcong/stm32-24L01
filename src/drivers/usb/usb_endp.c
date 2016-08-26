/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : usb_endp.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Endpoint routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_desc.h"
#include "usbio.h"
#include "usb_endp.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t *USB_RevTmp;

/* Private function prototypes -----------------------------------------------*/
/* Public variables ---------------------------------------------------------*/
uint8_t USB_EP2_Receive_Buffer[USB_FROM_PHONE_MAX_LEN];
uint8_t USB_EP1_Receive_Buffer[100];
uint8_t USB_Send_Buffer[USB_FROM_PHONE_MAX_LEN];

/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
* Function Name  : EP1_IN_Callback.
* Description    : EP1 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback(void)
{
	//SetEPTxStatus(ENDP1, EP_TX_STALL);
	//printf("EP2_IN_Callback\n\r");
}
/*******************************************************************************
* Function Name  : EP1_OUT_Callback.
* Description    : EP1 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_OUT_Callback(void)
{
    volatile uint32_t EP1_ReceivedCount=0;
    
	EP1_ReceivedCount = GetEPRxCount(ENDP1);
	PMAToUserBufferCopy(USB_EP1_Receive_Buffer, ENDP1_RXADDR, EP1_ReceivedCount);
	SetEPRxStatus(ENDP1, EP_RX_VALID);
    
    EP1_RevFlag=0;  //接收完成
}


/*******************************************************************************
* Function Name  : EP2_IN_Callback.
* Description    : EP2 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP2_IN_Callback(void)
{
	//SetEPTxStatus(ENDP2, EP_TX_STALL);
	//printf("EP2_IN_Callback\n\r");
}

/*******************************************************************************
* Function Name  : EP2_OUT_Callback.
* Description    : EP2 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/

void EP2_OUT_Callback(void)
{
    static uint16_t rev_count_tmp=0;
    static uint8_t flag=1;
    
    volatile uint32_t EP2_ReceivedCount=0;
    
    
	EP2_ReceivedCount = GetEPRxCount(ENDP2);
    
    if( EP2_RevNum <= 64 )
    {
        PMAToUserBufferCopy(USB_EP2_Receive_Buffer, ENDP2_RXADDR, EP2_ReceivedCount);
        SetEPRxStatus(ENDP2, EP_RX_VALID);
        EP2_RevFlag = 0;    //指定长度数据接收完成
    }
    else 
    {
        if(flag)    //确保在指定长度的数据接收过程中只进行一次赋值
        {
            USB_RevTmp = USB_EP2_Receive_Buffer;
            flag = 0;
        }
        
        PMAToUserBufferCopy(USB_RevTmp, ENDP2_RXADDR, EP2_ReceivedCount);
        USB_RevTmp += EP2_ReceivedCount;
        
        rev_count_tmp += EP2_ReceivedCount;
        
        SetEPRxStatus(ENDP2, EP_RX_VALID);

        if(rev_count_tmp >= EP2_RevNum) //已经接收完指定长度
        {
                    
            EP2_RevFlag = 0;    //指定长度数据接收完成
            rev_count_tmp = 0;
            flag = 1;            
            
        }
    }
    
}


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

