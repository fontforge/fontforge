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
#include "string.h"

static void WriteBase(FILE *file, struct _GImage *base, char *stem, int instance) {
    int i,j,k;
    uint32 *ipt;
    uint8 *pt;

    if ( base->image_type==it_true ) {
	fprintf(file,"static uint32 %s%d_data[] = {\n", stem, instance );
	for ( i=0; i<base->height; ++i ) {
	    ipt = (uint32 *) (base->data+i*base->bytes_per_line);
	    for ( j=0; j<base->width; ) {
		fprintf( file, j==0?"    ":"\t");
		for ( k=0; k<8 && j<base->width; ++k, ++j, ++ipt )
		    fprintf( file, "0x%x%s", (unsigned int) *ipt, j==base->width-1 && i==base->height-1?"":", ");
		fprintf( file, "\n");
	    }
	}
    } else {
	fprintf(file,"static uint8 %s%d_data[] = {\n", stem, instance );
	for ( i=0; i<base->height; ++i ) {
	    pt = (uint8 *) (base->data+i*base->bytes_per_line);
	    for ( j=0; j<base->bytes_per_line; ) {
		fprintf( file, j==0?"    ":"\t");
		for ( k=0; k<8 && j<base->bytes_per_line; ++k, ++j, ++pt )
		    fprintf( file, "0x%x%s", *pt, j==base->width-1 && i==base->height-1?"":", ");
		fprintf( file, "\n");
	    }
	}
    }
    fprintf(file,"};\n" );

    if ( base->clut!=NULL ) {
	fprintf(file, "\nstatic GClut %s%d_clut = { %d, %d, %d,\n",
		stem, instance,
		base->clut->clut_len, base->clut->is_grey, (int) base->clut->trans_index );
	for ( i=0; i<base->clut->clut_len; ) {
	    fprintf( file, "    " );
	    for ( k=0; k<8 && i<base->clut->clut_len; ++k, ++i )
		fprintf( file, "0x%x%s", (int) base->clut->clut[i], i==base->clut->clut_len-1?" };":", ");
	    fprintf( file, "\n");
	}
    }
    fprintf(file,"\nstatic struct _GImage %s%d_base = {\n", stem, instance );
    fprintf(file,base->image_type==it_true?"    it_true,\n":
		 base->image_type==it_index?"    it_index,\n":
		 "    it_mono,\n" );
    fprintf( file,"    %d,%d,%d,%d,\n",(int) base->delay, (int) base->width, (int) base->height, (int) base->bytes_per_line );
    fprintf( file,"    (uint8 *) %s%d_data,\n", stem,instance );
    fprintf( file,base->clut==NULL?"    NULL,\n":"    &%s%d_clut,\n", stem, instance );
    fprintf( file,"    0x%x\n};\n\n", (int) base->trans );
}

int GImageWriteGImage(GImage *gi, char *filename) {
    FILE *file;
    int i;
    char stem[256];
    char *pt;

    if (( pt = strrchr(filename,'/'))==NULL )
	strcpy(stem,filename);
    else
	strcpy(stem,pt+1);
    if ( (pt = strchr(stem,'.'))!=NULL )
	*pt = '\0';

    if ((file=fopen(filename,"w"))==NULL )
return(false);
    fprintf(file,"#include \"gimage.h\"\n\n" );
    if ( gi->list_len==0 ) {
	WriteBase(file,gi->u.image,stem,0);
	fprintf( file, "GImage %s = { 0, &%s0_base };\n", stem, stem );
    } else {
	for ( i=0; i<gi->list_len; ++i )
	    WriteBase(file,gi->u.images[i],stem,i);
	fprintf( file, "static struct _GImage *%s_bases = {\n", stem );
	for ( i=0; i<gi->list_len; ++i )
	    fprintf( file, "    &%s%d_base%s\n", stem, i, i==gi->list_len-1?"":"," );
	fprintf( file, "};\n\n" );
	
	fprintf( file, "GImage %s = { %d, (struct _GImage *) %s_bases };\n",
		stem, gi->list_len, stem );
    }
    fflush(file);
    i = ferror(file);
    fclose(file);
return( !i );
}
