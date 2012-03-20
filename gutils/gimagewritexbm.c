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

int GImageWriteXbm(GImage *gi, char *filename) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    FILE *file;
    int i,j, val,val2,k;
    char stem[256];
    char *pt; uint8 *scanline;

    if ( base->image_type!=it_mono )
return(false );

    if (( pt = strrchr(filename,'/'))==NULL )
	strcpy(stem,filename);
    else
	strcpy(stem,pt+1);
    if ( (pt = strchr(stem,'.'))!=NULL )
	*pt = '\0';

    if ((file=fopen(filename,"w"))==NULL )
return(false);
    fprintf(file,"#define %s_width %d\n", stem, (int) base->width );
    fprintf(file,"#define %s_height %d\n", stem, (int) base->height );
    fprintf(file,"static unsigned char %s_bits[] = {\n", stem );
    for ( i=0; i<base->height; ++i ) {
	fprintf(file,"  ");
	scanline = base->data + i*base->bytes_per_line;
	for ( j=0; j<base->bytes_per_line; ++j ) {
	    val = *scanline++; val2=0;
	    for ( k=0; k<8 ; ++k ) {
		if ( (val&(1<<k)) )
		    val2 |= (0x80>>k);
	    }
	    fprintf(file,"0x%x%s", val2^0xff, i==base->height-1 && j==base->bytes_per_line-1?"":", " );
	}
	fprintf(file,"\n");
    }
    fprintf(file,"};\n");
    fflush(file);

    i = ferror(file);
    fclose(file);
return( !i );
}
