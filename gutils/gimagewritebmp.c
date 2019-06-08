/* Copyright (C) 2000-2012 by George Williams */
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
#include "gimagebmpP.h"

static void myputs(short s, FILE *file) {
    putc(s&0xff,file);
    putc(s>>8,file);
}

static void putl(short s, FILE *file) {
    putc(s&0xff,file);
    putc((s>>8)&0xff,file);
    putc((s>>16)&0xff,file);
    putc((s>>24)&0xff,file);
}

int GImageWrite_Bmp(GImage *gi, FILE *file) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    int headersize=40, preheadersize=14;
    int filesize, offset, imagesize;
    int bitsperpixel, clutsize, ncol;
    int row, col, i;

    if ( base->image_type == it_mono ) {
	ncol = 2;
	bitsperpixel = 1;
	clutsize = ncol*4;
    } else if ( base->image_type == it_index ) {
	ncol = base->clut->clut_len;
	if ( ncol<=16 )
	    bitsperpixel = 4;
	else
	    bitsperpixel = 8;
	clutsize = ncol*4;
    } else {
	bitsperpixel = 24;
	clutsize = 0;
	ncol = 0;
    }
    imagesize = ((base->bytes_per_line + 3) & ~3U) * base->height;
    offset = preheadersize + headersize + clutsize;
    filesize = offset + imagesize;

    putc('B',file);
    putc('M',file);
    putl(filesize,file);			/* filesize */
    myputs(0,file);				/* mbz1 */
    myputs(0,file);				/* mbz2 */
    putl(offset,file);				/* offset */
    putl(headersize,file);			/* headersize */
    putl(base->width,file);			/* width */
    putl(base->height,file);			/* height */
    myputs(1,file);				/* planes */
    myputs(bitsperpixel,file);			/* bitsperpixel */
    putl(0,file);				/* compression */
    putl(imagesize,file);			/* imagesize */
    putl(3000,file);				/* horizontal res, pixels/meter */
    putl(3000,file);				/* vertical res, pixels/meter */
    putl(ncol,file);				/* colours used */
    putl(0,file);				/* colours important */

    if ( clutsize!=0 ) {
	int i;
	if ( base->clut!=NULL ) {
	    for ( i=0; i<ncol; ++i ) {
		putc(COLOR_BLUE(base->clut->clut[i]),file);
		putc(COLOR_GREEN(base->clut->clut[i]),file);
		putc(COLOR_RED(base->clut->clut[i]),file);
		putc(0,file);
	    }
	} else {
	    putc(0,file); putc(0,file); putc(0,file); putc(0,file);
	    putc(0xff,file); putc(0xff,file); putc(0xff,file); putc(0,file);
	}
    }

    for ( row = base->height-1; row>=0; --row ) {
	int pad=0;

	if ( bitsperpixel==24 ) {
	    uint32 *pt = (uint32 *) (base->data+row*base->bytes_per_line);
	    for ( col=0; col<base->width; ++col ) {
		putc(COLOR_BLUE(pt[col]),file);
		putc(COLOR_GREEN(pt[col]),file);
		putc(COLOR_RED(pt[col]),file);
	    }
	    pad = base->width&3;
	} else if ( bitsperpixel==8 ) {
	    unsigned char *pt = (unsigned char *) (base->data+row*base->bytes_per_line);
	    fwrite(pt,1,base->width,file);
	    pad = 4-(base->width&3);
	} else if ( bitsperpixel==4 ) {
	    unsigned char *pt = (unsigned char *) (base->data+row*base->bytes_per_line);
	    for ( col=0; col<base->width/2; ++col ) {
		putc( (*pt<<4)|pt[1], file);
		pt += 2;
	    }
	    if ( base->width&1 )
		putc(*pt<<4,file);
	    pad = 4-(((base->width+1)>>1)&3);
	} else if ( bitsperpixel==1 ) {
	    unsigned char *pt = (unsigned char *) (base->data+row*base->bytes_per_line);
	    fwrite(pt,1,base->bytes_per_line,file);
	    pad = 4-(base->bytes_per_line&3);
	}
	if ( pad&1 )		/* pad to 4byte boundary */
	    putc('\0',file);
	if ( pad&2 )
	    myputs(0,file);
    }
    fflush(file);
    i = ferror(file);
return( !i );
}

int GImageWriteBmp(GImage *gi, char *filename) {
    FILE *file;
    int ret;

    if ((file=fopen(filename,"wb"))==NULL )
return(false);
    ret = GImageWrite_Bmp(gi,file);
    fclose(file);
return( ret );
}
