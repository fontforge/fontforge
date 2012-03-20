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
#include "gimage.h"

GImage *GImageReadXbm(char * filename) {
    FILE *file;
    int width, height;
    GImage *gi = NULL;
    struct _GImage *base;
    int i,j,k,pixels,val,val2, ch;
    uint8 *scanline;

    file = fopen(filename,"r");
    if ( file==NULL )
return NULL;
    if ( fscanf(file, "#define %*s %d\n", &width )!=1 )
	goto bad;
    if ( fscanf(file, "#define %*s %d\n", &height )!=1 )
	goto bad;
    if ( (ch = getc(file))=='#' ) {
	fscanf(file, "define %*s %*d\n" );	/* x hotspot */
	fscanf(file, "#define %*s %*d\n" );	/* y hotspot */
    } else
	ungetc(ch,file);
    fscanf(file,"static ");
    ungetc(ch=fgetc(file),file);
    if ( ch=='u' )
	fscanf(file,"unsigned ");
    ch = ch;		/* Compiler bug work-arround */
    fscanf(file,"char %*s = {");
    gi = GImageCreate(it_mono,width,height);
    base = gi->u.image;
    for ( i=0; i<height; ++i ) {
	scanline = base->data + i*base->bytes_per_line;
	for ( j=0; j<base->bytes_per_line; ++j ) {
	    fscanf(file," 0x%x",(unsigned *) &pixels);
	    val = pixels; val2=0;
	    for ( k=0; k<8 ; ++k ) {
		if ( (val&(1<<k)) )
		    val2 |= (0x80>>k);
	    }
	    *scanline++ = val2^0xff;		/* I default black the other way */
	    fscanf(file,",");
	}
    }
    fclose(file);
return(gi);

bad:
    if ( gi!=NULL )
	GImageDestroy(gi);
    if ( file!=NULL )
	fclose(file);
return(NULL);
}
