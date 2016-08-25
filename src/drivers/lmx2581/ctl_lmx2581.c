#include "ctl_lmx2581.h"
#include "lmx2581.h"
#include "stdio.h"
#include "stdint.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "hal.h"

// Description:LMX2581控制程序
// File Name:lmx2581.c
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

/*
 * 函数名：ctl_lmx2581_init
 * 描述  ：初始化芯片
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
extern struct R0 regR0;
extern struct R5 regR5;  

static uint16_t lmxFrequency;//保存lmx芯片频率

void ctl_lmx2581_init(void)
{
#ifndef USE_HSPI
	lmx2581_gpio_init();//引脚初始化
#else
		/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;
		/* 配置LE LMX_LE ---PB12 */
	GPIO_InitStructure.GPIO_Pin = LMX2581_LE_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	SPI2_Config();
	GPIO_SetBits(LMX2581_MX_GPIO,LMX2581_MX_PIN);
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	GPIO_ResetBits(LMX2581_DATA_GPIO,LMX2581_DATA_PIN);
	GPIO_ResetBits(LMX2581_CLK_GPIO,LMX2581_CLK_PIN);
#endif
	lmx2581_open();//使能芯片
	lmx2581_init();//寄存器初始化
	lmxFrequency=ctl_lmx2581_getFrequency();
}

/*
 * 函数名：ctl_lmx2581_setFrequency
 * 描述  ：设置芯片工作频率
 * 输入  ：frequency频率940~2400Mhz
 * 输出  : 正常返回1.异常返回0
 * 调用  ：外部调用
 */

uint8_t ctl_lmx2581_setFrequency(uint16_t frequency)
{
	uint8_t plln;
	uint16_t rem;
	static uint8_t preMUX=0;
	if(frequency<940 || frequency>2400)
	{
		printf("lmx2581.frequency is not between 940~2400M \n");
		return 0;
	}
	lmxFrequency=frequency;
	if(frequency<1880)//频率过低需要分频输出
	{//OUT=FVCO/2
		frequency=frequency*2;
		plln=frequency/100;
		rem=(frequency-plln*100)*10;
		regR0.PLL_N=plln;
		regR0.PLL_NUM_L=rem;
		lmx2581_write(&regR0,4);
		
		if(preMUX==0)//上次没有分频
		{
			regR5.OUTB_MUX=1;
			regR5.OUTA_MUX=1;	
			lmx2581_write(&regR5,4);	
			preMUX=1;
		}	
	}
	else
	{//FVCO=晶振~100MHz/PLL_R~1)*(PLL_N+PLL_NUM/PLL_DEN~1000)
		plln=frequency/100;
		rem=(frequency-plln*100)*10;
		regR0.PLL_N=plln;
		regR0.PLL_NUM_L=rem;
		lmx2581_write(&regR0,4);
		
		if(preMUX>0)//上次有分频
		{
			regR5.OUTB_MUX=0;
			regR5.OUTA_MUX=0;	
			lmx2581_write(&regR5,4);
			preMUX=0;
		}			
	}
	//printf(" \n  PLL_NUM=%d \n",regR0.PLL_NUM_L);
	return 1;
}
/*
 * 函数名：ctl_lmx2581_getFrequency
 * 描述  ：获取芯片工作频率
 * 输入  ：无
 * 输出  : 正常返回频率(MHz).异常返回0
 * 调用  ：外部调用
 */

uint16_t ctl_lmx2581_getFrequency(void)
{
	uint16_t fre;
	fre=regR0.PLL_N*100+regR0.PLL_NUM_L/10;
	if(regR5.OUTB_MUX)//分频了
	{
		return fre/2;
	}
	else
	{
		return fre;
	}
}

/*
 * 函数名：ctl_frequency_set
 * 描述  ：设置系统工作频率
 * 输入  ：frequency频率400~700Mhz
 * 输出  : 正常返回1.异常返回0
 * 调用  ：外部调用
 */
#define A7190_Channel LRWPAN_DEFAULT_START_CHANNEL
uint8_t ctl_frequency_set(uint16_t frequency)
{
	uint16_t afre;
	if(frequency<400 || frequency>700)
	{
		printf("frequency is not between 400~700M \n");
		return 0;
	}
	afre=100*A7190_Channel/200+2400;//A7190工作频率
	ctl_lmx2581_setFrequency(afre-frequency);
	//printf("#%u: freq=%uMHz\r\n",halGetMACTimer(),frequency);
	fprintf(stderr,"ZZIF#%u: freq=%uMHz@\r\n",halGetMACTimer(),frequency);
	return 1;
}

