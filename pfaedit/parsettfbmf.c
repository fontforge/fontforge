/* Copyright (C) 2001 by George Williams */
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
#include "pfaedit.h"
#include "chardata.h"
#include "utype.h"
#include "ustring.h"
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "ttf.h"

struct ttfsizehead {
    int ppem;
    int firstglyph, endglyph;
    int indexSubTableArrayOffset;
    int numIndexSubTables;
};

struct bigmetrics {
    int height, width;
    int hbearingX, hbearingY, hadvance;
    int vbearingX, vbearingY, vadvance;	/* Small metrics doesn't use this row */
};

static void ttfreadbmfglyph(FILE *ttf,struct ttfinfo *info,
	int32 offset, int32 len, struct bigmetrics *metrics, int imageformat,
	int enc, BDFFont *bdf) {
    BDFChar *bdfc = gcalloc(1,sizeof(BDFChar));
    struct bigmetrics big;
    int i,j,ch,l,p, num;

    if ( enc>=bdf->charcnt )
return;
    if ( len==0 )		/* Missing, but encoded, glyph */
return;

    if ( offset!=-1 )
	fseek(ttf,info->bitmapdata_start+offset,SEEK_SET);

    if ( imageformat==1 || imageformat==2 || imageformat==8 ) {
	/* I ignore vertical strikes, so all short metrics are horizontal for me*/
	big.height = getc(ttf);
	big.width = getc(ttf);
	big.hbearingX = (signed char) getc(ttf);
	big.hbearingY = (signed char) getc(ttf);
	big.hadvance = getc(ttf);
	big.vbearingX = 0;
	big.vbearingY = 0;
	big.vadvance = 0;
	metrics = &big;
	if ( imageformat==8 )
	    /* pad = */ getc(ttf);
    } else if ( imageformat==6 || imageformat==7 || imageformat==9 ) {
	big.height = getc(ttf);
	big.width = getc(ttf);
	big.hbearingX = (signed char) getc(ttf);
	big.hbearingY = (signed char) getc(ttf);
	big.hadvance = getc(ttf);
	big.vbearingX = (signed char) getc(ttf);
	big.vbearingY = (signed char) getc(ttf);
	big.vadvance = getc(ttf);
	metrics = &big;
    } else if ( imageformat==5 ) {
	/* metrics from EBLC */
	/* Do nothing here */
	if ( metrics==NULL ) {
	    fprintf( stderr, "Unexpected use of bitmap format 5, no metrics are appearant\n" );
	    /*fseek(ttf,len,SEEK_CUR);*/
return;
	}
    } else {
	/* format 3 is obsolete */
	/* format 4 is compressed apple and I'm not supporting it (Nor is MS) */
	if ( imageformat==3 && !info->obscomplain ) {
	    fprintf( stderr, "This font contains bitmaps in the obsolete format 3 (And I can't read them)\n" );
	    info->obscomplain = true;
	} else if ( imageformat==4 ) {
	    /* Apple doesn't describe it (fully) in their spec. */
	    /* MS doesn't support it (and doesn't describe) */
	    /* Adobe doesn't describe it (and says MS doesn't support it) */
	    fprintf( stderr, "This font contains bitmaps in Apple's compressed format 4 (And I don't support that)\n" );
	    info->cmpcomplain = true;
	} else if ( !info->unkcomplain ) {
	    fprintf( stderr, "This font contains bitmaps in a format %d that I've never heard of\n", imageformat );
	    info->unkcomplain = true;
	}
return;
    }
    if ( info->chars!=NULL ) {
	if ( info->chars[enc]==NULL ) {
	    info->chars[enc] = chunkalloc(sizeof(SplineChar));
	    info->chars[enc]->enc = enc;
	}
	bdfc->sc = info->chars[enc];
    }
    bdfc->enc = enc;
    if ( bdf->chars[enc]!=NULL ) /* Shouldn't happen of course */
	BDFCharFree(bdf->chars[enc]);
    bdf->chars[enc] = bdfc;

    bdfc->width = metrics->hadvance;
    bdfc->xmin = metrics->hbearingX;
    bdfc->xmax = bdfc->xmin+metrics->width-1;
    bdfc->ymax = metrics->hbearingY;
    bdfc->ymin = bdfc->ymax-metrics->height+1;
    bdfc->bytes_per_line = (metrics->width+7)/8;
    bdfc->bitmap = gcalloc(metrics->height*bdfc->bytes_per_line,sizeof(uint8));

    if ( imageformat==1 || imageformat==6 ) {
	/* each row is byte aligned */
	for ( i=0; i<metrics->height; ++i )
	    fread(bdfc->bitmap+i*bdfc->bytes_per_line,1,bdfc->bytes_per_line,ttf);
    } else if ( imageformat==2 || imageformat==5 || imageformat==7 ) {
	/* each row is bit aligned, but each glyph is byte aligned */
	for ( i=0; i<(metrics->height*metrics->width+7)/8; ++i ) {
	    ch = getc(ttf);
	    for ( j=0; j<8; ++j ) {
		l = (i*8+j)/metrics->width;
		p = (i*8+j)%metrics->width;
		if ( l<metrics->height && (ch&(1<<(7-j))) )
		    bdfc->bitmap[l*bdfc->bytes_per_line+(p>>3)] |= (1<<(7-(p&7)));
	    }
	}
    } else if ( imageformat==8 || imageformat==9 ) {
	/* composite, we don't support (but we will parse enough to skip over) */
	num = getushort(ttf);
	for ( i=0; i<num; ++i )
	    getlong(ttf);	/* composed of short glyphcode, byte xoff,yoff */
    }
    GProgressNext();
}
    
