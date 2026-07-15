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

#include "ffglib_compat.h"  /* For strcasecmp on MSVC */
#include "gimage.h"
#include "ustring.h"

#include "gfile.h"

GImage *GImageRead(char * filename) {
/* Go read an input image file. Return NULL if cannot guess file type */
/* First try filename dot3 extension then try sniffing if can't guess */
    char *mime;

    if ( filename!=NULL && GFileExists(filename) && (mime=GFileMimeType(filename))) {
	if ( strcasecmp(mime,"image/bmp")==0 ) {
	    free(mime);
	    return( GImageReadBmp(filename) );
	} else if ( strcasecmp(mime,"image/x-xbitmap")==0 ) {
	    free(mime);
	    return( GImageReadXbm(filename) );
	} else if ( strcasecmp(mime,"image/x-xpixmap")==0 ) {
	    free(mime);
	    return( GImageReadXpm(filename) );
#ifndef _NO_LIBTIFF
	} else if ( strcasecmp(mime,"image/tiff")==0 ) {
	    free(mime);
	    return( GImageReadTiff(filename) );
#endif
#ifndef _NO_LIBJPEG
	} else if ( strcasecmp(mime,"image/jpeg")==0 ) {
	    free(mime);
	    return( GImageReadJpeg(filename) );
#endif
#ifndef _NO_LIBPNG
	} else if ( strcasecmp(mime,"image/png")==0 ) {
	    free(mime);
	    return( GImageReadPng(filename) );
#endif
#ifndef _NO_LIBUNGIF
	} else if ( strcasecmp(mime,"image/gif")==0 ) {
	    free(mime);
	    return( GImageReadGif(filename) );
#endif
	} else if ( strcasecmp(mime,"image/x-cmu-raster")==0 || \
		  strcasecmp(mime,"image/x-sun-raster")==0 ) {
	    free(mime);
	    return( GImageReadRas(filename) );		/* Sun raster */
	} else if ( strcasecmp(mime,"image/x-rgb")==0 || \
		  strcasecmp(mime,"image/x-sgi")==0 ) {
	    free(mime);
	    return( GImageReadRgb(filename) );		/* SGI format */
	}
	free(mime);
    }

    return( NULL );
}
