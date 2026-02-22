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

static int a_file_must_define_something=0;	/* ANSI says so */

#else

#include <png.h>

#include <vector>

#include "gimage.h"

static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr, "%s\n", error_msg );
#if (PNG_LIBPNG_VER < 10500)
    longjmp(png_ptr->jmpbuf,1);
#else
    png_longjmp (png_ptr, 1);
#endif
}

static void user_warning_fn(png_structp UNUSED(png_ptr), png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}

static void mem_write_fn(png_structp png_ptr, png_bytep data, png_size_t sz) {
    auto *vec = static_cast<std::vector<unsigned char>*>(png_get_io_ptr(png_ptr));
    vec->insert(vec->end(), data, data + sz);
}

static void mem_flush_fn(png_structp UNUSED(png_ptr)) {
}

static int GImageWritePngFull(GImage *gi, void *io, bool in_memory, int compression_level, bool progressive) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **rows;
    int i;
    int bit_depth;
    int color_type;
    int num_palette;
    png_bytep trans_alpha = NULL;
    png_color_16p trans_color = NULL;
    png_colorp palette = NULL;

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr) {
return(false);
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
   }

#if (PNG_LIBPNG_VER < 10500)
    if (setjmp(png_ptr->jmpbuf))
#else
   if (setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof (jmp_buf))))
#endif
   {
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
   }

   if (in_memory) {
        png_set_write_fn(png_ptr, io, mem_write_fn, mem_flush_fn);
   } else {
        png_init_io(png_ptr, (FILE*)io);
   }

   if (compression_level >= 0 && compression_level <= 9) {
        png_set_compression_level(png_ptr, compression_level);
   }

   bit_depth = 8;
   num_palette = base->clut==NULL?2:base->clut->clut_len;
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       color_type = PNG_COLOR_TYPE_PALETTE;
       if ( num_palette<=2 )
	   bit_depth=1;
       else if ( num_palette<=4 )
	   bit_depth=2;
       else if ( num_palette<=16 )
	   bit_depth=4;
   } else {
       color_type = PNG_COLOR_TYPE_RGB;
       if ( base->image_type == it_rgba )
	   color_type = PNG_COLOR_TYPE_RGB_ALPHA;
   }

   png_set_IHDR(png_ptr, info_ptr, base->width, base->height,
		bit_depth, color_type, progressive,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       palette = (png_color *) malloc(num_palette*sizeof(png_color));
       if ( base->clut==NULL ) {
	    palette[0].red = palette[0].green = palette[0].blue = 0;
	    palette[1].red = palette[1].green = palette[1].blue = 0xff;
       } else {
	   for ( i=0; i<num_palette; ++i ) {
		long col = base->clut->clut[i];
		palette[i].red = COLOR_RED(col);
		palette[i].green = COLOR_GREEN(col);
		palette[i].blue = COLOR_BLUE(col);
	   }
       }
       png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
       if ( num_palette<=16 )
	   png_set_packing(png_ptr);

       if ( base->trans!=(Color)-1 ) {
	   trans_alpha = (png_bytep) malloc(1);
	   trans_alpha[0] = base->trans;
       }
   } else {
       if ( base->trans!=(Color)-1 ) {
	   trans_color = (png_color_16p) malloc(sizeof(png_color_16));
	   trans_color->red = COLOR_RED(base->trans);
	   trans_color->green = COLOR_GREEN(base->trans);
	   trans_color->blue = COLOR_BLUE(base->trans);
       }
   }
   if ( base->trans!=(Color)-1 ) {
       png_set_tRNS(png_ptr, info_ptr, trans_alpha, 1, trans_color);
   }
   png_write_info(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_filler(png_ptr, '\0', PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        png_set_bgr(png_ptr);

    rows = (png_byte **) malloc(base->height*sizeof(png_byte *));
    for ( i=0; i<base->height; ++i )
	rows[i] = (png_byte *) (base->data + i*base->bytes_per_line);

    png_write_image(png_ptr,rows);

    png_write_end(png_ptr, info_ptr);

    free(trans_alpha);
    free(trans_color);
    free(palette);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(rows);
return( 1 );
}

int GImageWritePngBuf(GImage *gi, char** buf, size_t* sz, int compression_level, int progressive) {
    std::vector<unsigned char> vec;
    *buf = NULL;
    *sz = 0;

    if (!GImageWritePngFull(gi, &vec, true, compression_level, progressive)) {
        return false;
    }

    *buf = (char*) malloc(vec.size());
    if (*buf == NULL) {
        return false;
    }
    *sz = vec.size();

    memcpy(*buf, vec.data(), vec.size());
    return true;
}

int GImageWrite_Png(GImage *gi, FILE *fp, int progressive) {
    return GImageWritePngFull(gi, fp, false, -1, progressive);
}

int GImageWritePng(GImage *gi, char *filename, int progressive) {
    FILE *fp;
    int ret;

   /* open the file */
   fp = fopen(filename, "wb");
   if (!fp)
return(false);
    ret = GImageWrite_Png(gi,fp,progressive);
    fclose(fp);
return( ret );
}

#endif
