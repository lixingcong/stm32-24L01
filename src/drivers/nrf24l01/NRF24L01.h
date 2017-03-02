#ifndef NRF24L01_H_
#define NRF24L01_H_

// BYTE type definition
#ifndef _BYTE_DEF_
#define _BYTE_DEF_
typedef unsigned char BYTE;
#endif   /* _BYTE_DEF_ */

#define NRF_PLOAD_LENGTH  32  // 32bytes TX payload

void NRF_Send_Data(BYTE* data_buffer, BYTE Nb_bytes);
void NRF24L01_Init(void);
void NRF_interupt_handler(void);

#endif
