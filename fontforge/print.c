/* -*- coding: utf-8 -*- */
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

#include "print.h"

#include "cvexport.h"
#include "dumppfa.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "langfreq.h"
#include "mm.h"
#include "sflayoutP.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ustring.h>
#include <ffglib.h>
#include "utype.h"
#include <sys/types.h>
#if !defined(__MINGW32__)
#include <sys/wait.h>
#endif

int pagewidth = 0, pageheight=0;	/* In points */
char *printlazyprinter=NULL;
char *printcommand=NULL;
int printtype = pt_unknown;
int use_gv;

struct printdefaults pdefs[] = {
    { &custom, pt_fontdisplay, 0, NULL },
    { &custom, pt_chars, 0, NULL },
    { &custom, pt_fontdisplay, 0, NULL }
};


/* ************************************************************************** */
/* ***************************** Printing Stuff ***************************** */
/* ************************************************************************** */

static int pdf_addobject(PI *pi) {
    if ( pi->next_object==0 ) {
	pi->max_object = 100;
	pi->object_offsets = malloc(pi->max_object*sizeof(int));
	pi->object_offsets[pi->next_object++] = 0;	/* Object 0 is magic */
    } else if ( pi->next_object>=pi->max_object ) {
	pi->max_object += 100;
	pi->object_offsets = realloc(pi->object_offsets,pi->max_object*sizeof(int));
    }
    pi->object_offsets[pi->next_object] = ftell(pi->out);
    fprintf( pi->out, "%d 0 obj\n", pi->next_object++ );
return( pi->next_object-1 );
}

static void pdf_addpage(PI *pi) {
    if ( pi->next_page==0 ) {
	pi->max_page = 100;
	pi->page_objects = malloc(pi->max_page*sizeof(int));
    } else if ( pi->next_page>=pi->max_page ) {
	pi->max_page += 100;
	pi->page_objects = realloc(pi->page_objects,pi->max_page*sizeof(int));
    }
    pi->page_objects[pi->next_page++] = pi->next_object;
    pdf_addobject(pi);
	/* Each page is its own dictionary */
    fprintf( pi->out, "<<\n" );
    fprintf( pi->out, "  /Parent 00000 0 R\n" );	/* Fixup later */
    fprintf( pi->out, "  /Type /Page\n" );
    fprintf( pi->out, "  /Contents %d 0 R\n", pi->next_object );
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n" );
	/* Each page has its own content stream */
    pdf_addobject(pi);
    fprintf( pi->out, "<< /Length %d 0 R >>\n", pi->next_object );
    fprintf( pi->out, "stream\n" );
    pi->start_cur_page = ftell( pi->out );
}

static void pdf_finishpage(PI *pi) {
    long streamlength;

    if ( pi->pt!=pt_fontsample )
	fprintf( pi->out, "Q\n" );
    streamlength = ftell(pi->out)-pi->start_cur_page;
    fprintf( pi->out, "\nendstream\n" );
    fprintf( pi->out, "endobj\n" );

    pdf_addobject(pi);
    fprintf( pi->out, " %ld\n", streamlength );
    fprintf( pi->out, "endobj\n\n" );
}

static int pfb_getsectionlength(FILE *pfb,int sec_type,int skip_sec) {
    int len=0, sublen, ch;

    for (;;) {
	ch = getc(pfb);
	if ( ch!=0x80 ) {
	    ungetc(ch,pfb);
	    if ( len!=0 )
return( len );
return( -1 );
	}
	ch = getc(pfb);
	if ( ch!=sec_type ) {
	    fseek(pfb,-2,SEEK_CUR);
	    if ( len!=0 )
return( len );
return( -1 );
	}
	ch = getc(pfb);
	sublen = ch;
	ch = getc(pfb);
	sublen += (ch<<8);
	ch = getc(pfb);
	sublen += (ch<<16);
	ch = getc(pfb);
	sublen += (ch<<24);
	if ( !skip_sec )
return( sublen );
	len += sublen;
	fseek(pfb,sublen,SEEK_CUR);
    }
}

struct fontdesc {
    DBounds bb;
    double ascent, descent, capheight, xheight, avgwidth, maxwidth;
    double stemh, stemv;
    int flags;
};

static int figure_fontdesc(PI *pi, int sfid, struct fontdesc *fd, int fonttype, int fontstream) {
    int i, j, first = true;
    SplineFont *sf = pi->sfbits[sfid].sf;
    EncMap *map = pi->sfbits[sfid].map;
    DBounds b;
    int capcnt=0, xhcnt=0, wcnt=0;
    double samewidth = -1;
    int beyond_std = false;
    int fd_num = pi->next_object;
    int cidmax;
    char *stemv;

    memset(fd,0,sizeof(*fd));

    cidmax = 0;
    if ( sf->subfonts!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( cidmax<sf->subfonts[i]->glyphcnt )
		cidmax = sf->subfonts[i]->glyphcnt;
    } else
	cidmax = map->enccount;

    for ( i=0; i<cidmax; ++i ) {
	SplineChar *sc = NULL;
	if ( sf->subfonts!=0 ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->glyphcnt &&
			SCWorthOutputting(sf->subfonts[j]->glyphs[i]) ) {
		    sc = sf->subfonts[j]->glyphs[i];
	    break;
		}
	} else if ( map->map[i]==-1 )
	    sc = NULL;
	else
	    sc = sf->glyphs[map->map[i]];
	if ( SCWorthOutputting(sc)) {
	    int uni = sc->unicodeenc;

	    SplineCharFindBounds(sc,&b);
	    if ( first ) {
		fd->bb = b;
		first = false;
		samewidth = sc->width;
		fd->maxwidth = sc->width;
	    } else {
		if ( b.minx<fd->bb.minx ) fd->bb.minx = b.minx;
		if ( b.miny<fd->bb.miny ) fd->bb.miny = b.miny;
		if ( b.maxx>fd->bb.maxx ) fd->bb.maxx = b.maxx;
		if ( b.maxy>fd->bb.maxy ) fd->bb.maxy = b.maxy;
		if ( samewidth!=sc->width )
		    samewidth = -1;
		if ( sc->width>fd->maxwidth ) fd->maxwidth = sc->width;
	    }
	    fd->avgwidth += sc->width; ++wcnt;
	    if ( sc->layers[ly_fore].refs==NULL ) {
		/* Ascent and Descent are defined on non-accented characters */
		if ( b.miny<fd->descent ) fd->descent = b.miny;
		if ( b.maxy>fd->ascent ) fd->ascent = b.maxy;
	    }
	    if ( uni=='B' || uni=='D' || uni=='E' || uni=='F' || uni=='H' ||
		    uni=='I' || uni=='J' || uni=='L' || uni=='M' || uni=='N' ||
		    uni=='P' || uni=='R' || uni=='T' || uni=='U' || uni=='W' ||
		    uni=='X' || uni=='Y' || uni=='Z' ||
		    uni==0x393 || uni==0x395 || uni==0x396 || uni==0x397 ||
		    uni==0x399 || uni==0x39a || uni==0x39c ||
		    (uni>=0x3a0 && uni<=0x3a8) ||
		    (uni>=0x411 && uni<=0x415) || uni==0x418 ||
		    (uni>=0x41a && uni<=0x41d) || uni==0x41f || uni==0x420 ||
		    (uni>=0x422 && uni<=0x42c)) {
		fd->capheight += b.maxy;
		++capcnt;
	    }
	    if ( (uni>='u' && uni<='z') ||
		    uni==0x3c0 || uni==0x3c4 || uni==0x3c7 || uni==0x3bd ||
		    (uni>=0x432 && uni<=0x434) || uni==0x438 ||
		    (uni>=0x43a && uni<=0x43d) || uni==0x43f || uni==0x432 ||
		    (uni>=0x445 && uni<=0x44c)) {
		fd->xheight += b.maxy;
		++xhcnt;
	    }
	    /* This is a stupid defn. Every font contains accented latin and */
	    /*  they aren't in adobe std */
	    if ( uni<=0x7e )
		/* It's in adobe std */;
	    else if ( uni>0x3000 && uni<0xfb00 )
		beyond_std = true;
	    else if ( !beyond_std ) {
		for ( j=0x80; j<0x100; ++j )
		    if ( uni==unicode_from_adobestd[j])
		break;
		if ( j==0x100 )
		    beyond_std = true;
	    }
	}
    }

    if ( capcnt!=0 ) fd->capheight /= capcnt;
    if ( xhcnt!=0 ) fd->xheight /= xhcnt;
    if ( wcnt!=0 ) fd->avgwidth /= wcnt;

    if ( samewidth!=-1 ) fd->flags = 1;
    /* I can't tell whether it's serifed (flag=2) */
    if ( !beyond_std ) fd->flags |= 4;
    else fd->flags |= 1<<(6-1);
    /* I can't tell whether it's script (flag=0x10) */
    if ( strstrmatch(sf->fontname,"script")) fd->flags |= 0x10;
    if ( sf->italicangle!=0 ) fd->flags |= (1<<(7-1));

    if (( i = PSDictFindEntry(sf->private,"StdHW"))!=-1 )
	fd->stemh = strtod(sf->private->values[i],NULL);
    if (( i = PSDictFindEntry(sf->private,"StdVW"))!=-1 )
	fd->stemv = strtod(sf->private->values[i],NULL);

    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /FontDescriptor\n" );
    fprintf( pi->out, "    /FontName /%s\n", sf->fontname );
    fprintf( pi->out, "    /Flags %d\n", fd->flags );
    fprintf( pi->out, "    /FontBBox [%g %g %g %g]\n",
	    (double) fd->bb.minx, (double) fd->bb.miny, (double) fd->bb.maxx, (double) fd->bb.maxy );
    stemv = PSDictHasEntry(sf->private,"StdVW");
    if ( stemv!=NULL )		/* Said to be required, but meaningless for cid fonts where there should be multiple values */
	fprintf( pi->out, "    /StemV %s\n", stemv );
    else
	fprintf( pi->out, "    /StemV 0\n" );
    stemv = PSDictHasEntry(sf->private,"StdHW");
    if ( stemv!=NULL )
	fprintf( pi->out, "    /StemH %s\n", stemv );
    fprintf( pi->out, "    /ItalicAngle %g\n", (double) sf->italicangle );
    fprintf( pi->out, "    /Ascent %g\n", fd->ascent );
    fprintf( pi->out, "    /Descent %g\n", fd->descent );
    if ( sf->pfminfo.pfmset )
	fprintf( pi->out, "    /Leading %d\n", sf->pfminfo.linegap+sf->ascent+sf->descent );
    fprintf( pi->out, "    /CapHeight %g\n", fd->capheight );
    fprintf( pi->out, "    /XHeight %g\n", fd->xheight );
    fprintf( pi->out, "    /AvgWidth %g\n", fd->avgwidth );
    fprintf( pi->out, "    /MaxWidth %g\n", fd->maxwidth );
    if ( fonttype==1 )
	fprintf( pi->out, "    /FontFile %d 0 R\n", fontstream );
    else if ( fonttype==2 )
	fprintf( pi->out, "    /FontFile2 %d 0 R\n", fontstream );
    else
	fprintf( pi->out, "    /FontFile3 %d 0 R\n", fontstream );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n\n" );
return( fd_num );
}

static void dump_pfb_encoding(PI *pi,int sfid, int base,int font_d_ref) {
    int i, first=-1, last, gid;
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineFont *sf = sfbit->sf;
    EncMap *map = sfbit->map;

    for ( i=base; i<base+256 && i<map->enccount; ++i ) {
	gid = map->map[i];
	if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid])) {
	    if ( first==-1 ) first = i-base;
	    last = i-base;
	}
    }
    if ( first==-1 )
return;			/* Nothing in this range */
    sfbit->our_font_objs[sfbit->next_font] = pi->next_object;
    sfbit->fonts[base/256] = sfbit->next_font++;

    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /Font\n" );
    fprintf( pi->out, "    /Subtype /Type1\n" );
    fprintf( pi->out, "    /BaseFont /%s\n", sf->fontname );
    fprintf( pi->out, "    /FirstChar %d\n", first );
    fprintf( pi->out, "    /LastChar %d\n", last );
    fprintf( pi->out, "    /Widths %d 0 R\n", pi->next_object );
    fprintf( pi->out, "    /FontDescriptor %d 0 R\n", font_d_ref );
    /* Contrary to my reading of the PDF spec, Adobe Acrobat & Apple's Preview*/
    /*  will reencode a font to AdobeStandard if an encoding is omitted */
    /* Ghostview agrees with me, and does not reencode */
    /*if ( base!=0 )*/
	fprintf( pi->out, "    /Encoding %d 0 R\n", pi->next_object+1 );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );
    /* The width vector is normalized to 1000 unit em from whatever the font really uses */
    pdf_addobject(pi);
    fprintf( pi->out, "  [\n" );
    for ( i=base+first; i<=base+last; ++i ) if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	fprintf( pi->out, "    %g\n", sf->glyphs[gid]->width*1000.0/(sf->ascent+sf->descent) );
    else
	fprintf( pi->out, "    0\n" );
    fprintf( pi->out, "  ]\n" );
    fprintf( pi->out, "endobj\n" );
    /*if ( base!=0 )*/ {
	pdf_addobject(pi);
	fprintf( pi->out, "  <<\n" );
	fprintf( pi->out, "    /Type /Encoding\n" );
	fprintf( pi->out, "    /Differences [ %d\n", first );
	for ( i=base+first; i<=base+last; ++i )
	    if ( (gid=map->map[i])!=-1 && SCWorthOutputting( sf->glyphs[gid] ))
		fprintf( pi->out, "\t/%s\n", sf->glyphs[gid]->name );
	    else
		fprintf( pi->out, "\t/.notdef\n" );
	fprintf( pi->out, "    ]\n" );
	fprintf( pi->out, "  >>\n" );
	fprintf( pi->out, "endobj\n" );
    }
}

static void pdf_dump_type1(PI *pi,int sfid) {
    struct sfbits *sfbit = &pi->sfbits[sfid];
    int font_stream = pi->next_object;
    int fd_obj;
    int length1, length2, length3;
    int i;
    struct fontdesc fd;

    length1 = pfb_getsectionlength(sfbit->fontfile,1,true);
    length2 = pfb_getsectionlength(sfbit->fontfile,2,true);
    length3 = pfb_getsectionlength(sfbit->fontfile,1,true);
    pdf_addobject(pi);
    fprintf( pi->out, "<< /Length %d /Length1 %d /Length2 %d /Length3 %d>>\n",
	    length1+length2+length3, length1, length2, length3 );
    fprintf( pi->out, "stream\n" );
    rewind(sfbit->fontfile);
    length1 = pfb_getsectionlength(sfbit->fontfile,1,false);
    for ( i=0; i<length1; ++i ) {
	int ch = getc(sfbit->fontfile);
	putc(ch,pi->out);
    }
    while ( (length2 = pfb_getsectionlength(sfbit->fontfile,2,false))!= -1 ) {
	for ( i=0; i<length2; ++i ) {
	    int ch = getc(sfbit->fontfile);
	    putc(ch,pi->out);
	}
    }
    length3 = pfb_getsectionlength(sfbit->fontfile,1,false);
    for ( i=0; i<length3; ++i ) {
	int ch = getc(sfbit->fontfile);
	putc(ch,pi->out);
    }
    fprintf( pi->out, "\nendstream\n" );
    fprintf( pi->out, "endobj\n\n" );

    fd_obj = figure_fontdesc(pi, sfid, &fd,1,font_stream);

    sfbit->our_font_objs = malloc((sfbit->map->enccount/256+1)*sizeof(int *));
    sfbit->fonts = malloc((sfbit->map->enccount/256+1)*sizeof(int *));
    for ( i=0; i<sfbit->map->enccount; i += 256 ) {
	sfbit->fonts[i/256] = -1;
	dump_pfb_encoding(pi,sfid,i,fd_obj);
    }
    sfbit->twobyte = false;
}

struct opac_state {
    int isfill;
    float opacity;
    int obj;
};

struct glyph_res {
    int pattern_cnt, pattern_max;
    char **pattern_names;
    int *pattern_objs;
    int image_cnt, image_max;
    char **image_names;
    int *image_objs;
    int opacity_cnt, opacity_max;
    struct opac_state *opac_state;
};

#define GLYPH_RES_EMPTY { 0, 0, NULL, NULL, 0, 0, NULL, NULL, 0, 0, NULL }


void makePatName(char *buffer,
	RefChar *ref,SplineChar *sc,int layer,int isstroke,int isgrad) {
    /* In PDF patterns (which include gradients) are fixed to the page. They */
    /*  do not alter with the Current Transformation Matrix. So if we have */
    /*  a reference to a glyph, then every reference to the same glyph will */
    /*  need a different pattern description where that description involves */
    /*  the reference's transform matrix */

    if ( ref==NULL )
	sprintf( buffer,"%s_ly%d_%s_%s", sc->name, layer,
		    isstroke ? "stroke":"fill", isgrad ? "grad": "pattern" );
    else {
	/* PDF names are significant up to 127 chars long and can contain */
	/*  all kinds of odd characters, just no spaces or slashes, so this */
	/*  name should be legal */
	sprintf( buffer,"%s_trans_%g,%g,%g,%g,%g,%g_ly%d_%s_%s", sc->name,
		(double) ref->transform[0], (double) ref->transform[1], (double) ref->transform[2],
		(double) ref->transform[3], (double) ref->transform[4], (double) ref->transform[5],
		layer,
		isstroke ? "stroke":"fill", isgrad ? "grad": "pattern" );
    }
}

