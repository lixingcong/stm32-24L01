#ifndef NRF24L01_H_
#define NRF24L01_H_

// BYTE type definition
#ifndef _BYTE_DEF_
#define _BYTE_DEF_
typedef unsigned char BYTE;
#endif   /* _BYTE_DEF_ */

#define NRF_ADDR_WIDTH    4   //  TX(RX) address width
#define NRF_PLOAD_WIDTH  32  // 32bytes TX payload

#define NRF_READ_REG        0x00  // Define read command to register
#define NRF_WRITE_REG       0x20  // Define write command to register
#define NRF_RD_RX_PLOAD     0x61   // Define RX payload register address
#define NRF_WR_TX_PLOAD     0xA0   // Define TX payload register address
#define NRF_FLUSH_TX        0xE1   // Define flush TX register command
#define NRF_FLUSH_RX        0xE2   // Define flush RX register command
#define NRF_REUSE_TX_PL     0xE3   // Define reuse TX payload register command
#define NRF_NOP             0xFF   // Define No Operation, might be used to read status register

//***************************************************//
// SPI(nRF24L01) registers(addresses)
#define NRF_CONFIG          0x00   // 'Config' register address
#define NRF_EN_AA           0x01   // 'Enable Auto Acknowledgment' register address
#define NRF_EN_RXADDR       0x02   // 'Enabled RX addresses' register address
#define NRF_SETUP_AW        0x03   // 'Setup address width' register address
#define NRF_SETUP_RETR      0x04   // 'Setup Auto. Retrans' register address
#define NRF_RF_CH           0x05   // 'RF channel' register address
#define NRF_RF_SETUP        0x06   // 'RF setup' register address
#define NRF_STATUS          0x07  // 'Status' register address
#define NRF_OBSERVE_TX      0x08  // 'Observe TX' register address
#define NRF_CD              0x09  // 'Carrier Detect' register address
#define NRF_RX_ADDR_P0      0x0A  // 'RX address pipe0' register address
#define NRF_RX_ADDR_P1      0x0B  // 'RX address pipe1' register address
#define NRF_RX_ADDR_P2      0x0C  // 'RX address pipe2' register address
#define NRF_RX_ADDR_P3      0x0D  // 'RX address pipe3' register address
#define NRF_RX_ADDR_P4      0x0E  // 'RX address pipe4' register address
#define NRF_RX_ADDR_P5      0x0F  // 'RX address pipe5' register address
#define NRF_TX_ADDR         0x10  // 'TX address' register address
#define NRF_RX_PW_P0        0x11  // 'RX payload width, pipe0' register address
#define NRF_RX_PW_P1        0x12  // 'RX payload width, pipe1' register address
#define NRF_RX_PW_P2        0x13  // 'RX payload width, pipe2' register address
#define NRF_RX_PW_P3        0x14  // 'RX payload width, pipe3' register address
#define NRF_RX_PW_P4        0x15  // 'RX payload width, pipe4' register address
#define NRF_RX_PW_P5        0x16  // 'RX payload width, pipe5' register address
#define NRF_FIFO_STATUS     0x17  // 'FIFO Status Register' register address

extern unsigned char rx_buf[NRF_PLOAD_WIDTH];

void NRF_Send_Data(BYTE* data_buffer, BYTE Nb_bytes);
void NRF24L01_Init(void);
void NRF_RX_Mode(void);
void NRF_TX_Mode(void);
unsigned char NRF_SPI_Read(BYTE reg);
unsigned char NRF_SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);
unsigned char NRF_SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);
unsigned char NRF_SPI_RW_Reg(BYTE data1, BYTE data2);
unsigned char NRF_check_if_exist(void);

#endif
