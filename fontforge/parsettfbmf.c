/* Copyright (C) 2000-2004 by George Williams */
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
    int depth;
    int firstglyph, endglyph;
    int indexSubTableArrayOffset;
    int numIndexSubTables;
};

struct bigmetrics {
    int height, width;
    int hbearingX, hbearingY, hadvance;
    int vbearingX, vbearingY, vadvance;	/* Small metrics doesn't use this row */
};

struct bdfcharlist {
    BDFChar *bc;
    struct bdfcharlist *next;
};

static void ttfreadbmfglyph(FILE *ttf,struct ttfinfo *info,
	int32 offset, int32 len, struct bigmetrics *metrics, int imageformat,
	int enc, BDFFont *bdf) {
    BDFChar *bdfc;
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
    bdfc = chunkalloc(sizeof(BDFChar));
    if ( info->chars!=NULL ) {
	if ( info->chars[enc]==NULL ) {
	    info->chars[enc] = SplineCharCreate();
	    info->chars[enc]->enc = enc;
	    info->chars[enc]->unicodeenc = -1;
	    info->chars[enc]->width = info->chars[enc]->vwidth = info->emsize;
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
    bdfc->ymax = metrics->hbearingY-1;
    bdfc->ymin = bdfc->ymax-metrics->height+1;
    if ( bdf->clut==NULL ) {
	bdfc->bytes_per_line = (metrics->width+7)/8;
	bdfc->depth = 1;
    } else {
	bdfc->bytes_per_line = metrics->width;
	bdfc->byte_data = true;
	bdfc->depth = bdf->clut->clut_len==4 ? 2 : bdf->clut->clut_len==16 ? 4 : 8;
    }
    bdfc->bitmap = gcalloc(metrics->height*bdfc->bytes_per_line,sizeof(uint8));

    if ( imageformat==8 || imageformat==9 ) {
	/* composite, we don't support (but we will parse enough to skip over) */
	num = getushort(ttf);
	for ( i=0; i<num; ++i )
	    getlong(ttf);	/* composed of short glyphcode, byte xoff,yoff */
    } else if ( imageformat==1 || imageformat==6 ) {
	/* each row is byte aligned */
	if ( bdf->clut==NULL || bdf->clut->clut_len==256 ) {
	    for ( i=0; i<metrics->height; ++i )
		fread(bdfc->bitmap+i*bdfc->bytes_per_line,1,bdfc->bytes_per_line,ttf);
	} else if ( bdf->clut->clut_len==4 ) {
	    for ( i=0; i<metrics->height; ++i ) {
		for ( j=0; j<metrics->width; j+=4 ) {
		    ch = getc(ttf);
		    bdfc->bitmap[i*bdfc->bytes_per_line+j] = (ch>>6);
		    if ( j+1 < metrics->width )
			bdfc->bitmap[i*bdfc->bytes_per_line+j+1] = (ch>>4)&3;
		    if ( j+2 < metrics->width )
			bdfc->bitmap[i*bdfc->bytes_per_line+j+2] = (ch>>2)&3;
		    if ( j+3 < metrics->width )
			bdfc->bitmap[i*bdfc->bytes_per_line+j+3] = ch&3;
		}
	    }
	} else if ( bdf->clut->clut_len==16 ) {
	    for ( i=0; i<metrics->height; ++i ) {
		for ( j=0; j<metrics->width; j+=2 ) {
		    ch = getc(ttf);
		    bdfc->bitmap[i*bdfc->bytes_per_line+j] = (ch>>4);
		    if ( j+1 < metrics->width )
			bdfc->bitmap[i*bdfc->bytes_per_line+j+1] = ch&0xf;
		}
	    }
	}
    } else if ( imageformat==2 || imageformat==5 || imageformat==7 ) {
	/* each row is bit aligned, but each glyph is byte aligned */
	if ( bdf->clut==NULL ) {
	    for ( i=0; i<(metrics->height*metrics->width+7)/8; ++i ) {
		ch = getc(ttf);
		for ( j=0; j<8; ++j ) {
		    l = (i*8+j)/metrics->width;
		    p = (i*8+j)%metrics->width;
		    if ( l<metrics->height && (ch&(1<<(7-j))) )
			bdfc->bitmap[l*bdfc->bytes_per_line+(p>>3)] |= (1<<(7-(p&7)));
		}
	    }
	} else if ( bdf->clut->clut_len==256 ) {
	    /* well, yeah, it's bit aligned, but since pixels are bytes, it's byte aligned */
	    for ( i=0; i<metrics->height; ++i )
		fread(bdfc->bitmap+i*bdfc->bytes_per_line,1,bdfc->bytes_per_line,ttf);
	} else if ( bdf->clut->clut_len==4 ) {
	    for ( i=0; i<metrics->height*metrics->width; i+=4 ) {
		ch = getc(ttf);
		bdfc->bitmap[(i/metrics->width)*bdfc->bytes_per_line+i%metrics->width] = (ch>>6);
		if ( i+1 < metrics->width*metrics->height )
		    bdfc->bitmap[((i+1)/metrics->width)*bdfc->bytes_per_line+(i+1)%metrics->width] = (ch>>4)&3;
		if ( i+2 < metrics->width*metrics->height )
		    bdfc->bitmap[((i+2)/metrics->width)*bdfc->bytes_per_line+(i+2)%metrics->width] = (ch>>2)&3;
		if ( i+3 < metrics->width*metrics->height )
		    bdfc->bitmap[((i+3)/metrics->width)*bdfc->bytes_per_line+(i+3)%metrics->width] = ch&3;
	    }
	} else if ( bdf->clut->clut_len==16 ) {
	    for ( i=0; i<metrics->height*metrics->width; i+=2 ) {
		ch = getc(ttf);
		bdfc->bitmap[(i/metrics->width)*bdfc->bytes_per_line+i%metrics->width] = (ch>>4);
		if ( i+1 < metrics->width*metrics->height )
		    bdfc->bitmap[((i+1)/metrics->width)*bdfc->bytes_per_line+(i+1)%metrics->width] = ch&0xf;
	    }
	}
    }
    GProgressNext();
}
    
static void readttfbitmapfont(FILE *ttf,struct ttfinfo *info,
	struct ttfsizehead *head, BDFFont *bdf) {
    int i, j, g;
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
		if ( info->inuse==NULL || info->inuse[i] ) {
		    ttfreadbmfglyph(ttf,info,offset,
			    size,&big,
			    imageformat,i,bdf);
		    offset += size;
		} else {
		    fseek(ttf,size,SEEK_CUR);
		    offset = -1;
		}
	    }
	  break;
	  case 4:
	    num = getlong(ttf);
	    glyphoffsets = galloc((num+1)*sizeof(int32));
	    glyphs = galloc((num+1)*sizeof(int32));
	    for ( g=0; g<num+1; ++g ) {
		glyphs[g] = getushort(ttf);
		glyphoffsets[g] = getushort(ttf);
	    }
	    for ( i=0, g=0; i<=last-first; ++i, ++g ) {
		if ( (info->inuse==NULL || info->inuse[i+first]) && g<num )
		    ttfreadbmfglyph(ttf,info,offset+glyphoffsets[g],
			    glyphoffsets[g+1]-glyphoffsets[g],NULL,
			    imageformat,glyphs[g],bdf);
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
	    for ( g=0; g<num; ++g ) {
		glyphs[g] = getushort(ttf);
	    }
	    if ( num&1 )
		getushort(ttf);		/* padding */
	    for ( i=first, g=0; i<=last; ++i, ++g ) {
		if ( (info->inuse==NULL || info->inuse[i+first]) && g<num )
		    ttfreadbmfglyph(ttf,info,offset,
			    size,&big,
			    imageformat,glyphs[g],bdf);
		offset = -1;
	    }
	    free(glyphs);
	  break;
	  default:
	    fprintf(stderr,"Didn't understand index format: %d\n", indexformat );
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
    /*  non-square pixels, others might be color, others might be */
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
	if ( (sizes[j].depth = getc(ttf)) != 1 && sizes[j].depth!=2 &&
		sizes[j].depth!=4 && sizes[j].depth!=8 )
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
	if ( sizes[i].depth==1 )
	    sprintf(buf,"%d", sizes[i].ppem );
	else
	    sprintf(buf,"%d@%d", sizes[i].ppem, sizes[i].depth );
	choices[i] = uc_copy(buf);
    }
    /* When loading a ttf font with only bitmaps, and there's only one strike */
    /*  then just load that strike. Don't ask */
    if ( info->glyphlocations_start==0 && info->cff_start==0 && cnt==1 )
	sel[0] = true;
    else if ( screen_display==NULL ) {
	if ( onlyone ) {
	    biggest=0;
	    for ( i=1; i<cnt; ++i )
		if ( sizes[i].ppem>sizes[biggest].ppem && sizes[i].depth==1 )
		    biggest = i;
	    sel[biggest] = true;
	} else {
	    for ( i=0; i<cnt; ++i )
		sel[i] = true;
	}
    } else if ( onlyone ) {
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
	bdf->res = -1;
	if ( sizes[i].depth!=1 )
	    BDFClut(bdf,1<<(sizes[i].depth/2));
	if ( last==NULL )
	    info->bitmaps = bdf;
	else
	    last->next = bdf;
	last = bdf;
#if defined(FONTFORGE_CONFIG_GDRAW)
	u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),GStringGetResource(_STR_DPixelBitmap,NULL),
#elif defined(FONTFORGE_CONFIG_GTK)
	u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),_("%d pixel bitmap"),
#endif
		sizes[i].ppem );
	GProgressChangeLine2(ubuf);
	readttfbitmapfont(ttf,info,&sizes[i],bdf);
	GProgressNextStage();
    }
    free(sizes); free(sel);
}

