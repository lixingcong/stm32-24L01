#ifndef SPI1_H
#define SPI1_H
#include "stm32f10x.h"


typedef enum{
	SLAVE_RX,
	SLAVE_TX,
}slave_mode_t;
typedef enum{
	SPI_SENDING,
	SPI_DONE,
}spi_mode_t;
void SPI1_Init(void);
void EXTI_config_for_A7190(void);

unsigned char SPI1_ReadWriteByte(unsigned char TxData);
unsigned char SPI1_ReadByte(unsigned char TxData);
#endif

