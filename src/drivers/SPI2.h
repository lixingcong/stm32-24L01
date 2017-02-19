#ifndef __SPI2_H_
#define	__SPI2_H_


#include "stm32f10x.h"

void SPI2_Init(void);
u8 SPI2_ReadWriteByte(u8 TxData);
void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler);

#endif /* __SPI_H_ */