/* ************************************************************************** */

static BDFChar glyph0, glyph1, glyph2;
static SplineChar sc0, sc1, sc2;
static struct bdfcharlist bl[3];

static struct bdfcharlist *BDFAddDefaultGlyphs(BDFFont *bdf) {
    /* when I dump out the glyf table I add 3 glyphs at the start. One is glyph*/
    /*  0, the one used when there is no match, and the other two are varients*/
    /*  on space. There will never be glyphs 1 and 2 in the font. There might */
    /*  be a glyph 0. Add the ones that don't exist */
    int width, w, i, j, bit, blpos=0;
    SplineFont *sf = bdf->sf;

    width = 0x7ffff;
    for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	if ( width == 0x7ffff )
	    width = bdf->chars[i]->width;
	else if ( width!=bdf->chars[i]->width ) {
	    width = 0x7fffe;
    break;
	}
    }
    if ( width>0x70000 )
	/* Proportional */
	width = bdf->pixelsize/3;

    memset(bl,0,sizeof(bl));
    if ( IsntBDFChar(bdf->chars[0])) {
	sc0.name = ".notdef";
	/* sc0.ttf_glyph = 0; /* already done */
	sc0.parent = sf;
	if ( width<4 ) w=4; else w=width;
	sc0.width = w*(sf->ascent+sf->descent)/bdf->pixelsize;
	sc0.widthset = true;
	glyph0.sc = &sc0;
	glyph0.width = w;
	if ( BDFDepth(bdf)!=1 ) {
	    glyph0.bytes_per_line = 1;	/* Needs to be 1 or BCRegularizeBitmap gets upset */
	    glyph0.ymax = 1;
	    glyph0.bitmap = gcalloc(8,1);
	} else {
	    glyph0.xmin = (w==4)?0:1;
	    glyph0.xmax = w-1;
	    glyph0.ymin = 0;
	    glyph0.ymax = 8*bdf->ascent/10;
	    glyph0.bytes_per_line = (glyph0.xmax-glyph0.xmin+8)/8;
	    glyph0.bitmap = gcalloc((glyph0.ymax-glyph0.ymin+1)*glyph0.bytes_per_line,1);
	    j = (glyph0.xmax-glyph0.xmin)>>3;
	    bit = 0x80>>((glyph0.xmax-glyph0.xmin)&7);
	    for ( i=0; i<=glyph0.ymax; ++i ) {
		glyph0.bitmap[i*glyph0.bytes_per_line] |= 0x80;
		glyph0.bitmap[i*glyph0.bytes_per_line+j] |= bit;
	    }
	    for ( i=0; i<glyph0.xmax-glyph0.xmin; ++i ) {
		glyph0.bitmap[i>>3] |= (0x80>>(i&7));
		glyph0.bitmap[glyph0.ymax*glyph0.bytes_per_line+(i>>3)] |= (0x80>>(i&7));
	    }
	}
	if ( bdf->chars[0]==NULL )
	    bdf->chars[0] = &glyph0;
	else
	    bl[blpos++].bc = &glyph0;
    }

    sc1.ttf_glyph = 1;
    sc2.ttf_glyph = 2;
    sc1.name = ".null"; sc2.name = "nonmarkingreturn";
    sc1.parent = sc2.parent = sf;
    sc1.width = sc2.width = width*(sf->ascent+sf->descent)/bdf->pixelsize;
    sc1.widthset = sc2.widthset = 1;
    glyph1.sc = &sc1; glyph2.sc = &sc2;
    /* xmin and so forth are zero, and are already clear */
    glyph1.width = glyph2.width = width;
    glyph1.bytes_per_line = glyph2.bytes_per_line = 1;	/* Needs to be 1 or BCRegularizeBitmap gets upset */
    glyph1.bitmap = glyph2.bitmap = (uint8 *) "";
    glyph1.enc = 1; glyph2.enc = 2;
    if ( bdf->chars[1]==NULL )
	bdf->chars[1] = &glyph1;
    else if ( (bdf->chars[0]!=NULL && bdf->chars[0]->sc->ttf_glyph==1) || bdf->chars[1]->sc->ttf_glyph==1 )
	/* Do nothing, ignore this dummy glyph */;
    else {
	if ( blpos!=0 )
	    bl[blpos-1].next = &bl[blpos];
	bl[blpos++].bc = &glyph1;
    }
    if ( bdf->chars[2]==NULL )
	bdf->chars[2] = &glyph2;
    else if ( (bdf->chars[1]!=NULL && bdf->chars[1]->sc->ttf_glyph==2) || bdf->chars[2]->sc->ttf_glyph==2 )
	/* Do nothing, ignore this dummy glyph */;
    else {
	if ( blpos!=0 )
	    bl[blpos-1].next = &bl[blpos];
	bl[blpos++].bc = &glyph2;
    }
