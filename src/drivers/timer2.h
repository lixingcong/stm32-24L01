/*
 * timer2.h
 *
 *  Created on: 2016年8月17日
 *      Author: li
 */

#ifndef SRC_DRIVERS_TIMER2_H_
#define SRC_DRIVERS_TIMER2_H_

extern unsigned int system_msecond;
void TIM2_Init();
void TIM2_NVIC_disable(void);

#endif /* SRC_DRIVERS_TIMER2_H_ */
