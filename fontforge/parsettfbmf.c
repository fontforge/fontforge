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
#include "bitmapchar.h"
#include "bvedit.h"
#include "fontforge.h"
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
    int ascent, descent;
};

struct bigmetrics {
    int height, width;
    int hbearingX, hbearingY, hadvance;
    int vbearingX, vbearingY, vadvance;	/* Small metrics doesn't use this row */
};

/* ************************************************************************** */
/* *********************** Input --- Reading Bitmaps ************************ */
/* ************************************************************************** */

static void ttfreadbmfglyph(FILE *ttf,struct ttfinfo *info,
	int32 offset, int32 len, struct bigmetrics *metrics, int imageformat,
	int gid, BDFFont *bdf) {
    BDFChar *bdfc;
    BDFRefChar *ref, *prev = NULL;
    struct bigmetrics big;
    int i,j,ch,l,p, num;

    if ( gid>=bdf->glyphcnt )
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
	big.vadvance = bdf->pixelsize;
    /* Sometimes the index format (e.g. 2) already specifies the metrics */
    /* Only use the provided metrics if it hasn't already been set */
    if (metrics == NULL)
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
	    LogError(_("Unexpected use of bitmap format 5, no metrics are appearant\n"));
	    info->bad_embedded_bitmap = true;
	    /*fseek(ttf,len,SEEK_CUR);*/
return;
	}
    } else {
	/* format 3 is obsolete */
	/* format 4 is compressed apple and I'm not supporting it (Nor is MS) */
	if ( imageformat==3 && !info->obscomplain ) {
	    LogError(_("This font contains bitmaps in the obsolete format 3 (And I can't read them)\n") );
	    info->bad_embedded_bitmap = true;
	    info->obscomplain = true;
	} else if ( imageformat==4 ) {
	    /* Apple doesn't describe it (fully) in their spec. */
	    /* MS doesn't support it (and doesn't describe) */
	    /* Adobe doesn't describe it (and says MS doesn't support it) */
	    LogError(_("This font contains bitmaps in Apple's compressed format 4 (And I don't support that)\n"));
	    info->cmpcomplain = true;
	} else if ( !info->unkcomplain ) {
	    LogError(_("This font contains bitmaps in a format %d that I've never heard of\n"), imageformat );
	    info->bad_embedded_bitmap = true;
	    info->unkcomplain = true;
	}
return;
    }
    bdfc = chunkalloc(sizeof(BDFChar));
    if ( info->chars!=NULL ) {
	if ( gid>=info->glyph_cnt || info->chars[gid]==NULL ) {
	    if ( gid>=info->glyph_cnt ) {
		int i;
	        info->chars = realloc(info->chars,(gid+1)*sizeof(SplineChar *));
		for ( i=info->glyph_cnt; i<gid; ++i ) {
		    info->chars[i] = SplineCharCreate(2);
		    info->chars[i]->orig_pos = gid;
		    info->chars[i]->unicodeenc = -1;
		    info->chars[i]->width = info->chars[i]->vwidth = info->emsize;
		}
	        info->glyph_cnt = gid+1;
	    }
	    info->chars[gid] = SplineCharCreate(2);
	    info->chars[gid]->orig_pos = gid;
	    info->chars[gid]->unicodeenc = -1;
	    info->chars[gid]->width = info->chars[gid]->vwidth = info->emsize;
	}
	bdfc->sc = info->chars[gid];
	/* in the case of a cid font orig_pos will be the cid, not the gid */
	/* We'll need to rearrange the glyph array to match later. We need */
	/*  gid order until we've fixed up references			   */
	bdfc->orig_pos = info->chars[gid]->orig_pos;
    } else {
	bdfc->orig_pos = gid;
    }
    if ( bdf->glyphs[gid]!=NULL ) /* Shouldn't happen of course */
	BDFCharFree(bdf->glyphs[gid]);
    bdf->glyphs[gid] = bdfc;

    bdfc->width = metrics->hadvance;
    bdfc->vwidth = metrics->vadvance;
    bdfc->xmin = metrics->hbearingX;
    bdfc->xmax = bdfc->xmin+metrics->width-1;
    bdfc->ymax = metrics->hbearingY-1;
    bdfc->ymin = bdfc->ymax-metrics->height+1;
    bdfc->refs = NULL;
    bdfc->dependents = NULL;
    if ( imageformat == 5 ) bdfc->widthgroup = true;
    if ( bdf->clut==NULL ) {
	bdfc->bytes_per_line = (metrics->width+7)/8;
	bdfc->depth = 1;
    } else {
	bdfc->bytes_per_line = metrics->width;
	bdfc->byte_data = true;
	bdfc->depth = bdf->clut->clut_len==4 ? 2 : bdf->clut->clut_len==16 ? 4 : 8;
    }
    if ( imageformat!=8 && imageformat!=9 )
	bdfc->bitmap = calloc(metrics->height*bdfc->bytes_per_line,sizeof(uint8));

    if ( imageformat==8 || imageformat==9 ) {
	/* composite */
	num = getushort(ttf);
	bdfc->bitmap = calloc( 1,sizeof( uint8 ));
	bdfc->bytes_per_line = 1;
	for ( i=0; i<num; ++i ) {
	    ref = calloc( 1,sizeof( BDFRefChar ));
	    ref->gid = getushort(ttf);
	    ref->xoff = (int8) getc(ttf);
	    ref->yoff = (int8) getc(ttf);
	    if ( prev == NULL )
		bdfc->refs = ref;
	    else
		prev->next = ref;
	    prev = ref;
	}
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
    ff_progress_next();
}

