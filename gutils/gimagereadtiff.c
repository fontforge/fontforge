/* Copyright (C) 2000-2012 by George Williams */
/* 2013feb15, added file and mem error checks, Jose Da Silva */
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

#ifdef _NO_LIBTIFF

static int a_file_must_define_something=0;	/* ANSI says so */

#else

#include <tiffio.h>

#define int32 _int32
#define uint32 _uint32
#define int16 _int16
#define uint16 _uint16
#define int8 _int8
#define uint8 _uint8

#include "gimage.h"

#undef uint32

GImage *GImageReadTiff(char *filename) {
/* Import a TIF image, else return NULL if error  */
    TIFF* tif;
    uint32 w,h,i,j;
    uint32 *ipt,*fpt,*raster=NULL;
    GImage *ret=NULL;
    struct _GImage *base;

    if ( (tif=TIFFOpen(filename,"rb"))==NULL ) {
	/* if error, then report "built-in" error message and then exit	*/
	return( NULL );
    }

    /* Get width and height of TIF image, exit if error */
    if ( TIFFGetField(tif,TIFFTAG_IMAGEWIDTH,&w)!=1 || \
	 TIFFGetField(tif,TIFFTAG_IMAGELENGTH,&h)!=1 )
	goto errorGImageReadTiff;

    /* Create memory to hold image & raster, exit if not enough memory	*/
    if ( (ret=GImageCreate(it_true,w,h))==NULL )
	goto errorGImageReadTiffMem;
    if ( (raster=(uint32*) malloc(w*h*sizeof(uint32)))==NULL ) {
	NoMoreMemMessage();
	goto errorGImageReadTiffMem;
    }

    /* Read TIF image and process it into an internal FF usable format	*/
    if ( TIFFReadRGBAImage(tif,w,h,raster,0) ) {
	TIFFClose(tif);
	base=ret->u.image;
	for ( i=0; i<h; ++i ) {
	    ipt=(uint32 *)(base->data+i*base->bytes_per_line);
	    fpt=raster+(h-1-i)*w;
	    for ( j=0; j<w; ++j )
		*ipt++ =COLOR_CREATE(
			TIFFGetR(fpt[j]),TIFFGetG(fpt[j]),TIFFGetB(fpt[j]));
	}
	free(raster);
	return( ret );
    }

errorGImageReadTiff:
    fprintf(stderr,"Bad input file \"%s\"\n",filename );
errorGImageReadTiffMem:
    free(raster); free(ret);
    TIFFClose(tif);
    return( NULL );
}

#endif
