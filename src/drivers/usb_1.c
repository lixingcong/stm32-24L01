/*
 * usb_1.c
 *
 *  Created on: 2016年8月25日
 *      Author: li
 */

#include "usb_1.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "platform_config.h"
#include "misc.h"

void init_USB_GPIO(){
	GPIO_InitTypeDef  GPIO_InitStructure;
	  /* enable the PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Enable the USB disconnect GPIO clock */
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);

  	/* USB_DISCONNECT used as USB pull-up */
  	GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC , ENABLE);

}