static void BdfCRefFixup(BDFFont *bdf, int gid, int *warned, struct ttfinfo *info) {
    BDFChar *bdfc = bdf->glyphs[gid], *bdfcRefd;
    BDFRefChar *head, *prev = NULL, *next;

    for ( head=bdfc->refs; head!=NULL; head=next ) {
        bdfcRefd = head->gid < bdf->glyphcnt ? bdf->glyphs[head->gid] : NULL;
        next = head->next;
        if ( bdfcRefd != NULL ) {
            head->bdfc = bdfcRefd;
            BCMakeDependent( bdfc, bdfcRefd );
            prev = head;
            /* According to the TTF spec, the xOffset and yOffset values specify   */
            /* the top-left corner position of the component in the composite.     */
            /* In our program it is more convenient to manipulate values specified */
            /* relatively to the original position of the reference's parent glyph.*/
            /* So we have to perform this conversion each time we read or write    */
            /* an embedded TTF bitmap. */
            head->xoff = head->xoff - bdfcRefd->xmin + bdfc->xmin;
            head->yoff = bdfc->ymax - bdfcRefd->ymax - head->yoff;
        } else {
            if ( !*warned ) {
                /* Glyphs aren't named yet */
                LogError(_("Glyph %d in bitmap strike %d pixels refers to a missing glyph (%d)"),
                        gid, bdf->pixelsize, head->gid );
                info->bad_embedded_bitmap = true;
                *warned = true;
            }
            /* Remove bad reference node from the singly-linked list */
            if ( prev == NULL )
                bdfc->refs = head->next;
            else
                prev->next = head->next;
            free( head );
        }
    }
}

static void BdfRefFixup(BDFFont *bdf, struct ttfinfo *info) {
    int i;
    int warned = false;
    BDFChar *bc;

    /* Associate each reference with its parent glyph and recalculate */
    /* x and y offsets */
    for ( i=0; i<bdf->glyphcnt; ++i )
	if ( bdf->glyphs[i]!=NULL && bdf->glyphs[i]->refs != NULL )
	    BdfCRefFixup( bdf,i,&warned,info );

    /* When offsets are recalculated, reset the composite glyph bounding */
    /* box to zero values */
    /* For glyphs which were stored in subtables format 5 compress bitmaps */
    /* to the image size (we could not do this before recalculating reference */
    /* offsets) */
    for ( i=0; i<bdf->glyphcnt; ++i ) if (( bc = bdf->glyphs[i] ) != NULL ) {
	if ( bc->refs != NULL ) {
	    bdf->glyphs[i]->xmin = 0; bdf->glyphs[i]->xmax = 0;
	    bdf->glyphs[i]->ymin = 0; bdf->glyphs[i]->ymax = 0;
	} else if ( bc->widthgroup ) {
	    BCCompressBitmap( bc );
	    bc->widthgroup = false;
	}
    }
}

static void BdfCleanup(BDFFont *bdf,struct ttfinfo  *info) {
    /* for gulim (and doubtless others) bitmap references do not match */
    /*  outline glyph references and there are a bunch of extra glyphs */
    /*  at the end of the list that we can now get rid of	       */
    /* As bitmap references are directly supported now, we no longer   */
    /*  can ignore the extra glyphs in Gulim, and instead create       */
    /*  a faked spline char for each of them. Thus the situation where */
    /*  info->glyph_cnt is less than bdf->glyphcnt is no longer        */
    /*  possible.                                                      */
    /* For cid keyed fonts we want to order things by cid rather than  */
    /*  by gid so we may also need to move things around	       */
    int i, cnt;
    BDFChar **glyphs, *bdfc;

    if ( info->subfonts==NULL ) {
	if ( info->glyph_cnt<bdf->glyphcnt ) {
	    for ( i=info->glyph_cnt ; i<bdf->glyphcnt; ++i )
		BDFCharFree(bdf->glyphs[i]);
	    bdf->glyphs = realloc(bdf->glyphs,info->glyph_cnt*sizeof(BDFChar *));
	    bdf->glyphcnt = info->glyph_cnt;
	}
    } else {
	cnt = 0;
	for ( i=0; i<info->subfontcnt; ++i )
	    if ( info->subfonts[i]->glyphcnt > cnt )
		cnt = info->subfonts[i]->glyphcnt;
	glyphs = calloc(cnt,sizeof(BDFChar *));
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( (bdfc = bdf->glyphs[i])!=NULL ) {
	    if ( bdfc->orig_pos<cnt )
		glyphs[ bdfc->orig_pos ] = bdfc;
	    else
		BDFCharFree(bdfc);
	}
	free(bdf->glyphs);
	bdf->glyphs = glyphs;
	bdf->glyphcnt = cnt;
    }
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
	if ( last<first ) {
	    LogError(_("Bad format of subtable %d (of %d) in bloc/EBLC of strike with pixelsize=%d. First=%d, last=%d.\n"),
		    j, head->numIndexSubTables, bdf->pixelsize, first, last );
	    info->bad_embedded_bitmap = true;
    continue;
	}
	loc = ftell(ttf);
	fseek(ttf,info->bitmaploc_start+head->indexSubTableArrayOffset+moreoff,SEEK_SET);

	indexformat=getushort(ttf);
	imageformat=getushort(ttf);
	offset = getlong(ttf);
	switch ( indexformat ) {
	  case 1: case 3:
	    glyphoffsets = malloc((last-first+2)*sizeof(int32));
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
	    glyphoffsets = malloc((num+1)*sizeof(int32));
	    glyphs = malloc((num+1)*sizeof(int32));
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
	    glyphs = malloc((num+1)*sizeof(int32));
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
	    LogError(_("Didn't understand index format: %d\n"), indexformat );
	    info->bad_embedded_bitmap = true;
	  break;
	}
	fseek(ttf,loc,SEEK_SET);
    }
    BdfRefFixup(bdf,info);
    BdfCleanup(bdf,info);
}