static void pdf_BrushCheck(PI *pi,struct glyph_res *gr,struct brush *brush,
	int isfill,int layer,SplineChar *sc, RefChar *ref) {
    char buffer[400];
    int function_obj, shade_obj;
    int i,j;
    struct gradient *grad = brush->gradient;
    struct pattern *pat;

    if ( grad!=NULL ) {
	function_obj = pdf_addobject(pi);
	fprintf( pi->out, "<<\n" );
	fprintf( pi->out, "  /FunctionType 0\n" );	/* Iterpolation between samples */
	fprintf( pi->out, "  /Domain [%g %g]\n", (double) grad->grad_stops[0].offset,
		(double) grad->grad_stops[grad->stop_cnt-1].offset );
	fprintf( pi->out, "  /Range [0 1.0 0 1.0 0 1.0]\n" );
	fprintf( pi->out, "  /Size [%d]\n", grad->stop_cnt==2?2:101 );
	fprintf( pi->out, "  /BitsPerSample 8\n" );
	fprintf( pi->out, "  /Decode [0 1.0 0 1.0 0 1.0]\n" );
	fprintf( pi->out, "  /Length %d\n", 3*(grad->stop_cnt==2?2:101) );
	fprintf( pi->out, ">>\n" );
	fprintf( pi->out, "stream\n" );
	if ( grad->stop_cnt==2 ) {
	    unsigned col = grad->grad_stops[0].col;
	    if ( col==COLOR_INHERITED ) col = 0x000000;
	    putc((col>>16)&0xff,pi->out);
	    putc((col>>8 )&0xff,pi->out);
	    putc((col    )&0xff,pi->out);
	    col = grad->grad_stops[1].col;
	    if ( col==COLOR_INHERITED ) col = 0x000000;
	    putc((col>>16)&0xff,pi->out);
	    putc((col>>8 )&0xff,pi->out);
	    putc((col    )&0xff,pi->out);
	} else {
	    /* Rather than try and figure out the minimum common divisor */
	    /*  off all the offsets, I'll just assume they are all percent*/
	    for ( i=0; i<=100; ++i ) {
		int col;
		double t = grad->grad_stops[0].offset +
			(grad->grad_stops[grad->stop_cnt-1].offset-
			 grad->grad_stops[0].offset)* i/100.0;
		for ( j=0; j<grad->stop_cnt; ++j )
		    if ( t<=grad->grad_stops[j].offset )
		break;
		if ( j==grad->stop_cnt )
		    col = grad->grad_stops[j-1].col;
		else if ( t==grad->grad_stops[j].offset )
		    col = grad->grad_stops[j].col;
		else {
		    double percent = (t-grad->grad_stops[j-1].offset)/ (grad->grad_stops[j].offset-grad->grad_stops[j-1].offset);
		    uint32 col1 = grad->grad_stops[j-1].col;
		    uint32 col2 = grad->grad_stops[j  ].col;
		    if ( col1==COLOR_INHERITED ) col1 = 0x000000;
		    if ( col2==COLOR_INHERITED ) col2 = 0x000000;
		    int red   = ((col1>>16)&0xff)*(1-percent) + ((col2>>16)&0xff)*percent;
		    int green = ((col1>>8 )&0xff)*(1-percent) + ((col2>>8 )&0xff)*percent;
		    int blue  = ((col1    )&0xff)*(1-percent) + ((col2    )&0xff)*percent;
		    col = (red<<16) | (green<<8) | blue;
		}
		if ( col==COLOR_INHERITED ) col = 0x000000;
		putc((col>>16)&0xff,pi->out);
		putc((col>>8 )&0xff,pi->out);
		putc((col    )&0xff,pi->out);
	    }
	}
	fprintf( pi->out, "\nendstream\n" );
	fprintf( pi->out, "endobj\n" );

	shade_obj = pdf_addobject(pi);
	fprintf( pi->out, "<<\n" );
	fprintf( pi->out, "  /ShadingType %d\n", grad->radius==0?2:3 );
	fprintf( pi->out, "  /ColorSpace /DeviceRGB\n" );
	if ( grad->radius==0 ) {
	    fprintf( pi->out, "  /Coords [%g %g %g %g]\n",
		    (double) grad->start.x, (double) grad->start.y, (double) grad->stop.x, (double) grad->stop.y);
	} else {
	    fprintf( pi->out, "  /Coords [%g %g 0 %g %g %g]\n",
		    (double) grad->start.x, (double) grad->start.y, (double) grad->stop.x, (double) grad->stop.y,
		    (double) grad->radius );
	}
	fprintf( pi->out, "  /Function %d 0 R\n", function_obj );
	fprintf( pi->out, "  /Extend [true true]\n" );	/* implies pad */
	fprintf( pi->out, ">>\n" );
	fprintf( pi->out, "endobj\n" );

	if ( gr->pattern_cnt>=gr->pattern_max ) {
	    gr->pattern_names = realloc(gr->pattern_names,(gr->pattern_max+=100)*sizeof(char *));
	    gr->pattern_objs  = realloc(gr->pattern_objs ,(gr->pattern_max     )*sizeof(int   ));
	}
	makePatName(buffer,ref,sc,layer,!isfill,true);
	gr->pattern_names[gr->pattern_cnt  ] = copy(buffer);
	gr->pattern_objs [gr->pattern_cnt++] = pdf_addobject(pi);
	fprintf( pi->out, "<<\n" );
	fprintf( pi->out, "  /Type /Pattern\n" );
	fprintf( pi->out, "  /PatternType 2\n" );
	fprintf( pi->out, "  /Shading %d 0 R\n", shade_obj );
	fprintf( pi->out, ">>\n" );
	fprintf( pi->out, "endobj\n" );
    } else if ( (pat=brush->pattern)!=NULL ) {
	SplineChar *pattern_sc = SFGetChar(sc->parent,-1,pat->pattern);
	DBounds b;
	real scale[6], result[6];
	int respos, resobj;
	int lenpos, lenstart, len;

	if ( pattern_sc==NULL )
	    LogError(_("No glyph named %s, used as a pattern in %s\n"), pat->pattern, sc->name);
	PatternSCBounds(pattern_sc,&b);

	if ( gr->pattern_cnt>=gr->pattern_max ) {
	    gr->pattern_names = realloc(gr->pattern_names,(gr->pattern_max+=100)*sizeof(char *));
	    gr->pattern_objs  = realloc(gr->pattern_objs ,(gr->pattern_max     )*sizeof(int   ));
	}
	makePatName(buffer,ref,sc,layer,!isfill,false);
	gr->pattern_names[gr->pattern_cnt  ] = copy(buffer);
	gr->pattern_objs [gr->pattern_cnt++] = pdf_addobject(pi);
	fprintf( pi->out, "<<\n" );
	fprintf( pi->out, "  /Type /Pattern\n" );
	fprintf( pi->out, "  /PatternType 1\n" );
	fprintf( pi->out, "  /PaintType 1\n" );		/* The intricacies of uncolored tiles are not something into which I wish to delve */
	fprintf( pi->out, "  /TilingType 1\n" );
	fprintf( pi->out, "  /BBox [%g %g %g %g]\n", (double) b.minx, (double) b.miny, (double) b.maxx, (double) b.maxy );
	fprintf( pi->out, "  /XStep %g\n", (double) (b.maxx-b.minx) );
	fprintf( pi->out, "  /YStep %g\n", (double) (b.maxy-b.miny) );
	memset(scale,0,sizeof(scale));
	scale[0] = pat->width/(b.maxx-b.minx);
	scale[3] = pat->height/(b.maxy-b.miny);
	MatMultiply(scale,pat->transform, result);
	fprintf( pi->out, "  /Matrix [%g %g %g %g %g %g]\n", (double) result[0], (double) result[1],
		(double) result[2], (double) result[3], (double) result[4], (double) result[5]);
	fprintf( pi->out, "    /Resources " );
	respos = ftell(pi->out);
	fprintf( pi->out, "000000 0 R\n" );
	fprintf( pi->out, "    /Length " );
	lenpos = ftell(pi->out);
	fprintf( pi->out, "00000000\n" );
	fprintf( pi->out, ">>\n" );
	fprintf( pi->out, " stream \n" );
	lenstart = ftell(pi->out);
	SC_PSDump((void (*)(int,void *)) fputc,pi->out,pattern_sc,true,true,ly_all);
	len = ftell(pi->out)-lenstart;
	fprintf( pi->out, " endstream\n" );
	fprintf( pi->out, "endobj\n" );

	resobj = PdfDumpGlyphResources(pi,pattern_sc);
	fseek(pi->out,respos,SEEK_SET);
	fprintf(pi->out,"%6d", resobj );
	fseek(pi->out,lenpos,SEEK_SET);
	fprintf(pi->out,"%8d", len );
	fseek(pi->out,0,SEEK_END);
    }
    if ( brush->opacity<1.0 && brush->opacity>=0 ) {
	for ( i=gr->opacity_cnt-1; i>=0; --i ) {
	    if ( brush->opacity==gr->opac_state[i].opacity && isfill==gr->opac_state[i].opacity )
	break;	/* Already done */
	}
	if ( i==-1 ) {
	    if ( gr->opacity_cnt>=gr->opacity_max ) {
		gr->opac_state = realloc(gr->opac_state,(gr->opacity_max+=100)*sizeof(struct opac_state));
	    }
	    gr->opac_state[gr->opacity_cnt].opacity = brush->opacity;
	    gr->opac_state[gr->opacity_cnt].isfill  = isfill;
	    gr->opac_state[gr->opacity_cnt].obj     = function_obj = pdf_addobject(pi);
	    ++gr->opacity_cnt;
	    fprintf( pi->out, "<<\n" );
	    fprintf( pi->out, "  /Type /ExtGState\n" );
	    if ( isfill )
		fprintf( pi->out, "  /ca %g\n", brush->opacity );
	    else
		fprintf( pi->out, "  /CA %g\n", brush->opacity );
	    fprintf( pi->out, "  /AIS false\n" );	/* alpha value */
	    fprintf( pi->out, ">>\n" );
	    fprintf( pi->out, "endobj\n\n" );
	}
    }
}

static void pdf_ImageCheck(PI *pi,struct glyph_res *gr,ImageList *images,
	int layer,SplineChar *sc) {
    char buffer[400];
    int icnt=0;
    GImage *img;
    struct _GImage *base;
    int i;

    while ( images!=NULL ) {
	img = images->image;
	base = img->list_len==0 ? img->u.image : img->u.images[1];

	if ( gr->image_cnt>=gr->image_max ) {
	    gr->image_names = realloc(gr->image_names,(gr->image_max+=100)*sizeof(char *));
	    gr->image_objs  = realloc(gr->image_objs ,(gr->image_max     )*sizeof(int   ));
	}
	sprintf( buffer, "%s_ly%d_%d_image", sc->name, layer, icnt );
	gr->image_names[gr->image_cnt  ] = copy(buffer);
	gr->image_objs [gr->image_cnt++] = pdf_addobject(pi);
	++icnt;

	fprintf( pi->out, "<<\n" );
	fprintf( pi->out, "  /Type /XObject\n" );
	fprintf( pi->out, "  /Subtype /Image\n" );
	fprintf( pi->out, "  /Width %d\n", base->width );
	fprintf( pi->out, "  /Height %d\n", base->height );
	if ( base->image_type == it_mono ) {
	    fprintf( pi->out, "  /BitsPerComponent 1\n" );
	    fprintf( pi->out, "  /ImageMask true\n" );
	    fprintf( pi->out, "  /Length %d\n", base->height*base->bytes_per_line );
	} else if ( base->image_type == it_true ) {
	    fprintf( pi->out, "  /BitsPerComponent 8\n" );
	    fprintf( pi->out, "  /ColorSpace /DeviceRGB\n" );
	    fprintf( pi->out, "  /Length %d\n", base->height*base->width*3 );
	} else if ( base->image_type == it_index ) {
	    fprintf( pi->out, "  /BitsPerComponent 8\n" );
	    fprintf( pi->out, "  /ColorSpace [/Indexed /DeviceRGB %d\n<",
		    base->clut->clut_len );
	    for ( i=0; i<base->clut->clut_len; ++i )
		fprintf( pi->out, "%06x ", base->clut->clut[i] );
	    fprintf( pi->out, ">\n" );
	    fprintf( pi->out, "  /Length %d\n", base->height*base->width );
	}
	fprintf( pi->out, ">>\n" );
	fprintf( pi->out, "stream\n" );
	if ( base->image_type!=it_true ) {
	    fwrite(base->data,1,base->height*base->bytes_per_line,pi->out);
	} else {
	    /* My image representation of colors includes a pad byte, pdf's does not */
	    uint32 *pt = (uint32 *) base->data;
	    for ( i=0; i<base->width*base->height; ++i, ++pt ) {
		int red   = (*pt>>16)&0xff;
		int green = (*pt>>8 )&0xff;
		int blue  = (*pt    )&0xff;
		putc(red,pi->out);
		putc(green,pi->out);
		putc(blue,pi->out);
	    }
	}
	fprintf( pi->out, "\nendstream\n" );
	fprintf( pi->out, "endobj\n" );
	images = images->next;
    }
}

/* We need different gradients and patterns for different transform */
/*  matrices of references to the same glyph. Sigh. */
int PdfDumpGlyphResources(PI *pi,SplineChar *sc) {
    int resobj;
    struct glyph_res gr = GLYPH_RES_EMPTY;
    int i;
    int layer;
    RefChar *ref;

    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	if ( sc->layers[layer].dofill )
	    pdf_BrushCheck(pi,&gr,&sc->layers[layer].fill_brush,true,layer,sc,NULL);
	if ( sc->layers[layer].dostroke )
	    pdf_BrushCheck(pi,&gr,&sc->layers[layer].stroke_pen.brush,false,layer,sc,NULL);
	pdf_ImageCheck(pi,&gr,sc->layers[layer].images,layer,sc);
	for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	    for ( i=0; i<ref->layer_cnt; ++i ) {
		if ( ref->layers[i].dofill )
		    pdf_BrushCheck(pi,&gr,&ref->layers[i].fill_brush,true,i,ref->sc,ref);
		if ( ref->layers[i].dostroke )
		    pdf_BrushCheck(pi,&gr,&ref->layers[i].stroke_pen.brush,false,i,ref->sc,ref);
		pdf_ImageCheck(pi,&gr,ref->layers[i].images,i,ref->sc);
	    }
	}
    }
    resobj = pdf_addobject(pi);
    fprintf(pi->out,"<<\n" );
    if ( gr.pattern_cnt!=0 ) {
	fprintf( pi->out, "  /Pattern <<\n" );
	for ( i=0; i<gr.pattern_cnt; ++i ) {
	    fprintf( pi->out, "    /%s %d 0 R\n", gr.pattern_names[i], gr.pattern_objs[i] );
	    free(gr.pattern_names[i]);
	}
	free(gr.pattern_names); free(gr.pattern_objs);
	fprintf( pi->out, "  >>\n");
    }
    if ( gr.image_cnt!=0 ) {
	fprintf( pi->out, "  /XObject <<\n" );
	for ( i=0; i<gr.image_cnt; ++i ) {
	    fprintf( pi->out, "    /%s %d 0 R\n", gr.image_names[i], gr.image_objs[i] );
	    free(gr.image_names[i]);
	}
	free(gr.image_names); free(gr.image_objs);
	fprintf( pi->out, "  >>\n");
    }
    if ( gr.opacity_cnt!=0 ) {
	fprintf( pi->out, "  /ExtGState <<\n" );
	for ( i=0; i<gr.opacity_cnt; ++i ) {
	    fprintf( pi->out, "    /gs_%s_opacity_%g %d 0 R\n",
		    gr.opac_state[i].isfill ? "fill" : "stroke",
		    gr.opac_state[i].opacity, gr.opac_state[i].obj );
	}
	free(gr.opac_state);
	fprintf( pi->out, "  >>\n");
    }
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );
return( resobj );
}

static int PdfDumpSFResources(PI *pi,SplineFont *sf) {
    int resobj;
    struct glyph_res gr = GLYPH_RES_EMPTY;
    int i;
    int layer, gid;
    SplineChar *sc;
    RefChar *ref;

    for ( gid=0; gid<sf->glyphcnt; ++gid) if ( (sc=sf->glyphs[gid])!=NULL ) {
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    if ( sc->layers[layer].dofill )
		pdf_BrushCheck(pi,&gr,&sc->layers[layer].fill_brush,true,layer,sc,NULL);
	    if ( sc->layers[layer].dostroke )
		pdf_BrushCheck(pi,&gr,&sc->layers[layer].stroke_pen.brush,false,layer,sc,NULL);
	    pdf_ImageCheck(pi,&gr,sc->layers[layer].images,layer,sc);
	    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
		for ( i=0; i<ref->layer_cnt; ++i ) {
		    if ( ref->layers[i].dofill )
			pdf_BrushCheck(pi,&gr,&ref->layers[i].fill_brush,true,i,ref->sc,ref);
		    if ( ref->layers[i].dostroke )
			pdf_BrushCheck(pi,&gr,&ref->layers[i].stroke_pen.brush,false,i,ref->sc,ref);
		    pdf_ImageCheck(pi,&gr,ref->layers[layer].images,i,ref->sc);
		}
	    }
	}
    }
    resobj = pdf_addobject(pi);
    fprintf(pi->out,"<<\n" );
    if ( gr.pattern_cnt!=0 ) {
	fprintf( pi->out, "  /Pattern <<\n" );
	for ( i=0; i<gr.pattern_cnt; ++i ) {
	    fprintf( pi->out, "    /%s %d 0 R\n", gr.pattern_names[i], gr.pattern_objs[i] );
	    free(gr.pattern_names[i]);
	}
	free(gr.pattern_names); free(gr.pattern_objs);
	fprintf( pi->out, "  >>\n");
    }
    if ( gr.image_cnt!=0 ) {
	fprintf( pi->out, "  /XObject <<\n" );
	for ( i=0; i<gr.image_cnt; ++i ) {
	    fprintf( pi->out, "    /%s %d 0 R\n", gr.image_names[i], gr.image_objs[i] );
	    free(gr.image_names[i]);
	}
	free(gr.image_names); free(gr.image_objs);
	fprintf( pi->out, "  >>\n");
    }
    if ( gr.opacity_cnt!=0 ) {
	fprintf( pi->out, "  /ExtGState <<\n" );
	for ( i=0; i<gr.opacity_cnt; ++i ) {
	    fprintf( pi->out, "    /gs_%s_opacity_%g %d 0 R\n",
		    gr.opac_state[i].isfill ? "fill" : "stroke",
		    gr.opac_state[i].opacity, gr.opac_state[i].obj );
	}
	free(gr.opac_state);
	fprintf( pi->out, "  >>\n");
    }
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );
return( resobj );
}

