/********************************************************************************
*
*  File Name     : usb_endp.h
*  Author   	 : 王保礼
*  Create Date   : 2016.06.01
*  Version   	 : 1.0
*  Description   : 
*  History       : 1. Data:
*                     Author:
*                     Modification:
*
********************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _USB_ENDP_H_
#define _USB_ENDP_H_

#include "usb_1.h"


extern uint8_t USB_EP2_Receive_Buffer[USB_FROM_PHONE_MAX_LEN];
extern uint8_t USB_EP1_Receive_Buffer[100];
extern uint8_t USB_Send_Buffer[USB_FROM_PHONE_MAX_LEN];



#endif
