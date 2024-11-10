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
#include "ustring.h"

#include "gfile.h"

typedef GImage *(*GImageReadFunc)(FILE *, int *);

GImage *GImageRead(char * filename) {
/* Go read an input image file. Return NULL if cannot guess file type */
    FILE *file;
    GImage *ret = NULL;
    int success = 0;
    char *mime = NULL;
    GImageReadFunc reader = NULL;

    if ( filename == NULL || !GFileExists(filename) )
        return NULL;

    mime = GFileMimeType(filename);
    if ( mime == NULL )
        return NULL;

	if ( strcasecmp(mime,"image/bmp")==0 ) {
        reader = GImageRead_Bmp;
	} else if ( strcasecmp(mime,"image/x-xbitmap")==0 ) {
        reader = GImageRead_Xbm;
	} else if ( strcasecmp(mime,"image/x-xpixmap")==0 ) {
        reader = GImageRead_Xpm;
#ifndef _NO_LIBTIFF
	} else if ( strcasecmp(mime,"image/tiff")==0 ) {
        free(mime);
        // Different pattern, just call directly
        return GImageReadTiff(filename);
#endif
#ifndef _NO_LIBJPEG
	} else if ( strcasecmp(mime,"image/jpeg")==0 ) {
        reader = GImageRead_Jpeg;
#endif
#ifndef _NO_LIBPNG
	} else if ( strcasecmp(mime,"image/png")==0 ) {
        reader = GImageRead_Png;
#endif
#ifndef _NO_LIBUNGIF
	} else if ( strcasecmp(mime,"image/gif")==0 ) {
        free(mime);
        // Different pattern, just call directly
        return GImageReadGif(filename);
#endif
	} else if ( strcasecmp(mime,"image/x-cmu-raster")==0 || \
		  strcasecmp(mime,"image/x-sun-raster")==0 ) {
        reader = GImageRead_Ras;
	} else if ( strcasecmp(mime,"image/x-rgb")==0 || \
		  strcasecmp(mime,"image/x-sgi")==0 ) {
        reader = GImageRead_Rgb;
	}

    free(mime);

    if ( (file=fopen(filename,"rb"))==NULL ) {
        fprintf(stderr,"\"%s\" is a Bad input file\n", filename);
        return( NULL );
    }

    if (reader == NULL) {
        fprintf(stderr,"Not recognized file type for \"%s\"\n", filename);
    } else {
        ret = (*reader)(file, &success);

        if ( success!=1 ) {
            fprintf(stderr,"Failure reading file \"%s\"\n", filename);
            ret = NULL;
        }
    }
    fclose(file);
    return( ret );
}

GImage *GImageReadBuf(char *buffer, int size, char* ext) {
    if ( buffer==NULL || size == 0 ){
        return NULL;
    }

    FILE *file = NULL;
    file = fmemoryopen(buffer, size, "rb");

    GImage *ret;

    if (file == NULL) {
        fprintf(stderr, "Can't open file buffer\n");
        return (NULL);
    }

    const char** elem = GFileMimeTypeMatching(ext);
    char *mime = copy(elem ? elem[1] : "application/octet-stream");

    if ( strcasecmp(mime,"image/bmp")==0 ) {
        free(mime);
        ret = GImageRead_Bmp(file, NULL);
        fclose(file);
        return( ret );
    } else if ( strcasecmp(mime,"image/x-xbitmap")==0 ) {
        free(mime);
        ret = GImageRead_Xbm(file, NULL);
        fclose(file);
        return( ret );
    } else if ( strcasecmp(mime,"image/x-xpixmap")==0 ) {
        free(mime);
        ret = GImageRead_Xpm(file, NULL);
        fclose(file);
        return( ret );
    #ifndef _NO_LIBTIFF
    } else if ( strcasecmp(mime,"image/tiff")==0 ) {
        free(mime);
        fclose(file);
        /* libtiff does not support assigning TIFF type using buffer */
        return( NULL );
    #endif
    #ifndef _NO_LIBJPEG
    } else if ( strcasecmp(mime,"image/jpeg")==0 ) {
        free(mime);
        ret = GImageRead_Jpeg(file, NULL);
        fclose(file);
        return( ret );
    #endif
    #ifndef _NO_LIBPNG
    } else if ( strcasecmp(mime,"image/png")==0 ) {
        free(mime);
        ret = GImageReadPngBuf(buffer, size);
        fclose(file);
        return( ret );
    #endif
    #ifndef _NO_LIBUNGIF
    } else if ( strcasecmp(mime,"image/gif")==0 ) {
        free(mime);
        fclose(file);
        /* giflib does not support assigning GifFileType type using buffer */
        return( NULL );
    #endif
    } else if ( strcasecmp(mime,"image/x-cmu-raster")==0 || \
              strcasecmp(mime,"image/x-sun-raster")==0 ) {
        free(mime);
        ret = GImageRead_Ras(file, NULL);		/* Sun raster */
        fclose(file);
        return( ret );
    } else if ( strcasecmp(mime,"image/x-rgb")==0 || \
              strcasecmp(mime,"image/x-sgi")==0 ) {
        free(mime);
        ret = GImageRead_Rgb(file, NULL);		/* SGI format */
        fclose(file);
        return( ret );
    }

    free(mime);
    fclose(file);

    return( NULL );
}
