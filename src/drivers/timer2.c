/*
 * timer2.c
 *
 *  Created on: 2016年8月17日
 *      Author: li
 *      定时器2初始化
 */
#include "timer2.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "stdio.h"
#include "misc.h"

void TIM2_NVIC_enable(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 中断优先级较低
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void TIM2_Init()
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = (TIM2_UPDATE_TIME*10000-1); // every 2 second
	TIM_TimeBaseStructure.TIM_Prescaler = 7199;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_Cmd(TIM2,ENABLE);
}

void TIM2_NVIC_disable(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
}
