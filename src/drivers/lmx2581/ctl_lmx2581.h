#ifndef __CTR_LMX2581_H_
#define	__CTR_LMX2581_H_
#include "lmx2581.h"
#include "stdio.h"
#include "stdint.h"

// Description:LMX2581控制程序
// File Name:lmx2581.h
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

void ctl_lmx2581_init(void);
uint8_t ctl_lmx2581_setFrequency(uint16_t frequency);
uint16_t ctl_lmx2581_getFrequency(void);
uint8_t ctl_frequency_set(uint16_t frequency);

#endif /* __LMX2581_H_ */

