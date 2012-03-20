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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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
static void (*_jpeg_destroy_compress)(j_compress_ptr);
static void (*_jpeg_create_compress)(j_compress_ptr,int,size_t);
static void (*_jpeg_stdio_src)(j_compress_ptr, FILE *);
static int (*_jpeg_set_defaults)(j_compress_ptr);
static int (*_jpeg_set_quality)(j_compress_ptr,int,boolean);
static int (*_jpeg_simple_progression)(j_compress_ptr);
static boolean (*_jpeg_start_compress)(j_compress_ptr,boolean);
static boolean (*_jpeg_write_scanlines)(j_compress_ptr, JSAMPARRAY, JDIMENSION);
static boolean (*_jpeg_finish_compress)(j_compress_ptr);
static void (*_jpeg_stdio_dest)(j_compress_ptr, FILE *);

static int loadjpeg() {
    char *err;

    libjpeg = dlopen("libjpeg" SO_EXT,RTLD_LAZY);
    if ( libjpeg==NULL ) {
	fprintf(stderr,"%s\n", dlerror());
return( 0 );
    }
    _jpeg_std_error = (struct jpeg_error_mgr *(*)(struct jpeg_error_mgr *))
		dlsym(libjpeg,"jpeg_std_error");
    _jpeg_destroy_compress = (void (*)(j_compress_ptr))
		dlsym(libjpeg,"jpeg_destroy_compress");
    _jpeg_create_compress = (void (*)(j_compress_ptr,int,size_t))
		dlsym(libjpeg,"jpeg_CreateCompress");
    _jpeg_stdio_src = (void (*)(j_compress_ptr, FILE *))
		dlsym(libjpeg,"jpeg_stdio_src");
    _jpeg_set_defaults = (int (*)(j_compress_ptr))
		dlsym(libjpeg,"jpeg_set_defaults");
    _jpeg_set_quality = (int (*)(j_compress_ptr,int,boolean))
		dlsym(libjpeg,"jpeg_set_quality");
    _jpeg_simple_progression = (int (*)(j_compress_ptr))
		dlsym(libjpeg,"jpeg_simple_progression");
    _jpeg_start_compress = (boolean (*)(j_compress_ptr,boolean))
		dlsym(libjpeg,"jpeg_start_compress");
    _jpeg_write_scanlines = (boolean (*)(j_compress_ptr, JSAMPARRAY, JDIMENSION))
		dlsym(libjpeg,"jpeg_write_scanlines");
    _jpeg_finish_compress = (boolean (*)(j_compress_ptr))
		dlsym(libjpeg,"jpeg_finish_compress");
    _jpeg_stdio_dest = (void (*)(j_compress_ptr, FILE *))
		dlsym(libjpeg,"jpeg_stdio_dest");
    if ( _jpeg_std_error && _jpeg_destroy_compress && _jpeg_create_compress &&
	    _jpeg_stdio_src && _jpeg_set_defaults && _jpeg_set_quality &&
	    _jpeg_simple_progression && _jpeg_start_compress && _jpeg_write_scanlines &&
	    _jpeg_finish_compress && _jpeg_stdio_dest )
return( 1 );
    dlclose(libjpeg);
    err = dlerror();
    if ( err==NULL )
	err = "Couldn't load needed symbol from libjpeg.so";
    fprintf(stderr,"%s\n", err);
return( 0 );
}

/******************************************************************************/

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