return( blpos==0 ? NULL : &bl[0] );
}

static void BDFCleanupDefaultGlyphs(BDFFont *bdf) {
    if ( bdf->chars[0] == &glyph0 )
	bdf->chars[0] = NULL;
    if ( bdf->chars[1] == &glyph1 )
	bdf->chars[1] = NULL;
    if ( bdf->chars[2] == &glyph2 )
	bdf->chars[2] = NULL;
    free(glyph0.bitmap);
    glyph0.bitmap = NULL;
}

static void ttfdumpsmallmetrics(FILE *bdat, BDFChar *bc) {

    /* dump small metrics */
    putc(bc->ymax-bc->ymin+1,bdat);		/* height */
    putc(bc->xmax-bc->xmin+1,bdat);		/* width */
    putc(bc->xmin,bdat);			/* horiBearingX */
    putc(bc->ymax+1,bdat);			/* horiBearingY */
    putc(bc->width,bdat);			/* advance width */
}

static int32 ttfdumpf1bchar(FILE *bdat, BDFChar *bc,BDFFont *bdf) {
    /* format 1 character dump. small metrics, byte aligned data */
    int32 pos = ftell(bdat);
    int r,c,val;

    ttfdumpsmallmetrics(bdat,bc);

    /* dump image */
    for ( r=0; r<=bc->ymax-bc->ymin; ++r ) {
	if ( bdf->clut==NULL || bdf->clut->clut_len==256 ) {
	    for ( c=0; c<bc->bytes_per_line; ++c )
		putc(bc->bitmap[r*bc->bytes_per_line+c],bdat);
	} else if ( bdf->clut->clut_len==4 ) {
	    for ( c=0; c<bc->bytes_per_line; c+=2 ) {
		val = bc->bitmap[r*bc->bytes_per_line+c]<<4;
		if ( c+1<bc->bytes_per_line )
		    val += bc->bitmap[r*bc->bytes_per_line+c+1];
		putc(val,bdat);
	    }
	} else {
	    for ( c=0; c<bc->bytes_per_line; c+=4 ) {
		val = bc->bitmap[r*bc->bytes_per_line+c]<<6;
		if ( c+1<bc->bytes_per_line )
		    val += bc->bitmap[r*bc->bytes_per_line+c+1]<<4;
		if ( c+2<bc->bytes_per_line )
		    val += bc->bitmap[r*bc->bytes_per_line+c+2]<<2;
		if ( c+3<bc->bytes_per_line )
		    val += bc->bitmap[r*bc->bytes_per_line+c+3];
		putc(val,bdat);
	    }
	}
    }
return( pos );
}

