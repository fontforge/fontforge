/* Copyright (C) 2000-2012 by George Williams */
/* 2013mar3, bug-fixes plus type-formatting, Jose Da Silva */
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

static void WriteBase(FILE *file, struct _GImage *base, char *stem, int instance) {
/* Write one image in C code which can be compiled into FontForge. */
/* This routine is called and used by GImageWriteGImage() */
    int i,j,k;
    uint32 *ipt;
    uint8 *pt;
    long val;

    if ( base->image_type==it_true ) {
	fprintf(file,"static uint32 %s%d_data[] = {\n",stem,instance);
	for ( i=0; i<base->height; ++i ) {
	    ipt = (uint32 *) (base->data+i*base->bytes_per_line);
	    for ( j=0; j<base->width; ) {
		fprintf(file,j==0?"    ":"\t");
		for ( k=0; k<8 && j<base->width; ++k, ++j, ++ipt ) {
		    val=*ipt&0xffffffff;
		    fprintf(file,"0x%.8x%s",(unsigned int) val,j==base->width-1 && i==base->height-1?"":", ");
		}
		fprintf(file,"\n");
	    }
	}
    } else {
	fprintf(file,"static uint8 %s%d_data[] = {\n",stem,instance);
	for ( i=0; i<base->height; ++i ) {
	    pt = (uint8 *) (base->data+i*base->bytes_per_line);
	    for ( j=0; j<base->bytes_per_line; ) {
		fprintf(file,j==0?"    ":"\t");
		for ( k=0; k<8 && j<base->bytes_per_line; ++k, ++j, ++pt )
		    fprintf(file,"0x%.2x%s",*pt,j==base->width-1 && i==base->height-1?"":", ");
		fprintf(file,"\n");
	    }
	}
    }
    fprintf(file,"};\n");

    if ( base->clut!=NULL ) {
	fprintf(file,"\nstatic GClut %s%d_clut = { %d, %d, %ld,\n",
		stem,instance,
		base->clut->clut_len, base->clut->is_grey,(unsigned long) base->clut->trans_index&0xfffffff );
	for ( i=0; i<base->clut->clut_len; ) {
	    fprintf(file,"    ");
	    for ( k=0; k<8 && i<base->clut->clut_len; ++k, ++i ) {
		val=base->clut->clut[i]&0xffffffff;
		fprintf(file,"0x%.8x%s",(unsigned int) val,i==base->clut->clut_len-1?" };":", ");
	    }
	    fprintf(file,"\n");
	}
    }
    fprintf(file,"\nstatic struct _GImage %s%d_base = {\n",stem,instance);
    fprintf(file,base->image_type==it_true?"    it_true,\n":
		 base->image_type==it_index?"    it_index,\n":
		 "    it_mono,\n" );
    fprintf(file,"    %d,%ld,%ld,%ld,\n",(int) base->delay,(long) base->width,(long) base->height,(long) base->bytes_per_line);
    fprintf(file,"    (uint8 *) %s%d_data,\n",stem,instance);
    if (base->clut==NULL)
        fprintf(file,"    NULL,\n" );
    else
        fprintf(file,"    &%s%d_clut,\n",stem,instance);
    fprintf(file,"    0x%.8x\n};\n\n",(unsigned int) base->trans&0xffffffff);
}

int GImageWriteGImage(GImage *gi, char *filename) {
/* Export a GImage that can be used by FontForge. Return 0 if all done okay */
    FILE *file;
    int i;
    char stem[256];
    char *pt;

    if ( gi==NULL )
	return( -1 );

    /* get filename stem (255chars max) */
    if ( (pt=strrchr(filename,'/'))!=NULL )
	++pt;
    else
	pt=filename;
    strncpy(stem,pt,sizeof(stem)); stem[255]='\0';
    if ( (pt=strrchr(stem,'.'))!=NULL && pt!=stem )
	*pt = '\0';

    /* Begin writing C code to the file */
    if ( (file=fopen(filename,"w"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( -1 );
    }
    fprintf(file,"/* This file was generated using GImageWriteGImage(gi,\"%s\") */\n",filename);
    fprintf(file,"#include \"gimage.h\"\n\n" );
    if ( gi->list_len==0 ) {
	/* Construct a single image */
	WriteBase(file,gi->u.image,stem,0);
	fprintf(file,"GImage %s = { 0, &%s0_base };\n",stem,stem);
    } else {
	/* Construct an array of images */
	for ( i=0; i<gi->list_len; ++i )
	    WriteBase(file,gi->u.images[i],stem,i);
	fprintf(file,"static struct _GImage *%s_bases = {\n",stem);
	for ( i=0; i<gi->list_len; ++i )
	    fprintf(file,"    &%s%d_base%s\n", stem, i, i==gi->list_len-1?"":"," );
	fprintf(file,"};\n\n" );
	
	fprintf(file,"GImage %s = { %d, (struct _GImage *) %s_bases };\n",stem,gi->list_len,stem);
    }
    fflush(file);
    i=ferror(file);
    fclose(file);
    return( i );
}