void TTFLoadBitmaps(FILE *ttf,struct ttfinfo *info,int onlyone) {
    int i, cnt, j, k, good, glyphcnt;
    int bigval, biggest;
    struct ttfsizehead *sizes;
    BDFFont *bdf, *last;
    const char **choices;
    char *sel;
    char buf[300];
    char *buttons[3];
    buttons[0] = _("_Yes");
    buttons[1] = _("_No");
    buttons[2] = NULL;

    fseek(ttf,info->bitmaploc_start,SEEK_SET);
    /* version = */ getlong(ttf);		/* Had better be 0x00020000, or 2.0 */
    cnt = getlong(ttf);
    sizes = malloc(cnt*sizeof(struct ttfsizehead));
    /* we may not like all the possible bitmaps. Some might be designed for */
    /*  non-square pixels, others might be color, others might be */
    /*  vertical data. So only pick out the ones we can use */
    bigval = biggest = -1;
    for ( i = j = 0; i<cnt; ++i ) {
	if ( feof(ttf)) {
	    IError( "End of file while reading embedded bitmaps" );
    break;
	}
	good = true;
	sizes[j].indexSubTableArrayOffset = getlong(ttf);
	/* size = */ getlong(ttf);
	sizes[j].numIndexSubTables = getlong(ttf);
	if ( /* colorRef = */ getlong(ttf)!=0 )
	    good = false;
	sizes[j].ascent = getc(ttf);
	sizes[j].descent = getc(ttf);
	for ( k=0; k<12-2; ++k ) getc(ttf);	/* Horizontal Line Metrics */
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
    if ( cnt==0 ) {
        free(sizes);
        return;
    }

    /* Ask user which (if any) s/he is interested in */
    choices = calloc(cnt+1,sizeof(char *));
    sel = calloc(cnt,sizeof(char));
    for ( i=0; i<cnt; ++i ) {
	if ( sizes[i].depth==1 )
	    sprintf(buf,"%d", sizes[i].ppem );
	else
	    sprintf(buf,"%d@%d", sizes[i].ppem, sizes[i].depth );
	choices[i] = copy(buf);
    }
    /* When loading a ttf font with only bitmaps, and there's only one strike */
    /*  then just load that strike. Don't ask */
    if ( info->glyphlocations_start==0 && info->cff_start==0 && cnt==1 )
	sel[0] = true;
    else if ( no_windowing_ui ) {
	if ( onlyone ) {
	    biggest=-1; // We may not find one.
	    for ( i=1; i<cnt; ++i )
		if ( sizes[i].depth==1 && (biggest < 0 || sizes[i].ppem>sizes[biggest].ppem) )
		    biggest = i;
	    sel[biggest] = true;
	} else {
	    biggest=-1;
	    for ( i=0; i<cnt; ++i ) {
		sel[i] = true;
		if ( sizes[i].depth==1 && (biggest < 0 || sizes[i].ppem>sizes[biggest].ppem) )
		    biggest = i;
	    }
	}
    } else if ( onlyone ) {
	biggest=ff_choose(_("Load Bitmap Fonts"), choices,cnt,biggest,
		_("Do you want to load the bitmap fonts embedded in this true/open type file?\n(And if so, which)"));
	if ( biggest!=-1 ) sel[biggest] = true;
    } else {
	biggest = ff_choose_multiple(_("Load Bitmap Fonts"), choices,sel,cnt,buttons,
		_("Do you want to load the bitmap fonts embedded in this true/open type file?\n(And if so, which)"));
    }
    for ( i=0; i<cnt; ++i ) free( (unichar_t *) (choices[i]));
    free(choices);
    if ( biggest<0 ) {		/* Cancelled */
	free(sizes); free(sel);
return;
    }
    /* Remove anything not selected */
    for ( i=j=0; i<cnt; ++i ) {
	if ( sel[i] )
	    sizes[j++] = sizes[i];
    }
    cnt = j;

    ff_progress_change_stages(3+cnt);
    info->bitmaps = last = NULL;
    if ( info->subfonts==NULL )
	glyphcnt = info->glyph_cnt;
    else {
	glyphcnt = 0;
	for ( i=0; i<info->subfontcnt; ++i )
	    if ( info->subfonts[i]->glyphcnt > glyphcnt )
		glyphcnt = info->subfonts[i]->glyphcnt;
    }
    for ( i=0; i<cnt; ++i ) {
	bdf = calloc(1,sizeof(BDFFont));
	/* In cid fonts fontforge stores things by cid rather than gid */
	bdf->glyphcnt = glyphcnt;
	if ( sizes[i].endglyph > bdf->glyphcnt )
	    bdf->glyphcnt = sizes[i].endglyph+1;	/* Important if we have reference glyphs */
	bdf->glyphmax = bdf->glyphcnt;
	bdf->glyphs = calloc(bdf->glyphcnt,sizeof(BDFChar *));
	bdf->pixelsize = sizes[i].ppem;
	if ( sizes[i].ppem == sizes[i].ascent - sizes[i].descent )
	    bdf->ascent = sizes[i].ascent;
	else
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
	snprintf(buf,sizeof(buf),_("%d pixel bitmap"), sizes[i].ppem );
	ff_progress_change_line2(buf);
	readttfbitmapfont(ttf,info,&sizes[i],bdf);
	ff_progress_next_stage();
    }
    free(sizes); free(sel);

    ttf_bdf_read(ttf,info);
}

/* ************************************************************************** */
/* *********************** Output --- Writing Bitmaps *********************** */
/* ************************************************************************** */

static BDFChar glyph0, glyph1, glyph2;
static SplineChar sc0, sc1, sc2;
static struct bdfcharlist bl[3];

static struct bdfcharlist *BDFAddDefaultGlyphs(BDFFont *bdf, int format) {
    /* when I dump out the glyf table I add 3 glyphs at the start. One is glyph*/
    /*  0, the one used when there is no match, and the other two are varients*/
    /*  on space. These might or might not be in the font.  Add the ones that */
    /*  don't exist */
    /* Werner tells me he doesn't want them added. He thinks sparse bitmaps */
    /*  are reasonable. Hadn't occurred to me. So I wanted complete strikes */
    /* Perhaps a better approach is to give them fake bitmaps if we gave them */
    /*  fake outlines, so look at the spline font not the bitmap font for the */
    /*  magic glyphs */
    int width, w, i, j, bit, blpos=0;
    SplineFont *sf = bdf->sf;
    int notdefpos = -1, nullpos = -1, nmretpos = -1;
    int extracnt = format==ff_otf || format==ff_otfcid ? 1 : 3;

    width = 0x7ffff;
    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	if ( width == 0x7ffff )
	    width = bdf->glyphs[i]->width;
	else if ( width!=bdf->glyphs[i]->width ) {
	    width = 0x7fffe;
    break;
	}
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	int ttf_glyph = sf->glyphs[i]->ttf_glyph;
	if ( ttf_glyph==0 )
	    notdefpos = i;
	else if ( ttf_glyph==1 )
	    nullpos = i;
	else if ( ttf_glyph==2 )
	    nmretpos = i;
    }
    if ( width>0x70000 )
	/* Proportional */
	width = bdf->pixelsize/3;

    memset(bl,0,sizeof(bl));
    if ( notdefpos==-1 ) {
	sc0.name = ".notdef";
	/* sc0.ttf_glyph = 0; */	/* already done */
	sc0.parent = sf;
	if ( width<4 ) w=4; else w=width;
	sc0.width = w*(sf->ascent+sf->descent)/bdf->pixelsize;
	sc0.widthset = true;
	sc0.vwidth = (sf->ascent+sf->descent);
	glyph0.sc = &sc0;
	glyph0.width = w;
	glyph0.vwidth = bdf->pixelsize;
	if ( BDFDepth(bdf)!=1 ) {
	    glyph0.bytes_per_line = 1;	/* Needs to be 1 or BCRegularizeBitmap gets upset */
	    glyph0.ymax = 1;
	    glyph0.bitmap = calloc(8,1);
	} else {
	    glyph0.xmin = (w==4)?0:1;
	    glyph0.xmax = w-1;
	    glyph0.ymin = 0;
	    glyph0.ymax = 8*bdf->ascent/10;
	    glyph0.bytes_per_line = (glyph0.xmax-glyph0.xmin+8)/8;
	    glyph0.bitmap = calloc((glyph0.ymax-glyph0.ymin+1)*glyph0.bytes_per_line,1);
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
	bl[blpos++].bc = &glyph0;
    }

    if ( extracnt==3 ) {
	sc1.ttf_glyph = 1;
	sc2.ttf_glyph = 2;
	sc1.name = ".null"; sc2.name = "nonmarkingreturn";
	sc1.parent = sc2.parent = sf;
	sc1.width = sc2.width = width*(sf->ascent+sf->descent)/bdf->pixelsize;
	sc1.widthset = sc2.widthset = 1;
	sc1.vwidth = sc2.vwidth = (sf->ascent+sf->descent);
	glyph1.sc = &sc1; glyph2.sc = &sc2;
	/* xmin and so forth are zero, and are already clear */
	glyph1.width = glyph2.width = width;
	glyph1.vwidth = glyph2.vwidth = bdf->pixelsize;
	glyph1.bytes_per_line = glyph2.bytes_per_line = 1;	/* Needs to be 1 or BCRegularizeBitmap gets upset */
	glyph1.bitmap = glyph2.bitmap = (uint8 *) "";
	glyph1.orig_pos = 1; glyph2.orig_pos = 2;
	if ( nullpos==-1 ) {
	    if ( blpos!=0 )
		bl[blpos-1].next = &bl[blpos];
	    bl[blpos++].bc = &glyph1;
	}
	if ( nmretpos==-1 ) {
	    if ( blpos!=0 )
		bl[blpos-1].next = &bl[blpos];
	    bl[blpos++].bc = &glyph2;
	}
    }
return( blpos==0 ? NULL : &bl[0] );
}

static void BDFCleanupDefaultGlyphs(BDFFont *bdf) {
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

static void ttfdumpbigmetrics(FILE *bdat, BDFChar *bc) {

    /* dump big metrics */
    putc(bc->ymax-bc->ymin+1,bdat);		/* height */
    putc(bc->xmax-bc->xmin+1,bdat);		/* width */
    putc(bc->xmin,bdat);			/* horiBearingX */
    putc(bc->ymax+1,bdat);			/* horiBearingY */
    putc(bc->width,bdat);			/* advance width */
    putc(-bc->width/2,bdat);			/* vertBearingX */
    putc(0,bdat);				/* vertBearingY */
    putc(bc->vwidth,bdat);			/* vert advance width */
}

static int32 ttfdumpf1_6bchar(FILE *bdat, BDFChar *bc,BDFFont *bdf) {
    /* format 1 character dump. small metrics, byte aligned data */
    int32 pos = ftell(bdat);
    int r,c,val;

    if ( bdf->sf->hasvmetrics )
	ttfdumpbigmetrics(bdat,bc);
    else
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

static int32 ttfdumpf2_7bchar(FILE *bdat, BDFChar *bc, BDFFont *bdf,int do_metrics) {
    /* format 2 character dump. small metrics, bit aligned data */
    int32 pos = ftell(bdat);
    int r,c,ch,bit,sh;

    if ( do_metrics ) {
	if ( bdf->sf->hasvmetrics )
	    ttfdumpbigmetrics(bdat,bc);
	else
	    ttfdumpsmallmetrics(bdat,bc);
    }

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

static int32 ttfdumpf8_9bchar(FILE *bdat, BDFChar *bc,BDFFont *bdf) {
    /* format 8 character dump. small metrics, component data */
    int32 pos = ftell(bdat);
    int numc = 0;
    BDFRefChar *head;

    if ( bdf->sf->hasvmetrics )
	ttfdumpbigmetrics(bdat,bc);
    else {
	ttfdumpsmallmetrics(bdat,bc);
	putc( 0,bdat );
    }

    /* dump component data */
    for ( head = bc->refs; head!=NULL; head=head->next )
	numc++;
    putshort( bdat,numc );
    for ( head = bc->refs; head!=NULL; head=head->next ) {
	putshort( bdat,head->bdfc->sc->ttf_glyph );
	putc( head->bdfc->xmin - bc->xmin + head->xoff,bdat );
	putc( bc->ymax - head->bdfc->ymax - head->yoff,bdat );
    }
return( pos );
}

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
};

struct bitmapSizeTable {
    int32 subtableoffset;
    int32 tablesize;
    int32 numsubtables;
    int32 colorRef;
    struct sbitLineMetrics hori, vert;
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
    for ( i=0; i<bdf->glyphcnt ; ++i ) if ( (bc=bdf->glyphs[i])!=NULL ) {
	if ( first ) {
/*	    size->hori.ascender = bc->ymax+1;*/
/*	    size->hori.descender = bc->ymin; */
	    size->hori.widthMax = bc->xmax-bc->xmin+1;
	    size->hori.minoriginsb = bc->xmin;
	    size->hori.minAdvancesb = bc->width-bc->xmax;
	    size->hori.minafterbl = bc->ymin;
	    size->hori.maxbeforebl = bc->ymax;
	    first = false;
	} else {
/*	    if ( bc->ymax+1 > size->hori.ascender ) size->hori.ascender = bc->ymax+1; */
/*	    if ( bc->ymin < size->hori.descender ) size->hori.descender = bc->ymin;   */
	    if ( bc->xmax-bc->xmin+1 > size->hori.widthMax ) size->hori.widthMax = bc->xmax-bc->xmin+1;
	    if ( bc->xmin < size->hori.minoriginsb ) size->hori.minoriginsb = bc->xmin;
	    if ( bc->width-bc->xmax < size->hori.minAdvancesb ) size->hori.minAdvancesb = bc->width-bc->xmax;
	    if ( bc->ymin < size->hori.minafterbl ) size->hori.minafterbl = bc->ymin;
	    if ( bc->ymax+1 > size->hori.maxbeforebl ) size->hori.maxbeforebl = bc->ymax+1;
	}
    }
    size->hori.ascender = BdfPropHasInt(bdf,"FONT_ASCENT",bdf->ascent);
    size->hori.descender = -BdfPropHasInt(bdf,"FONT_DESCENT",bdf->descent);

    /* There are some very cryptic pictures supposed to document the meaning */
    /*  of the metrics fields. MS and Apple use the same picture. The data */
    /*  present in font files do not match my understanding of those pictures */
    /*  But that's ok because different font files contain wildly different */
    /*  values for the same data, so they don't match each other either */
    /* GRRRRR */
    /* Apple's ftxvalidator gets unhappy if there are no vertical line metrics*/
    /*  so even though I've no idea what they should be, I'll include something*/
    /* Apple seems to say that the vertical ascender/descender are half the */
    /*  pixel size (which makes sense), but MS does something else */
    size->vert.ascender = bdf->pixelsize/2;
    size->vert.descender = -bdf->pixelsize/2;
    size->vert.widthMax = bdf->pixelsize;
}

static int HasOutputtableBitmap( BDFChar *bc ) {
    int i;
    
    for ( i=0; i<bc->bytes_per_line * ( bc->ymax - bc->ymin + 1 ); i++ ) {
	if ( bc->bitmap[i] != 0 )
return( true );	
    }
return( false );	
}

static void BCPreserveAndExpand( BDFChar *bc,IBounds *ib ) {
    int bmp_width = ( bc->ymax - bc->ymin + 1 );
    
    bc->backup = calloc( 1,sizeof( BDFFloat ));
    bc->backup->bytes_per_line = bc->bytes_per_line;
    bc->backup->xmin = bc->xmin; bc->backup->xmax = bc->xmax;
    bc->backup->ymin = bc->ymin; bc->backup->ymax = bc->ymax;
    bc->backup->bitmap = calloc( bc->bytes_per_line * bmp_width,sizeof( uint8 ));
    memcpy( bc->backup->bitmap,bc->bitmap,bc->bytes_per_line * bmp_width );
    
    BCExpandBitmapToEmBox( bc,ib->minx,ib->miny,ib->maxx,ib->maxy );
}

static void DetectWidthGroups( struct glyphinfo *gi,BDFFont *bdf,int apple ) {
    BDFChar *bc, *bc2;
    IBounds ib, ib2;
    int i, j, cnt, final;
    
    for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i] >= 0 && gi->bygid[i] < bdf->glyphcnt && bdf->glyphs[gi->bygid[i]]!=NULL )
	bdf->glyphs[gi->bygid[i]]->widthgroup = false;

    for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i] >= 0 && gi->bygid[i] < bdf->glyphcnt && (bc=bdf->glyphs[gi->bygid[i]])!=NULL ) {
	BDFCharQuickBounds( bc,&ib,0,0,true,true );
	if (( apple || HasOutputtableBitmap( bc ) || bc->refs == NULL ) &&
		ib.minx >= 0 && ib.maxx <= bc->width &&
		ib.maxy < bdf->ascent && ib.miny >= -bdf->descent ) {
	    cnt = 1;
	    for ( j=i+1; j<gi->gcnt; ++j ) if ( gi->bygid[j] >= 0 && gi->bygid[j] < bdf->glyphcnt && (bc2=bdf->glyphs[gi->bygid[j]])!=NULL ) {
		BDFCharQuickBounds( bc2,&ib2,0,0,true,true );
		if (( !apple && !HasOutputtableBitmap( bc2 ) && bc2->refs != NULL ) ||
			ib2.minx < 0 || ib2.maxx > bc->width || ib2.miny < -bdf->descent ||
			ib2.maxy >= bdf->ascent ||
			bc2->width != bc->width || bc2->vwidth != bc->vwidth ||
			bc2->sc->ttf_glyph != bc->sc->ttf_glyph+cnt )
	    break;
		if ( ib2.maxx > ib.maxx ) ib.maxx = ib2.maxx;
		if ( ib2.minx < ib.minx ) ib.minx = ib2.minx;
		if ( ib2.maxy > ib.maxy ) ib.maxy = ib2.maxy;
		if ( ib2.miny < ib.miny ) ib.miny = ib2.miny;
		++cnt;
		final = j;
	    }
	    if ( cnt>20 ) {		/* We must have at least, oh, 20 glyphs with the same metrics */
		bc->widthgroup = true;
		BCPreserveAndExpand( bc,&ib );
		for ( j=i+1; j<=final; ++j ) if ( gi->bygid[j] >= 0 && gi->bygid[j] < bdf->glyphcnt && ( bc2=bdf->glyphs[gi->bygid[j]])!=NULL ) {
		    bc2->widthgroup = true;
		    BCPreserveAndExpand( bc2,&ib );
		}
		i = j-1;
	    }
	}
    }
}

