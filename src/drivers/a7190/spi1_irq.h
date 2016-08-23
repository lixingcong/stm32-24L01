#ifndef SPI1_IRQ_H
#define SPI1_IRQ_H

#include "hal.h"

void spi1_irq_a7190(void);
void SPI1_handle(void);


// A7190数据接收缓冲区
extern unsigned char ack_bytes[LRWPAN_MAX_FRAME_SIZE];

#endif
