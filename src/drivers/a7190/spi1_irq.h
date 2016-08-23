#ifndef SPI1_IRQ_H
#define SPI1_IRQ_H
#include "compiler.h"
#include "ieee_lrwpan_defs.h"

void spi1_irq_a7190(void);
void SPI1_handle(void);

// 全局使用定时器（返回当前的时刻）
extern UINT32 spi_timer;

// A7190数据接收缓冲区
extern BYTE ack_bytes[LRWPAN_MAX_FRAME_SIZE];

#endif