static int32 ttfdumpf2bchar(FILE *bdat, BDFChar *bc,BDFFont *bdf) {
    /* format 2 character dump. small metrics, bit aligned data */
    int32 pos = ftell(bdat);
    int r,c,ch,bit,sh;

    ttfdumpsmallmetrics(bdat,bc);

    /* dump image */
    ch = 0; bit = 0x80; sh=7;
    for ( r=0; r<=bc->ymax-bc->ymin; ++r ) {
	for ( c = 0; c<=bc->xmax-bc->xmin; ++c ) {
	    if ( bdf->clut==NULL ) {
		if ( bc->bitmap[r*bc->bytes_per_line+(c>>3)] & (1<<(7-(c&7))) )
		    ch |= bit;
		bit >>= 1;
	    } else if ( bdf->clut->clut_len==4 ) {
		ch |= (bc->bitmap[r*bc->bytes_per_line+c]<<(sh-1));
		sh-=2;
		bit >>= 2;
	    } else {
		ch |= (bc->bitmap[r*bc->bytes_per_line+c]<<(sh-3));
		sh-=4;
		bit >>= 4;
	    }
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		sh = 7;
		bit = 0x80;
	    }
	}
    }
    if ( bit!=0x80 )
	putc(ch,bdat);
return( pos );
}

struct mymets {
    int ymin, ymax;
    int xmin, xmax;
    int width;			/* xmax-xmin+1 */
};

static int32 ttfdumpf5bchar(FILE *bdat, BDFChar *bc, struct mymets *mets,BDFFont *bdf) {
    /* format 5 character dump. no metrics, bit aligned data */
    /* now it may be that some of the character we are dumping have bitmaps */
    /*  that are a little smaller than the size specified in mymets. But that's*/
    /*  ok, we just pad them out with null bits */
    int32 pos = ftell(bdat);
    int r,c,ch,bit, sh;
    int depth = BDFDepth(bdf);

    /* dump image */
    ch = 0; bit = 0x80; sh = 7;
    for ( r=bc->ymax+1; r<=mets->ymax; ++r ) {
	for ( c=0; c<mets->width; ++c ) {
	    bit >>= depth;
	    sh -= depth;
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		sh = 7; bit = 0x80;
	    }
	}
    }
    for ( r=0; r<=bc->ymax-bc->ymin; ++r ) {
	for ( c = mets->xmin; c<bc->xmin; ++c ) {
	    bit >>= depth;
	    sh -= depth;
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		sh = 7; bit = 0x80;
	    }
	}
	for ( c = 0; c<=bc->xmax-bc->xmin; ++c ) {
	    if ( depth==1 ) {
		if ( bc->bitmap[r*bc->bytes_per_line+(c>>3)] & (1<<(7-(c&7))) )
		    ch |= bit;
	    } else if ( depth==8 )
		ch = bc->bitmap[r*bc->bytes_per_line+c];
	    else if ( depth==2 )
		ch |= (bc->bitmap[r*bc->bytes_per_line+c]<<(sh-1));
	    else if ( depth==4 )
		ch |= (bc->bitmap[r*bc->bytes_per_line+c]<<(sh-3));
	    bit >>= depth;
	    sh -= depth;
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		sh = 7; bit = 0x80;
	    }
	}
	for ( c = bc->xmax+1; c<=mets->xmax; ++c ) {
	    bit >>= depth;
	    sh -= depth;
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		sh = 7; bit = 0x80;
	    }
	}
    }
    for ( r=bc->ymin-1; r>=mets->ymin; --r ) {
	for ( c=0; c<mets->width; ++c ) {
	    bit >>= depth;
	    if ( bit==0 ) {
		putc(ch,bdat);
		ch = 0;
		bit = 0x80;
	    }
	}
    }
    if ( bit!=0x80 )
	putc(ch,bdat);