static int pdf_charproc(PI *pi, SplineChar *sc) {
    int ret = pi->next_object;
    long streamstart, streamlength;
    int i,last;

    pdf_addobject(pi);
    /* Now page 96 of the PDFReference.pdf manual claims that Resource dicas */
    /*  for type3 fonts should reside in the content stream dictionary. This */
    /*  isn't very meaningful because type3 fonts are not content streams. I */
    /*  assumed it meant in the stream dictionary for each glyph (which is a */
    /*  content stream) but that is not the case. It's in the font dictionary*/
    fprintf( pi->out, "<< /Length %d 0 R >>", pi->next_object );
    fprintf( pi->out, "stream\n" );
    streamstart = ftell(pi->out);

    /* In addition to drawing the glyph, we must provide some metrics */
    last = ly_fore;
    if ( sc->parent->multilayer )
	last = sc->layer_cnt-1;
    for ( i=ly_fore; i<=last; ++i ) {
	ImageList *img;
	RefChar *ref;
	int j;
	if ( (sc->layers[i].dofill && (sc->layers[i].fill_brush.col!=COLOR_INHERITED || sc->layers[i].fill_brush.gradient!=NULL || sc->layers[i].fill_brush.pattern!=NULL)) ||
		(sc->layers[i].dostroke && (sc->layers[i].stroke_pen.brush.col!=COLOR_INHERITED|| sc->layers[i].stroke_pen.brush.gradient!=NULL || sc->layers[i].stroke_pen.brush.pattern!=NULL)) )
    break;
	for ( img=sc->layers[i].images; img!=NULL; img=img->next )
	    if ( img->image->u.image->image_type != it_mono )
	break;
	if ( img!=NULL )
    break;
	for ( ref=sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
	    for ( j=0; j<ref->layer_cnt; ++j ) {
		if ( (ref->layers[j].dofill && (ref->layers[j].fill_brush.col!=COLOR_INHERITED || ref->layers[j].fill_brush.gradient!=NULL || ref->layers[j].fill_brush.pattern!=NULL)) ||
			(ref->layers[j].dostroke && (ref->layers[j].stroke_pen.brush.col!=COLOR_INHERITED|| ref->layers[j].stroke_pen.brush.gradient!=NULL || ref->layers[j].stroke_pen.brush.pattern!=NULL)) )
	    break;
		for ( img=ref->layers[j].images; img!=NULL; img=img->next )
		    if ( img->image->u.image->image_type != it_mono )
		break;
		if ( img!=NULL )
	    break;
	    }
	    if ( j!=ref->layer_cnt )
	break;
	}
	if ( ref!=NULL )
    break;
    }

    if ( i==sc->layer_cnt ) {
	/* We never set the color, use d1 to specify metrics */
	DBounds b;
	SplineCharFindBounds(sc,&b);
	fprintf( pi->out, "%d 0 %g %g %g %g d1\n",
		sc->width,
		(double) b.minx, (double) b.miny, (double) b.maxx, (double) b.maxy );
    } else
	fprintf( pi->out, "%d 0 d0\n", sc->width );

    SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,true,ly_fore);

    streamlength = ftell(pi->out)-streamstart;
    fprintf( pi->out, "\nendstream\n" );
    fprintf( pi->out, "endobj\n" );

    pdf_addobject(pi);
    fprintf( pi->out, " %ld\n", streamlength );
    fprintf( pi->out, "endobj\n\n" );
return( ret );
}

static void dump_pdf3_encoding(PI *pi,int sfid, int base,DBounds *bb, int notdefproc) {
    int i, first=-1, last, gid;
    int charprocs[256];
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineFont *sf = sfbit->sf;
    EncMap *map = sfbit->map;
    int respos, resobj;

    for ( i=base; i<base+256 && i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 ) {
	if ( SCWorthOutputting(sf->glyphs[gid]) && strcmp(sf->glyphs[gid]->name,".notdef")!=0 ) {
	    if ( first==-1 ) first = i-base;
	    last = i-base;
	}
    }
    if ( first==-1 )
return;			/* Nothing in this range */

    memset(charprocs,0,sizeof(charprocs));
    for ( i=base; i<base+256 && i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 ) {
	if ( SCWorthOutputting(sf->glyphs[gid]) && strcmp(sf->glyphs[gid]->name,".notdef")!=0 )
	    charprocs[i-base] = pdf_charproc(pi,sf->glyphs[gid]);
    }

    sfbit->our_font_objs[sfbit->next_font] = pi->next_object;
    sfbit->fonts[base/256] = sfbit->next_font++;

    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /Font\n" );
    fprintf( pi->out, "    /Subtype /Type3\n" );
    fprintf( pi->out, "    /Name /%s\n", sf->fontname );
    fprintf( pi->out, "    /FontBBox [%g %g %g %g]\n",
	    floor(bb->minx), floor(bb->miny), ceil(bb->maxx), ceil(bb->maxy) );
    fprintf( pi->out, "    /FontMatrix [%g 0 0 %g 0 0]\n",
	    1.0/(sf->ascent+sf->descent), 1.0/(sf->ascent+sf->descent));
    fprintf( pi->out, "    /FirstChar %d\n", first );
    fprintf( pi->out, "    /LastChar %d\n", last );
    fprintf( pi->out, "    /Widths %d 0 R\n", pi->next_object );
    fprintf( pi->out, "    /Encoding %d 0 R\n", pi->next_object+1 );
    fprintf( pi->out, "    /CharProcs %d 0 R\n", pi->next_object+2 );
    fprintf( pi->out, "    /Resources " );
    respos = ftell(pi->out);
    fprintf( pi->out, "000000 0 R\n" );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );

    /* Widths array */
    pdf_addobject(pi);
    fprintf( pi->out, "  [\n" );
    for ( i=base+first; i<=base+last; ++i )
	if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    fprintf( pi->out, "    %d\n", sf->glyphs[gid]->width );
	else
	    fprintf( pi->out, "    0\n" );
    fprintf( pi->out, "  ]\n" );
    fprintf( pi->out, "endobj\n" );

    /* Encoding dictionary */
    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /Encoding\n" );
    fprintf( pi->out, "    /Differences [ %d\n", first );
    for ( i=base+first; i<=base+last; ++i )
	if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    fprintf( pi->out, "\t/%s\n", sf->glyphs[gid]->name );
	else
	    fprintf( pi->out, "\t/.notdef\n" );
    fprintf( pi->out, "    ]\n" );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );

    /* Char procs dictionary */
    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "\t/.notdef %d 0 R\n", notdefproc );
    for ( i=base+first; i<=base+last; ++i )
	if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    fprintf( pi->out, "\t/%s %d 0 R\n", sf->glyphs[gid]->name, charprocs[i-base] );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );


    resobj = PdfDumpSFResources(pi,sf);
    fseek(pi->out,respos,SEEK_SET);
    fprintf(pi->out,"%06d", resobj );
    fseek(pi->out,0,SEEK_END);
}

static void pdf_gen_type3(PI *pi,int sfid) {
    int i, notdefproc;
    DBounds bb;
    SplineChar sc;
    Layer layers[2];
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineFont *sf = sfbit->sf;
    EncMap *map = sfbit->map;
    int notdefpos = SFFindNotdef(sf,-2);

    if ( notdefpos!=-1 )
	notdefproc = pdf_charproc(pi,sf->glyphs[notdefpos]);
    else {
	memset(&sc,0,sizeof(sc));
	sc.name = ".notdef";
	sc.parent = sf;
	sc.width = sf->ascent+sf->descent;
	sc.layer_cnt = 2;
	memset(layers,0,sizeof(layers));
	sc.layers = layers;
	notdefproc = pdf_charproc(pi, &sc);
    }

    SplineFontFindBounds(sf,&bb);
    sfbit->our_font_objs = malloc((map->enccount/256+1)*sizeof(int *));
    sfbit->fonts = malloc((map->enccount/256+1)*sizeof(int *));
    for ( i=0; i<map->enccount; i += 256 ) {
	sfbit->fonts[i/256] = -1;
	dump_pdf3_encoding(pi,sfid,i,&bb,notdefproc);
    }
    sfbit->twobyte = false;
}

static void pdf_build_type0(PI *pi, int sfid) {
    int cidfont_ref, fd_obj, font_stream = pi->next_object;
    long len;
    int ch, cidmax, i,j;
    struct fontdesc fd;
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineFont *sf = sfbit->sf;
    SplineFont *cidmaster = sf->cidmaster!=NULL?sf->cidmaster:sf;
    uint16 *widths;
    int defwidth = sf->ascent+sf->descent;

    fseek( sfbit->fontfile,0,SEEK_END);
    len = ftell(sfbit->fontfile );

    pdf_addobject(pi);
    fprintf( pi->out, "<< /Length %ld ", len );
    if ( sfbit->istype42cid )
	fprintf( pi->out, "/Length1 %ld>>\n", len );
    else
	fprintf( pi->out, "/Subtype /CIDFontType0C>>\n" );
    fprintf( pi->out, "stream\n" );
    rewind(sfbit->fontfile);
    while ( (ch=getc(sfbit->fontfile))!=EOF )
	putc(ch,pi->out);
    fprintf( pi->out, "\nendstream\n" );
    fprintf( pi->out, "endobj\n\n" );

    fd_obj = figure_fontdesc(pi, sfid, &fd,sfbit->istype42cid?2:3,font_stream);

    cidfont_ref = pi->next_object;
    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /Font\n" );
    fprintf( pi->out, "    /Subtype /CIDFontType%d\n", sfbit->istype42cid?2:0 );
    fprintf( pi->out, "    /BaseFont /%s\n", cidmaster->fontname);
    if ( cidmaster->cidregistry!=NULL && strmatch(cidmaster->cidregistry,"Adobe")==0 )
	fprintf( pi->out, "    /CIDSystemInfo << /Registry (%s) /Ordering (%s) /Supplement %d >>\n",
		cidmaster->cidregistry, cidmaster->ordering, cidmaster->supplement );
    else
	fprintf( pi->out, "    /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0>>\n" );
    fprintf( pi->out, "    /DW %d\n", defwidth );
    fprintf( pi->out, "    /W %d 0 R\n", pi->next_object );
    fprintf( pi->out, "    /FontDescriptor %d 0 R\n", fd_obj );
    if ( sfbit->istype42cid )
	fprintf( pi->out, "    /CIDToGIDMap /Identity\n" );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );

    cidmax = 0;
    if ( cidmaster->subfonts!=0 ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i )
	    if ( cidmax<cidmaster->subfonts[i]->glyphcnt )
		cidmax = cidmaster->subfonts[i]->glyphcnt;
    } else
	cidmax = cidmaster->glyphcnt + 2;	/* two extra useless glyphs in ttf */

    widths = malloc(cidmax*sizeof(uint16));

    for ( i=0; i<cidmax; ++i ) {
	SplineChar *sc = NULL;
	if ( cidmaster->subfonts!=0 ) {
	    for ( j=0; j<cidmaster->subfontcnt; ++j )
		if ( i<cidmaster->subfonts[j]->glyphcnt &&
			SCWorthOutputting(cidmaster->subfonts[j]->glyphs[i]) ) {
		    sc = cidmaster->subfonts[j]->glyphs[i];
	    break;
		}
	} else if ( i<cidmaster->glyphcnt )
	    sc = cidmaster->glyphs[i];
	if ( sc!=NULL )
	    widths[i] = sc->width;
	else
	    widths[i] = defwidth;
    }
    /* Width vector */
    pdf_addobject(pi);
    fprintf( pi->out, "  [\n" );
    i=0;
    while ( i<cidmax ) {
	if ( widths[i]==defwidth ) {
	    ++i;
    continue;
	}
	if ( i<cidmax-1 && widths[i]==widths[i+1] ) {
	    for ( j=i; j<cidmax && widths[j]==widths[i]; ++j );
	    --j;
	    fprintf( pi->out, "    %d %d %d\n", i,j, widths[i]);
	    i = j+1;
    continue;
	}
	fprintf( pi->out, "    %d [", i );
	j=i;
	while ( j==cidmax-1 || (j<cidmax-1 && widths[j+1]!=widths[j])) {
	    fprintf( pi->out, "%d ", widths[j]);
	    ++j;
	}
	fprintf( pi->out, "]\n" );
	i = j;
    }
    fprintf( pi->out, "  ]\n" );
    fprintf( pi->out, "endobj\n" );
    free(widths);

    fprintf( pi->out, "\n" );

    /* OK, now we've dumped up the CID part, we need to create a Type0 Font */
    sfbit->our_font_objs = malloc(sizeof(int));
    sfbit->our_font_objs[0] = pi->next_object;
    sfbit->next_font = 1;
    pdf_addobject(pi);
    fprintf( pi->out, "  <<\n" );
    fprintf( pi->out, "    /Type /Font\n" );
    fprintf( pi->out, "    /Subtype /Type0\n" );
    if ( sfbit->istype42cid )
	fprintf( pi->out, "    /BaseFont /%s\n", sfbit->sf->fontname );
    else
	fprintf( pi->out, "    /BaseFont /%s-Identity-H\n", cidmaster->fontname);
    fprintf( pi->out, "    /Encoding /Identity-H\n" );
    fprintf( pi->out, "    /DescendantFonts [%d 0 R]\n", cidfont_ref);
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n\n" );
}

static void dump_pdfprologue(PI *pi) {
/* TODO: Note, maybe this routine can be combined somehow with cvexports.c _ExportPDF() */
    time_t now;
    struct tm *tm;
    const char *author = GetAuthor();
    int sfid;

    fprintf( pi->out, "%%PDF-1.4\n%%\201\342\202\203\n" );	/* Header comment + binary comment */

    /* Output metadata */
    pdf_addobject(pi);
    fprintf( pi->out, "<<\n" );
    if ( pi->pt==pt_fontdisplay ) {
	fprintf( pi->out, "  /Title (Font Display for %s)\n", pi->mainsf->fullname );
    } else if ( pi->pt==pt_fontsample ) {
	fprintf( pi->out, "  /Title (Text Sample of %s)\n", pi->mainsf->fullname );
    } else if ( pi->sc!=NULL )
	fprintf( pi->out, "  /Title (Character Display for %s from %s)\n", pi->sc->name, pi->mainsf->fullname );
    else
	fprintf( pi->out, "  /Title (Character Displays from %s)\n", pi->mainsf->fullname );
    fprintf( pi->out, "  /Creator (FontForge)\n" );
    fprintf( pi->out, "  /Producer (FontForge)\n" );
    time(&now);
    tm = localtime(&now);
    fprintf( pi->out, "    /CreationDate (D:%04d%02d%02d%02d%02d%02d",
	    tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
#ifdef _NO_TZSET
    fprintf( pi->out, "Z)\n" );
#else
    if ( timezone==0 )
	fprintf( pi->out, "Z)\n" );
    else {
	if ( timezone<0 ) /* fprintf bug - this is a kludge to print +/- in front of a %02d-padded value */
	    fprintf( pi->out, "-" );
	else
	    fprintf( pi->out, "+" );
	fprintf( pi->out, "%02d'%02d')\n", (int)(timezone/3600),(int)(timezone/60-(timezone/3600)*60) );
    }
#endif
    if ( author!=NULL )
	fprintf( pi->out, "  /Author (%s)\n", author );
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );

    /* Output document catalog */
    pdf_addobject(pi);
    fprintf( pi->out, "<<\n" );
    fprintf( pi->out, "  /Pages 00000 0 R\n" );		/* Fix this up later */
    fprintf( pi->out, "  /Type /Catalog\n" );
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );

    for ( sfid=0; sfid<pi->sfcnt; ++sfid ) {
	struct sfbits *sfbit = &pi->sfbits[sfid];
	if ( sfbit->fontfile!=NULL ) {
	    if ( sfbit->sf->multilayer )
		/* We can't use a postscript type3 font, we have to build up a  */
		/* pdf font out of pdf graphics. Should be a one to one mapping */
		/* but syntax is a little different. Hence we don't generate a  */
		/* postscript type3 we do this instead */
		pdf_gen_type3(pi,sfid);
	    else if ( sfbit->iscid )
		pdf_build_type0(pi,sfid);
	    else
		pdf_dump_type1(pi,sfid);
	}
    }
}

static void dump_pdftrailer(PI *pi) {
    int i;
    int xrefloc;
    int sfid;

    /* Fix up the document catalog to point to the Pages dictionary */
    /*  which we will now create */
    /* Document catalog is object 2 */
    fseek(pi->out, pi->object_offsets[2], SEEK_SET );
    fprintf( pi->out, "2 0 obj\n<<\n  /Pages %05d 0 R\n", pi->next_object );

    /* Fix up every page dictionary to point to the Pages dictionary */
    for ( i=0 ; i<pi->next_page; ++i ) {
	fseek(pi->out, pi->object_offsets[pi->page_objects[i]], SEEK_SET );
	fprintf( pi->out, "%d 0 obj\n<<\n  /Parent %05d 0 R\n",
		pi->page_objects[i], pi->next_object );
    }
    fseek(pi->out, 0, SEEK_END );

    /* Now the pages dictionary */
    pdf_addobject(pi);
    fprintf( pi->out, "<<\n" );
    fprintf( pi->out, "  /Type /Pages\n" );
    fprintf( pi->out, "  /Kids [\n" );
    for ( i=0 ; i<pi->next_page; ++i )
	fprintf( pi->out, "    %d 0 R\n", pi->page_objects[i]);
    fprintf( pi->out, "  ]\n" );
    fprintf( pi->out, "  /Count %d\n", pi->next_page );
    fprintf( pi->out, "  /MediaBox [0 0 %d %d]\n", pi->pagewidth, pi->pageheight );
    fprintf( pi->out, "  /Resources <<\n" );
    /* In case we have a type3 font, include the image procsets */
    fprintf( pi->out, "    /ProcSet [/PDF /Text /ImageB /ImageC /ImageI]\n" );
    fprintf( pi->out, "    /Font <<\n" );
    fprintf( pi->out, "      /FTB %d 0 R\n", pi->next_object );
    for ( sfid=0; sfid<pi->sfcnt; ++sfid ) {
	struct sfbits *sfbit = &pi->sfbits[sfid];
	for ( i=0; i<sfbit->next_font; ++i )
	    fprintf( pi->out, "      /F%d-%d %d 0 R\n", sfid, i, sfbit->our_font_objs[i] );
    }
    fprintf( pi->out, "    >>\n" );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );

    /* Now times bold font (which is guarantteed to be present so we don't */
    /* need to include it or much info about it */
    pdf_addobject(pi);
    fprintf( pi->out, "<<\n" );
    fprintf( pi->out, "  /Type /Font\n" );
    fprintf( pi->out, "  /Subtype /Type1\n" );
    /*fprintf( pi->out, "  /Name /FTB\n" );*/		/* Obsolete and not recommended (even though required in 1.0) */
    fprintf( pi->out, "  /BaseFont /Times-Bold\n" );
    /*fprintf( pi->out, "  /Encoding /WinAnsiEncoding\n" );*/
    fprintf( pi->out, ">>\n" );
    fprintf( pi->out, "endobj\n\n" );

    xrefloc = ftell(pi->out);
    fprintf( pi->out, "xref\n" );
    fprintf( pi->out, " 0 %d\n", pi->next_object );
    fprintf( pi->out, "0000000000 65535 f \n" );	/* object 0 is magic */
    for ( i=1; i<pi->next_object; ++i )
	fprintf( pi->out, "%010d %05d n \n", pi->object_offsets[i], 0 );
    fprintf( pi->out, "trailer\n" );
    fprintf( pi->out, " <<\n" );
    fprintf( pi->out, "    /Size %d\n", pi->next_object );
    fprintf( pi->out, "    /Root 2 0 R\n" );
    fprintf( pi->out, "    /Info 1 0 R\n" );
    fprintf( pi->out, " >>\n" );
    fprintf( pi->out, "startxref\n" );
    fprintf( pi->out, "%d\n",xrefloc );
    fprintf( pi->out, "%%%%EOF\n" );

    for ( i=0; i<pi->sfcnt; ++i ) {
	free(pi->sfbits[i].our_font_objs);
	free(pi->sfbits[i].fonts);
    }
    free(pi->object_offsets);
    free(pi->page_objects);
}

