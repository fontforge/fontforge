#include "mem.h"

#include "inc/intl.h"

#include "uiinterface.h"

int32 memlong(uint8 *data,int len, int offset) {
	if (offset>=0 && offset+3<len) {
		int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
		return (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4;
	} else {
		LogError( _("Bad font, offset out of bounds.\n") );
		return 0;
	}
}

int memushort(uint8 *data,int len, int offset) {
	if (offset>=0 && offset+1<len) {
		int ch1 = data[offset], ch2 = data[offset+1];
		return (ch1<<8)|ch2;
	} else {
		LogError( _("Bad font, offset out of bounds.\n") );
		return 0;
	}
}

void memputshort(uint8 *data,int offset,uint16 val) {
	data[offset] = (val>>8);
	data[offset+1] = val&0xff;
}


int getushort(FILE *ttf) {
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);

	if (ch2==EOF)
		return EOF;

	return (ch1<<8)|ch2;
}

int get3byte(FILE *ttf) {
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);
	int ch3 = getc(ttf);

	if (ch3==EOF)
		return EOF;

	return (ch1<<16)|(ch2<<8)|ch3;
}

int32 getlong(FILE *ttf) {
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);
	int ch3 = getc(ttf);
	int ch4 = getc(ttf);

	if (ch4==EOF)
		return EOF;

	return (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4;
}

real getfixed(FILE *ttf) {
	int32 val = getlong(ttf);
	int mant = val&0xffff;
	/* This oddity may be needed to deal with the first 16 bits being signed */
	/*  and the low-order bits unsigned */
	return (real) (val>>16) + (mant/65536.0);
}

real get2dot14(FILE *ttf) {
	int32 val = getushort(ttf);
	int mant = val&0x3fff;
	/* This oddity may be needed to deal with the first 2 bits being signed */
	/*  and the low-order bits unsigned */
	return (real) ((val<<16)>>(16+14)) + (mant/16384.0);
}
