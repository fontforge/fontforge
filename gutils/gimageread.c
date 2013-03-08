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
#include <string.h>
#include <ustring.h>

#include "gio.h"
#include "gfile.h"

GImage *GImageRead(char * filename) {
/* Go read an input image file. Return NULL if cannot guess file type */
    char *mime, *pt;

    if (filename == NULL )
	return( NULL );

    /* Try finding correct routine to use based on GTK mime type */
    if ( GFileExists(filename) ) {
	mime=GIOGetMimeType(filename, true);

	if ( strcasecmp(mime,"image/bmp")==0 )
	    return( GImageReadBmp(filename) );
	else if ( strcasecmp(mime,"image/x-xbitmap")==0 )
	    return( GImageReadXbm(filename) );
	else if ( strcasecmp(mime,"image/x-xpixmap")==0 )
	    return( GImageReadXpm(filename) );
#ifndef _NO_LIBTIFF
	else if ( strcasecmp(mime,"image/tiff")==0 )
	    return( GImageReadTiff(filename) );
#endif
#ifndef _NO_LIBJPEG
	else if ( strcasecmp(mime,"image/jpeg")==0 )
	    return( GImageReadJpeg(filename) );
#endif
#ifndef _NO_LIBPNG
	else if ( strcasecmp(mime,"image/png")==0 )
	    return( GImageReadPng(filename) );
#endif
#ifndef _NO_LIBUNGIF
	else if ( strcasecmp(mime,"image/gif")==0 )
	    return( GImageReadGif(filename) );
#endif
	else if ( strcasecmp(mime,"image/x-cmu-raster")==0 || \
		  strcasecmp(mime,"image/x-sun-raster")==0 )
	    return( GImageReadRas(filename) );		/* Sun raster */
	else if ( strcasecmp(mime,"image/x-rgb")==0 || \
		  strcasecmp(mime,"image/x-sgi")==0 )
	    return( GImageReadRgb(filename) );		/* SGI format */
    }

    /* Try finding correct routine to use based on filename suffix */
    if ( (pt=strrchr(filename,'.'))!=NULL ) {

	if ( strmatch(pt,".bmp")==0 )
	    return( GImageReadBmp(filename) );
	else if ( strmatch(pt,".xbm")==0 )
	    return( GImageReadXbm(filename) );
	else if ( strmatch(pt,".xpm")==0 )
	    return( GImageReadXpm(filename) );
#ifndef _NO_LIBTIFF
	else if ( strmatch(pt,".tiff")==0 || strmatch(pt,".tif")==0 )
	    return( GImageReadTiff(filename) );
#endif
#ifndef _NO_LIBJPEG
	else if ( strmatch(pt,".jpeg")==0 || strmatch(pt,".jpg")==0 )
	    return( GImageReadJpeg(filename) );
#endif
#ifndef _NO_LIBPNG
	else if ( strmatch(pt,".png")==0 )
	    return( GImageReadPng(filename) );
#endif
#ifndef _NO_LIBUNGIF
	else if ( strmatch(pt,".gif")==0 )
	    return( GImageReadGif(filename) );
#endif
	else if ( strmatch(pt,".ras")==0 || strmatch(pt,".im1")==0 || \
		  strmatch(pt,".im8")==0 || strmatch(pt,".im24")==0 || \
		  strmatch(pt,".im32")==0 || strmatch(pt,".rs")==0 || \
		  strmatch(pt,".sun")==0 )
	    return( GImageReadRas(filename) );		/* Sun raster */
	else if ( strmatch(pt,".rgb")==0 || strmatch(pt,".rgba")==0 || \
		  strmatch(pt,".sgi")==0 || strmatch(pt,".bw")==0 )
	    return( GImageReadRgb(filename) );		/* SGI format */
    }

    return( NULL );
}