static void transferImageToBuffer(struct _GImage *base,JSAMPLE *buffer,int w,int ypos) {
    JSAMPLE *pt, *end;
    uint32 *ppt;

    ppt = (uint32 *) (base->data + ypos*base->bytes_per_line);
    if ( base->image_type==it_index && base->clut==NULL ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	register int bit=0x80;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    if ( *px&bit ) col = 0xffffff;
	    else col = 0;
	    if (( bit >>= 1 )== 0 ) { ++px; bit = 0x80; }
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else if ( base->image_type==it_index ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	register int bit=0x80;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    if ( *px&bit ) col = base->clut->clut[1];
	    else col = base->clut->clut[0];
	    if (( bit >>= 1 )== 0 ) { ++px; bit = 0x80; }
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else if ( base->image_type==it_index ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    col = base->clut->clut[ *px++ ];
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else {
	for ( pt = buffer, end = pt+3*w; pt<end; ++ppt) {
	    *pt++ = COLOR_RED(*ppt);
	    *pt++ = COLOR_GREEN(*ppt);
	    *pt++ = COLOR_BLUE(*ppt);
	}
    }
}

static void setColorSpace(struct jpeg_compress_struct *cinfo, struct _GImage *base) {
  int i;

  cinfo->input_components = 3;		/* # of color components per pixel */
  cinfo->in_color_space = JCS_RGB; 	/* colorspace of input image */

  if ( base->image_type==it_index ) {
    if ( base->clut->clut_len!=256 )
return;
    for ( i=0; i<256; ++i )
      if ( base->clut->clut[i]!=COLOR_CREATE(i,i,i))
    break;
    if ( i==256 ) {
      cinfo->input_components = 1;
      cinfo->in_color_space = JCS_GRAYSCALE;
    }
  }
}

/* quality is a number between 0 and 100 */
int GImageWrite_Jpeg(GImage *gi, FILE *outfile, int quality, int progressive) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */

    if ( libjpeg==NULL )
	if ( !loadjpeg())
return( 0 );

  cinfo.err = _jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    _jpeg_destroy_compress(&cinfo);
return 0;
  }
  _jpeg_create_compress(&cinfo,JPEG_LIB_VERSION,(size_t) sizeof(struct jpeg_compress_struct));
  _jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = base->width;
  cinfo.image_height = base->height;
  setColorSpace(&cinfo,base);
  _jpeg_set_defaults(&cinfo);
  _jpeg_set_quality(&cinfo, quality, TRUE );
  if ( progressive )
    _jpeg_simple_progression(&cinfo);
  _jpeg_start_compress(&cinfo, TRUE);

  if ( cinfo.in_color_space != JCS_GRAYSCALE )
    row_pointer[0]=galloc(3*base->width);
  while (cinfo.next_scanline < cinfo.image_height) {
    if ( cinfo.in_color_space == JCS_GRAYSCALE )
      row_pointer[0] = (unsigned char *) (base->data + cinfo.next_scanline*base->bytes_per_line);
    else
      transferImageToBuffer(base,row_pointer[0],base->width,cinfo.next_scanline);
    (void) _jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  _jpeg_finish_compress(&cinfo);
  _jpeg_destroy_compress(&cinfo);
  if ( cinfo.in_color_space != JCS_GRAYSCALE )
    gfree(row_pointer[0]);
return( 1 );
}

int GImageWriteJpeg(GImage *gi, char *filename, int quality, int progressive) {
    FILE * outfile;		/* target file */
    int ret;

    if ( libjpeg==NULL ) {
	if ( !loadjpeg())
return( 0 );
    }

  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
return(0);
  }
  ret = GImageWrite_Jpeg(gi,outfile,quality,progressive);
  fclose(outfile);
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

static void transferImageToBuffer(struct _GImage *base,JSAMPLE *buffer,int w,int ypos) {
    JSAMPLE *pt, *end;
    uint32 *ppt;

    ppt = (uint32 *) (base->data + ypos*base->bytes_per_line);
    if ( base->image_type==it_index && base->clut==NULL ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	register int bit=0x80;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    if ( *px&bit ) col = 0xffffff;
	    else col = 0;
	    if (( bit >>= 1 )== 0 ) { ++px; bit = 0x80; }
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else if ( base->image_type==it_index ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	register int bit=0x80;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    if ( *px&bit ) col = base->clut->clut[1];
	    else col = base->clut->clut[0];
	    if (( bit >>= 1 )== 0 ) { ++px; bit = 0x80; }
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else if ( base->image_type==it_index ) {
	unsigned char *px = (unsigned char *) ppt; int col;
	for ( pt = buffer, end = pt+3*w; pt<end; ) {
	    col = base->clut->clut[ *px++ ];
	    *pt++ = COLOR_RED(col);
	    *pt++ = COLOR_GREEN(col);
	    *pt++ = COLOR_BLUE(col);
	}
    } else {
	for ( pt = buffer, end = pt+3*w; pt<end; ++ppt) {
	    *pt++ = COLOR_RED(*ppt);
	    *pt++ = COLOR_GREEN(*ppt);
	    *pt++ = COLOR_BLUE(*ppt);
	}
    }
}

static void setColorSpace(struct jpeg_compress_struct *cinfo, struct _GImage *base) {
  int i;

  cinfo->input_components = 3;		/* # of color components per pixel */
  cinfo->in_color_space = JCS_RGB; 	/* colorspace of input image */

  if ( base->image_type==it_index ) {
    if ( base->clut->clut_len!=256 )
return;
    for ( i=0; i<256; ++i )
      if ( base->clut->clut[i]!=COLOR_CREATE(i,i,i))
    break;
    if ( i==256 ) {
      cinfo->input_components = 1;
      cinfo->in_color_space = JCS_GRAYSCALE;
    }
  }
}

/* quality is a number between 0 and 100 */
int GImageWrite_Jpeg(GImage *gi, FILE *outfile, int quality, int progressive) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_compress(&cinfo);
return 0;
  }
  jpeg_CreateCompress(&cinfo,JPEG_LIB_VERSION,(size_t) sizeof(struct jpeg_compress_struct));
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = base->width;
  cinfo.image_height = base->height;
  setColorSpace(&cinfo,base);
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE );
  if ( progressive )
    jpeg_simple_progression(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);

  if ( cinfo.in_color_space != JCS_GRAYSCALE )
    row_pointer[0]=galloc(3*base->width);
  while (cinfo.next_scanline < cinfo.image_height) {
    if ( cinfo.in_color_space == JCS_GRAYSCALE )
      row_pointer[0] = (unsigned char *) (base->data + cinfo.next_scanline*base->bytes_per_line);
    else
      transferImageToBuffer(base,row_pointer[0],base->width,cinfo.next_scanline);
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  if ( cinfo.in_color_space != JCS_GRAYSCALE )
    gfree(row_pointer[0]);
return( 1 );
}

int GImageWriteJpeg(GImage *gi, char *filename, int quality, int progressive) {
    FILE * outfile;		/* target file */
    int ret;

  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
return(0);
  }
  ret = GImageWrite_Jpeg(gi,outfile,quality,progressive);
  fclose(outfile);
return( ret );
}
#endif
