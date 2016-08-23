#ifndef __LMX2581_H_
#define	__LMX2581_H_

// Description:LMX2581驱动程序
// File Name:lmx2581.h
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "lmx2581_reg.h"
#include "SPI2.h"

#define USE_HSPI//使用硬件SPI2
#define LMX_SPI SPI2

#define LMX2581_DATA_GPIO	GPIOB
#define LMX2581_CLK_GPIO	GPIOB
#define LMX2581_LE_GPIO		GPIOB
#define LMX2581_CE_GPIO		GPIOB
#define LMX2581_MX_GPIO		GPIOB

#define LMX2581_DATA_PIN		GPIO_Pin_15
#define LMX2581_CLK_PIN			GPIO_Pin_13
#define LMX2581_LE_PIN			GPIO_Pin_8
#define LMX2581_CE_PIN			GPIO_Pin_12
#define LMX2581_MX_PIN			GPIO_Pin_14

void lmx2581_gpio_init(void);
void lmx2581_init(void);
void lmx2581_open(void);
void lmx2581_close(void);

rt_size_t lmx2581_read(rt_off_t pos, void* buffer, rt_size_t size);
rt_size_t lmx2581_write(const void* buffer, rt_size_t size);
void LMXDelay(__IO u32 nCount);

#endif /* __LMX2581_H_ */
