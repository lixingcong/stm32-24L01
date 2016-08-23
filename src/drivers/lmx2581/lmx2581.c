#include "lmx2581.h"
#include "stdio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_rcc.h"

// Description:LMX2581驱动程序
// File Name:lmx2581.c
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

struct R0 regR0;
struct R1 regR1;
struct R2 regR2;
struct R3 regR3;
struct R4 regR4;
struct R5 regR5;
struct R6 regR6;
struct R7 regR7;
struct R8_9_10 regR8,regR9,regR10;
struct R13 regR13;
struct R15 regR15;

/*
 * 函数名：lmx2581_gpio_init
 * 描述  ：配置与芯片相连的引脚
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void lmx2581_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_lmx2581;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_lmx2581.GPIO_Pin=LMX2581_DATA_PIN;//DATA----B15
	GPIO_lmx2581.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_lmx2581.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(LMX2581_DATA_GPIO,&GPIO_lmx2581);
	
	GPIO_lmx2581.GPIO_Pin=LMX2581_CLK_PIN;//CLK----B13
	GPIO_Init(LMX2581_CLK_GPIO,&GPIO_lmx2581);
	
	GPIO_lmx2581.GPIO_Pin=LMX2581_LE_PIN;//LE----B8
	GPIO_Init(LMX2581_LE_GPIO,&GPIO_lmx2581);
	
	GPIO_lmx2581.GPIO_Pin=LMX2581_CE_PIN;//CE----B12
	GPIO_Init(LMX2581_CE_GPIO,&GPIO_lmx2581);	
	
	GPIO_lmx2581.GPIO_Pin=LMX2581_MX_PIN;//MUXout----B14
	GPIO_lmx2581.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(LMX2581_MX_GPIO,&GPIO_lmx2581);
	
	GPIO_SetBits(LMX2581_MX_GPIO,LMX2581_MX_PIN);
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	GPIO_ResetBits(LMX2581_DATA_GPIO,LMX2581_DATA_PIN);
	//GPIO_ResetBits(LMX2581_CLK_GPIO,LMX2581_CLK_PIN);
	GPIO_SetBits(LMX2581_CLK_GPIO,LMX2581_DATA_PIN);
}
/*
 * 函数名：lmx2581_init
 * 描述  ：配置寄存器的初始值,并初始化寄存器
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void lmx2581_init(void)
{
	uint32_t reg;
	reg=0x40870015;
	lmx2581_write(&reg,4);//配置R5
	LMXDelay(0x00FFFF);
	
	regR15.reserve_A=0x10FF;
	regR15.VCO_CAP_MAN=0;
	regR15.VCO_CAPCODE=128;
	regR15.addr=15;
	lmx2581_write(&regR15,4);
	
	regR13.DLD_ERR_CNT=0x4;
	regR13.DLD_PASS_CNT=0x020;
	regR13.DLD_TOL=0x5;
	regR13.reserve_A=0x410;
	regR13.addr=13;
	lmx2581_write(&regR13,4);
	
	regR10.reserve_A=0x210050C;
	regR10.addr=10;
	lmx2581_write(&regR10,4);
	
	regR9.reserve_A=0x03C7C03;
	regR9.addr=9;
	lmx2581_write(&regR9,4);
	
	regR8.reserve_A=0x207DDBF;
	regR8.addr=8;
	lmx2581_write(&regR8,4);
	
	regR7.reserve_A=0;
	regR7.FL_SELECT=0;
	regR7.FL_PINMODE=0;
	regR7.FL_INV=0;
	regR7.MUXOUT_SELECT=0x5;
	regR7.MUX_INV=0;
	regR7.MUXOUT_PINMODE=0x01;
	regR7.LD_SELECT=0x03;
	regR7.LD_INV=0;
	regR7.LD_PINMODE=0x1;
	regR7.addr=7;
	lmx2581_write(&regR7,4);
	
	regR6.reserve_A=0;
	regR6.RD_DIAGNOSTICS=0;
	regR6.reserve_B=2;
	regR6.RDADDR=0x6;
	regR6.uWIRE_LOCK=0;
	regR6.addr=6;
	lmx2581_write(&regR6,4);
	
	regR5.reserve_A=0;
	regR5.OUT_LDEN=0;
	regR5.OSC_FREQ=1;
	regR5.BUFEN_DIS=0;
	regR5.reserve_B=0;
	regR5.VCO_SEL_MODE=0;
	//OUTx_MUX=0直接输出,OUTB_MUX=1,out=fVCO/VCO_DIV
	regR5.OUTB_MUX=0;//**
	regR5.OUTA_MUX=0;//**
	regR5.ZERO_DLY=0;
	regR5.MODE=0;
	regR5.PWDN_MODE=0;
	regR5.RESET=0;
	regR5.addr=5;
	lmx2581_write(&regR5,4);
	
	regR4.PFD_DLY=0;
	regR4.FL_FRCE=0;
	regR4.FL_TOC=0;
	regR4.FL_CPG=0;
	regR4.reserve_A=0;
	regR4.CPG_BLEED=0;
	regR4.addr=4;
	lmx2581_write(&regR4,4);
	
	regR3.reserve_A=0x040;
	regR3.VCO_DIV=0;//二分频
	regR3.OUTB_PWR=14;
	regR3.OUTA_PWR=14;
	regR3.OUTB_PD=0;
	regR3.OUTA_PD=0;
	regR3.addr=3;
	lmx2581_write(&regR3,4);
	
	regR2.reserve_A=0;
	regR2.OSC_2X=0;
	regR2.reserve_B=0;
	regR2.CPP=1;
	regR2.reserve_C=1;
	//regR2.PLL_DEN=0x3D0900;//4000000
	regR2.PLL_DEN=1000;//**
	regR2.addr=2;
	lmx2581_write(&regR2,4);
	
	regR1.CPG=24;
	regR1.VCO_SEL=2;
	regR1.PLL_NUM_H=0;
	regR1.FRAC_ORDER=2;
	regR1.PLL_R=1;
	regR1.addr=1;
	lmx2581_write(&regR1,4);
	
	regR0.ID=0;
	regR0.FRAC_DITHER=2;
	regR0.NO_FCAL=0;
	regR0.PLL_N=20;//**
	regR0.PLL_NUM_L=0;//**
	regR0.addr=0;
	lmx2581_write(&regR0,4);
	LMXDelay(0x00FFF);	
	/*The VCO frequency range is from 1880 to 3760 MHz*/	
}
/*
 * 函数名：lmx2581_open
 * 描述  ：使能选择芯片
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void lmx2581_open(void)
{
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	GPIO_SetBits(LMX2581_CE_GPIO,LMX2581_CE_PIN);	
}
/*
 * 函数名：lmx2581_close
 * 描述  ：取消芯片使能
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void lmx2581_close(void)
{
	GPIO_ResetBits(LMX2581_CE_GPIO,LMX2581_CE_PIN);
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
}
/*
 * 函数名：lmx2581_read
 * 描述  ：读芯片寄存器
 * 输入  ：pos寄存器地址,buffer读取到的值,size大小
 * 输出  : 成功返回4,异常返回其他的值
 * 调用  ：外部调用
 */
