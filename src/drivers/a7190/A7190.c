/*********************************************************************
**  Device: A7130                                                   **
**  File:   main.c                                                  **
**                                                                  **
**  Description:                                                    **
**  This file is a sample code for your reference.                  **
**  Data Rate:4MHz                                                  ** 
**                                                                  **
**  Copyright (C) 2011 AMICCOM Corp.                                **
**                                                                  **
*********************************************************************/
#include "A7190.h"
#include "stm32f10x_spi.h"

 static  rtx_state_t rtx_state;
Uint8			RfBuf[64];
Uint8			TmpUint8;
Uint32   j1=0,j2=0;
Uint8 tmp;

const Uint8 BitCount_Tab[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
const Uint8  KeyData_Tab[16]={0x00,0x00,0x00,0x00,0x00,0x0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //keyData code
const Uint8  FCB_Tab[20]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //FCB code
const Uint8  PageTab[11]={0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0};//page select

//-----------------------------------------------------------------------------
// 64 bytes PN9 pseudo random code
//-----------------------------------------------------------------------------
 Uint8  PN9_Tab[]=
{   0xFF,0x83,0xDF,0x17,0x32,0x09,0x4E,0xD1,
    0xE7,0xCD,0x8A,0x91,0xC6,0xD5,0xC4,0xC4,
    0x40,0x21,0x18,0x4E,0x55,0x86,0xF4,0xDC,
    0x8A,0x15,0xA7,0xEC,0x92,0xDF,0x93,0x53,
    0x30,0x18,0xCA,0x34,0xBF,0xA2,0xC7,0x59,
    0x67,0x8F,0xBA,0x0D,0x6D,0xD8,0x2D,0x7D,
    0x54,0x0A,0x57,0x97,0x70,0x39,0xD2,0x7A,
    0xEA,0x24,0x33,0x85,0xED,0x9A,0x1D,0xE0
};
//-----------------------------------------------------------------------------
// RF ID code
//-----------------------------------------------------------------------------
// const Uint8 ID_Tab[8]=
// {
//     0x54, 0x75, 0xC5, 0x2A
// }; //ID code
#ifdef LRWPAN_COORDINATOR
const Uint8 ID_Tab[8]={0x34,0x75,0xC5,0x2A,0xC7,0x33,0x45,0xEA}; //ID code
#else
const Uint8 ID_Tab[8]={0x34,0x75,0xC5,0x2A,0xC7,0x33,0x45,0xF0}; //ID code
#endif
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                                   NOTE                                    !!
// !!         THIS CONFIG TABLE ONLY USE ON RF CRYSTAL = 16MHz               !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const Uint8  A7190Config[] =
{
    //      address   name              Descript
    //      -------   ----              ---------
    0x00,   //0x00  ; MODE_REG
    0x62,   //0x01  ; MODECTRL_REG	FIFO mode, Enable ARSSI, Enable AIF
    0x00,   //0x02  ; CALIBRATION_REG
    0x3F,   //0x03  ; FIFO1_REG 	FIFO end = 63+1
    0x00,   //0x04  ; FIFO2_REG
    0x00,   //0x05  ; FIFO_REG
    0x00,   //0x06  ; IDCODE_REG
    0x00,   //0x07  ; RCOSC1_REG
    0x00,   //0x08  ; RCOSC2_REG
    0x14,   //0x09  ; RCOSC3_REG
    0x00,   //0x0A  ; CKO_REG
    0x19,   //0x0B  ; GIO1 register,    SDO
    0x01,   //0x0C  ; GIO2 register,    WTR       GPIO1_REG ; GPIO2_REG
    0x1F,   //0x0D  ; DATARATE_REG
    0x5A,   //0x0E  ; PLL1_REG
    0x0E,   //0x0F  ; PLL2_REG
    0x96,   //0x10  ; PLL3_REG
    0x00,   //0x11  ; PLL4_REG
    0x04,   //0x12  ; PLL5_REG
    0x3C,   //0x13  ; CHGROUP1_REG
    0x78,   //0x14  ; CHGROUP2_REG
    0x2F,   //0x15  ; TX1_REG
    0x40,   //0x16  ; TX2_REG
    0x18,   //0x17  ; DELAY1_REG
    0x40,   //0x18  ; DELAY2_REG
    0x70,   //0x19  ; RX_REG
    0xFC,   //0x1A  ; RXGAIN1_REG
    0xC0,   //0x1B  ; RXGAIN2_REG
    0xFC,   //0x1C  ; RXGAIN3_REG
    0xCA,   //0x1D  ; RXGAIN4_REG
    0x00,   //0x1E  ; RSSI_REG
    0xF1,   //0x1F  ; ADC_REG
    0x0C,   //0x20  ; CODE1_REG
    0x0F,   //0x21  ; CODE2_REG
    0x2A,   //0x22  ; CODE3_REG
    0xE5,   //0x23  ; IFCAL1_REG
    0x01,   //0x24  ; IFCAL2_REG
    0x8F,   //0x25  ; VCOCCAL_REG
    0xD0,   //0x26  ; VCOCAL1_REG
    0x80,   //0x27  ; VCOCAL2_REG
    0x70,   //0x28  ; VCODEVCAL1_REG
    0x36,   //0x29  ; VCODEVCAL2_REG
    0x00,   //0x2A  ; DASP_REG
    0xFF,   //0x2B  ; VCOMODDELAY_REG
    0x50,   //0x2C  ; BATTERY_REG
    0xD0,   //0x2D  ; TXTEST_REG
    0x57,   //0x2E  ; RXDEM1_REG
    0x74,   //0x2F  ; RXDEM2_REG
    0xF3,   //0x30  ; CPC1_REG
    0x33,   //0x31  ; CPC2_REG
    0xCD,   //0x32  ; CRYSTALTEST_REG
    0x15,   //0x33  ; PLLTEST_REG
    0x09,   //0x34  ; VCOTEST_REG
    0x00,   //0x35  ; RFANALOG_REG
    0x00,   //0x36  ; KEYDATA_REG
    0x77,   //0x37  ; CHSELECT_REG
    0x00,   //0x38  ; ROMP_REG
    0x00,   //0x39  ; DATARATECLOCK
    0x00,   //0x3A  ; FCR_REG
    0x00,   //0x3B  ; ARD_REG
    0x00,   //0x3C  ; AFEP_REG
    0x00,   //0x3D  ; FCB_REG
    0x00,   //0x3E  ; KEYC_REG
    0x00    //0x3F  ; USID_REG
};
const Uint8  A7190_Addr2A_Config[]=
{
	0xF7, //page0,
	0x51, //page1,
	0xF0, //Page2,
	0x80, //page3,
	0x80, //page4,
	0x08, //page5,
	0x00, //page6,
	0x00, //page7,
	0x00, //page8,
	0x3C, //page9,
	0x00, //pageA,
};
const Uint8  A7190_Addr38_Config[]=
{
	0x00, //page0,
	0x00, //page1,
	0x30, //page2,
	0xA4, //page3,
	0x20, //page4,
};
/*********************************************************************
**  function Declaration
*********************************************************************/
/*********************************************************************
** Err_State
*********************************************************************/
void Err_State(void)
{
    //ERR display
    //Error Proc...
    while(1)
    {
        ;
    }
}
/*********************************************************************
** A7190_WriteReg_Page
*********************************************************************/
void A7190_WriteReg_Page(Uint8 addr, Uint8 wbyte, Uint8 page)
{
	A7190_WriteReg(RFANALOG_REG, A7190Config[RFANALOG_REG] | PageTab[page]);//page select
	A7190_WriteReg(addr, wbyte);
}
/************************************************************************
**  A7190_WriteReg
************************************************************************/
void A7190_WriteReg(Uint8 addr, Uint8 dataByte)
{
    CS_ENABLE();
	  __NOP();
	  __NOP();
    addr |= 0x00; //bit cmd=0,r/w=0
    ByteSend(addr);
    //send data byte
    ByteSend(dataByte);
    CS_DISABLE();
}
/************************************************************************
**  A7190_ReadReg
************************************************************************/
Uint8 A7190_ReadReg(Uint8 addr)
{
    CS_ENABLE();
	  __NOP();
	  __NOP();
    
    addr |= 0x40; //bit cmd=0,r/w=1
    ByteSend(addr);
    //read data
    tmp=ByteSend(0xff);//0xff is dummy data
    CS_DISABLE();
    return tmp;
}
/*********************************************************************
** Strobe Command
*********************************************************************/
void StrobeCmd(Uint8 src)
{
    CS_ENABLE();
	   __NOP();
	  __NOP();
    ByteSend(src);
    CS_DISABLE();
}
/************************************************************************
**  ByteSend
************************************************************************/
Uint8 ByteSend(Uint8 src)
{
	  while((SPI1->SR & SPI_I2S_FLAG_TXE)==RESET);
	  SPI1->DR = src;
	  while((SPI1->SR & SPI_I2S_FLAG_RXNE)==RESET);
	  return SPI1->DR;
//     while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
//     SPI_I2S_SendData(SPI1,src);
//     while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_RXNE) == RESET);
//     return SPI_I2S_ReceiveData(SPI1);
}
/************************************************************************
**  WriteID
************************************************************************/
void A7190_WriteID(void)
{
    Uint8 i;
    Uint8 addr;

    addr = IDCODE_REG; //send address 0x06, bit cmd=0, r/w=0
    CS_ENABLE();
		__NOP();
	  __NOP();
    ByteSend(addr);
    for (i=0; i < 8; i++)
        ByteSend(ID_Tab[i]);
    CS_DISABLE();    
}
void A7190_ReadID(Uint8 *ID)
{
		Uint8 i;
    Uint8 addr;

    addr = IDCODE_REG|0x40;
		CS_ENABLE();
	  	  __NOP();
	  __NOP();
    ByteSend(addr);
    for (i=0; i < 8; i++)
        *(ID++)=ByteSend(0xff);
    CS_DISABLE();
}
Uint8 A7190_ReadRSSI(void)
{
		Uint8 i;
    Uint8 addr;

    addr = RSSI_REG|0x40;
		CS_ENABLE();
	  __NOP();
	  __NOP();
    ByteSend(addr);
    for (i=0; i < 1; i++)
        i=ByteSend(0xff);
    CS_DISABLE();
	return i;
}
/************************************************************************
**  Reset_RF
************************************************************************/
void A7190_Reset(void)
{
    A7190_WriteReg(MODE_REG, 0x00); //reset RF chip
}
/*********************************************************************
** CHGroupCal
*********************************************************************/
void CHGroupCal(Uint8 ch)
{
   
    Uint8 tmp;
    Uint8 vb,vbcf;
    Uint8 deva,adev;

    A7190_WriteReg(PLL1_REG, ch);

    A7190_WriteReg(CALIBRATION_REG, 0x0C);
    do
    {
    	    tmp = A7190_ReadReg(CALIBRATION_REG);
    	    tmp &= 0x0C;
    }while (tmp);
    
    //for check
    tmp = A7190_ReadReg(VCOCAL1_REG);
    vb = tmp & 0x07;
    vbcf = (tmp >>3) & 0x01;
    
    tmp = A7190_ReadReg(VCODEVCAL1_REG);
    deva = tmp;
    
    tmp = A7190_ReadReg(VCODEVCAL2_REG);
    adev = tmp;
    
    if (vbcf)
        Err_State();
}
/*********************************************************************
** calibration
*********************************************************************/
void A7190_Cal(void)
{
		Uint8 tmp;
    Uint8 fb, rh, rl;

    //==================================================
    // calibration FBC, RCC, RSSC
    StrobeCmd( CMD_PLL );
    A7190_WriteReg( RX_REG, 0x30 );
    A7190_WriteReg( CALIBRATION_REG, 0x23 );
    do
    {
        tmp = A7190_ReadReg( CALIBRATION_REG );
    } while ( tmp & 0x23 );

    //calibration VBC,VDC procedure
    CHGroupCal(30); //calibrate channel group Bank I
    CHGroupCal(90); //calibrate channel group Bank II
    CHGroupCal(150); //calibrate channel group Bank III
    StrobeCmd(CMD_STBY); //return to STBY state

    tmp = A7190_ReadReg( IFCAL1_REG );
    if ( tmp & 0x10)
        Err_State();
    fb = tmp & 0x0F;

    rh = A7190_ReadReg( RXGAIN2_REG );
    rl = A7190_ReadReg( RXGAIN3_REG );

    A7190_WriteReg( RX_REG, 0x70 );
}
/*********************************************************************
** A7190_Config
*********************************************************************/
void A7190_Config(void)
{
   	 Uint8 i;

    //0x00 mode register, for reset
    //0x05 fifo data register
    //0x06 id code register
    //0x3F USID register, read only
    //0x36 key data, 16 bytes
    //0x3D FCB register,4 bytes

    for(i=0x01; i<=0x02; i++)
        A7190_WriteReg(i, A7190Config[i]);
	    
    A7190_WriteReg(0x04, A7190Config[0x04]);
	
    for(i=0x07; i<=0x29; i++)
        A7190_WriteReg(i, A7190Config[i]);

    for(i=0; i<=10; i++)//0x2A DAS
	A7190_WriteReg_Page(0x2A, A7190_Addr2A_Config[i], i);

    for (i=0x2B; i<=0x34; i++)
	A7190_WriteReg(i, A7190Config[i]);
	
    A7190_WriteReg(0x35,0x00);
    A7190_WriteReg(0x37, A7190Config[0x37]);

    for (i=0; i<=4; i++)//0x38 ROM
        A7190_WriteReg_Page(0x38, A7190_Addr38_Config[i], i);

    for (i=0x39; i<=0x3C; i++)
        A7190_WriteReg(i, A7190Config[i]);
	
    A7190_WriteReg(0x3E, A7190Config[0x3E]);
    A7190_WriteReg(0x3F, A7190Config[0x3F]);
}
/*********************************************************************
** Write A7190_KeyData                                                   
*********************************************************************/ 
void A7190_KeyData(void)
{
    Uint8 i;
    Uint8 addr;

    addr = KEYDATA_REG; //send address 0x06, bit cmd=0, r/w=0
    CS_ENABLE();
    ByteSend(addr);
    for (i=0; i < 16; i++)
        ByteSend(KeyData_Tab[i]);
     CS_DISABLE();
}
/*********************************************************************
** Write A7190_FCB                                                   
*********************************************************************/ 
void A7190_FCB(void)
{
    Uint8 i;
    Uint8 addr;

    addr = FCB_REG; //send address 0x06, bit cmd=0, r/w=0
    CS_ENABLE();
    ByteSend(addr);
    for (i=0; i < 20; i++)
			ByteSend(FCB_Tab[i]);
     CS_DISABLE();
}
/*********************************************************************
** SetCH
*********************************************************************/
void SetCH(Uint8 ch)
{
	A7190_WriteReg(PLL1_REG, ch); //RF freq = RFbase + (CH_Step * ch)
}
/*********************************************************************
** initRF
*********************************************************************/
void initRF(void)
{
	  DelayMs(100);
    A7190_Reset(); //reset A7190 RF chip
	  DelayMs(100);
    A7190_WriteID(); //write ID code
    A7190_Config(); //config A7190 chip
    A7190_Cal(); //calibration IF,VCO,VCOC
		StrobeCmd( CMD_STBY );      // wakeup RFIC
		DelayUs(200);
	
    A7190_WriteReg( TXTEST_REG, 0xCB );     //  TX power = 20dBm
    A7190_WriteReg( DATARATECLOCK, 0x00 );      //  Data rate = 4M
    A7190_WriteReg( MODECTRL_REG, 0x62 );   //  FIFO mode
    // TODO: 计算PLL获得跳频改变发射频率，需要参考手册14章 2016年7月14日 上午10:00:16
    // 跳频实现方法：
    // 1 设置F_lo_base = 2400MHz，这个F_lo_base是有计算公式的，参考cp14.1
    // 2 设置F_chsp = 500KHz，这个F_chsp直接从PLL2_REG[4:1]设置
    // 3 设置F_offset = CHN[7:0] x F_chsp, 其中CHN寄存器就是下面的PLL1_REG1寄存器
    // 4 可以获得F_lo = F_lo_base + F_offset的发射频率
    A7190_WriteReg( PLL1_REG, LRWPAN_DEFAULT_START_CHANNEL);        // set Local Oscillator(本机振荡) channel

    A7190_WriteReg( CODE1_REG, 0x0F);       //  disable CRC check function
    // FIFO len 0x01ff=512Bytes
    Set_FIFO_len(0xff,0x01);
		StrobeCmd( CMD_SLEEP );     // let RF to sleep mode
		DelayUs(200);
		// 睡醒后接收数据这里需要仔细斟酌，之前是delayMs(10)
		StrobeCmd( CMD_STBY );      // wakeup RFIC
		DelayUs(200);
		StrobeCmd(CMD_RX);
		StrobeCmd(CMD_RFR);
	  A7190_set_state(IDLE);
}
//**********************************************************************
//  WriteFIFO
//**********************************************************************
void WriteFIFO(Uint8 *txbuf,Uint8 length)
{
    Uint16 i;
  // set FIFO length
                       // Reset TX FIFO point
    
    CS_ENABLE();
	  __NOP();
	  __NOP();
    ByteSend(FIFO_REG);

    for(i=0; i <length; i++)
	{
        ByteSend(*txbuf);
		txbuf++;
	}
    CS_DISABLE();
}
void Set_FIFO_len(Uint8 length_low,Uint8 length_high)
{
	  CS_ENABLE();
	  __NOP();
	  __NOP();
    ByteSend(FIFO1_REG);
	  ByteSend(length_low);	//low byte
		ByteSend(length_high);	//high byte
	  CS_DISABLE();
}
void WriteFIFO1(Uint8 buf)
{
	Uint16 i;
//	Uint8 buf;
	CS_ENABLE();
	ByteSend(FIFO_REG);
	//for(i=0; i <length; i++)
	//{
// 			//ByteSend(PN9_Tab[i]);
// 		 if( i>=256)
// 			 buf=(Uint8)(i+3)%256;
// 		 else
// 			 buf=(Uint8)i;
			ByteSend(buf);
//	}
  CS_DISABLE();
}
void WriteFIFO2(Uint16 length)
{
    Uint16 i;
	  Uint8 Num[8];
	  Uint8 temp[3];
	  CS_ENABLE();
	 // DelayUs(10);
	  __NOP();
	  __NOP();
	  ByteSend(FIFO_REG);
	  if(j2>5000000)
			j2=0;
    j2++;
		temp[0]=j2&0xFF;
 		temp[1]=(j2>>8)&0xFF;
 		temp[2]=(j2>>16)&0xFF;
    for(i=0;i<3;i++)
		  ByteSend(temp[i]);
    for(i=3; i <length; i++)
        ByteSend(PN9_Tab[i]);
    CS_DISABLE();
}
void ReadFIFO(Uint8 *RfBuf,Uint8 length)
{
    Uint16 i;

    CS_ENABLE();
    ByteSend(FIFO_REG | 0x40);
    for(i = 0; i <length; i++)
        RfBuf[i] = ByteSend(0xff);  //0xff is dummy data
    CS_DISABLE();
}
Uint8 ReadFIFO1(Uint8 length)
{
    Uint8 i;

    CS_ENABLE();
    ByteSend(FIFO_REG | 0x40);
  //  for(i = 0; i <length; i++)
        i = ByteSend(0xff);  //0xff is dummy data
    CS_DISABLE();
	return i;
}

Uint32 RxPacket(Uint8 *tmpbuf,int length)
{
    Uint8 i;
    Uint8 recv;
	  Uint32 error=0;

	for(i=0; i <length; i++)
	{
		recv = tmpbuf[i];
		if((recv ^ PN9_Tab[i])!=0)
    {
       error++;
    }
  }
	return error;
}
void A7190_SetFrequency(Uint8 channel)
{
	A7190_WriteReg( PLL1_REG, channel );
}
rtx_state_t A7190_read_state()
{
	return rtx_state;
}
void A7190_set_state(rtx_state_t state)
{
	rtx_state = state;
}
