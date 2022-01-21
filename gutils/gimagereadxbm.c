/* Copyright (C) 2000-2012 by George Williams */
/* 2013feb15, fileread and mem error checks, plus test for short, Jose Da Silva */
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

#include "gimage.h"

static int ConvertXbmByte(int pixels) {
    int i,val=0;

    for ( i=0; i<8; ++i )
	if ( pixels&(1<<i) )
	    val |= (0x80>>i);
    return( val^0xff );	/* I default black the other way */
}

GImage *GImageReadXbm(char * filename) {
/* Import an *.xbm image, else return NULL if error */
    FILE *file;
    int width, height;
    GImage *gi=NULL;
    struct _GImage *base;
    int ch,i,j,l = 0;
    long pixels;
    uint8_t *scanline;

    if ( (file=fopen(filename,"r"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    /* Get width and height */
    if ( fscanf(file,"#define %*s %d\n",&width)!=1 || \
	 fscanf(file,"#define %*s %d\n",&height)!=1 )
	goto errorGImageReadXbm;

    /* Check for, and skip past hotspot */
    if ( (ch=getc(file))<0 )
	goto errorGImageReadXbm;
    if ( ch=='#' ) {
	if ( fscanf(file,"define %*s %*d\n")<0 ||	/* x hotspot */ \
	     fscanf(file,"#define %*s %*d\n")<0 )	/* y hotspot */
	    goto errorGImageReadXbm;
    } else
	if ( ungetc(ch,file)<0 )
	    goto errorGImageReadXbm;

    /* Data starts after finding "static unsigned " or "static " */
    if ( fscanf(file,"static ")<0 || (ch=getc(file))<0 )
	goto errorGImageReadXbm;
    if ( ch=='u' ) {
	if ( fscanf(file,"nsigned ")<0 || (ch=getc(file))<0 )
	    goto errorGImageReadXbm;
	/* TODO: Do we want to track signed/unsigned data? */
    }

    /* Data may be in char,short or long formats */
    if ( ch=='c' ) {
	if ( fscanf(file,"har %*s = {")<0 )
	    goto errorGImageReadXbm;
	l=8;
    } else if ( ch=='s' ) {
	if ( fscanf(file,"hort %*s = {")<0 )
	    goto errorGImageReadXbm;
	l=16;
    } else if ( ch=='l' ) {
	if ( fscanf(file,"ong %*s = {")<0 )
	    goto errorGImageReadXbm;
	l=32;
    }

    /* Create memory to hold image, exit with NULL if not enough memory */
    if ( (gi=GImageCreate(it_mono,width,height))==NULL )
	goto errorGImageReadXbmMem;

    /* Convert *.xbm graphic into one that FF can use */
    base = gi->u.image;
    for ( i=0; i<height; ++i ) {
	scanline = base->data + i*base->bytes_per_line;
	for ( j=0; j<base->bytes_per_line; ++j ) {
	    if ( fscanf(file," 0x%x",(unsigned *) &pixels)!=1 )
		goto errorGImageReadXbm;
	    *scanline++ = ConvertXbmByte(pixels);
	    if ( l>8 && j+1<base->bytes_per_line ) {
		*scanline++ = ConvertXbmByte(pixels>>8);
		++j;
	    }
	    if ( l>16 && j+1<base->bytes_per_line ) {
		*scanline++ = ConvertXbmByte(pixels>>16);
		++j;
		if ( j+1<base->bytes_per_line ) {
		    *scanline++ = ConvertXbmByte(pixels>>24);
		    ++j;
		}
	    }
	    fscanf(file,",");
	}
    }
    fclose(file);
    return( gi );

errorGImageReadXbm:
    fprintf(stderr,"Bad input file \"%s\"\n",filename );
errorGImageReadXbmMem:
    GImageDestroy(gi);
    fclose(file);
    return( NULL );
}