static struct bitmapSizeTable *ttfdumpstrikelocs(FILE *bloc,FILE *bdat,
	BDFFont *bdf, struct bdfcharlist *defs, struct glyphinfo *gi) {
    struct bitmapSizeTable *size = calloc(1,sizeof(struct bitmapSizeTable));
    struct indexarray *cur, *last=NULL, *head=NULL;
    int i,j, final,cnt;
    FILE *subtables = tmpfile();
    int32 pos = ftell(bloc), startofsubtables, base, stlen;
    BDFChar *bc, *bc2;
    int depth = BDFDepth(bdf), format, mwidth, mheight;
    struct bdfcharlist *def;

    for ( i=0; i<gi->gcnt && gi->bygid[i]<bdf->glyphcnt &&
	    (gi->bygid[i]==-1 || bdf->glyphs[gi->bygid[i]]==NULL); ++i );
    for ( j=gi->gcnt-1; j>=0 &&
	    (gi->bygid[j]==-1 || gi->bygid[j]>=bdf->glyphcnt || bdf->glyphs[gi->bygid[j]]==NULL); --j );
    if ( j==-1 ) {
	IError("No characters to output in strikes");
	free(size);
	fclose(subtables);
return(NULL);
    }

    size->flags = 0x01;		/* horizontal */
    size->bitdepth = depth;
    size->ppemX = size->ppemY = bdf->ascent+bdf->descent;
    size->endGlyph = bdf->glyphs[gi->bygid[j]]->sc->ttf_glyph;
    size->startGlyph = bdf->glyphs[gi->bygid[i]]->sc->ttf_glyph;
    size->subtableoffset = pos;

    for ( def = defs; def != NULL; def = def->next ) {
	if ( defs->bc->sc->ttf_glyph < size->startGlyph )
	    size->startGlyph = defs->bc->sc->ttf_glyph;
    }
    FillLineMetrics(size,bdf);

    /* first we dump out the subtables (to temp file) */
    /* then we dump out the indexsubtablearray (to bloc) */
    /* then we copy the subtables from the temp file to bloc */

    /* the pointers */
    for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]>=0 && gi->bygid[i]<bdf->glyphcnt && (bc=bdf->glyphs[gi->bygid[i]])!=NULL ) {
	int wasdef = false;
	if ( defs!=NULL && defs->bc->sc->ttf_glyph < bc->sc->ttf_glyph ) {
	    --i;
	    bc = defs->bc;
	    defs = defs->next;
	    wasdef = true;
	}
	cur = malloc(sizeof(struct indexarray));
	cur->next = NULL;
	if ( last==NULL ) head = cur;
	else last->next = cur;
	last = cur;
	cur->first = bc->sc->ttf_glyph;
	cur->additionalOffset = ftell(subtables);

	cnt = 1;
	final = i;
	if ( wasdef )
	    cur->last = bc->sc->ttf_glyph;
	else {
	    for ( j=i+1; j<gi->gcnt ; ++j ) {
		if ( gi->bygid[j]<0 || gi->bygid[j]>=bdf->glyphcnt || (bc2=bdf->glyphs[gi->bygid[j]])==NULL )
	    break;
		else if ( bc2->widthgroup!=bc->widthgroup ||
			(bc->widthgroup && (bc->width!=bc2->width || bc->vwidth!=bc2->vwidth)) )
	    break;
		else if ( bc->ticked != bc2->ticked )
	    break;
		else {
		    ++cnt;
		    final = j;
		}
	    }
	    cur->last = bdf->glyphs[gi->bygid[final]]->sc->ttf_glyph;
	}

	if ( !bc->widthgroup ) {
	    putshort(subtables,1);	/* index format, 4byte offset, no metrics here */
	    if ( bdf->sf->hasvmetrics && bc->ticked )
		format = 9;		/* data format, big metrics, component data */
	    else if ( bdf->sf->hasvmetrics && depth!=8 )
		format = 7;		/* data format, big metrics, bit aligned */
	    else if ( bdf->sf->hasvmetrics )
		format = 6;		/* data format, big metrics, byte aligned */
	    else if ( bc->ticked )
		format = 8;		/* data format, short metrics, component data */
	    else if ( depth!=8 )
		format = 2;		/* data format, short metrics, bit aligned */
	    else
		format = 1;		/* data format, short metrics, byte aligned */
	    putshort( subtables,format );
	    base = ftell(bdat);
	    putlong( subtables,base );	/* start of glyphs in bdat */
	    if ( bc->ticked )
		stlen = ttfdumpf8_9bchar(bdat,bc,bdf)-base;
	    else if ( depth!=8 )
		stlen = ttfdumpf2_7bchar(bdat,bc,bdf,true)-base;
	    else
		stlen = ttfdumpf1_6bchar(bdat,bc,bdf)-base;
	    putlong( subtables,stlen );
	    for ( j=i+1; j<=final ; ++j ) if ( (bc2=bdf->glyphs[gi->bygid[j]])!=NULL ) {
		if ( bc2->ticked )
		    stlen = ttfdumpf8_9bchar(bdat,bc2,bdf)-base;
		else if ( depth!=8 )
		    stlen = ttfdumpf2_7bchar(bdat,bc2,bdf,true)-base;
		else
		    stlen = ttfdumpf1_6bchar(bdat,bc2,bdf)-base;
		putlong( subtables,stlen );
	    }
	    putlong( subtables,ftell(bdat)-base );	/* Length of last entry */
	} else {
	    mwidth = bc->xmax - bc->xmin + 1;
	    mheight = bc->ymax - bc->ymin + 1;
	    putshort(subtables,2);	/* index format, big metrics, all glyphs same size */
	    putshort(subtables,5);	/* data format, bit aligned no metrics */
	    putlong(subtables,ftell(bdat));	/* start of glyphs in bdat */
	    putlong(subtables,( mwidth * (bc->ymax - bc->ymin + 1)*depth + 7)/8);
					/* glyph size (in bytes) */
	    /* big metrics */
	    putc(mheight,subtables);			/* image height */
	    putc(mwidth,subtables);			/* image width */
	    putc(bc->xmin,subtables);			/* horiBearingX */
	    putc(bc->ymax+1,subtables);			/* horiBearingY */
	    putc(bc->width,subtables);			/* advance width */
	    putc(-bc->width/2,subtables);		/* vertBearingX */
	    putc(0,subtables);				/* vertBearingY */
	    putc(bc->vwidth,subtables);			/* advance height */
	    ttfdumpf2_7bchar(bdat,bc,bdf,false);
	    for ( j=i+1; j<=final ; ++j ) if ( (bc2=bdf->glyphs[gi->bygid[j]])!=NULL )
		ttfdumpf2_7bchar(bdat,bc2,bdf,false);
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
    if ( !ttfcopyfile(bloc,subtables,pos+startofsubtables,"EBLC/bloc-subtables"))
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
    int i, j;
    static struct bitmapSizeTable space;
    struct bitmapSizeTable *head=NULL, *cur, *last;
    BDFFont *bdf;
    BDFChar *bc;
    struct bdfcharlist *bl;

    at->bdat = tmpfile();
    at->bloc = tmpfile();
    /* aside from the names the version number is about the only difference */
    /*  I'm aware of. Oh MS adds a couple new sub-tables, but I haven't seen */
    /*  them used, and Apple also has a subtable MS doesn't support, but so what? */
    putlong(at->bdat,0x20000);
    putlong(at->bloc,0x20000);
    putlong(at->bloc,at->gi.strikecnt);

    /* leave space for the strike array at start of file */
    for ( i=0; sizes[i]!=0; ++i )
	dumpbitmapSizeTable(at->bloc,&space);

    /* Dump out the strikes... */
    ff_progress_change_line1(_("Saving Bitmap Font(s)"));
    for ( i=0; sizes[i]!=0; ++i ) {
	ff_progress_next_stage();
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf==NULL )
    continue;

	/* Used to detect glyphs which can be placed into width groups (EBDT format 5) */
	/* immediately during the output process. But using this format means most glyph */
	/* bounding boxes are expanded to larger values than the actual bitmap size, */
	/* and we need to know these new bounding box values in order to be able to */
	/* correctly calculate reference placement in composite glyphs */
	DetectWidthGroups( &at->gi,bdf,at->applebitmaps );
	/* Apple doesn't support composite bitmaps ( EBDT formats 8 and 9) */
	for ( j=0; j < at->gi.gcnt; ++j ) if ( at->gi.bygid[j] >= 0 && at->gi.bygid[j] < bdf->glyphcnt && ( bc = bdf->glyphs[at->gi.bygid[j]] ) != NULL )
            BCPrepareForOutput( bc,at->applebitmaps );
	bl = BDFAddDefaultGlyphs(bdf, at->format);
	cur = ttfdumpstrikelocs(at->bloc,at->bdat,bdf,bl,&at->gi);
	BDFCleanupDefaultGlyphs(bdf);
	for ( j=0; j < at->gi.gcnt; ++j ) if ( at->gi.bygid[j] >= 0 && at->gi.bygid[j] < bdf->glyphcnt && ( bc = bdf->glyphs[at->gi.bygid[j]] ) != NULL )
	    BCRestoreAfterOutput( bc );

	if ( cur==NULL )
    continue;
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

    ff_progress_change_line1(_("Saving TrueType Font"));
    ttf_bdf_dump(sf,at,sizes);
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
	     putc((int) rint(size.hori.ascender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
	     putc((int) rint(size.hori.descender*expected_sizes[i]/bdf->pixelsize),at->ebsc);
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