static void DumpIdentCMap(PI *pi, int sfid) {
    struct sfbits *sfbit = &pi->sfbits[pi->sfid];
    SplineFont *sf = sfbit->sf;
    SplineFont *master;
    int i, j, k, max;

    master = sf->subfontcnt!=0 ? sf : sf->cidmaster;
    max = 0;
    if ( sfbit->istype42cid )
	max = sfbit->map->enccount;
    else {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->glyphcnt>max ) max = sf->subfonts[i]->glyphcnt;
    }
    sfbit->cidcnt = max;

    if ( max>65535 ) max = 65535;

    fprintf( pi->out, "%%%%BeginResource: CMap (Noop)\n" );
    fprintf( pi->out, "%%!PS-Adobe-3.0 Resource-CMap\n" );
    fprintf( pi->out, "%%%%BeginResource: CMap (Noop)\n" );
    fprintf( pi->out, "%%%%DocumentNeededResources: ProcSet (CIDInit)\n" );
    fprintf( pi->out, "%%%%IncludeResource: ProcSet (CIDInit)\n" );
    fprintf( pi->out, "%%%%BeginResource: CMap (Noop)\n" );
    if ( master!=NULL && master->cidregistry!=NULL )
	fprintf( pi->out, "%%%%Title: (Noop %s %s %d)\n", master->cidregistry, master->ordering, master->supplement );
    else
	fprintf( pi->out, "%%%%Title: (Noop Adobe Identity 0)\n" );
    fprintf( pi->out, "%%%%EndComments\n" );

    fprintf( pi->out, "/CIDInit /ProcSet findresource begin\n" );

    fprintf( pi->out, "12 dict begin\n" );

    fprintf( pi->out, "begincmap\n" );

    fprintf( pi->out, "/CIDSystemInfo 3 dict dup begin\n" );
    if ( master!=NULL && master->cidregistry!=NULL ) {
	fprintf( pi->out, "  /Registry (%s) def\n", master->cidregistry );
	fprintf( pi->out, "  /Ordering (%s) def\n", master->ordering );
	fprintf( pi->out, "  /Supplement %d def\n", master->supplement );
    } else {
	fprintf( pi->out, "  /Registry (%s) def\n", "Adobe" );
	fprintf( pi->out, "  /Ordering (%s) def\n", "Identity" );
	fprintf( pi->out, "  /Supplement %d def\n", 0 );
    }
    fprintf( pi->out, "end def\n" );

    fprintf( pi->out, "/CMapName /Noop-%d def\n", sfid );
    fprintf( pi->out, "/CMapVersion 1.0 def\n" );
    fprintf( pi->out, "/CMapType 1 def\n" );

    fprintf( pi->out, "1 begincodespacerange\n" );
    fprintf( pi->out, "  <0000> <%04x>\n", ((max+255)&0xffff00)-1 );
    fprintf( pi->out, "endcodespacerange\n" );

    for ( j=0; j<=max/256; j += 100 ) {
	k = ( max/256-j > 100 )? 100 : max/256-j;
	fprintf(pi->out, "%d begincidrange\n", k );
	for ( i=0; i<k; ++i )
	    fprintf( pi->out, " <%04x> <%04x> %d\n", (j+i)<<8, ((j+i)<<8)|0xff, (j+i)<<8 );
	fprintf( pi->out, "endcidrange\n\n" );
    }

    fprintf( pi->out, "endcmap\n" );
    fprintf( pi->out, "CMapName currentdict /CMap defineresource pop\n" );
    fprintf( pi->out, "end\nend\n" );

    fprintf( pi->out, "%%%%EndResource\n" );
    fprintf( pi->out, "%%%%EndResource\n" );
    fprintf( pi->out, "%%%%EOF\n" );
    fprintf( pi->out, "%%%%EndResource\n" );
}

static void dump_prologue(PI *pi) {
    time_t now;
    int ch, i, j, base, sfid;
    const char *author = GetAuthor();

    if ( pi->printtype==pt_pdf ) {
	dump_pdfprologue(pi);
return;
    }

    fprintf( pi->out, "%%!PS-Adobe-3.0\n" );
    fprintf( pi->out, "%%%%BoundingBox: 20 20 %d %d\n", pi->pagewidth-30, pi->pageheight-20 );
    fprintf( pi->out, "%%%%Creator: FontForge\n" );
    time(&now);
    fprintf( pi->out, "%%%%CreationDate: %s", ctime(&now) );
    fprintf( pi->out, "%%%%DocumentData: Binary\n" );
    if ( author!=NULL )
	fprintf( pi->out, "%%%%For: %s\n", author);
    fprintf( pi->out, "%%%%LanguageLevel: %d\n", 3 );
    fprintf( pi->out, "%%%%Orientation: Portrait\n" );
    fprintf( pi->out, "%%%%Pages: (atend)\n" );
    if ( pi->pt==pt_fontdisplay ) {
	fprintf( pi->out, "%%%%Title: Font Display for %s\n", pi->mainsf->fullname );
	fprintf( pi->out, "%%%%DocumentSuppliedResources: font %s\n", pi->mainsf->fontname );
    } else if ( pi->pt==pt_fontsample ) {
	fprintf( pi->out, "%%%%Title: Text Sample of %s\n", pi->mainsf->fullname );
	fprintf( pi->out, "%%%%DocumentSuppliedResources: font %s\n", pi->mainsf->fontname );
    } else if ( pi->sc!=NULL )
	fprintf( pi->out, "%%%%Title: Character Display for %s from %s\n", pi->sc->name, pi->mainsf->fullname );
    else
	fprintf( pi->out, "%%%%Title: Character Displays from %s\n", pi->mainsf->fullname );
    fprintf( pi->out, "%%%%DocumentNeededResources: font Times-Bold\n" );
    /* Just in case they have a unicode font without dingbats */
    fprintf( pi->out, "%%%%DocumentNeededResources: font ZapfDingbats\n" );
    /* Just in case they have a cid keyed font */
    fprintf( pi->out, "%%%%DocumentNeededResources: ProcSet (CIDInit)\n" );
    fprintf( pi->out, "%%%%EndComments\n\n" );

    fprintf( pi->out, "%%%%BeginSetup\n" );
    if ( pi->hadsize )
	fprintf( pi->out, "<< /PageSize [%d %d] >> setpagedevice\n", pi->pagewidth, pi->pageheight );

    fprintf( pi->out, "%% <font> <encoding> font_remap <font>	; from the cookbook\n" );
    fprintf( pi->out, "/reencodedict 5 dict def\n" );
    fprintf( pi->out, "/font_remap { reencodedict begin\n" );
    fprintf( pi->out, "  /newencoding exch def\n" );
    fprintf( pi->out, "  /basefont exch def\n" );
    fprintf( pi->out, "  /newfont basefont  maxlength dict def\n" );
    fprintf( pi->out, "  basefont {\n" );
    fprintf( pi->out, "    exch dup dup /FID ne exch /Encoding ne and\n" );
    fprintf( pi->out, "	{ exch newfont 3 1 roll put }\n" );
    fprintf( pi->out, "	{ pop pop }\n" );
    fprintf( pi->out, "    ifelse\n" );
    fprintf( pi->out, "  } forall\n" );
    fprintf( pi->out, "  newfont /Encoding newencoding put\n" );
    fprintf( pi->out, "  /foo newfont definefont	%%Leave on stack\n" );
    fprintf( pi->out, "  end } def\n" );
    fprintf( pi->out, "/n_show { moveto show } bind def\n" );

    fprintf( pi->out, "%%%%IncludeResource: font Times-Bold\n" );
    fprintf( pi->out, "/Times-Bold-ISO-8859-1 /Times-Bold findfont ISOLatin1Encoding font_remap definefont\n" );
    fprintf( pi->out, "/Times-Bold__12 /Times-Bold-ISO-8859-1 findfont 12 scalefont def\n" );

    if ( pi->showvm )
	fprintf( pi->out, " vmstatus pop /VM1 exch def pop\n" );
    for ( sfid=0; sfid<pi->sfcnt; ++sfid ) {
	struct sfbits *sfbit = &pi->sfbits[pi->sfid];
	if ( sfbit->fontfile!=NULL ) {
	    fprintf( pi->out, "%%%%BeginResource: font %s\n", sfbit->sf->fontname );
	    while ( (ch=getc(sfbit->fontfile))!=EOF )
		putc(ch,pi->out);
	    fclose( sfbit->fontfile ); sfbit->fontfile = NULL;
	    fprintf( pi->out, "\n%%%%EndResource\n" );
	    if ( sfbit->iscid )
		DumpIdentCMap(pi,sfid);
	    if ( pi->pt!=pt_fontsample ) {
		sprintf(sfbit->psfontname,"%s__%d", sfbit->sf->fontname, pi->pointsize );
		if ( !sfbit->iscid )
		    fprintf(pi->out,"/%s /%s findfont %d scalefont def\n",
			    sfbit->psfontname, sfbit->sf->fontname, pi->pointsize );
		else
		    fprintf(pi->out,"/%s /%s--Noop /Noop-%d [ /%s ] composefont %d scalefont def\n",
			    sfbit->psfontname, sfbit->sf->fontname, 0, sfbit->sf->fontname, pi->pointsize );
	    }

	    if ( !sfbit->iscid ) {
		/* Now see if there are any unencoded characters in the font, and if so */
		/*  reencode enough fonts to display them all. These will all be 256 char fonts */
		if ( sfbit->twobyte )
		    i = 65536;
		else
		    i = 256;
		for ( ; i<sfbit->map->enccount; ++i ) {
		    int gid = sfbit->map->map[i];
		    if ( gid!=-1 && SCWorthOutputting(sfbit->sf->glyphs[gid]) ) {
			base = i&~0xff;
			if ( pi->pt!=pt_fontsample )
			    fprintf( pi->out, "/%s-%x__%d /%s-%x /%s%s findfont [\n",
				    sfbit->sf->fontname, base>>8, pi->pointsize,
				    sfbit->sf->fontname, base>>8,
				    sfbit->sf->fontname, sfbit->twobyte?"Base":"" );
			else
			    fprintf( pi->out, "/%s-%x /%s%s findfont [\n",
				    sfbit->sf->fontname, base>>8,
				    sfbit->sf->fontname, sfbit->twobyte?"Base":"" );
			for ( j=0; j<0x100 && base+j<sfbit->map->enccount; ++j ) {
			    int gid2 = sfbit->map->map[base+j];
			    if ( gid2!=-1 && SCWorthOutputting(sfbit->sf->glyphs[gid2]))
				fprintf( pi->out, "\t/%s\n", sfbit->sf->glyphs[gid2]->name );
			    else
				fprintf( pi->out, "\t/.notdef\n" );
			}
			for ( ;j<0x100; ++j )
			    fprintf( pi->out, "\t/.notdef\n" );
			if ( pi->pt!=pt_fontsample )
			    fprintf( pi->out, " ] font_remap definefont %d scalefont def\n",
				    pi->pointsize );
			else
			    fprintf( pi->out, " ] font_remap definefont\n" );
			i = base+0xff;
		    }
		}
	    }
	}
    }
    if ( pi->showvm )
	fprintf( pi->out, "vmstatus pop dup VM1 sub (Max VMusage: ) print == flush\n" );

    fprintf( pi->out, "%%%%EndSetup\n\n" );
}

static int PIDownloadFont(PI *pi, SplineFont *sf, EncMap *map) {
    int is_mm = sf->mm!=NULL && MMValid(sf->mm,false);
    int error = false;
    struct sfbits *sfbit = &pi->sfbits[pi->sfid];

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    sfbit->sf = sf;
    sfbit->map = map;
    sfbit->twobyte = map->enc->has_2byte;
    sfbit->wastwobyte = sfbit->twobyte;
    sfbit->isunicode = map->enc->is_unicodebmp;
    sfbit->isunicodefull = map->enc->is_unicodefull;
    sfbit->istype42cid = sf->layers[ly_fore].order2;
    sfbit->iscid = sf->subfontcnt!=0 || sfbit->istype42cid;
    if ( pi->pointsize==0 )
	pi->pointsize = sfbit->iscid && !sfbit->istype42cid?18:20;		/* 18 fits 20 across, 20 fits 16 */

    sfbit->fontfile = tmpfile();
    if ( sfbit->fontfile==NULL ) {
	ff_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return(false);
    }
    if ( pi->sfid==0 )
	ff_progress_start_indicator(10,_("Printing Font"),_("Printing Font"),
		_("Generating PostScript Font"),sf->glyphcnt,1);
    else
	ff_progress_reset();
    ff_progress_enable_stop(false);
    if ( pi->printtype==pt_pdf && sf->multilayer ) {
	/* These need to be done in line as pdf objects */
	/* I leave fontfile open as a flag, even though we don't use it */
    } else if ( pi->printtype==pt_pdf && sfbit->iscid ) {
	if ( !_WriteTTFFont(sfbit->fontfile,sf,
		sfbit->istype42cid?ff_type42cid:ff_cffcid,NULL,bf_none,
		ps_flag_nocffsugar,map,ly_fore))
	    error = true;
    } else if ( !_WritePSFont(sfbit->fontfile,sf,
		pi->printtype==pt_pdf ? ff_pfb :
		sf->multilayer?ff_ptype3:
		is_mm?ff_mma:
		sfbit->istype42cid?ff_type42cid:
		sfbit->iscid?ff_cid:
		sfbit->twobyte?ff_ptype0:
		ff_pfa,ps_flag_identitycidmap,map,NULL,ly_fore))
	error = true;

    ff_progress_end_indicator();

    if ( error ) {
	ff_post_error(_("Failed to generate postscript font"),_("Failed to generate postscript font") );
	fclose(sfbit->fontfile);
return(false);
    }

    rewind(sfbit->fontfile);
    ++ pi->sfcnt;
return( true );
}

static void endpage(PI *pi ) {
    if ( pi->printtype!=pt_pdf )
	fprintf(pi->out,"showpage cleartomark restore\t\t%%End of Page\n" );
    else
	pdf_finishpage(pi);
}

static void dump_trailer(PI *pi) {
    if ( pi->page!=0 )
	endpage(pi);
    if ( pi->printtype==pt_pdf )
	dump_pdftrailer(pi);
    else {
	fprintf( pi->out, "%%%%Trailer\n" );
	fprintf( pi->out, "%%%%Pages: %d\n", pi->page );
	fprintf( pi->out, "%%%%EOF\n" );
    }
}

/* ************************************************************************** */
/* ************************* Code for full font dump ************************ */
/* ************************************************************************** */

static void startpage(PI *pi ) {
    int i;
    /* I used to have a progress indicator here showing pages. But they went */
    /*  by so fast that even for CaslonRoman it was pointless */

    if ( pi->page!=0 )
	endpage(pi);
    ++pi->page;
    pi->ypos = -60-.9*pi->pointsize;

    if ( pi->printtype==pt_pdf ) {
	pdf_addpage(pi);
	if ( pi->pt == pt_chars )
return;
	fprintf(pi->out,"q 1 0 0 1 40 %d cm\n", pi->pageheight-54 );
	fprintf( pi->out, "BT\n  /FTB 12 Tf\n  193.2 -10.92 Td\n" );
	fprintf(pi->out,"(Font Display for %s) Tj\n", pi->mainsf->fontname );
	fprintf( pi->out, "-159.8 -43.98 Td\n" );
	if ( pi->sfbits[0].iscid && !pi->sfbits[0].istype42cid)
	    for ( i=0; i<pi->max; ++i )
		fprintf(pi->out,"%d 0 Td (%d) Tj\n", (pi->pointsize+pi->extrahspace), i );
	else
	    for ( i=0; i<pi->max; ++i )
		fprintf(pi->out,"%d 0 Td (%X) Tj\n", (pi->pointsize+pi->extrahspace), i );
	fprintf( pi->out, "ET\n" );
return;
    }

    fprintf(pi->out,"%%%%Page: %d %d\n", pi->page, pi->page );
    fprintf(pi->out,"%%%%PageResources: font Times-Bold font %s\n", pi->mainsf->fontname );
    fprintf(pi->out,"save mark\n" );
    fprintf(pi->out,"40 %d translate\n", pi->pageheight-54 );
    fprintf(pi->out,"Times-Bold__12 setfont\n" );
    fprintf(pi->out,"(Font Display for %s) 193.2 -10.92 n_show\n", pi->mainsf->fontname);

    if ( pi->sfbits[0].iscid && !pi->sfbits[0].istype42cid )
	for ( i=0; i<pi->max; ++i )
	    fprintf(pi->out,"(%d) %d -54.84 n_show\n", i, 60+(pi->pointsize+pi->extrahspace)*i );
    else
	for ( i=0; i<pi->max; ++i )
	    fprintf(pi->out,"(%X) %d -54.84 n_show\n", i, 60+(pi->pointsize+pi->extrahspace)*i );
}