rt_size_t lmx2581_read(rt_off_t pos, void* buffer, rt_size_t size)
{
	rt_int8_t cnt;
	rt_uint32_t reg=0;
	
	if(!buffer)
		return 0;
	
	if(size!=4)
		return 0;
	
	if(pos==14||pos==11||pos==12||pos>15||pos<0)
		return 0;
	
	regR6.RDADDR=pos;
	if(lmx2581_write(&regR6,4)!=4)
		return 0;
	
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	
	for(cnt=31;cnt>=0;cnt--)
	{
		LMX2581_CLK_GPIO->ODR|=LMX2581_CLK_PIN;
		LMX2581_CLK_GPIO->ODR;
		LMX2581_CLK_GPIO->IDR;
		LMX2581_CLK_GPIO->ODR&=~LMX2581_CLK_PIN;
		if((LMX2581_DATA_GPIO->IDR)&LMX2581_DATA_PIN)
			reg|=(1<<cnt);
		else
			reg&=(~1<<cnt);
		
		LMX2581_LE_GPIO->ODR;
		LMX2581_LE_GPIO->IDR;
	}
	GPIO_ResetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	
	*(int*)buffer=reg;
	
	return 4;
}
/*
 * 函数名：lmx2581_write
 * 描述  ：写芯片寄存器
 * 输入  ：pos寄存器地址,buffer读取到的值,size大小
 * 输出  : 成功返回4,异常返回其他的值
 * 调用  ：外部调用
 */

rt_size_t lmx2581_write(const void* buffer, rt_size_t size)
{
	rt_uint32_t reg;
	rt_int8_t cnt;
	cnt=cnt;
	
	if(!buffer)
		return 0;	
	
	if(size!=4)
		return 0;
	
	reg=*((rt_uint32_t*)buffer);
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;	
	GPIO_ResetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	
#ifndef USE_HSPI	
	for(cnt=31;cnt>=0;cnt--)
	{
		if(reg&(1<<cnt))
			LMX2581_DATA_GPIO->ODR|=LMX2581_DATA_PIN;
		else
			LMX2581_DATA_GPIO->ODR&=~LMX2581_DATA_PIN;
		
		LMX2581_LE_GPIO->ODR;
		LMX2581_LE_GPIO->IDR;
		
		LMX2581_CLK_GPIO->ODR|=LMX2581_CLK_PIN;
		LMX2581_CLK_GPIO->ODR;
		LMX2581_CLK_GPIO->IDR;
		LMX2581_CLK_GPIO->ODR;
		LMX2581_CLK_GPIO->IDR;
		LMX2581_CLK_GPIO->ODR;
		LMX2581_CLK_GPIO->IDR;
		LMX2581_CLK_GPIO->ODR;
		LMX2581_CLK_GPIO->IDR;
		LMX2581_CLK_GPIO->ODR&=~LMX2581_CLK_PIN;
	}
#else
	while(SPI_I2S_GetFlagStatus(LMX_SPI,SPI_I2S_FLAG_TXE)==RESET);//等待发送完成
	SPI_I2S_SendData(LMX_SPI,(reg>>16)&0xFFFF);	
	while(SPI_I2S_GetFlagStatus(LMX_SPI,SPI_I2S_FLAG_TXE)==RESET);//等待发送完成
	SPI_I2S_SendData(LMX_SPI,reg&0xFFFF);
	while(SPI_I2S_GetFlagStatus(LMX_SPI,SPI_I2S_FLAG_TXE)==RESET);//等待发送完成
	LMXDelay(0x98);//根据
#endif
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	GPIO_SetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	LMX2581_LE_GPIO->ODR;
	LMX2581_LE_GPIO->IDR;
	GPIO_ResetBits(LMX2581_LE_GPIO,LMX2581_LE_PIN);
	
	return 4;
}

void LMXDelay(__IO u32 nCount)	 //简单的延时函数
{
	for(; nCount != 0; nCount--);
} 


