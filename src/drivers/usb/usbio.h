/********************************************************************************
*
*  File Name     : usbio.h
*  Author   	 : 王保礼
*  Create Date   : 2016.06.01
*  Version   	 : 1.0
*  Description   : USB端点读写函数
*  History       : 1. Data:
*                     Author:
*                     Modification:
*
********************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _USBIO_H_
#define _USBIO_H_


extern uint16_t EP2_RevNum;
extern uint8_t EP2_RevFlag;
extern uint8_t EP1_RevFlag;



/* Exported Functions --------------------------------------------------------*/
extern uint32_t USB_SendData(uint8_t bEpNum,uint8_t *data,uint32_t dataNum);
extern uint8_t USB_GetData(uint8_t bEpNum,uint8_t *data,uint32_t dataNum);



#endif //_USBIO_H_