static int DumpLine(PI *pi) {
    int i=0, line, gid;
    struct sfbits *sfbit = &pi->sfbits[0];

    /* First find the next line with stuff on it */
    if ( !sfbit->iscid || sfbit->istype42cid ) {
	for ( line = pi->chline ; line<sfbit->map->enccount; line += pi->max ) {
	    for ( i=0; i<pi->max && line+i<sfbit->map->enccount; ++i )
		if ( (gid=sfbit->map->map[line+i])!=-1 )
		    if ( SCWorthOutputting(sfbit->sf->glyphs[gid]) )
	    break;
	    if ( i!=pi->max )
	break;
	}
    } else {
	for ( line = pi->chline ; line<sfbit->cidcnt; line += pi->max ) {
	    for ( i=0; i<pi->max && line+i<sfbit->cidcnt; ++i )
		if ( CIDWorthOutputting(sfbit->sf,line+i)!= -1 )
	    break;
	    if ( i!=pi->max )
	break;
	}
    }
    if ( line+i>=sfbit->cidcnt )		/* Nothing more */
return(0);

    if ( sfbit->iscid )
	/* No encoding worries */;
    else if ( (sfbit->wastwobyte && line>=65536) || ( !sfbit->wastwobyte && line>=256 ) ) {
	/* Nothing more encoded. Can't use the normal font, must use one of */
	/*  the funky reencodings we built up at the beginning */
	if ( pi->lastbase!=(line>>8) ) {
	    if ( !pi->overflow ) {
		/* draw a line to indicate the end of the encoding */
		/* but we've still got more (unencoded) glyphs coming */
		if ( pi->printtype==pt_pdf ) {
		    fprintf( pi->out, "%d %d m %d %d l S\n",
			100, pi->ypos+8*pi->pointsize/10-1, 400, pi->ypos+8*pi->pointsize/10-1 );
		} else
		    fprintf( pi->out, "%d %d moveto %d %d rlineto stroke\n",
			100, pi->ypos+8*pi->pointsize/10-1, 300, 0 );
		pi->ypos -= 5;
	    }
	    pi->overflow = true;
	    pi->lastbase = (line>>8);
	    sprintf(sfbit->psfontname,"%s-%x__%d", sfbit->sf->fontname, pi->lastbase,
		    pi->pointsize );
	}
    }

    if ( pi->chline==0 ) {
	/* start the first page */
	startpage(pi);
    } else {
	/* start subsequent pages by displaying the one before */
	if ( pi->ypos - pi->pointsize < -(pi->pageheight-90) ) {
	    startpage(pi);
	}
    }
    pi->chline = line;

    if ( pi->printtype==pt_pdf ) {
	int lastfont = -1;
	if ( !pi->overflow || (line<17*65536 && sfbit->isunicodefull)) {
	    fprintf(pi->out, "BT\n  /FTB 12 Tf\n  26.88 %d Td\n", pi->ypos );
	    if ( sfbit->iscid && !sfbit->istype42cid )
		fprintf(pi->out,"(%d) Tj\n", pi->chline );
	    else
		fprintf(pi->out,"(%04X) Tj\n", pi->chline );
	    fprintf(pi->out, "ET\n" );
	}
	fprintf(pi->out, "BT\n  %d %d Td\n", 58-(pi->pointsize+pi->extrahspace), pi->ypos );
	if ( sfbit->iscid )
	    fprintf(pi->out, "  /F0 %d Tf\n", pi->pointsize );
	for ( i=0; i<pi->max ; ++i ) {
	    fprintf( pi->out, "  %d 0 TD\n", pi->pointsize+pi->extrahspace );
	    if ( i+pi->chline<sfbit->cidcnt &&
			((sfbit->iscid && !sfbit->istype42cid && CIDWorthOutputting(sfbit->sf,i+pi->chline)!=-1) ||
			 ((!sfbit->iscid || sfbit->istype42cid) && (gid=sfbit->map->map[i+pi->chline])!=-1 &&
				 SCWorthOutputting(sfbit->sf->glyphs[gid]))) ) {
		/*int x = 58 + i*(pi->pointsize+pi->extrahspace);*/
		if ( !sfbit->iscid && (i+pi->chline)/256 != lastfont ) {
		    lastfont = (i+pi->chline)/256;
		    fprintf(pi->out, "  /F%d-%d %d Tf\n", pi->sfid, sfbit->fonts[lastfont], pi->pointsize );
		}
		if ( sfbit->istype42cid ) {
		    int gid = sfbit->map->map[pi->chline+i];
		    SplineChar *sc = gid==-1? NULL : sfbit->sf->glyphs[gid];
		    fprintf( pi->out, "  <%04x> Tj\n", sc==NULL ? 0 : sc->ttf_glyph );
		} else if ( sfbit->iscid )
		    fprintf( pi->out, "  <%04x> Tj\n", pi->chline+i );
		else
		    fprintf( pi->out, "  <%02x> Tj\n", (pi->chline+i)%256 );
	    }
	}
	fprintf(pi->out, "ET\n" );
    } else {
	if ( !pi->overflow || (line<17*65536 && sfbit->isunicodefull)) {
	    fprintf(pi->out,"Times-Bold__12 setfont\n" );
	    if ( sfbit->iscid && !sfbit->istype42cid )
		fprintf(pi->out,"(%d) 26.88 %d n_show\n", pi->chline, pi->ypos );
	    else
		fprintf(pi->out,"(%04X) 26.88 %d n_show\n", pi->chline, pi->ypos );
	}
	fprintf(pi->out,"%s setfont\n", sfbit->psfontname );
	for ( i=0; i<pi->max ; ++i ) {
	    if ( i+pi->chline<sfbit->cidcnt &&
			((sfbit->iscid && !sfbit->istype42cid && CIDWorthOutputting(sfbit->sf,i+pi->chline)!=-1) ||
			 ((!sfbit->iscid || sfbit->istype42cid) && (gid=sfbit->map->map[i+pi->chline])!=-1 &&
				 SCWorthOutputting(sfbit->sf->glyphs[gid]))) ) {
		int x = 58 + i*(pi->pointsize+pi->extrahspace);
		if ( sfbit->istype42cid ) {
		    int gid = sfbit->map->map[pi->chline+i];
		    if ( gid!=-1 ) gid = sfbit->sf->glyphs[gid]->ttf_glyph;
		    fprintf( pi->out, "<%04x> %d %d n_show\n", gid==-1?0:gid,
			    x, pi->ypos );
		} else if ( pi->overflow ) {
		    fprintf( pi->out, "<%02x> %d %d n_show\n", pi->chline +i-(pi->lastbase<<8),
			    x, pi->ypos );
		} else if ( sfbit->iscid ) {
		    fprintf( pi->out, "<%04x> %d %d n_show\n", pi->chline +i,
			    x, pi->ypos );
		} else if ( sfbit->twobyte ) {
		    fprintf( pi->out, "<%04x> %d %d n_show\n", pi->chline +i,
			    x, pi->ypos );
		} else {
		    fprintf( pi->out, "<%02x> %d %d n_show\n", pi->chline +i,
			    x, pi->ypos );
		}
	    }
	}
    }
    pi->ypos -= pi->pointsize+pi->extravspace;
    pi->chline += pi->max;
return(true);
}

static void PIFontDisplay(PI *pi) {
    SplineFont *sf = pi->mainsf;

    if ( !PIDownloadFont(pi,sf,pi->mainmap))
return;
    dump_prologue(pi);

    pi->extravspace = pi->pointsize/6;
    pi->extrahspace = pi->pointsize/3;
    pi->max = (pi->pagewidth-100)/(pi->extrahspace+pi->pointsize);
    pi->sfbits[0].cidcnt = pi->sfbits[0].map->enccount;
    if ( sf->subfontcnt!=0 && pi->sfbits[0].iscid ) {
	int i,max=0;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->glyphcnt>max )
		max = sf->subfonts[i]->glyphcnt;
	pi->sfbits[0].cidcnt = max;
    }

    if ( pi->sfbits[0].iscid && !pi->sfbits[0].istype42cid ) {
	if ( pi->max>=20 ) pi->max = 20;
	else if ( pi->max>=10 ) pi->max = 10;
	else if ( pi->max >= 5 ) pi->max = 5;
	else if ( pi->max >= 2 ) pi->max = 2;
    } else {
	if ( pi->max>=32 ) pi->max = 32;
	else if ( pi->max>=16 ) pi->max = 16;
	else if ( pi->max>=8 ) pi->max = 8;
	else if ( pi->max >= 4 ) pi->max = 4;
	else if ( pi->max >= 2 ) pi->max = 2;
    }

    while ( DumpLine(pi))
	;

    if ( pi->chline==0 )
	ff_post_notice(_("Print Failed"),_("Warning: Font contained no glyphs"));
    else
	dump_trailer(pi);
}

/* ************************************************************************** */
/* ********************* Code for single character dump ********************* */
/* ************************************************************************** */

static void SCPrintPage(PI *pi,SplineChar *sc) {
    DBounds b, page;
    real scalex, scaley;

    if ( pi->page!=0 )
	endpage(pi);
    ++pi->page;
    if ( pi->printtype!=pt_pdf ) {
	fprintf(pi->out,"%%%%Page: %d %d\n", pi->page, pi->page );
	fprintf(pi->out,"%%%%PageResources: font Times-Bold\n" );
	fprintf(pi->out,"save mark\n" );
    } else {
	startpage(pi);
    }

    SplineCharFindBounds(sc,&b);
    if ( b.maxy<sc->parent->ascent+5 ) b.maxy = sc->parent->ascent + 5;
    if ( b.miny>-sc->parent->descent ) b.miny =-sc->parent->descent - 5;
    if ( b.minx>00 ) b.minx = -5;
    if ( b.maxx<=0 ) b.maxx = 5;
    if ( b.maxx<=sc->width+5 ) b.maxx = sc->width+5;

    /* From the default bounding box */
    page.minx = 40; page.maxx = pi->pagewidth-15;
    page.miny = 20; page.maxy = pi->pageheight-20;

    if ( pi->printtype!=pt_pdf ) {
	fprintf(pi->out,"Times-Bold__12 setfont\n" );
	fprintf(pi->out,"(%s from %s) 80 %g n_show\n", sc->name, sc->parent->fullname, (double) (page.maxy-12) );
    } else {
	fprintf( pi->out, "BT\n" );
	fprintf( pi->out, "  /FTB 12 Tf\n" );
	fprintf( pi->out, "  80 %g Td\n", (double) (page.maxy-12) );
	fprintf( pi->out, "  (%s from %s) Tj\n", sc->name, sc->parent->fullname );
	fprintf( pi->out, "ET\n" );
    }
    page.maxy -= 20;

    scalex = (page.maxx-page.minx)/(b.maxx-b.minx);
    scaley = (page.maxy-page.miny)/(b.maxy-b.miny);
    pi->scale = (scalex<scaley)?scalex:scaley;
    pi->xoff = page.minx - b.minx*pi->scale;
    pi->yoff = page.miny - b.miny*pi->scale;

    if ( pi->printtype!=pt_pdf ) {
	fprintf(pi->out,"gsave .2 setlinewidth\n" );
	fprintf(pi->out,"%g %g moveto %g %g lineto stroke\n", (double) page.minx, (double) pi->yoff, (double) page.maxx, (double) pi->yoff );
	fprintf(pi->out,"%g %g moveto %g %g lineto stroke\n", (double) pi->xoff, (double) page.miny, (double) pi->xoff, (double) page.maxy );
	fprintf(pi->out,"%g %g moveto %g %g lineto stroke\n", (double) page.minx, (double) (sc->parent->ascent*pi->scale+pi->yoff), (double) page.maxx, (double) (sc->parent->ascent*pi->scale+pi->yoff) );
	fprintf(pi->out,"%g %g moveto %g %g lineto stroke\n", (double) page.minx, (double) (-sc->parent->descent*pi->scale+pi->yoff), (double) page.maxx, (double) (-sc->parent->descent*pi->scale+pi->yoff) );
	fprintf(pi->out,"%g %g moveto %g %g lineto stroke\n", (double) (pi->xoff+sc->width*pi->scale), (double) page.miny, (double) (pi->xoff+sc->width*pi->scale), (double) page.maxy );
	fprintf(pi->out,"grestore\n" );
	fprintf(pi->out,"gsave\n %g %g translate\n", (double) pi->xoff, (double) pi->yoff );
	fprintf(pi->out," %g %g scale\n", (double) pi->scale, (double) pi->scale );
	SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,false,ly_fore);
	if ( sc->parent->multilayer )
	    /* All done */;
	else if ( sc->parent->strokedfont )
	    fprintf( pi->out, "%g setlinewidth stroke\n", (double) sc->parent->strokewidth );
	else
	    fprintf( pi->out, "fill\n" );
	fprintf(pi->out,"grestore\n" );
    } else {
	fprintf(pi->out,"q .2 w\n" );
	fprintf(pi->out,"%g %g m %g %g l S\n", (double) page.minx, (double) pi->yoff, (double) page.maxx, (double) pi->yoff );
	fprintf(pi->out,"%g %g m %g %g l S\n", (double) pi->xoff, (double) page.miny, (double) pi->xoff, (double) page.maxy );
	fprintf(pi->out,"%g %g m %g %g l S\n", (double) page.minx, (double) (sc->parent->ascent*pi->scale+pi->yoff), (double) page.maxx, (double) (sc->parent->ascent*pi->scale+pi->yoff) );
	fprintf(pi->out,"%g %g m %g %g l S\n", (double) page.minx, (double) (-sc->parent->descent*pi->scale+pi->yoff), (double) page.maxx, (double) (-sc->parent->descent*pi->scale+pi->yoff) );
	fprintf(pi->out,"%g %g m %g %g l S\n", (double) (pi->xoff+sc->width*pi->scale), (double) page.miny, (double) (pi->xoff+sc->width*pi->scale), (double) page.maxy );
	fprintf(pi->out,"Q\n" );
	fprintf(pi->out,"q \n %g 0 0 %g %g %g cm\n", (double) pi->scale, (double) pi->scale,
		(double) pi->xoff, (double) pi->yoff );
	SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,true,ly_fore);
	if ( sc->parent->multilayer )
	    /* All done */;
	else if ( sc->parent->strokedfont )
	    fprintf( pi->out, "%g w S\n", (double) sc->parent->strokewidth );
	else
	    fprintf( pi->out, "f\n" );
	fprintf(pi->out,"Q\n" );
    }
}

static void PIChars(PI *pi) {
    int i, gid;

    dump_prologue(pi);
    if ( pi->fv!=NULL )
	for ( i=0; i<pi->mainmap->enccount; ++i ) {
	    if ( pi->fv->selected[i] && (gid=pi->mainmap->map[i])!=-1 &&
		    SCWorthOutputting(pi->mainsf->glyphs[gid]) )
		SCPrintPage(pi,pi->mainsf->glyphs[gid]);
    } else if ( pi->sc!=NULL )
	SCPrintPage(pi,pi->sc);
    else {
	for ( i=0; i<MVGlyphCount(pi->mv); ++i )
	    if ( SCWorthOutputting(MVGlyphIndex(pi->mv,i)))
		SCPrintPage(pi,MVGlyphIndex(pi->mv,i));
    }
    dump_trailer(pi);
}

/* ************************************************************************** */
/* ************************** Code for sample text ************************** */
/* ************************************************************************** */

static void samplestartpage(PI *pi ) {
    struct sfbits *sfbit = &pi->sfbits[0];

    if ( pi->page!=0 )
	endpage(pi);
    ++pi->page;
    if ( pi->printtype==pt_pdf ) {
	pdf_addpage(pi);
	fprintf( pi->out, "BT\n  /FTB 12 Tf\n  80 %d Td\n", pi->pageheight-84 );
	if ( pi->pt==pt_fontsample )
	    fprintf(pi->out,"(Sample Text from %s) Tj\nET\n", sfbit->sf->fullname );
	else {
	    fprintf(pi->out,"(Sample Sizes of %s) Tj\n", sfbit->sf->fullname );
	    fprintf(pi->out,"ET\nq 1 0 0 1 40 %d cm\n", pi->pageheight-34-
		    pi->pointsize*sfbit->sf->ascent/(sfbit->sf->ascent+sfbit->sf->descent) );
	}
	pi->lastfont = -1;
	pi->wasps = -1;
    } else {
	fprintf(pi->out,"%%%%Page: %d %d\n", pi->page, pi->page );
	fprintf(pi->out,"%%%%PageResources: font %s\n", sfbit->sf->fontname );
	fprintf(pi->out,"save mark\n" );
	fprintf(pi->out,"Times-Bold__12 setfont\n" );
	if ( pi->pt==pt_fontsample )
	    fprintf(pi->out,"(Sample Text from %s) 80 %d n_show\n", sfbit->sf->fullname, pi->pageheight-84 );
	else {
	    fprintf(pi->out,"(Sample Sizes of %s) 80 %d n_show\n", sfbit->sf->fullname, pi->pageheight-84 );
	    fprintf(pi->out,"40 %d translate\n", pi->pageheight-34-
		    pi->pointsize*sfbit->sf->ascent/(sfbit->sf->ascent+sfbit->sf->descent) );
	}
	if ( sfbit->iscid )
	    fprintf(pi->out,"/Noop-%d [ /%s ] composefont %d scalefont setfont\n",
		    0, sfbit->sf->fontname, pi->pointsize );
	else
	    fprintf(pi->out,"/%s findfont %d scalefont setfont\n", sfbit->sf->fontname,
		    pi->pointsize);
    }

    pi->ypos = -30;
}

