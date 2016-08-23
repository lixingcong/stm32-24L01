#ifndef __LMX2581_REG_H_
#define	__LMX2581_REG_H_

// Description:LMX2581驱动程序
// File Name:lmx2581.h
// Author:彭朋
// Date:2016-8-2
// Encoding:UTF-8

#include "stm32f10x.h"
#include "rtdef.h"
/*
struct RX
{
rt_uint32_t D0:8;		//Data[7:0]
	rt_uint32_t D1:8;	//Data[15:8]
	rt_uint32_t D2:8;	//Data[23:16]
	rt_uint32_t D3:8;	//Data[31:24]
};*/
struct R0
{
	rt_uint32_t addr:4;
	rt_uint32_t PLL_NUM_L:12;
	rt_uint32_t PLL_N:12;
	rt_uint32_t NO_FCAL:1;
	rt_uint32_t FRAC_DITHER:2;
	rt_uint32_t ID:1; 
};

struct R1
{
	rt_uint32_t addr:4;
	rt_uint32_t PLL_R:8;
	rt_uint32_t FRAC_ORDER:3;
	rt_uint32_t PLL_NUM_H:10;
	rt_uint32_t VCO_SEL:2;
	rt_uint32_t CPG:5;
};

struct R2
{
	rt_uint32_t addr:4;
	rt_uint32_t PLL_DEN:22;
	rt_uint32_t reserve_C:1;
	rt_uint32_t CPP:1;
	rt_uint32_t reserve_B:1;
	rt_uint32_t OSC_2X:1;
	rt_uint32_t reserve_A:2;
};

struct R3
{
	rt_uint32_t addr:4;
	rt_uint32_t OUTA_PD:1;
	rt_uint32_t OUTB_PD:1;
	rt_uint32_t OUTA_PWR:6;
	rt_uint32_t OUTB_PWR:6;
	rt_uint32_t VCO_DIV:5;
	rt_uint32_t reserve_A:9;
};

struct R4
{
	rt_uint32_t addr:4;
	rt_uint32_t CPG_BLEED:6;
	rt_uint32_t reserve_A:1;
	rt_uint32_t FL_CPG:5;
	rt_uint32_t FL_TOC:12;
	rt_uint32_t FL_FRCE:1;
	rt_uint32_t PFD_DLY:3;
};

struct R5
{
	rt_uint32_t addr:4;
	rt_uint32_t RESET:1;
	rt_uint32_t PWDN_MODE:3;
	rt_uint32_t MODE:2;
	rt_uint32_t ZERO_DLY:1;
	rt_uint32_t OUTA_MUX:2;
	rt_uint32_t OUTB_MUX:2;
	rt_uint32_t VCO_SEL_MODE:2;
	rt_uint32_t reserve_B:3;
	rt_uint32_t BUFEN_DIS:1;
	rt_uint32_t OSC_FREQ:3;
	rt_uint32_t OUT_LDEN:1;	
	rt_uint32_t reserve_A:7;
};

struct R6
{
	rt_uint32_t addr:4;
	rt_uint32_t uWIRE_LOCK:1;
	rt_uint32_t RDADDR:4;
	rt_uint32_t reserve_B:2;
	rt_uint32_t RD_DIAGNOSTICS:20;	
	rt_uint32_t reserve_A:1;
};

struct R7
{
	rt_uint32_t addr:4;
	rt_uint32_t LD_PINMODE:3;
	rt_uint32_t LD_INV:1;
	rt_uint32_t LD_SELECT:5;
	rt_uint32_t MUXOUT_PINMODE:3;
	rt_uint32_t MUX_INV:1;
	rt_uint32_t MUXOUT_SELECT:5;
	rt_uint32_t FL_INV:1;
	rt_uint32_t FL_PINMODE:3;
	rt_uint32_t FL_SELECT:5;
	rt_uint32_t reserve_A:1;
};

struct R8_9_10
{
	rt_uint32_t addr:4;
	rt_uint32_t reserve_A:28;
};

struct R13
{
	rt_uint32_t addr:4;
	rt_uint32_t reserve_A:11;
	rt_uint32_t DLD_TOL:3;
	rt_uint32_t DLD_PASS_CNT:10;
	rt_uint32_t DLD_ERR_CNT:4;
};

struct R15
{
	rt_uint32_t addr:4;
	rt_uint32_t VCO_CAPCODE:8;
	rt_uint32_t VCO_CAP_MAN:1;
	rt_uint32_t reserve_A:19;
};

#endif /* __LMX2581_REG_H_ */