return( pos );
}

struct bitmapSizeTable {
    int32 subtableoffset;
    int32 tablesize;
    int32 numsubtables;
    int32 colorRef;
    struct sbitLineMetrics {
	int8 ascender;
	int8 descender;
	uint8 widthMax;
	int8 caretRise;
	int8 caretRun;
	int8 caretOff;
	int8 minoriginsb;
	int8 minAdvancesb;
	int8 maxbeforebl;
	int8 minafterbl;
	int8 pad1, pad2;
    } hori, vert;
    uint16 startGlyph;
    uint16 endGlyph;
    uint8 ppemX;
    uint8 ppemY;
    uint8 bitdepth;
    int8 flags;
    struct bitmapSizeTable *next;
    unsigned int error: 1;
};
struct indexarray {
    uint16 first;
    uint16 last;
    uint32 additionalOffset;
    struct indexarray *next;
};

static void FillLineMetrics(struct bitmapSizeTable *size,BDFFont *bdf) {
    int i, first;
    BDFChar *bc;

    memset(&size->hori,0,sizeof(struct sbitLineMetrics));
    memset(&size->vert,0,sizeof(struct sbitLineMetrics));

    /* I shall ignore vertical metrics */
    size->hori.caretRise = 1; /* other caret fields may be 0 */
    size->vert.caretRise = 1; /* other caret fields may be 0 */
    first = true;
    for ( i=0; i<bdf->charcnt ; ++i ) if ( (bc=bdf->chars[i])!=NULL ) {
	if ( first ) {
	    size->hori.ascender = bc->ymax;
	    size->hori.descender = bc->ymin;
	    size->hori.widthMax = bc->xmax-bc->xmin+1;
	    size->hori.minoriginsb = bc->xmin;
	    size->hori.minAdvancesb = bc->width-bc->xmax;
	    size->hori.minafterbl = bc->ymin;
	    size->hori.maxbeforebl = bc->ymax;
	    first = false;
	} else {
	    if ( bc->ymax > size->hori.ascender ) size->hori.ascender = bc->ymax;
	    if ( bc->ymin < size->hori.descender ) size->hori.descender = bc->ymin;
	    if ( bc->xmax-bc->xmin+1 > size->hori.widthMax ) size->hori.widthMax = bc->xmax-bc->xmin+1;
	    if ( bc->xmin < size->hori.minoriginsb ) size->hori.minoriginsb = bc->xmin;
	    if ( bc->width-bc->xmax < size->hori.minAdvancesb ) size->hori.minAdvancesb = bc->width-bc->xmax;
	    if ( bc->ymin < size->hori.minafterbl ) size->hori.minafterbl = bc->ymin;
	    if ( bc->ymax > size->hori.maxbeforebl ) size->hori.maxbeforebl = bc->ymax;
	}
    }

    /* There are some very cryptic pictures supposed to document the meaning */
    /*  of the metrics fields. MS and Apple use the same picture. The data */
    /*  present in font files do not match my understanding of those pictures */
    /*  But that's ok because different font files contain wildly different */
    /*  values for the same data, so they don't match each other either */
    /* GRRRRR */
#if 0
    size->vert.minoriginsb = size->hori.maxbeforebl;
    size->vert.minAdvancesb = size->hori.minafterbl;
    /* Apple seems to say that the vertical ascender/descender are half the */
    /*  pixel size (which makes sense), but MS does something else */
    size->vert.ascender = bdf->pixelsize/2;
    size->vert.descender = bdf->pixelsize/2;
    size->vert.widthMax = bdf->pixelsize;
#endif
}