static void outputchar(PI *pi, int sfid, SplineChar *sc) {
    int enc;

    if ( sc==NULL )
return;
    /* type42cid output uses a CIDMap indexed by GID */
    if ( pi->sfbits[sfid].istype42cid ) {
 	fprintf( pi->out, "%04X", sc->ttf_glyph );
    } else {
	enc = pi->sfbits[sfid].map->backmap[sc->orig_pos];
	if ( enc==-1 )
return;
	if ( pi->sfbits[sfid].iscid ) {
	    fprintf( pi->out, "%04X", enc );
	} else if ( pi->sfbits[sfid].twobyte && enc<=0xffff ) {
	    fprintf( pi->out, "%04X", enc );
	} else {
	    fprintf( pi->out, "%02X", enc&0xff );
	}
    }
}

static void outputotchar(PI *pi,struct opentype_str *osc,int x,int baseline) {
    struct fontlist *fl = osc->fl;
    FontData *fd = fl->fd;
    struct sfmaps *sfmap = fd->sfmap;
    int sfid = sfmap->sfbit_id;
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineChar *sc = osc->sc;
    int enc = sfbit->map->backmap[sc->orig_pos];

    if ( pi->printtype==pt_pdf ) {
	int fn = sfbit->iscid?0:sfbit->fonts[enc/256];
	if ( pi->wassfid!=sfid || fn!=pi->wasfn || fd->pointsize!=pi->wasps ) {
	    fprintf(pi->out,"/F%d-%d %d Tf\n ", sfid, fn, fd->pointsize);
	    pi->wassfid = sfid; pi->wasfn = fn; pi->wasps = fd->pointsize;
	}
	fprintf(pi->out, "%g %g Td ",
		(double) ((x+osc->vr.xoff-pi->lastx)*pi->scale),
		(double) ((baseline+osc->vr.yoff+osc->bsln_off-pi->lasty)*pi->scale) );
	pi->lastx = x+osc->vr.xoff; pi->lasty = baseline+osc->vr.yoff+osc->bsln_off;
	putc('<',pi->out);
	outputchar(pi,sfid,sc);
	fprintf( pi->out, "> Tj\n" );
    } else {
	int fn = 0;
	fprintf(pi->out, "%g %g moveto ",
		(double) ((x+osc->vr.xoff)*pi->scale),
		(double) ((baseline+osc->vr.yoff+osc->bsln_off)*pi->scale) );
	if ( (sfbit->twobyte && enc>0xffff) || (!sfbit->twobyte && enc>0xff) )
	    fn = enc>>8;
	if ( pi->wassfid!=sfid || fn!=pi->wasfn || fd->pointsize!=pi->wasps ) {
	    if ( sfbit->iscid )
		putc('<',pi->out);
	    else if ( (sfbit->twobyte && enc>0xffff) || (!sfbit->twobyte && enc>0xff) )
		fprintf(pi->out,"/%s-%x findfont %d scalefont setfont\n  <",
			sfbit->sf->fontname, enc>>8,
			fd->pointsize);
	    else
		fprintf(pi->out,"/%s findfont %d scalefont setfont\n  <", sfbit->sf->fontname,
			fd->pointsize);
	    pi->wassfid = sfid; pi->wasfn = fn; pi->wasps = fd->pointsize;
	} else
	    putc('<',pi->out);
	outputchar(pi,sfid,sc);
	fprintf( pi->out, "> show\n" );
    }
}

static void PIFontSample(PI *pi) {
    struct sfmaps *sfmaps;
    int cnt=0;
    int y,x, bottom, top, baseline;
    LayoutInfo *li = pi->sample;
    struct opentype_str **line;
    int i,j;

    pi->pointsize = 12;		/* no longer meaningful */
    pi->extravspace = pi->pointsize/6;
    pi->scale = 72.0/li->dpi;
    pi->lastfont = -1; pi->intext = false;
    pi->wassfid = -1;

    for ( cnt=0, sfmaps = li->sfmaps; sfmaps!=NULL; sfmaps = sfmaps->next, ++cnt ) {
	pi->sfid = cnt;
	sfmaps->sfbit_id = cnt;
	pi->sfbits[cnt].sfmap = sfmaps;
	if ( !PIDownloadFont(pi,sfmaps->sf,sfmaps->map))
return;
    }
    dump_prologue(pi);

    samplestartpage(pi);
    if ( pi->printtype==pt_pdf ) {
	fprintf(pi->out, "BT\n" );
	pi->lastx = pi->lasty = 0;
    }
    y = top = rint((pi->pageheight - 96)/pi->scale);	/* In dpi units */
    bottom = rint(36/pi->scale);			/* multiply by scale to get ps points */

    for ( i=0; i<li->lcnt; ++i ) {
	if ( y - li->lineheights[i].fh < bottom ) {
	    if ( pi->printtype==pt_pdf )
		fprintf(pi->out, "ET\n" );
	    samplestartpage(pi);
	    if ( pi->printtype==pt_pdf ) {
		fprintf(pi->out, "BT\n" );
		pi->lastx = pi->lasty = 0;
	    }
	    y = top;
	}
	x = rint(36/pi->scale);
	baseline = y - li->lineheights[i].as;
	y -= li->lineheights[i].fh;
	line = li->lines[i];
	for ( j=0; line[j]!=NULL; ++j ) {
	    outputotchar(pi,line[j],x,baseline);
	    x += line[j]->advance_width + line[j]->vr.h_adv_off;
	}
    }
    if ( pi->printtype==pt_pdf )
	fprintf(pi->out, "ET\n" );
    dump_trailer(pi);
}

/* ************************************************************************** */
/* ************************** Code for multi size *************************** */
/* ************************************************************************** */
static double pointsizes[] = { 72, 48, 36, 24, 20, 18, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7.5, 7, 6.5, 6, 5.5, 5, 4.5, 4.2, 4, 0 };

static void SCPrintSizes(PI *pi,SplineChar *sc) {
    int xstart = 10, i;
    int enc;
    struct sfbits *sfbit = &pi->sfbits[0];

    if ( !SCWorthOutputting(sc))
return;
    enc = sfbit->map->backmap[sc->orig_pos];
    if ( pi->ypos - pi->pointsize < -(pi->pageheight-90) && pi->ypos!=-30 ) {
	samplestartpage(pi);
    }
    if ( pi->printtype==pt_pdf ) {
	fprintf(pi->out,"BT\n%d %d Td\n", xstart, pi->ypos );
    } else {
	fprintf(pi->out,"%d %d moveto ", xstart, pi->ypos );
    }
    for ( i=0; pointsizes[i]!=0; ++i ) {
	if ( pi->printtype==pt_pdf ) {
	    fprintf(pi->out,"/F%d-%d %g Tf\n  <", pi->sfid, sfbit->iscid?0:sfbit->fonts[enc/256], pointsizes[i]);
	    outputchar(pi,0,sc);
	    fprintf( pi->out, "> Tj\n" );
	    /* Don't need to use TJ here, no possibility of kerning */
	} else {
	    if ( (sfbit->twobyte && enc>0xffff) || (!sfbit->twobyte && enc>0xff) )
		fprintf(pi->out,"/%s-%x findfont %g scalefont setfont\n  <",
			sfbit->sf->fontname, enc>>8,
			pointsizes[i]);
	    else
		fprintf(pi->out,"/%s findfont %g scalefont setfont\n  <", sfbit->sf->fontname,
			pointsizes[i]);
	    outputchar(pi,0,sc);
	    fprintf( pi->out, "> show\n" );
	}
    }
    if ( pi->printtype==pt_pdf )
	fprintf(pi->out, "ET\n");
    pi->ypos -= pi->pointsize+pi->extravspace;
}

static void PIMultiSize(PI *pi) {
    int i, gid;
    struct sfbits *sfbit = &pi->sfbits[0];

    pi->pointsize = pointsizes[0];
    pi->extravspace = pi->pointsize/6;
    if ( !PIDownloadFont(pi,pi->mainsf,pi->mainmap))
return;
    dump_prologue(pi);

    samplestartpage(pi);

    if ( pi->fv!=NULL ) {
	for ( i=0; i<sfbit->map->enccount; ++i )
	    if ( pi->fv->selected[i] && (gid=sfbit->map->map[i])!=-1 &&
		    SCWorthOutputting(sfbit->sf->glyphs[gid]) )
		SCPrintSizes(pi,sfbit->sf->glyphs[gid]);
    } else if ( pi->sc!=NULL )
	SCPrintSizes(pi,pi->sc);
    else {
	for ( i=0; i<MVGlyphCount(pi->mv); ++i )
	    if ( SCWorthOutputting(MVGlyphIndex(pi->mv,i)))
		SCPrintSizes(pi,MVGlyphIndex(pi->mv,i));
    }

    dump_trailer(pi);
}

/* ************************************************************************** */
/* ***************** Sample Text in Many Scripts/Languages ****************** */
/* ************************************************************************** */

/* English */
static char *_simple[] = {
    " A quick brown fox jumps over the lazy dog.",
    NULL
};
static char *_simplelatnchoices[] = {
/* English */
    " A quick brown fox jumps over the lazy dog.",
    " Few zebras validate my paradox, quoth Jack Xeno",
    " A quick brown vixen jumps for the lazy dog.",
    " The quick blonde doxy jumps over an unfazed wag.",
/* Swedish */
    "flygande bäckasiner söka hwila på mjuka tuvor",
/* German (from http://shiar.net/misc/txt/pangram.en) */
/* Twelve boxing fighters hunted Eva across the great dike of Sylt */
    "zwölf Boxkämpfer jagten Eva quer über den großen Sylter Deich",
/* French (from http://shiar.net/misc/txt/pangram.en) */
/* Carry this old wisky to the blond smoking judge */
    "portez ce vieux whisky au juge blond qui fume",
    "Les naïfs ægithales hâtifs pondant à Noël où il gèle sont sûrs d'être déçus et de voir leurs drôles d'œufs abîmés.",
/* Dutch (from http://shiar.net/misc/txt/pangram.en) */
/* Sexy by body, though scared by the swimsuit */
    " sexy qua lijf, doch bang voor 't zwempak",
/* Polish (from http://shiar.net/misc/txt/pangram.en) */
/* to push a hedgehog or eight bins of figs in this boat */
    " pchnąć w tę łódź jeża lub ośm skrzyń fig ",
/* Slovaka (from http://shiar.net/misc/txt/pangram.en) */
    " starý kôň na hŕbe kníh žuje tíško povädnuté ruže, na stĺpe sa ďateľ učí kvákať novú ódu o živote ",
/* Czech (from http://shiar.net/misc/txt/pangram.en) */
    " příliš žluťoučký kůň úpěl ďábelské kódy ",
    NULL
};
static uint32 _simplelatnlangs[] = {
    CHR('E','N','G',' '),
    CHR('E','N','G',' '),
    CHR('E','N','G',' '),
    CHR('E','N','G',' '),
    CHR('S','V','E',' '),
    CHR('D','E','U',' '),
    CHR('F','R','A',' '),
    CHR('F','R','A',' '),
    CHR('N','L','D',' '),
    CHR('P','L','K',' '),
    CHR('S','L','V',' '),
    CHR('C','S','Y',' ')
};

