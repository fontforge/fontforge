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

#ifdef _NO_LIBJPEG

static int a_file_must_define_something=0;	/* ANSI says so */

#else /* We can build with jpeglib - therefore import jpg files */

#include "basics.h"
#include "gimage.h"

#include <jerror.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <stdio.h>
#include <sys/types.h>

/******************************************************************************/

struct jpegState {
    struct jpeg_decompress_struct *cinfo;
    int state;
    struct _GImage *base;
    JSAMPLE *buffer;
    int scanpos;
};

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
  int padding[8];		/* On my solaris box jmp_buf is the wrong size */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/* jpeg routines use 24 bit pixels, xvt routines pad out to 32 */
static void transferBufferToImage(struct jpegState *js,int ypos) {
    struct jpeg_decompress_struct *cinfo = js->cinfo;
    JSAMPLE *pt, *end;
    Color *ppt;

    ppt = (Color *) (js->base->data+ypos*js->base->bytes_per_line);
    for ( pt = js->buffer, end = pt+3*cinfo->image_width; pt<end; ) {
	register int r,g,b;
	r = *(pt++); g= *(pt++); b= *(pt++);
	*(ppt++) = COLOR_CREATE(r,g,b);
    }
}

GImage *GImageRead_Jpeg(FILE *infile) {
    GImage *ret;
    struct _GImage *base;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPLE *rows[1];
    struct jpegState js;
    int ypos;


  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
return( NULL );
  }

  jpeg_CreateDecompress(&cinfo,JPEG_LIB_VERSION,(size_t) sizeof(struct jpeg_decompress_struct));
  jpeg_stdio_src(&cinfo, infile);
  (void) jpeg_read_header(&cinfo, TRUE);

    if ( cinfo.jpeg_color_space == JCS_GRAYSCALE )
	cinfo.out_color_space = JCS_RGB;
    ret = GImageCreate(it_true,cinfo.image_width, cinfo.image_height);
    if ( ret==NULL ) {
	jpeg_destroy_decompress(&cinfo);
return( NULL );
    }
    base = ret->u.image;

    (void) jpeg_start_decompress(&cinfo);
    rows[0] = (JSAMPLE *) malloc(3*cinfo.image_width);
    js.cinfo = &cinfo; js.base = base; js.buffer = rows[0];
    while (cinfo.output_scanline < cinfo.output_height) {
	ypos = cinfo.output_scanline;
	(void) jpeg_read_scanlines(&cinfo, rows, 1);
	transferBufferToImage(&js,ypos);
    }

  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(rows[0]);

return( ret );
}

GImage *GImageReadJpeg(char *filename) {
/* Import a jpeg image, else return NULL if error  */
    GImage *ret;
    FILE * infile;		/* source file */

    if ((infile = fopen(filename, "rb")) == NULL) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    ret = GImageRead_Jpeg(infile);
    fclose(infile);
    return( ret );
}
#endif
