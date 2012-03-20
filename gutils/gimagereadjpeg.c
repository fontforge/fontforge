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
#include <basics.h>

#ifdef _NO_LIBJPEG
static int a_file_must_define_something=0;	/* ANSI says so */
#elif !defined(_STATIC_LIBJPEG) && !defined(NODYNAMIC)	/* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#include <dynamic.h>

#undef HAVE_STDLIB_H		/* Defined by libtool, redefined by jconfig.h, annoying warning msg */
#include <sys/types.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>

#include <setjmp.h>

#include "gimage.h"

static DL_CONST void *libjpeg=NULL;
static struct jpeg_error_mgr *(*_jpeg_std_error)(struct jpeg_error_mgr *);
static void (*_jpeg_destroy_decompress)(j_decompress_ptr);
static void (*_jpeg_create_decompress)(j_decompress_ptr,int,size_t);
static void (*_jpeg_stdio_src)(j_decompress_ptr, FILE *);
static int (*_jpeg_read_header)(j_decompress_ptr, boolean);
static boolean (*_jpeg_start_decompress)(j_decompress_ptr);
static boolean (*_jpeg_read_scanlines)(j_decompress_ptr, JSAMPARRAY, JDIMENSION);
static boolean (*_jpeg_finish_decompress)(j_decompress_ptr);

static int loadjpeg() {
    char *err;

    libjpeg = dlopen("libjpeg" SO_EXT,RTLD_LAZY);
    if ( libjpeg==NULL ) {
	fprintf(stderr,"%s\n", dlerror());
return( 0 );
    }
    _jpeg_std_error = (struct jpeg_error_mgr *(*)(struct jpeg_error_mgr *))
		dlsym(libjpeg,"jpeg_std_error");
    _jpeg_destroy_decompress = (void (*)(j_decompress_ptr))
		dlsym(libjpeg,"jpeg_destroy_decompress");
    _jpeg_create_decompress = (void (*)(j_decompress_ptr,int,size_t))
		dlsym(libjpeg,"jpeg_CreateDecompress");
    _jpeg_stdio_src = (void (*)(j_decompress_ptr, FILE *))
		dlsym(libjpeg,"jpeg_stdio_src");
    _jpeg_read_header = (int (*)(j_decompress_ptr,boolean))
		dlsym(libjpeg,"jpeg_read_header");
    _jpeg_start_decompress = (boolean (*)(j_decompress_ptr))
		dlsym(libjpeg,"jpeg_start_decompress");
    _jpeg_read_scanlines = (boolean (*)(j_decompress_ptr, JSAMPARRAY, JDIMENSION))
		dlsym(libjpeg,"jpeg_read_scanlines");
    _jpeg_finish_decompress = (boolean (*)(j_decompress_ptr))
		dlsym(libjpeg,"jpeg_finish_decompress");
    if ( _jpeg_std_error && _jpeg_destroy_decompress && _jpeg_create_decompress &&
	    _jpeg_stdio_src && _jpeg_read_header && _jpeg_start_decompress &&
	    _jpeg_read_scanlines && _jpeg_finish_decompress )
return( 1 );
    dlclose(libjpeg);
    err = dlerror();
    if ( err==NULL )
	err = "Couldn't load needed symbol from libjpeg.so";
    fprintf(stderr,"%s\n", err);
return( 0 );
}

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

    if ( libjpeg==NULL )
	if ( !loadjpeg())
return( NULL );

  cinfo.err = _jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    _jpeg_destroy_decompress(&cinfo);
return( NULL );
  }
  
  _jpeg_create_decompress(&cinfo,JPEG_LIB_VERSION,(size_t) sizeof(struct jpeg_decompress_struct));
  _jpeg_stdio_src(&cinfo, infile);
  (void) _jpeg_read_header(&cinfo, TRUE);

    if ( cinfo.jpeg_color_space == JCS_GRAYSCALE )
	cinfo.out_color_space = JCS_RGB;
    ret = GImageCreate(it_true,cinfo.image_width, cinfo.image_height);
    if ( ret==NULL ) {
	_jpeg_destroy_decompress(&cinfo);
return( NULL );
    }
    base = ret->u.image;

    (void) _jpeg_start_decompress(&cinfo);
    rows[0] = galloc(3*cinfo.image_width);
    js.cinfo = &cinfo; js.base = base; js.buffer = rows[0];
    while (cinfo.output_scanline < cinfo.output_height) {
	ypos = cinfo.output_scanline;
	(void) _jpeg_read_scanlines(&cinfo, rows, 1);
	transferBufferToImage(&js,ypos);
    }

  (void) _jpeg_finish_decompress(&cinfo);
  _jpeg_destroy_decompress(&cinfo);
  gfree(rows[0]);

return( ret );
}

GImage *GImageReadJpeg(char *filename) {
    GImage *ret;
    FILE * infile;		/* source file */

    if ( libjpeg==NULL )
	if ( !loadjpeg())
return( NULL );

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr,"can't open %s\n", filename);
return( NULL );
  }
  ret = GImageRead_Jpeg(infile);
  fclose(infile);
return( ret );
}
#else
#include <sys/types.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>

#include <setjmp.h>

#include "gimage.h"

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
    rows[0] = galloc(3*cinfo.image_width);
    js.cinfo = &cinfo; js.base = base; js.buffer = rows[0];
    while (cinfo.output_scanline < cinfo.output_height) {
	ypos = cinfo.output_scanline;
	(void) jpeg_read_scanlines(&cinfo, rows, 1);
	transferBufferToImage(&js,ypos);
    }

  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  gfree(rows[0]);

return( ret );
}

GImage *GImageReadJpeg(char *filename) {
    GImage *ret;
    FILE * infile;		/* source file */

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr,"can't open %s\n", filename);
return( NULL );
  }
  ret = GImageRead_Jpeg(infile);
  fclose(infile);
return( ret );
}
#endif