/* Hebrew (from http://shiar.net/misc/txt/pangram.en) */
static char *_simplehebrew[] = {
    " ?דג סקרן שט בים מאוכזב ולפתע מצא לו חברה איך הקליטה ",
    NULL
};
/* Katakana (from http://shiar.net/misc/txt/pangram.en) */
static char *_simplekata[] = {
    " イロハニホヘト チリヌルヲ ワカヨタレソ ツネナラム/ ウヰノオクヤマ ケフコエテ アサキユメミシ ヱヒモセスン ",
    NULL
};
/* Hiragana (from http://shiar.net/misc/txt/pangram.en) */
static char *_simplehira[] = {
    " いろはにほへとちりぬるを/ わかよたれそつねならむ/ うゐのおくやまけふこえて/ あさきゆめみしゑひもせす ",
    NULL
};
/* Russian */
static char *_simplecyrill[] = {
    " Съешь ещё этих мягких французских булок, да выпей чаю!",
    NULL
};
static char *_simplecyrillchoices[] = {
/* Eat more those soft french 'little-sweet-breads' and drink tea */
    " Съешь ещё этих мягких французских булок, да выпей чаю!",
/* "In the deep forests of South citrus lived... /answer/Yeah but falsificated one!" */
    " В чащах юга жил-был цитрус -- да, но фальшивый экземпляръ!",
/* A kind lamplighter with grimy face wants to show me a stunt. */
    "Љубазни фењерџија чађавог лица хоће да ми покаже штос.",
/* More frequent filtering through the reticular bag improves
fertilization of genetic hybrids. */
    "Чешће цeђење мрeжастим џаком побољшава фертилизацију генских хибрида.",
    NULL
};
static uint32 _simplecyrilliclangs[] = {
    CHR('R','U','S',' '),	/* Russian */
    CHR('R','U','S',' '),
    CHR('S','R','B',' '),	/* Serbian */
    CHR('S','R','B',' ')
};
/* Russian */
static char *_annakarenena[] = {
    " Всѣ счастливыя семьи похожи другъ на друга, каждая несчастливая семья несчастлива по-своему.",
    " Все смѣшалось въ домѣ Облонскихъ. Жена узнала, что мужъ былъ связи съ бывшею въ ихъ домѣ француженкою-гувернанткой, и объявила мужу, что не можетъ жить съ нимъ въ одномъ домѣ.",
    NULL
};
/* Serbian (Cyrillic) */
static char *_serbcyriljohn[] = {
    "У почетку беше Реч Божија, и та Реч беше у Бога, и Бог беше Реч.",
    NULL
};
/* Spanish */
static char *_donquixote[] = {
    " En un lugar de la Mancha, de cuyo nombre no quiero acordarme, no ha mucho tiempo que vivía un hidalgo de los de lanza en astillero, adarga antigua, rocín flaco y galgo corredor.",
    NULL
};
/* German */
static char *_faust[] = {
    "Ihr naht euch wieder, schwankende Gestalten,",
    "Die früh sich einst dem trüben Blick gezeigt.",
    "Versuch ich wohl, euch diesmal festzuhalten?",
    "Fühl ich mein Herz noch jenem Wahn geneigt?",
    "Ihr drängt euch zu! Nun gut, so mögt ihr walten,",
    "Wie ihr aus Dunst und Nebel um mich steigt;",
    "Mein Busen fühlt sich jugendlich erschüttert",
    "Vom Zauberhauch, der euren Zug umwittert.",
    NULL
};
/* Anglo Saxon */
static char *_beorwulf[] = {
    "Hwæt, we Gar-Dena  in geardagum",
    "þeodcyninga  þrym gefrunon,",
    "hu ða æþelingas  ellen fremedon.",
    " Oft Scyld Scefing  sceaþena þreatum,",
    "monegum mægþum  meodosetla ofteah;",
    "egsode Eorle.  Syððan ærest wearð",
    "feasceaft funden,  (he þæs frofre gebad)",
    "weox under wolcnum,  weorðmyndum þah,",
    "oðþæt him æghwyle  þara ymbsittendra",
    "ofer hronrade  hyan scolde,",
    "gomban gyldan: þæt wæs god cyning!",
    NULL
};
/* Italian */
static char *_inferno[] = {
    " Nel mezzo del cammin di nostra vita",
    "mi ritrovai per una selva obscura,",
    "ché la diritta via era smarrita.",
    " Ahi quanto a dir qual era è cosa dura",
    "esta selva selvaggia e aspra e forte",
    "che nel pensier rinova la paura!",
    NULL
};
/* Latin */
static char *_debello[] = {
    " Gallia est omnis dīvīsa in partēs trēs, quārum ūnum incolunt Belgae, aliam Aquītānī, tertiam, quī ipsōrum linguā Celtae, nostrā Gallī appelantur. Hī omnēs linguā, īnstitūtīs, lēgibus inter sē differunt. Gallōs ab Aquītānīs Garumna flūmen, ā Belgīs Matrona et Sēquana dīvidit.",
    NULL
};
/* French */
static char *_pheadra[] = {
    "Le dessein en est pris: je pars, cher Théramène,",
    "Et quitte le séjour de l'aimable Trézène.",
    "Dans le doute mortel dont je suis agité,",
    "Je commence à rougir de mon oisiveté.",
    "Depuis plus de six mois éloigné de mon père,",
    "J'ignore le destin d'une tête si chère;",
    "J'ignore jusqu'aux lieux qui le peuvent cacher.",
    NULL
};
/* Classical Greek */
static char *_antigone[] = {
    "Ὦ κοινὸν αὐτάδελφον Ἰσμήνης κάρα,",
    "ἆῤ οἶσθ᾽ ὅτι Ζεὺς τῶν ἀπ᾽ Οἰδίπου κακῶν",
    "ὁποῖον οὐχὶ νῷν ἔτι ζσαιν τελεῖ;",
    NULL
};
/* Hebrew */ /* Seder */
static char *_hebrew[] = {
    "וְאָתָא מַלְאַךְ הַמָּוֶת, וְשָׁחַט לְשּׁוׂחֵט, רְּשָׁחַט לְתוׂרָא, רְּשָׁתַה לְמַּיָּא, דְּכָכָה לְנוּרָא, דְּשָׂרַף לְחוּטְרָא, דְּהִכָּה לְכַלְכָּא, דְּנָשַׁךְ לְשׁוּנְרָא, דְּאָכְלָה לְגַדְיָא, דִּזְבַן אַבָּא בִּתְרֵי זוּזֵי. חַד גַּדְיָא, חַד גַּדְיָא.",
    "וְאָתָא הַקָּדוֹשׁ כָּדוּךְ הוּא, וְשָׁחַט לְמַלְאַךְ הַמָּוֶת, רְּשָׁחַט לְשּׁוׂחֵט, רְּשָׁחַט לְתוׂרָא, רְּשָׁתַה לְמַּיָּא, דְּכָכָה לְנוּרָא, דְּשָׂרַף לְחוּטְרָא, דְּהִכָּה לְכַלְכָּא, דְּנָשַׁךְ לְשׁוּנְרָא, דְּאָכְלָה לְגַדְיָא, דִּזְבַן אַבָּא בִּתְרֵי זוּזֵי. חַד גַּדְיָא, חַד גַּדְיָא.",
    NULL
};
/* Arabic with no dots or vowel marks */
static char *_arabic[] = {
    "لقيت الحكمة العادلة حبا عظيما من الشعب. انسحبت من النادی فی السنة الماضية. وقعت فی الوادی فانكسرت يدی. قابلت صديقی عمرا الكاتب القدير فی السوق فقال لی انه ارسل الى الجامعة عددا من مجلته الجديدة. احتل الامير فيصل مدينة دمسق فی الحرب العالمية ودخلها راكبا على فرسه المحبوبة. ارسل عمر خالدا الى العراق ولاكن بعد مدة قصيرة وجه خالد جيشه الى سورية. قدم على دمشق واستطاع فتحها. قبل احتل عمر القدس وعقد جلستة مع حاكم القدس وقد تكلم معه عن فتح المدينة. ثم رجع عمر الى المدنة المنورة.",
    NULL
};
/* Renaissance English with period ligatures */
static char *_muchado[] = {
    " But till all graces be in one woman, one womã ſhal not com in my grace: rich ſhe ſhall be thats certain, wiſe, or ile none, vertuous, or ile neuer cheapen her.",
    NULL
};
/* Modern (well, Dickens) English */
static char *_chuzzlewit[] = {
    " As no lady or gentleman, with any claims to polite breeding, can"
    " possibly sympathize with the Chuzzlewit Family without being first"
    " assured of the extreme antiquity of the race, it is a great satisfaction"
    " to know that it undoubtedly descended in a direct line from Adam and"
    " Eve; and was, in the very earliest times, closely connected with the"
    " agricultural interest. If it should ever be urged by grudging and"
    " malicious persons, that a Chuzzlewit, in any period of the family"
    " history, displayed an overweening amount of family pride, surely the"
    " weakness will be considered not only pardonable but laudable, when the"
    " immense superiority of the house to the rest of mankind, in respect of"
    " this its ancient origin, is taken into account.",
    NULL
};
/* Middle Welsh */
static char *_mabinogion[] = {
    " Gan fod Argraffiad Rhydychen o'r Mabinogion yn rhoi'r testun yn union fel y digwydd yn y llawysgrifau, ac felly yn cyfarfod â gofyn yr ysgolhaig, bernais mai gwell mewn llawlyfr fel hwn oedd golygu peth arno er mwyn helpu'r ieuainc a'r dibrofiad yn yr hen orgraff.",
    NULL
};
/* Swedish */
static char *_PippiGarOmBord[] = {
    "Om någon människa skulle komma resande till den lilla, lilla staden och så kanske ett tu tre råka förirra sig lite för långt bort åt ena utkanten, då skulle den månniskan få se Villa Villekulla. Inte för att huset var så mycket att titta på just, en rätt fallfårdig gammal villa och en rätt vanskött gammal trädgård runt omkring, men främlingen skulle kanske i alla fall stanna och undra vem som bodde där.",
    NULL
};
/* Czech */
static char *_goodsoldier[] = {
    " „Tak nám zabili Ferdinanda,“ řekla posluhovačka panu Švejkovi, který opustiv před léty vojenskou službu, když byl definitivně prohlášen vojenskou lékařskou komisí za blba, živil se prodejem psů, ošklivých nečistokrevných oblud, kterým padělal rodokmeny.",
    " Kromě tohoto zaměstnání byl stižen revmatismem a mazal si právě kolena opodeldokem.",
    NULL
};
/* Lithuanian */
static char *_lithuanian[] = {
    " Kiekviena šventė yra surišta su praeitimi. Nešvenčiamas gimtadienis, kai, kūdikis gimsta. Ir po keliolikos metų gimtinės arba vardinės nėra tiek reikšmingos, kaip sulaukus 50 ar 75 metų. Juo tolimesnis įvykis, tuo šventė darosi svarbesnė ir iškilmingesnė.",
    NULL
};
/* Polish */
static char *_polish[] = {
    " Język prasłowiański miał w zakresie deklinacji (fleksji imiennej) następujące kategorie gramatyczne: liczby, rodzaju i przypadku. Poza tym istniały w nim (w zakresie fleksji rzeczownika) różne «odmiany», czyli typy deklinacyjne. Im dawniej w czasie, tym owe różnice deklinacyjne miały mniejszy związek z semantyką rzeczownika.",
    NULL
};
/* Slovene */
static char *_slovene[] = {
    " Razvoj glasoslovja je diametralno drugačen od razvoja morfologije.",
    " V govoru si besede slede. V vsaki sintagmi dobi beseda svojo vrednost, če je zvezana z besedo, ki je pred njo, in z besedo, ki ji sledi.",
    NULL
};
/* Macedonian */
static char *_macedonian[] = {
    " Македонскиот јазик во балканската јазична средина и наспрема соседните словенски јаеици. 1. Македонскиот јазик се говори во СР Македонија, и надвор од нејзините граници, во оние делови на Македонија што по балканските војни влегоа во составот на Грција и Бугарија.",
    NULL
};
/* Bulgarian */
static char *_bulgarian[] = {
   " ПРЕДМЕТ И ЗАДАЧИ НА ФОНЕТИКАТА Думата фонетика произлиза от гръцката дума фоне, която означава „звук“, „глас“, „тон“.",
    NULL
};
/* Korean Hangul */
static char *_hangulsijo[] = {
    "어버이 살아신 제 섬길 일란 다 하여라",
    "지나간 후면 애닯다 어이 하리",
    "평생에 고쳐 못할 일이 이뿐인가 하노라",
    "- 정철",
    "",
    "이고 진 저 늙은이 짐 벗어 나를 주오",
    "나는 젊었거니 돌이라 무거울까",
    "늙기도 설워라커든 짐을 조차 지실까",
    "- 정철",
    NULL
};
/* Chinese traditional */
/* https://en.wikipedia.org/wiki/Tao_Te_Ching */
static char *_TaoTeChing[] = {
    "道可道非常道，",
    "名可名非常名。",
    NULL
};
/* http://gan.wikipedia.org/wiki/%E5%B0%87%E9%80%B2%E9%85%92 */
static char *_LiBai[] = {
    "將進酒",
    "",
    "君不見 黃河之水天上來 奔流到海不復回",
    "君不見 高堂明鏡悲白髮 朝如青絲暮成雪",
    "人生得意須盡歡 莫使金樽空對月",
    "天生我材必有用 千金散盡還復來",
    "烹羊宰牛且為樂 會須一飲三百杯",
    "岑夫子 丹丘生 將進酒 君莫停",
    "與君歌一曲 請君為我側耳聽",
    "鐘鼓饌玉不足貴 但願長醉不願醒",
    "古來聖賢皆寂寞 惟有飲者留其名",
    "陳王昔時宴平樂 斗酒十千恣讙謔",
    "主人何為言少錢 徑須沽取對君酌",
    "五花馬 千金裘 呼兒將出換美酒 與爾同消萬古愁",
    NULL
};
static char *_LiBaiShort[] = {
    "將進酒",
    "",
    "君不見 黃河之水天上來 奔流到海不復回",
    "君不見 高堂明鏡悲白髮 朝如青絲暮成雪",
    NULL
};
/* Japanese */
/* https://ja.wikipedia.org/wiki/%E6%BA%90%E6%B0%8F%E7%89%A9%E8%AA%9E */
static char *_Genji[] = {
    "源氏物語（紫式部）：いづれの御時にか、女御・更衣あまた さぶらひたまひけるなかに、いとやむごとなき 際にはあらぬが、すぐれてときめきたまふありけり。",
    NULL
};
/* http://www.geocities.jp/sybrma/42souseki.neko.html */
static char *_IAmACat[] = {
    "吾輩は猫である（夏目漱石）：吾輩は猫である",
    NULL
};

/* The following translations of the gospel according to John are all from */
/*  Compendium of the world's languages. 1991 Routledge. by George L. Campbell*/

/* Belorussian */
static char *_belorussianjohn[] = {
    "У пачатку было Слова, і Слова было ў Бога, і Богам было Слова. Яно было ў пачатку ў Бога",
    NULL
};
/* basque */
static char *_basquejohn[] = {
    "Asieran Itza ba-zan, ta Itza Yainkoagan zan, ta Itza Yainko zan.",
    "Asieran Bera Yainkoagan zan.",
    NULL
};
/* danish */
static char *_danishjohn[] = {
    "Begyndelsen var Ordet, og Ordet var hos Gud, og Ordet var Gud.",
    "Dette var i Begyndelsen hos Gud.",
    NULL
};
/* dutch */
static char *_dutchjohn[] = {
    "In den beginne was het Woord en het Woord was bij God en het Woord was God.",
    "Dit was in den beginne bij God.",
    NULL
};
/* finnish */
static char *_finnishjohn[] = {
    "Alussa oli Sana, ja Sana oli Jumalan luona, Sana oli Jumala.",
    "ja hä oli alussa Jumalan luona.",
    NULL
};
/* georgian */
    /* Hmm, the first 0x10e0 might be 0x10dd, 0x301 */
static char *_georgianjohn[] = {
    "პირველითგან იყო სიტყუუ̂ა, და სიტყუუ̂ა იგი იყო ღუ̂თისა თანა, და დიერთი იყო სიტყუუ̂ა იგი.",
    "ესე იყო პირველითგან დიერთი თინი.",
    NULL
};
/* icelandic */
static char *_icelandicjohn[] = {
    "Í upphafi var Orðið og Orðið var hjà Guði, og Orðið var Guði.",
    "Það var í upphafi hjá Guði.",
    NULL
};
/* irish */
static char *_irishjohn[] = {
    "Bhí an Briathar(I) ann i dtús báire agus bhí an Briathar in éineacht le Dia, agus ba Dhia an Briathar.",
    "Bhí sé ann i dtús báire in éineacht le Dia.",
    NULL
};
/* Bokmål norwegian */
static char *_norwegianjohn[] = {
    "I begynnelsen var Ordet, Ordet var hos Gud, og Ordet var Gud.",
    "Han var i begynnelsen hos Gud.",
    "Alt er blitt til ved ham; uten ham er ikke noe blitt til av alt som er til.",
    NULL
};
/* Nynorsk norwegian */
static char *_nnorwegianjohn[] = {
    "I opphavet var Ordet, og Ordet var hjå Gud, og Ordet var Gud.",
    "Han var i opphavet hjå Gud.",
    NULL
};
/* old church slavonic */
static char *_churchjohn[] = {
    "Въ нача́лѣ бѣ̀ сло́во и҆ сло́во бѣ̀ къ бг҃ꙋ, и҆ бг҃ъ бѣ̀ сло́во.",
    "Се́й бѣ̀ и҆сконѝ къ бг҃ꙋ.",
    NULL
};
/* swedish */
static char *_swedishjohn[] = {
    "I begynnelsen var Ordet, och Ordet var hos Gud, och Ordet var Gud.",
    "Han var i begynnelsen hos Gud.",
    NULL
};
/* portuguese */
static char *_portjohn[] = {
    "No Principio era a Palavra, e a Palavra estava junto de Deos, e a Palavra era Deos.",
    "Esta estava no principio junto de Deos.",
    NULL
};
/* cherokee */
static char *_cherokeejohn[] = {
    "ᏗᏓᎴᏂᏯᎬ ᎧᏃᎮᏘ ᎡᎮᎢ, ᎠᎴ ᎾᏯᎩ ᎧᏃᎮᏘ ᎤᏁᎳᏅᎯ ᎢᏧᎳᎭ ᎠᏘᎮᎢ, ᎠᎴ ᎾᏯᎩ ᎧᏃᎮᏘ ᎤᏁᎳᏅᎯ ᎨᏎᎢ.",
    "ᏗᏓᎴᏂᏯᎬ ᎾᏯᎩ ᎤᏁᎳᏅᎯ ᎢᏧᎳᎭ ᎠᏘᎮᎢ",
    NULL
};
/* swahili */
static char *_swahilijohn[] = {
    "Hapo mwanzo kulikuwako Neno, naye Neno alikuwako kwa Mungo, naye Neno alikuwa Mungu, Huyo mwanzo alikuwako kwa Mungu.",
    "Vyote vilvanyika kwa huyo; wala pasipo yeye hakikufanyika cho chote kilichofanyiki.",
    NULL
};
/* thai */	/* I'm sure I've made transcription errors here, I can't figure out what "0xe27, 0xe38, 0xe4d" really is */
static char *_thaijohn[] = {
    "๏ ในทีเดิมนะนพวุํลอโฆเปนอยู่ แลเปนอยู่ดว้ยกันกับ พวุํเฆ้า",
    NULL
};
/* Mayan K'iche' of Guatemala */ /* Prolog to Popol Wuj */ /* Provided by Daniel Johnson */
static char *_mayanPopolWuj[] = {
    "Are u xe' ojer tzij waral, C'i Che' u bi'. Waral xchikatz'ibaj-wi, xchikatiquiba-wi ojer tzij, u ticaribal, u xe'nabal puch ronojel xban pa tinamit C'i Che', ramak C'i Che' winak.",
    NULL
};

/* I've omitted cornish. no interesting letters. no current speakers */

  /* http://www.ethnologue.com/iso639/codes.asp */