static struct bitmapSizeTable *ttfdumpstrikelocs(FILE *bloc,FILE *bdat,
	BDFFont *bdf, struct bdfcharlist *defs) {
    struct bitmapSizeTable *size = gcalloc(1,sizeof(struct bitmapSizeTable));
    struct indexarray *cur, *last=NULL, *head=NULL;
    int i,j, final,cnt;
    FILE *subtables = tmpfile();
    struct mymets met;
    int32 pos = ftell(bloc), startofsubtables, base;
    BDFChar *bc, *bc2;

    for ( i=0; i<bdf->charcnt && (bdf->chars[i]==NULL || bdf->chars[i]->sc->ttf_glyph==-1); ++i );
    for ( j=bdf->charcnt-1; j>=0 && (bdf->chars[j]==NULL || bdf->chars[j]->sc->ttf_glyph==-1); --j );
    if ( j==-1 ) {
	GDrawIError("No characters to output in strikes");
return(NULL);
    }
    size->flags = 0x01;		/* horizontal */
    size->bitdepth = BDFDepth(bdf);
    size->ppemX = size->ppemY = bdf->ascent+bdf->descent;
    size->endGlyph = bdf->chars[j]->sc->ttf_glyph;
    size->startGlyph = bdf->chars[i]->sc->ttf_glyph;
    size->subtableoffset = pos;
    FillLineMetrics(size,bdf);

    /* First figure out sequences of characters which all have about the same metrics */
    /* then we dump out the subtables (to temp file) */
    /* then we dump out the indexsubtablearray (to bloc) */
    /* then we copy the subtables from the temp file to bloc */

    for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	bdf->chars[i]->widthgroup = false;
	BCCompressBitmap(bdf->chars[i]);
    }
    for ( i=0; i<bdf->charcnt; ++i ) {
	if ( (bc=bdf->chars[i])!=NULL && bc->sc->ttf_glyph!=1 &&
		bc->xmin>=0 && bc->xmax<=bc->width &&
		bc->ymax<bdf->ascent && bc->ymin>=-bdf->descent ) {
	    cnt = 1;
	    for ( j=i+1; j<bdf->charcnt; ++j )
		    if ( (bc2=bdf->chars[i])!=NULL && bc2->sc->ttf_glyph!=-1 ) {
		if ( bc2->xmin<0 || bc2->xmax>bc->width || bc2->ymin<-bdf->descent ||
		    bc2->ymax>=bdf->ascent || bc2->width!=bc->width ||
		    bc2->sc->ttf_glyph!=bc->sc->ttf_glyph+cnt )
	    break;
		++cnt;
	    }
	    if ( cnt>20 ) {		/* We must have at least, oh, 20 chars with the same metrics */
		bc->widthgroup = true;
		cnt = 1;
		for ( j=i+1; j<bdf->charcnt; ++j )
			if ( (bc2=bdf->chars[i])!=NULL && bc2->sc->ttf_glyph!=-1 ) {
		    if ( bc2->xmin<0 || bc2->xmax>bc->width || bc2->ymin<-bdf->descent ||
			bc2->ymax>=bdf->ascent || bc2->width!=bc->width ||
			bc2->sc->ttf_glyph!=bc->sc->ttf_glyph+cnt )
		break;
		    ++cnt;
		    bc2->widthgroup = true;
		}
	    }
	}
    }

    /* Now the pointers */
    for ( i=0; i<bdf->charcnt; ++i )
	    if ( (bc=bdf->chars[i])!=NULL && bc->sc->ttf_glyph!=-1) {
	if ( defs!=NULL && defs->bc->sc->ttf_glyph<bc->sc->ttf_glyph ) {
	    --i;
	    bc = defs->bc;
	    defs = defs->next;
	}
	cur = galloc(sizeof(struct indexarray));
	cur->next = NULL;
	if ( last==NULL ) head = cur;
	else last->next = cur;
	last = cur;
	cur->first = bc->sc->ttf_glyph;
	cur->additionalOffset = ftell(subtables);

	cnt = 1;
	final = i;
	if ( bc!=bdf->chars[i] )
	    cur->last = bc->sc->ttf_glyph;
	else { 
	    for ( j=i+1; j<bdf->charcnt ; ++j ) {
		if ( (bc2=bdf->chars[j])==NULL )
		    /* Ignore it */;
		else if ( bc2->sc->ttf_glyph!=bc->sc->ttf_glyph+cnt )
	    break;
		else if ( bc2->widthgroup!=bc->widthgroup ||
			(bc->widthgroup && bc->width!=bc2->width) )
	    break;
		else {
		    ++cnt;
		    final = j;
		}
	    }
	    cur->last = bdf->chars[final]->sc->ttf_glyph;
	}

	if ( !bc->widthgroup ) {
	    putshort(subtables,1);	/* index format, 4byte offset, no metrics here */
	    if ( BDFDepth(bdf)!=8 )
		putshort(subtables,2);	/* data format, short metrics, bit aligned */
	    else
		putshort(subtables,1);	/* data format, short metrics, byte aligned */
	    base = ftell(bdat);
	    putlong(subtables,base);	/* start of glyphs in bdat */
	    if ( BDFDepth(bdf)!=8 )
		putlong(subtables,ttfdumpf2bchar(bdat,bc,bdf)-base);
	    else
		putlong(subtables,ttfdumpf1bchar(bdat,bc,bdf)-base);
	    for ( j=i+1; j<=final ; ++j ) if ( (bc2=bdf->chars[j])!=NULL ) {
		if ( BDFDepth(bdf)!=8 )
		    putlong(subtables,ttfdumpf2bchar(bdat,bc2,bdf)-base);
		else
		    putlong(subtables,ttfdumpf1bchar(bdat,bc2,bdf)-base);
	    }
	    putlong(subtables,ftell(bdat)-base);	/* Length of last entry */
	} else {
	    met.xmin = bc->xmin; met.xmax = bc->xmax; met.ymin = bc->ymin; met.ymax = bc->ymax;
	    for ( j=i+1; j<=final ; ++j ) if ( (bc2=bdf->chars[j])!=NULL ) {
		if ( bc2->xmax>met.xmax ) met.xmax = bc2->xmax;
		if ( bc2->xmin<met.xmin ) met.xmin = bc2->xmin;
		if ( bc2->ymax>met.ymax ) met.ymax = bc2->ymax;
		if ( bc2->ymin<met.ymin ) met.ymin = bc2->ymin;
	    }
	    met.width = (met.xmax-met.xmin+1);

	    putshort(subtables,2);	/* index format, big metrics, all glyphs same size */
	    putshort(subtables,5);	/* data format, bit aligned no metrics */
	    putlong(subtables,ftell(bdat));	/* start of glyphs in bdat */
	    putlong(subtables,met.width*(met.ymax-met.ymin+1));	/* glyph size */
	    /* big metrics */
	    putc(met.ymax-met.ymin+1,subtables);	/* image height */
	    putc(met.width,subtables);			/* image width */
	    putc(met.xmin,subtables);			/* horiBearingX */
	    putc(met.ymax,subtables);			/* horiBearingY */
	    putc(bc->width,subtables);			/* advance width */
	    putc(-bc->width/2,subtables);		/* vertBearingX */
	    putc(0,subtables);				/* vertBearingY */
	    putc(bdf->ascent+bdf->descent,subtables);	/* advance height */
	    ttfdumpf5bchar(bdat,bc,&met,bdf);
	    for ( j=i+1; j<=final ; ++j ) if ( (bc2=bdf->chars[j])!=NULL ) {
		ttfdumpf5bchar(bdat,bc2,&met,bdf);
	    }
	}
	i = final;
    }

    /* now output the pointers */
    for ( cur=head, cnt=0; cur!=NULL; cur=cur->next ) ++cnt;
    startofsubtables = cnt*(2*sizeof(short)+sizeof(int32));
    size->numsubtables = cnt;
    for ( cur=head; cur!=NULL; cur = cur->next ) {
	putshort(bloc,cur->first);
	putshort(bloc,cur->last);
	putlong(bloc,startofsubtables+cur->additionalOffset);
    }

    /* copy the index file and close it (and delete it) */
    if ( !ttfcopyfile(bloc,subtables,pos+startofsubtables))
	size->error = true;

    size->tablesize = ftell(bloc)-pos;

    for ( cur=head; cur!=NULL; cur = last ) {
	last = cur->next;
	free(cur);
    }
