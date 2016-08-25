#ifndef SPI1_IRQ_H
#define SPI1_IRQ_H

#include "hal.h"

void EXTI8_irq_a7190(void);
void SPI1_handle(void);


// A7190数据接收缓冲区
extern unsigned char recv_buffer_a7190[LRWPAN_MAX_FRAME_SIZE];

#endif