static void readttfbitmapfont(FILE *ttf,struct ttfinfo *info,
	struct ttfsizehead *head, BDFFont *bdf) {
    int i, j;
    int indexformat, imageformat, size, num, moreoff;
    int32 offset, *glyphoffsets, *glyphs, loc;
    int first, last;
    struct bigmetrics big;

    fseek(ttf,info->bitmaploc_start+head->indexSubTableArrayOffset,SEEK_SET);
    for ( j=0; j<head->numIndexSubTables; ++j ) {
	first = getushort(ttf);
	last =  getushort(ttf);
	moreoff = getlong(ttf);
	loc = ftell(ttf);
	fseek(ttf,info->bitmaploc_start+head->indexSubTableArrayOffset+moreoff,SEEK_SET);

	indexformat=getushort(ttf);
	imageformat=getushort(ttf);
	offset = getlong(ttf);
	switch ( indexformat ) {
	  case 1: case 3:
	    glyphoffsets = galloc((last-first+2)*sizeof(int32));
	    for ( i=0; i<(last-first+2); ++i )
		glyphoffsets[i] = indexformat==3?getushort(ttf):getlong(ttf);
	    if ( indexformat==3 && ((last-first)&1) )
		getushort(ttf);
	    for ( i=0; i<=last-first; ++i ) {
		if ( info->inuse==NULL || info->inuse[i+first] )
		    ttfreadbmfglyph(ttf,info,offset+glyphoffsets[i],
			    glyphoffsets[i+1]-glyphoffsets[i],NULL,
			    imageformat,i+first,bdf);
	    }
	    free(glyphoffsets);
	  break;
	  case 2:
	    size = getlong(ttf);
	    big.height = getc(ttf);
	    big.width = getc(ttf);
	    big.hbearingX = (signed char) getc(ttf);
	    big.hbearingY = (signed char) getc(ttf);
	    big.hadvance = getc(ttf);
	    big.vbearingX = (signed char) getc(ttf);
	    big.vbearingY = (signed char) getc(ttf);
	    big.vadvance = getc(ttf);
	    for ( i=first; i<=last; ++i ) {
		if ( info->inuse==NULL || info->inuse[i+first] )
		    ttfreadbmfglyph(ttf,info,offset,
			    size,&big,
			    imageformat,i,bdf);
		else
		    fseek(ttf,size,SEEK_CUR);
		offset = -1;
	    }
	  break;
	  case 4:
	    num = getlong(ttf);
	    glyphoffsets = galloc((num+1)*sizeof(int32));
	    glyphs = galloc((num+1)*sizeof(int32));
	    for ( i=0; i<num+1; ++i ) {
		glyphs[i] = getushort(ttf);
		glyphoffsets[i] = getushort(ttf);
	    }
	    for ( i=0; i<=last-first; ++i ) {
		if ( info->inuse==NULL || info->inuse[i+first] )
		    ttfreadbmfglyph(ttf,info,offset+glyphoffsets[i],
			    glyphoffsets[i+1]-glyphoffsets[i],NULL,
			    imageformat,glyphs[i],bdf);
	    }
	    free(glyphoffsets);
	    free(glyphs);
	  break;
	  case 5:
	    size = getlong(ttf);
	    big.height = getc(ttf);
	    big.width = getc(ttf);
	    big.hbearingX = (signed char) getc(ttf);
	    big.hbearingY = (signed char) getc(ttf);
	    big.hadvance = getc(ttf);
	    big.vbearingX = (signed char) getc(ttf);
	    big.vbearingY = (signed char) getc(ttf);
	    big.vadvance = getc(ttf);
	    num = getlong(ttf);
	    glyphs = galloc((num+1)*sizeof(int32));
	    for ( i=0; i<num; ++i ) {
		glyphs[i] = getushort(ttf);
	    }
	    if ( num&1 )
		getushort(ttf);		/* padding */
	    for ( i=first; i<=last; ++i ) {
		if ( info->inuse==NULL || info->inuse[i+first] )
		    ttfreadbmfglyph(ttf,info,offset,
			    size,&big,
			    imageformat,glyphs[i],bdf);
		offset = -1;
	    }
	    free(glyphs);
	  break;
	}
	fseek(ttf,loc,SEEK_SET);
    }
 fflush(stdout);
}

