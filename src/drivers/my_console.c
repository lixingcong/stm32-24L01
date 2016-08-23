/*
 *2016/07/13 lxc 2.0
 *
 */

/*
 V0.2 added PC-based binding         21/July/2006
 V0.1 Initial Release                10/July/2006
 */

#include "compiler.h"               //compiler specific
#include "hal.h"
#include "halStack.h"
#include "lrwpan_config.h"
#include "console.h"
#include "msstate_lrwpan.h"

//utility print functions that do not use printf and expect ROM strings
//these assume that constants and strings are stored in code memory

void conPCRLF(void) {
	printf("\r\n");
}

void conPrintROMString_func(const ROMCHAR *s) {
	printf("%s",s);
}

void conPrintString(const char *s) {
    printf("%s",s);
}

void conPrintUINT8_noleader(const UINT8 x) {
	if(x>16)
		printf("%2x",x);
	else
		printf("0%x",x);
}

void conPrintStringLength(const char *s,const BYTE length) {
	unsigned char len=length;
	while (len--)
		putchar(*s++);
}

void conPrintUINT8(const UINT8 x) {
	conPrintROMString("0x");
	conPrintUINT8_noleader(x);
}

void conPrintUINT16(const UINT16 x) {
	BYTE c;
	conPrintROMString("0x");
	c = (x >> 8);
	conPrintUINT8_noleader(c);
	c = (BYTE) x;
	conPrintUINT8_noleader(c);
}
//
void conPrintUINT32(const UINT32 x) {
	BYTE c;
	conPrintROMString("0x");
	c = (x >> 24);
	conPrintUINT8_noleader(c);
	c = (x >> 16);
	conPrintUINT8_noleader(c);
	c = (x >> 8);
	conPrintUINT8_noleader(c);
	c = x;
	conPrintUINT8_noleader(c);
}
//
//assumed little endian
void conPrintLADDR_bytes(const BYTE *ptr) {
	char i;
	conPrintROMString("0x");
	for (i = 8; i != 0; i--) {
		conPrintUINT8_noleader(*(ptr + i - 1));
	}
}
void conPrintLADDR(const LADDR *laddr) {
	const BYTE *ptr;

	ptr = &laddr->bytes[0];
	conPrintLADDR_bytes(ptr);
}

void conPrintConfig(void) {
	BYTE b[8];

	conPrintROMString("MSState LRWPAN Version ");
	conPrintROMString(LRWPAN_VERSION)
	conPCRLF();
#ifdef LRWPAN_COORDINATOR
	conPrintROMString("Coordinator, ");
#endif
#ifdef LRWPAN_ROUTER
	conPrintROMString("Router, ");
#endif
#ifdef LRWPAN_RFD
	conPrintROMString("RFD, ");
#endif
	conPrintROMString("Address: ");
	halGetProcessorIEEEAddress(b);
	conPrintLADDR_bytes(b);
	conPCRLF();
	conPrintROMString("Default PAN: ");
	conPrintUINT32(LRWPAN_DEFAULT_PANID);
	conPrintROMString(",Default Channel: ");
	conPrintUINT8(LRWPAN_DEFAULT_START_CHANNEL);
	conPCRLF();
	conPCRLF();
}
