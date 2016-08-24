#ifndef A7190_H
#define A7190_H
#include "A7190reg.h"
#include "define.h"
#include "delay.h"
#include "stm32f10x_gpio.h"


#define CS_ENABLE()  GPIO_ResetBits(GPIOA,GPIO_Pin_4);__NOP();__NOP();__NOP()
#define CS_DISABLE() GPIO_SetBits(GPIOA,GPIO_Pin_4)
#define RF_WTR       GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8)
typedef enum{
	 BUSY_TX = 0x01,
	 IDLE = 0x02,
	 BUSY_RX = 0x03,
	WAIT_TX = 0x04,
	LOCK_A7190= 0x05
}rtx_state_t;
rtx_state_t A7190_read_state(void);
void A7190_WriteReg_Page(Uint8, Uint8 , Uint8 );
void A7190_WriteReg(Uint8, Uint8);
Uint8 A7190_ReadReg(Uint8);
void StrobeCmd(Uint8 src);
Uint8 ByteSend(Uint8 src);
void A7190_Reset(void);
void A7190_WriteID(void);
void A7190_ReadID(Uint8 *ID);
Uint8 A7190_ReadRSSI(void);
void initRF(void);
void A7190_KeyData(void);
void A7190_FCB(void);
void A7190_Config(void);
void A7190_Cal(void);

void WriteFIFO(Uint8 *txbuf,Uint16 length);
void WriteFIFO1(Uint8 buf);

void ReadFIFO(Uint8 *RfBuf,Uint16 length);
Uint8 ReadFIFO1(Uint8 length);

void Set_FIFO_len(Uint8 length_low,Uint8 length_high);
void A7190_set_state(rtx_state_t state);
void A7190_SetFrequency(Uint8 channel);

Uint32 RxPacket(Uint8 *tmpbuf,int length);
#endif