enum scripts { sc_latin, sc_greek, sc_cyrillic, sc_georgian, sc_hebrew,
	sc_arabic, sc_hangul, sc_chinesetrad, sc_chinesemod, sc_kanji,
	sc_hiragana, sc_katakana
};
static struct langsamples {
    char **sample;
    char *iso_lang;		/* ISO 639 two character abbreviation */
    enum scripts script;
    uint32 otf_script, lang;
} sample[] = {
    { _simple, "various", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ') },
    { _simplecyrill, "various", sc_cyrillic, CHR('c','y','r','l'), CHR('R','U','S',' ')},
    { _simplehebrew, "he", sc_hebrew, CHR('h','e','b','r'), CHR('I','W','R',' ') },
    { _simplekata, "ja", sc_katakana, CHR('k','a','n','a'), CHR('J','A','N',' ')},
    { _simplehira, "ja", sc_hiragana, CHR('k','a','n','a'), CHR('J','A','N',' ')},
    { _faust, "de", sc_latin, CHR('l','a','t','n'), CHR('D','E','U',' ')},
    { _pheadra, "fr", sc_latin, CHR('l','a','t','n'), CHR('F','R','A',' ')},
    { _antigone, "el", sc_greek, CHR('g','r','e','k'), CHR('P','G','R',' ')},	/* Is this polytonic? */
    { _annakarenena, "ru", sc_cyrillic, CHR('c','y','r','l'), CHR('R','U','S',' ')},
    { _serbcyriljohn, "sr", sc_cyrillic, CHR('c','y','r','l'), CHR('S','R','B',' ')},
    { _debello, "la", sc_latin, CHR('l','a','t','n'), CHR('L','A','T',' ')},
    { _hebrew, "he", sc_hebrew, CHR('h','e','b','r'), CHR('I','W','R',' ') },
    { _arabic, "ar", sc_arabic, CHR('a','r','a','b'), CHR('A','R','A',' ')},
    { _hangulsijo, "ko", sc_hangul, CHR('h','a','n','g'), CHR('K','O','R',' ')},
    { _TaoTeChing, "zh", sc_chinesetrad, CHR('h','a','n','i'), CHR('Z','H','T',' ')},
    { _LiBai, "zh", sc_chinesetrad, CHR('h','a','n','i'), CHR('Z','H','T',' ')},
    { _Genji, "ja", sc_kanji, CHR('h','a','n','i'), CHR('J','A','N',' ')},
    { _IAmACat, "ja", sc_kanji, CHR('h','a','n','i'), CHR('J','A','N',' ')},
    { _donquixote, "es", sc_latin, CHR('l','a','t','n'), CHR('E','S','P',' ')},
    { _inferno, "it", sc_latin, CHR('l','a','t','n'), CHR('I','T','A',' ')},
    { _beorwulf, "enm", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ')},		/* 639-2 name for middle english */
    { _muchado, "eng", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ')},		/* 639-2 name for modern english */
    { _chuzzlewit, "en", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ')},		/* 639-2 name for modern english */
    { _PippiGarOmBord, "sv", sc_latin, CHR('l','a','t','n'), CHR('S','V','E',' ')},
    { _mabinogion, "cy", sc_latin, CHR('l','a','t','n'), CHR('W','E','L',' ')},
    { _goodsoldier, "cs", sc_latin, CHR('l','a','t','n'), CHR('C','S','Y',' ')},
    { _macedonian, "mk", sc_cyrillic, CHR('c','y','r','l'), CHR('M','K','D',' ')},
    { _bulgarian, "bg", sc_cyrillic, CHR('c','y','r','l'), CHR('B','G','R',' ')},
    { _belorussianjohn, "be", sc_cyrillic, CHR('c','y','r','l'), CHR('B','E','L',' ')},
    { _churchjohn, "cu", sc_cyrillic, CHR('c','y','r','l'), CHR('C','S','L',' ')},
    { _lithuanian, "lt", sc_latin, CHR('l','a','t','n'), CHR('L','T','H',' ')},
    { _polish, "pl", sc_latin, CHR('l','a','t','n'), CHR('P','L','K',' ')},
    { _slovene, "sl", sc_latin, CHR('l','a','t','n'), CHR('S','L','V',' ')},
    { _irishjohn, "ga", sc_latin, CHR('l','a','t','n'), CHR('I','R','I',' ')},
    { _basquejohn, "eu", sc_latin, CHR('l','a','t','n'), CHR('E','U','Q',' ')},
    { _portjohn, "pt", sc_latin, CHR('l','a','t','n'), CHR('P','T','G',' ')},
    { _icelandicjohn, "is", sc_latin, CHR('l','a','t','n'), CHR('I','S','L',' ')},
    { _danishjohn, "da", sc_latin, CHR('l','a','t','n'), CHR('D','A','N',' ')},
    { _swedishjohn, "sv", sc_latin, CHR('l','a','t','n'), CHR('S','V','E',' ')},
    { _norwegianjohn, "no", sc_latin, CHR('l','a','t','n'), CHR('N','O','R',' ')},
    { _nnorwegianjohn, "no", sc_latin, CHR('l','a','t','n'), CHR('N','O','R',' ')},
    { _dutchjohn, "nl", sc_latin, CHR('l','a','t','n'), CHR('N','L','D',' ')},
    { _finnishjohn, "fi", sc_latin, CHR('l','a','t','n'), CHR('F','I','N',' ')},
    { _cherokeejohn, "chr", sc_latin, CHR('l','a','t','n'), CHR('C','H','R',' ')},
    { _thaijohn, "th", sc_latin, CHR('l','a','t','n'), CHR('T','H','A',' ')},
    { _georgianjohn, "ka", sc_georgian, CHR('g','e','o','r'), CHR('K','A','T',' ') },
    { _swahilijohn, "sw", sc_latin, CHR('l','a','t','n'), CHR('S','W','K',' ')},
    { _mayanPopolWuj, "QUT", sc_latin, CHR('l','a','t','n'), CHR('Q','U','T',' ')},
    { NULL, NULL, 0, 0, 0 }
};

static void OrderSampleByLang(void) {
    const char *lang = getenv("LANG");
    char langbuf[12], *pt;
    int i,j;
    int simple_pos;

    if ( lang==NULL )
return;

    strncpy(langbuf,lang,10);
    langbuf[10] = '\0';
    for ( j=0; j<3; ++j ) {
	if ( j==1 ) {
	    for ( pt=langbuf; *pt!='\0' && *pt!='.'; ++pt );
	    *pt = '\0';
	} else if ( j==2 ) {
	    for ( pt=langbuf; *pt!='\0' && *pt!='_'; ++pt );
	    *pt = '\0';
	}
	for ( i=0; sample[i].sample!=NULL; ++i )
	    if ( strcmp(sample[i].iso_lang,langbuf)==0 ) {
		struct langsamples temp;
		temp = sample[i];
		sample[i] = sample[2];
		sample[2] = temp;
    goto out;
	    }
    }
    out:
    simple_pos = 0;
    if ( strcmp(langbuf,"sv")==0 )
	simple_pos = 4;
    else if ( strcmp(langbuf,"de")==0 )
	simple_pos = 5;
    else if ( strcmp(langbuf,"fr")==0 )
	simple_pos = 6 + (rand()&1);
    else if ( strcmp(langbuf,"nl")==0 )
	simple_pos = 8;
    else if ( strcmp(langbuf,"pl")==0 )
	simple_pos = 9;
    else if ( strcmp(langbuf,"sl")==0 )
	simple_pos = 10;
    else if ( strcmp(langbuf,"cs")==0 )
	simple_pos = 11;
    else
	simple_pos = rand()&3;
    _simple[0] = _simplelatnchoices[simple_pos];
    sample[0].lang = _simplelatnlangs[simple_pos];

    for ( j=0; _simplecyrillchoices[j]!=NULL; ++j );
    simple_pos = rand()%(j+1);
    _simplecyrill[0] = _simplecyrillchoices[simple_pos];
    sample[1].lang = _simplecyrilliclangs[simple_pos];
}

static int AllChars( SplineFont *sf, const char *str) {
    int i, ch;
    SplineChar *sc;
    struct altuni *alt;

    if ( sf->subfontcnt==0 ) {
	while ( (ch = utf8_ildb(&str))!='\0' ) {
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
		if ( sc->unicodeenc == ch )
	    break;
		for ( alt=sc->altuni ; alt!=NULL ; alt=alt->next )
		    if ( alt->vs==-1 && alt->unienc==ch )
		break;
		if ( alt!=NULL )
	    break;
	    }
	    if ( i==sf->glyphcnt || !SCWorthOutputting(sf->glyphs[i]) )
return( false );
	}
    } else {
	int max = 0, j;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->glyphcnt>max ) max = sf->subfonts[i]->glyphcnt;
	while ( (ch = utf8_ildb(&str))!='\0' ) {
	    for ( i=0; i<max; ++i ) {
		for ( j=0; j<sf->subfontcnt; ++j )
		    if ( i<sf->subfonts[j]->glyphcnt && sf->subfonts[j]->glyphs[i]!=NULL )
		break;
		if ( j!=sf->subfontcnt )
		    if ( sf->subfonts[j]->glyphs[i]->unicodeenc == ch )
	    break;
	    }
	    if ( i==max || !SCWorthOutputting(sf->subfonts[j]->glyphs[i]))
return( false );
	}
    }
return( true );
}

static int ScriptInList(uint32 script, uint32 *scripts, int scnt) {
    int s;

    for ( s=0; s<scnt; ++s )
	if ( script == scripts[s] )
return( true );

return( false );
}

unichar_t *PrtBuildDef( SplineFont *sf, void *tf,
	void (*langsyscallback)(void *tf, int end, uint32 script, uint32 lang) ) {
    int i, j, gotem, len, any=0, foundsomething=0;
    unichar_t *ret=NULL;
    char **cur;
    uint32 scriptsdone[100], scriptsthere[100], langs[100];
    char *randoms[100];
    char buffer[220], *pt;
    int scnt,s,therecnt, rcnt;

    OrderSampleByLang();
    therecnt = SF2Scripts(sf,scriptsthere);

    scnt = 0;

    while ( 1 ) {
	len = any = 0;
	for ( i=0; sample[i].sample!=NULL; ++i ) {
	    gotem = true;
	    cur = sample[i].sample;
	    for ( j=0; cur[j]!=NULL && gotem; ++j )
		gotem = AllChars(sf,cur[j]);
	    if ( !gotem && sample[i].sample==_LiBai ) {
		cur = _LiBaiShort;
		gotem = true;
		for ( j=0; cur[j]!=NULL && gotem; ++j )
		    gotem = AllChars(sf,cur[j]);
	    }
	    if ( gotem ) {
		for ( s=0; s<scnt; ++s )
		    if ( scriptsdone[s]==sample[i].otf_script )
		break;
		if ( s==scnt )
		    scriptsdone[scnt++] = sample[i].otf_script;

		foundsomething = true;
		++any;
		for ( j=0; cur[j]!=NULL; ++j ) {
		    if ( ret )
			utf82u_strcpy(ret+len,cur[j]);
		    len += g_utf8_strlen( cur[j], -1 );
		    if ( ret )
			ret[len] = '\n';
		    ++len;
		}
		if ( ret )
		    ret[len] = '\n';
		++len;
		if ( ret && langsyscallback!=NULL )
		    (langsyscallback)(tf,len,sample[i].otf_script,sample[i].lang);
	    }
	}

	rcnt = 0;
	for ( s=0; s<therecnt; ++s ) if ( !ScriptInList(scriptsthere[s],scriptsdone,scnt)) {
	    if ( ret ) {
		if ( randoms[rcnt]!='\0' ) {
		    utf82u_strcpy(ret+len,randoms[rcnt]);
		    len += u_strlen(ret+len);
		    ret[len++] = '\n';
		    ret[len] = '\0';
		    if ( langsyscallback!=NULL )
			(langsyscallback)(tf,len,scriptsthere[s],langs[rcnt]);
		}
		free(randoms[rcnt]);
	    } else {
		randoms[rcnt] = RandomParaFromScript(scriptsthere[s],&langs[rcnt],sf);
		for ( pt=randoms[rcnt]; *pt==' '; ++pt );
		if ( *pt=='\0' )
		    *randoms[rcnt] = '\0';
		else {
		    len += g_utf8_strlen( randoms[rcnt], -1 )+2;
		    foundsomething = true;
		}
	    }
	    ++rcnt;
	}

	if ( !foundsomething ) {
	    /* For example, Apostolos's Phaistos Disk font. There is no OT script*/
	    /*  code assigned for those unicode points */
	    int gid;
	    SplineChar *sc;

	    pt = buffer;
	    for ( gid=i=0; gid<sf->glyphcnt && pt<buffer+sizeof(buffer)-4 && i<50; ++gid ) {
		if ( (sc=sf->glyphs[gid])!=NULL && sc->unicodeenc!=-1 ) {
		    pt = utf8_idpb(pt,sc->unicodeenc,0);
		    ++i;
		}
	    }
	    *pt = '\0';
	    if ( i>0 ) {
		if ( ret ) {
		    utf82u_strcpy(ret+len,buffer);
		    len += u_strlen(ret+len);
		    ret[len++] = '\n';
		    ret[len] = '\0';
		    if ( langsyscallback!=NULL )
			(langsyscallback)(tf,len,DEFAULT_SCRIPT,DEFAULT_LANG);
		} else
		    len += g_utf8_strlen( buffer, -1 )+1;
	    }
	}

	if ( ret ) {
	    ret[len]='\0';
return( ret );
	}
	if ( len == 0 ) {
	    /* Er... We didn't find anything?! */
return( calloc(1,sizeof(unichar_t)));
	}
	ret = malloc((len+1)*sizeof(unichar_t));
    }
}

/* ************************************************************************** */
/* ******************************* Print Code ******************************* */
/* ************************************************************************** */

static void QueueIt(PI *pi) {
    #if !defined(__MINGW32__)
    int pid;
    int stdinno, i, status;
    char *argv[40], buf[10];

    if ( (pid=fork())==0 ) {
	stdinno = fileno(stdin);
	close(stdinno);
	dup2(fileno(pi->out),stdinno);
	i = 0;
	if ( pi->printtype == pt_ghostview ) {
	    if ( !use_gv )
		argv[i++] = "ghostview";
	    else {
		argv[i++] = "gv";
		argv[i++] = "-antialias";
	    }
	    argv[i++] = "-";		/* read from stdin */
	} else if ( pi->printtype == pt_lp ) {
	    argv[i++] = "lp";
	    if ( pi->printer!=NULL ) {
		argv[i++] = "-d";
		argv[i++] = pi->printer;
	    }
	    if ( pi->copies>1 ) {
		argv[i++] = "-n";
		sprintf(buf,"%d", pi->copies );
		argv[i++] = buf;
	    }
	} else if ( pi->printtype == pt_lpr ) {
	    argv[i++] = "lpr";
	    if ( pi->printer!=NULL ) {
		argv[i++] = "-P";
		argv[i++] = pi->printer;
	    }
	    if ( pi->copies>1 ) {
		sprintf(buf,"-#%d", pi->copies );
		argv[i++] = buf;
	    }
	} else {
	    char *temp, *pt, *start;
	    int quoted=0;
	    /* This is in the child. We're going to do an exec soon */
	    /*  We don't need to free things here */
	    temp = copy(printcommand);
	    for ( pt=start=temp; *pt ; ++pt ) {
		if ( *pt==quoted ) {
		    quoted = 0;
		    *pt = '\0';
		} else if ( quoted )
		    /* Do nothing */;
		else if ( *pt=='"' || *pt=='\'' ) {
		    start = pt+1;
		    quoted = *pt;
		} else if ( *pt==' ' )
		    *pt = '\0';
		if ( *pt=='\0' ) {
		    if ( i<sizeof(argv)/sizeof(argv[0])-1 )
			argv[i++] = start;
		    while ( pt[1]==' ' ) ++pt;
		    start = pt+1;
		}
	    }
	    if ( pt>start && i<sizeof(argv)/sizeof(argv[0])-1 )
		argv[i++] = start;
	}
	argv[i] = NULL;
 /*for ( i=0; argv[i]!=NULL; ++i ) printf( "%s ", argv[i]); printf("\n" );*/
	execvp(argv[0],argv);
	if ( pi->printtype == pt_ghostview ) {
	    argv[0] = "gv";
	    execvp(argv[0],argv);
	}
	fprintf( stderr, "Failed to exec print job\n" );
	/*IError("Failed to exec print job");*/ /* X Server gets confused by talking to the child */
	_exit(1);
    } else if ( pid==-1 )
	IError("Failed to fork print job");
    else if ( pi->printtype != pt_ghostview ) {
	waitpid(pid,&status,0);
	if ( !WIFEXITED(status) )
	    IError("Failed to queue print job");
    } else {
	sleep(1);
	if ( waitpid(pid,&status,WNOHANG)>0 ) {
	    if ( !WIFEXITED(status) )
		IError("Failed to run ghostview");
	}
    }
    waitpid(-1,&status,WNOHANG);	/* Clean up any zombie ghostviews */
    #endif
}

void DoPrinting(PI *pi,char *filename) {
    int sfmax=1;

    if ( pi->pt==pt_fontsample ) {
	struct sfmaps *sfmap;
	for ( sfmap=pi->sample->sfmaps, sfmax=0; sfmap!=NULL; sfmap=sfmap->next, ++sfmax );
	if ( sfmax==0 ) sfmax=1;
    }
    pi->sfmax = sfmax;
    pi->sfbits = calloc(sfmax,sizeof(struct sfbits));
    pi->sfcnt = 0;

    if ( pi->pt==pt_fontdisplay )
	PIFontDisplay(pi);
    else if ( pi->pt==pt_fontsample )
	PIFontSample(pi);
    else if ( pi->pt==pt_multisize )
	PIMultiSize(pi);
    else
	PIChars(pi);
    rewind(pi->out);
    if ( ferror(pi->out) )
	ff_post_error(_("Print Failed"),_("Failed to generate postscript in file %s"),
		filename==NULL?"temporary":filename );
    if ( pi->printtype!=pt_file && pi->printtype!=pt_pdf )
	QueueIt(pi);
    if ( fclose(pi->out)!=0 )
	ff_post_error(_("Print Failed"),_("Failed to generate postscript in file %s"),
		filename==NULL?"temporary":filename );
    free(pi->sfbits);
}

/* ************************************************************************** */
/* ******************************* Init Code ******************************** */
/* ************************************************************************** */

static void PIGetPrinterDefs(PI *pi) {
    pi->pagewidth = pagewidth;
    pi->pageheight = pageheight;
    pi->printtype = printtype;
    pi->printer = copy(printlazyprinter);
    pi->copies = 1;
    if ( pi->pagewidth!=0 && pi->pageheight!=0 )
	pi->hadsize = true;
    else {
	pi->pagewidth = 595;
	pi->pageheight = 792;
	/* numbers chosen to fit either A4 or US-Letter */
	pi->hadsize = false;	/* But we don't want to do a setpagedevice on this because then some printers will wait until fed this odd page size */
    }
}

void PI_Init(PI *pi,FontViewBase *fv,SplineChar *sc) {
    int di = fv!=NULL?0:sc!=NULL?1:2;

    memset(pi,'\0',sizeof(*pi));
    pi->fv = fv;
    pi->sc = sc;
    if ( fv!=NULL ) {
	pi->mainsf = fv->sf;
	pi->mainmap = fv->map;
    } else if ( sc!=NULL ) {
	pi->mainsf = sc->parent;
	pi->mainmap = pi->mainsf->fv->map;
    }
    if ( pi->mainsf->cidmaster )
	pi->mainsf = pi->mainsf->cidmaster;
    PIGetPrinterDefs(pi);
    pi->pointsize = pdefs[di].pointsize;
    if ( pi->pointsize==0 )
	pi->pointsize = pi->mainsf->subfontcnt!=0?18:20;		/* 18 fits 20 across, 20 fits 16 */
}


/* ************************************************************************** */
/* ******************************** Scripting ******************************* */
/* ************************************************************************** */

static unichar_t *FileToUString(char *filename,int max) {
    FILE *file;
    int ch, ch2;
    int format=0;
    unichar_t *space, *upt, *end;

    file = fopen( filename,"rb" );
    if ( file==NULL )
return( NULL );
    ch = getc(file); ch2 = getc(file);
    if ( ch==0xfe && ch2==0xff )
	format = 1;		/* normal ucs2 */
    else if ( ch==0xff && ch2==0xfe )
	format = 2;		/* byte-swapped ucs2 */
    else
	rewind(file);
    space = upt = malloc((max+1)*sizeof(unichar_t));
    end = space+max;
    if ( format!=0 ) {
	while ( upt<end ) {
	    ch = getc(file); ch2 = getc(file);
	    if ( ch2==EOF )
	break;
	    if ( format==1 )
		*upt ++ = (ch<<8)|ch2;
	    else
		*upt ++ = (ch2<<8)|ch;
	}
    } else {
	char buffer[400];
	while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	    def2u_strncpy(upt,buffer,end-upt);
	    upt += u_strlen(upt);
	}
    }
    *upt = '\0';
    fclose(file);
return( space );
}

void ScriptPrint(FontViewBase *fv,int type,int32 *pointsizes,char *samplefile,
	unichar_t *sample, char *outputfile) {
    PI pi;
    char buf[100];
    LayoutInfo *li;
    unichar_t temp[2];

    PI_Init(&pi,fv,NULL);
    if ( pointsizes!=NULL ) {
	pi.pointsizes = pointsizes;
	pi.pointsize = pointsizes[0];
    }
    pi.pt = type;
    if ( type==pt_fontsample ) {
	int width = (pi.pagewidth-1*72)*printdpi/72;
	li = calloc(1,sizeof(LayoutInfo));
	temp[0] = 0;
	li->wrap = true;
	li->dpi = printdpi;
	li->ps = -1;
	li->text = u_copy(temp);
	SFMapOfSF(li,fv->sf);
	LI_SetFontData(li,0,-1, fv->sf, fv->active_layer,sftf_otf,pi.pointsize,true,width);

	if ( samplefile!=NULL && *samplefile!='\0' )
	    sample = FileToUString(samplefile,65536);
	if ( sample==NULL )
	    sample = PrtBuildDef(pi.mainsf,li,(void (*)(void *, int, uint32, uint32))LayoutInfoInitLangSys);
	else
	    LayoutInfoInitLangSys(li,u_strlen(sample),DEFAULT_SCRIPT,DEFAULT_LANG);
	LayoutInfoSetTitle(li, sample, width);
	pi.sample = li;
	free(sample);
    }
    if ( pi.printtype==pt_file || pi.printtype==pt_pdf ) {
	if ( outputfile==NULL ) {
	    sprintf(buf,"pr-%.90s.%s", pi.mainsf->fontname,
		    pi.printtype==pt_file?"ps":"pdf" );
	    outputfile = buf;
	}
	pi.out = fopen(outputfile,"wb");
	if ( pi.out==NULL ) {
	    ff_post_error(_("Print Failed"),_("Failed to open file %s for output"), outputfile);
return;
	}
    } else {
	outputfile = NULL;
	pi.out = tmpfile();
	if ( pi.out==NULL ) {
	    ff_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return;
	}
    }

    DoPrinting(&pi,outputfile);

    if ( pi.pt==pt_fontsample ) {
	LayoutInfo_Destroy(pi.sample);
	free(pi.sample);
    }
}
