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

#ifdef _NO_LIBPNG

static void *a_file_must_define_something=(void *) &a_file_must_define_something;
		/* ANSI says so */

#else

# include <png.h>

# include "basics.h"
# include "gimage.h"

struct mem_buffer {
    char *buffer;
    size_t size;
    size_t read;
};
 
static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr,"%s\n", error_msg);
#if (PNG_LIBPNG_VER < 10500)
    longjmp(png_ptr->jmpbuf,1);
#else
    png_longjmp (png_ptr, 1);
#endif
}

static void user_warning_fn(png_structp UNUSED(png_ptr), png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}

static void mem_read_fn(png_structp png_ptr, png_bytep data, png_size_t sz) {
    struct mem_buffer* buf = (struct mem_buffer*)png_get_io_ptr(png_ptr);

    if (buf->read + sz > buf->size) {
        png_error(png_ptr, "memory buffer is too small");
        return;
    }

    memcpy(data, buf->buffer+buf->read, sz);
    buf->read += sz;
}

static GImage *GImageReadPngFull(void *io, int in_memory) {
    GImage *ret=NULL;
    struct _GImage *base;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers=NULL;
    png_bytep trans_alpha;
    int num_trans;
    png_color_16p trans_color;
    unsigned i;
    int test;

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr)
return( NULL );

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      png_destroy_read_struct(&png_ptr,  (png_infopp)NULL, (png_infopp)NULL);
return( NULL );
    }

#if (PNG_LIBPNG_VER < 10500)
    test = setjmp(png_ptr->jmpbuf);
#else
    test = setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof (jmp_buf)));
#endif
    if (test) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      if ( ret!=NULL ) {
	  GImageDestroy(ret);
	  free(row_pointers);
      }
      /* If we get here, we had a problem reading the file */
return( NULL );
    }

    if (in_memory) {
        png_set_read_fn(png_ptr, io, mem_read_fn);
    } else {
        png_init_io(png_ptr, (FILE*)io);
    }
    png_read_info(png_ptr, info_ptr);
    png_set_strip_16(png_ptr);
    if ( (png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_GRAY || png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_PALETTE ) &&
	    png_get_bit_depth(png_ptr,info_ptr) == 1 )
	/* Leave bitmaps packed */;
    else
	png_set_packing(png_ptr);
    if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_GRAY_ALPHA )
	png_set_strip_alpha(png_ptr);
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

    if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr,info_ptr) == 1 ) {
	ret = GImageCreate(it_mono,png_get_image_width(png_ptr,info_ptr),png_get_image_height(png_ptr,info_ptr));
    } else if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_GRAY || png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_GRAY_ALPHA ) {
	GClut *clut;
	ret = GImageCreate(it_index,png_get_image_width(png_ptr,info_ptr),png_get_image_height(png_ptr,info_ptr));
	clut = ret->u.image->clut;
	clut->is_grey = true;
	clut->clut_len = 256;
	for ( i=0; i<256; ++i )
	    clut->clut[i] = COLOR_CREATE(i,i,i);
    } else if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB_ALPHA ) {
	ret = GImageCreate(it_rgba,png_get_image_width(png_ptr,info_ptr),png_get_image_height(png_ptr,info_ptr));
    } else if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB || png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB_ALPHA )
	ret = GImageCreate(it_true,png_get_image_width(png_ptr,info_ptr),png_get_image_height(png_ptr,info_ptr));
    else {
	png_colorp palette;
	int num_palette;
	GClut *clut;
	ret = GImageCreate(png_get_bit_depth(png_ptr,info_ptr) != 1? it_index : it_mono,
		png_get_image_width(png_ptr,info_ptr),png_get_image_height(png_ptr,info_ptr));
	clut = ret->u.image->clut;
	if ( clut==NULL )
	    clut = ret->u.image->clut = (GClut *) calloc(1,sizeof(GClut));
	clut->is_grey = true;
	png_get_PLTE(png_ptr,info_ptr,&palette,&num_palette);
	clut->clut_len = num_palette;
	for ( i=0; i<(unsigned)num_palette; ++i )
	    clut->clut[i] = COLOR_CREATE(palette[i].red,
			palette[i].green,
			palette[i].blue);
    }
    png_get_tRNS(png_ptr,info_ptr,&trans_alpha,&num_trans,&trans_color);
    base = ret->u.image;
    if ( (png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) && num_trans>0 ) {
	if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB || png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB_ALPHA )
	    base->trans = COLOR_CREATE(
		    (trans_color->red>>8),
		    (trans_color->green>>8),
		    (trans_color->blue>>8));
        else if ( base->image_type == it_mono )
	    base->trans = trans_alpha ? trans_alpha[0] : 0;
	else
	    base->clut->trans_index = base->trans = trans_alpha ? trans_alpha[0] : 0;
    }

    row_pointers = (png_byte **) malloc(png_get_image_height(png_ptr,info_ptr)*sizeof(png_bytep));
    for ( i=0; i<png_get_image_height(png_ptr,info_ptr); ++i )
	row_pointers[i] = (png_bytep) (base->data + i*base->bytes_per_line);

    /* Ignore progressive loads for now */
    /* libpng wants me to do it with callbacks, but that doesn't sit well */
    /*  with my wish to be in control... */
    png_read_image(png_ptr,row_pointers);
    png_read_end(png_ptr, NULL);

    if ( png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB || png_get_color_type(png_ptr, info_ptr)==PNG_COLOR_TYPE_RGB_ALPHA ) {
	/* PNG orders its bytes as AABBGGRR instead of 00RRGGBB */
	uint32_t *ipt, *iend;
	for ( ipt = (uint32_t *) (base->data), iend=ipt+base->width*base->height; ipt<iend; ++ipt ) {
	    uint32_t r, g, b, a = *ipt&0xff000000;
	    r = (*ipt    )&0xff;
	    g = (*ipt>>8 )&0xff;
	    b = (*ipt>>16)&0xff;
	    *ipt = COLOR_CREATE( r,g,b ) | a;
	}
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    free(row_pointers);
    /* Note png b&w images come out as indexed */
return( ret );
}

GImage *GImageReadPngBuf(char* buf, size_t sz) {
    struct mem_buffer membuf = {buf, sz, 0};
    return GImageReadPngFull(&membuf, true);
}

GImage *GImageRead_Png(FILE *fp) {
    return GImageReadPngFull(fp, false);
}

GImage *GImageReadPng(char *filename) {
    GImage *ret=NULL;
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp)
return( NULL );

    ret = GImageRead_Png(fp);
    fclose(fp);
return( ret );
}
#endif
