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