return( size );
}

static void dumpbitmapSizeTable(FILE *bloc,struct bitmapSizeTable *size) {
    putlong(bloc,size->subtableoffset);
    putlong(bloc,size->tablesize);
    putlong(bloc,size->numsubtables);
    putlong(bloc,size->colorRef);
    fwrite(&size->hori,1,sizeof(struct sbitLineMetrics),bloc);
    fwrite(&size->vert,1,sizeof(struct sbitLineMetrics),bloc);
    putshort(bloc,size->startGlyph);
    putshort(bloc,size->endGlyph);
    putc(size->ppemX,bloc);
    putc(size->ppemY,bloc);
    putc(size->bitdepth,bloc);
    putc(size->flags,bloc);
}

void ttfdumpbitmap(SplineFont *sf,struct alltabs *at,int32 *sizes) {
    int i;
    static struct bitmapSizeTable space;
    struct bitmapSizeTable *head=NULL, *cur, *last;
    BDFFont *bdf;
    struct bdfcharlist *bl;

    at->bdat = tmpfile();
    at->bloc = tmpfile();
    /* aside from the names the version number is about the only difference */
    /*  I'm aware of. Oh MS adds a couple new sub-tables, but I haven't seen */
    /*  them used, and Apple also has a subtable MS doesn't support, but so what? */
    /* Oh, of course. Apple documents version 0x10000, but it actually uses */
    /*  version 0x20000. How silly of me to think the docs might be right */
    /* (Apple now documents version 0x20000) */
    /*if ( at->msbitmaps ) {*/
	putlong(at->bdat,0x20000);
	putlong(at->bloc,0x20000);
#if 0
    } else {
	putlong(at->bdat,0x10000);
	putlong(at->bloc,0x10000);
    }
