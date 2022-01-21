/* Copyright (C) 2005-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fontforge-config.h>

#include "intl.h"
#include "mem.h"
#include "uiinterface.h"

int32_t memlong(uint8_t *data,int len, int offset) {
	if (offset>=0 && offset+3<len) {
		int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
		return (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4;
	} else {
		LogError( _("Bad font, offset out of bounds.\n") );
		return 0;
	}
}

int memushort(uint8_t *data,int len, int offset) {
	if (offset>=0 && offset+1<len) {
		int ch1 = data[offset], ch2 = data[offset+1];
		return (ch1<<8)|ch2;
	} else {
		LogError( _("Bad font, offset out of bounds.\n") );
		return 0;
	}
}

void memputshort(uint8_t *data,int offset,uint16_t val) {
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

int32_t getlong(FILE *ttf) {
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);
	int ch3 = getc(ttf);
	int ch4 = getc(ttf);

	if (ch4==EOF)
		return EOF;

	return (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4;
}

real getfixed(FILE *ttf) {
	int32_t val = getlong(ttf);
	int mant = val&0xffff;
	/* This oddity may be needed to deal with the first 16 bits being signed */
	/*  and the low-order bits unsigned */
	return (real) (val>>16) + (mant/65536.0);
}

real get2dot14(FILE *ttf) {
	int32_t val = getushort(ttf);
	int mant = val&0x3fff;
	/* This oddity may be needed to deal with the first 2 bits being signed */
	/*  and the low-order bits unsigned */
	return (real) ((val<<16)>>(16+14)) + (mant/16384.0);
}
