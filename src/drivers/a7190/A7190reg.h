/********************************************************************************
* FILENAME  : A7190Reg.h                                                        *
*                                                                               *
* AUTHOR    : Ruemann (Amiccom)                                                 *
*                                                                               *
* DESCRIPTION   :                                                               *
*                                                                               *
*       A7190 register name and storbe command define                           *
*                                                                               *
*********************************************************************************
*  (c) Copyright 2008, AMICCOM                                                  *
*********************************************************************************/



#ifndef _A7190_REG_H_
#define _A7190_REG_H_

//===================================================================================
// A7190 register address
//===================================================================================
#define MODE_REG 		0x00
#define MODECTRL_REG		0x01
#define CALIBRATION_REG		0x02
#define FIFO1_REG 		0x03
#define FIFO2_REG 		0x04
#define FIFO_REG 		0x05
#define IDCODE_REG 		0x06
#define RCOSC1_REG 		0x07
#define RCOSC2_REG 		0x08
#define RCOSC3_REG 		0x09
#define CKO_REG 		0x0A
#define GPIO1_REG 		0x0B
#define GPIO2_REG 		0x0C
#define DATARATE_REG 		0x0D
#define PLL1_REG 		0x0E
#define PLL2_REG		0x0F
#define PLL3_REG 		0x10
#define PLL4_REG		0x11
#define PLL5_REG 		0x12
#define CHGROUP1_REG		0x13
#define CHGROUP2_REG		0x14
#define TX1_REG  		0x15
#define TX2_REG  		0x16
#define DELAY1_REG		0x17
#define DELAY2_REG		0x18
#define RX_REG			0x19
#define RXGAIN1_REG		0x1A
#define RXGAIN2_REG		0x1B
#define RXGAIN3_REG		0x1C
#define RXGAIN4_REG		0x1D
#define RSSI_REG		0x1E
#define ADC_REG  		0x1F
#define CODE1_REG 		0x20
#define CODE2_REG 		0x21
#define CODE3_REG 		0x22
#define IFCAL1_REG  		0x23
#define IFCAL2_REG  		0x24
#define VCOCCAL_REG  		0x25
#define VCOCAL1_REG  		0x26
#define VCOCAL2_REG  		0x27
#define VCODEVCAL1_REG      	0x28
#define VCODEVCAL2_REG      	0x29
#define DASP_REG 	 	0x2A
#define VCOMODDELAY_REG		0x2B
#define BATTERY_REG  		0x2C
#define TXTEST_REG  		0x2D
#define RXDEM1_REG  		0x2E
#define RXDEM2_REG  		0x2F
#define CPC1_REG		0x30
#define CPC2_REG		0x31
#define CRYSTALTEST_REG		0x32
#define PLLTEST_REG   		0x33
#define VCOTEST_REG 		0x34
#define RFANALOG_REG 		0x35
#define KEYDATA_REG 		0x36
#define CHSELECT_REG		0x37
#define ROMP_REG 		0x38
#define DATARATECLOCK		0x39
#define FCR_REG 		0x3A
#define ARD_REG 		0x3B
#define AFEP_REG 		0x3C
#define FCB_REG 		0x3D
#define KEYC_REG 		0x3E
#define USID_REG 		0x3F

//===================================================================================
// A7190 strobe command
//===================================================================================
#define CMD_SLEEP           0x80    //1000,xxxx SLEEP mode
#define CMD_IDLE            0x90    //1001,xxxx IDLE mode
#define CMD_STBY            0xA0    //1010,xxxx Standby mode
#define CMD_PLL             0xB0    //1011,xxxx PLL mode
#define CMD_RX              0xC0    //1100,xxxx RX mode
#define CMD_TX              0xD0    //1101,xxxx TX mode
#define CMD_TFR             0xE0    //1110,xxxx TX FIFO reset
#define CMD_RFR             0xF0    //1111,xxxx RX FIFO reset

#endif  // #ifndef  _A7190_REG_H_ 