#endif
    putlong(at->bloc,at->gi.strikecnt);

    /* leave space for the strike array at start of file */
    for ( i=0; sizes[i]!=0; ++i )
	dumpbitmapSizeTable(at->bloc,&space);

    /* Dump out the strikes... */
    GProgressChangeLine1R(_STR_SavingBitmapFonts);
    for ( i=0; sizes[i]!=0; ++i ) {
	GProgressNextStage();
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf==NULL )
    continue;
	bl = BDFAddDefaultGlyphs(bdf);
	cur = ttfdumpstrikelocs(at->bloc,at->bdat,bdf,bl);
	BDFCleanupDefaultGlyphs(bdf);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	if ( cur->error ) at->error = true;
    }

    fseek(at->bloc,2*sizeof(int32),SEEK_SET);
    for ( cur=head; cur!=NULL; cur=cur->next )
	dumpbitmapSizeTable(at->bloc,cur);

    for ( cur=head; cur!=NULL; cur=last ) {
	last = cur->next;
	free(cur);
    }

    at->bdatlen = ftell(at->bdat);
    if ( (at->bdatlen&1)!=0 )
	putc(0,at->bdat);
    if ( ftell(at->bdat)&2 )
	putshort(at->bdat,0);

    fseek(at->bloc,0,SEEK_END);
    at->bloclen = ftell(at->bloc);
    if ( (at->bloclen&1)!=0 )
	putc(0,at->bloc);
    if ( ftell(at->bloc)&2 )
	putshort(at->bloc,0);

    GProgressChangeLine1R(_STR_SavingTTFont);
}

static BDFFont *BDFSelect(SplineFont *sf,int32 *sizes,int wanted ) {
    int i;
    int best = -1;
    BDFFont *bdf;

    if ( wanted<=(sizes[0]&0xffff) ) {
	best = sizes[0]&0xffff;
	if ( best==wanted )
return(NULL );
    } else {
	for ( i=1; sizes[i]!=0 && wanted>(sizes[i]&0xffff); ++i );
	best = sizes[i-1]&0xffff;
	if ( best==wanted )	/* Exact match, no scaling needed */
return( NULL );
    }

    for ( bdf=sf->bitmaps; bdf!=NULL && bdf->pixelsize!=best; bdf = bdf->next );
return( bdf );
}

void ttfdumpbitmapscaling(SplineFont *sf,struct alltabs *at,int32 *sizes) {
    int i, cnt;
    BDFFont *bdf;
    static int expected_sizes[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	    19, 20, 21, 22, 23, 24, 25, 30, 32, 33, 40, 0 };
    struct bitmapSizeTable size;

/* I only generate this table if I'm trying to generate an ms bitmap-only */
/*  ttf file. Now MS doesn't really support bitmap-only fonts, so we kludge */
/*  we make the loca table point to a bunch of space glyphs. But to keep */
/*  the rasterizer from generating spaces, we put in a bunch of scaling */
/*  instructions so that for all expected bitmap sizes we might meet */
/*  we either get a bitmap strike or a scaled strike */
/* The sizes above were chosen to get the pixel sizes for */
/*	8,9,10,11,12,14,16,18,24 are present at 72,75,96,100,120 dpi */
/* (standard pointsizes) (Mac, X & Win standard screen dpis) */

    /* first count the number of scaled strikes */
    cnt = 0;
    for ( i=0; expected_sizes[i]!=0; ++i ) {
	if ( BDFSelect(sf,sizes,expected_sizes[i])!=NULL )
	    ++cnt;
    }

    at->ebsc = tmpfile();
    putlong( at->ebsc, 0x20000 );
    putlong( at->ebsc, cnt );
    for ( i=0; expected_sizes[i]!=0; ++i ) {
	if ( (bdf=BDFSelect(sf,sizes,expected_sizes[i]))!=NULL ) {
	    FillLineMetrics(&size,bdf);
	    /* Horizontal line metrics */
	     putc((int) rint(size.hori.ascender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.descender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.widthMax*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc(size.hori.caretRise,at->ebsc);
	     putc(size.hori.caretRun,at->ebsc);
	     putc(size.hori.caretOff,at->ebsc);
	     putc((int) rint(size.hori.minoriginsb*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.minAdvancesb*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.maxbeforebl*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.minafterbl*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc(0,at->ebsc);
	     putc(0,at->ebsc);
	    /* Vertical line metrics */
#if 1
	     putc((int) rint(size.hori.ascender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.descender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
#else
	     putc((int) rint(size.vert.ascender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.vert.descender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
#endif
	     putc((int) rint(size.vert.widthMax*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc(size.vert.caretRise,at->ebsc);
	     putc(size.vert.caretRun,at->ebsc);
	     putc(size.vert.caretOff,at->ebsc);
	     putc((int) rint(size.vert.minoriginsb*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.vert.minAdvancesb*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.vert.maxbeforebl*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.vert.minafterbl*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc(0,at->ebsc);
	     putc(0,at->ebsc);
	    /* Actual scaling info */
	    putc( expected_sizes[i], at->ebsc);
	    putc( expected_sizes[i], at->ebsc);
	    putc( bdf->pixelsize, at->ebsc);
	    putc( bdf->pixelsize, at->ebsc);
	}
    }

    at->ebsclen = ftell(at->ebsc);
    if ( (at->ebsclen&1)!=0 )
	putc(0,at->ebsc);
    if ( ftell(at->ebsc)&2 )
	putshort(at->ebsc,0);
}