void TTFLoadBitmaps(FILE *ttf,struct ttfinfo *info,int onlyone) {
    int i, cnt, j, k, good;
    int bigval, biggest;
    struct ttfsizehead *sizes;
    BDFFont *bdf, *last;
    const unichar_t **choices;
    char *sel;
    char buf[10];
    static int buttons[]= { _STR_Yes, _STR_No };
    unichar_t ubuf[100];

    fseek(ttf,info->bitmaploc_start,SEEK_SET);
    /* version = */ getlong(ttf);		/* Had better be 0x00020000, or 2.0 */
    cnt = getlong(ttf);
    sizes = galloc(cnt*sizeof(struct ttfsizehead));
    /* we may not like all the possible bitmaps. Some might be designed for */
    /*  non-square pixels, others might be grey scale, others might be */
    /*  vertical data. So only pick out the ones we can use */
    bigval = biggest = -1;
    for ( i = j = 0; i<cnt; ++i ) {
	good = true;
	sizes[j].indexSubTableArrayOffset = getlong(ttf);
	/* size = */ getlong(ttf);
	sizes[j].numIndexSubTables = getlong(ttf);
	if ( /* colorRef = */ getlong(ttf)!=0 )
	    good = false;
	for ( k=0; k<12; ++k ) getc(ttf);	/* Horizontal Line Metrics */
	for ( k=0; k<12; ++k ) getc(ttf);	/* Vertical   Line Metrics */
	sizes[j].firstglyph = getushort(ttf);
	sizes[j].endglyph = getushort(ttf);
	sizes[j].ppem = getc(ttf);		/* X */
	if ( /* ppemY */ getc(ttf) != sizes[j].ppem )
	    good = false;
	if ( /* bitDepth */ getc(ttf) != 1 )
	    good = false;
	if ( /* flags */ !(getc(ttf)&1) )		/* !Horizontal */
	    good = false;
	if ( good ) {
	    if ( sizes[j].ppem > bigval ) {
		biggest = j;
		bigval = sizes[j].ppem;
	    }
	}
	j += good;
    }
    cnt = j;
    if ( cnt==0 )
return;

    /* Ask user which (if any) s/he is interested in */
    choices = gcalloc(cnt+1,sizeof(unichar_t *));
    sel = gcalloc(cnt,sizeof(char));
    for ( i=0; i<cnt; ++i ) {
	sprintf(buf,"%d", sizes[i].ppem );
	choices[i] = uc_copy(buf);
    }
    if ( onlyone ) {
	biggest=GWidgetChoicesBR(_STR_LoadBitmapFonts, choices,cnt,biggest,buttons,_STR_LoadTTFBitmaps);
	if ( biggest!=-1 ) sel[biggest] = true;
    } else {
	biggest = GWidgetChoicesBRM(_STR_LoadBitmapFonts, choices,sel,cnt,buttons,_STR_LoadTTFBitmaps);
    }
    for ( i=0; i<cnt; ++i ) free( (unichar_t *) (choices[i]));
    free(choices);
    if ( biggest==-1 ) {		/* Cancelled */
	free(sizes); free(sel);
return;
    }
    /* Remove anything not selected */
    for ( i=j=0; i<cnt; ++i ) {
	if ( sel[i] )
	    sizes[j++] = sizes[i];
    }
    cnt = j;

    GProgressChangeStages(3+cnt);
    info->bitmaps = last = NULL;
    for ( i=0; i<cnt; ++i ) {
	bdf = gcalloc(1,sizeof(BDFFont));
	bdf->charcnt = info->glyph_cnt;
	bdf->chars = gcalloc(bdf->charcnt,sizeof(BDFChar *));
	bdf->pixelsize = sizes[i].ppem;
	bdf->ascent = (info->ascent*bdf->pixelsize + info->emsize/2)/info->emsize;
	bdf->descent = bdf->pixelsize - bdf->ascent;
	if ( last==NULL )
	    info->bitmaps = bdf;
	else
	    last->next = bdf;
	last = bdf;
	u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),GStringGetResource(_STR_DPixelBitmap,NULL),
		sizes[i].ppem );
	GProgressChangeLine2(ubuf);
	readttfbitmapfont(ttf,info,&sizes[i],bdf);
	GProgressNextStage();
    }
    free(sizes); free(sel);
}
