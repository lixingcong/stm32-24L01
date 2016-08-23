#ifndef __SPI2_H_
#define	__SPI2_H_

// Description:SPI函数
// File Name: spi.h
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

#include "stm32f10x.h"

/* SPI2 */
/*
SPI2_NSS ---PB12	//软件控制模式下,该引脚可以设置成其他脚
SPI2_SCK ---PB13
SPI2_MISO---PB14
SPI2_MOSI---PB15
*/

#define SPI2_NSS GPIO_Pin_12
#define SPI2_CS_HIGH() GPIO_SetBits   (GPIOB,SPI2_NSS);//使能
#define SPI2_CS_LOW()  GPIO_ResetBits (GPIOB,SPI2_NSS);

void SPI2_Config(void);
#endif /* __SPI_H_ */

