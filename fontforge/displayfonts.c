/* Copyright (C) 2000-2007 by George Williams */
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
#include "pfaeditui.h"
#include "sftextfieldP.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ustring.h>
#include "utype.h"
#include <sys/types.h>
#include <sys/wait.h>
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# include <gkeysym.h>
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

enum { pt_lp, pt_lpr, pt_ghostview, pt_file, pt_other, pt_pdf, pt_unknown=-1 };
int pagewidth = 0, pageheight=0;	/* In points */
char *printlazyprinter=NULL;
char *printcommand=NULL;
int printtype = pt_unknown;
static int use_gv;
static const int printdpi = 600;

typedef struct printinfo {
    FontView *fv;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    MetricsView *mv;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SplineChar *sc;
    SplineFont *mainsf;
    EncMap *mainmap;
    enum printtype { pt_fontdisplay, pt_chars, pt_multisize, pt_fontsample } pt;
    int pointsize;
    int32 *pointsizes;
    int extrahspace, extravspace;
    FILE *out;
    unsigned int showvm: 1;
    unsigned int overflow: 1;
    unsigned int done: 1;
    unsigned int hadsize: 1;
    int ypos;
    int max;		/* max chars per line */
    int chline;		/* High order bits of characters we're outputting */
    int page;
    int lastbase;
    real xoff, yoff, scale;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    GWindow gw;
    GWindow setup;
    GTimer *sizechanged;
    GTimer *dpichanged;
    GTextInfo *scriptlangs;
#else
    void *gw, *setup, *sizechanged, *scriptlang;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    char *printer;
    int copies;
    int pagewidth, pageheight, printtype;
  /* data for pdf files */
    int *object_offsets;
    int *page_objects;
    int next_object, max_object;
    int next_page, max_page;
    /* In most print styles sfcnt==1 and we only print one font, but with */
    /*  sample text there may be many logical fonts. And each one may need to */
    /*  be represented by many actual fonts to encode all our glyphs */
    int sfcnt, sfmax, sfid;
    struct sfbits {
	/* If it's a CID font we'll only have one. Otherwise we might have */
	/*  several different encodings to get all the glyphs we need. Each */
	/*  one counts as a font */
	SplineFont *sf;
	EncMap *map;
	char psfontname[300];
	int *our_font_objs;
	int next_font, max_font;
	int *fonts;	/* An array of sf->charcnt/256 entries indicating */
			/* the font number of encodings on that page of   */
			/* the font. -1 => not mapped (no encodings) */
	FILE *fontfile;
	int cidcnt;
	unsigned int twobyte: 1;
	unsigned int istype42cid: 1;
	unsigned int iscid: 1;
	unsigned int wastwobyte: 1;
	unsigned int isunicode: 1;
	unsigned int isunicodefull: 1;
	struct sfmaps *sfmap;
    } *sfbits;
    long start_cur_page;
    int lastfont, intext;
    SFTextArea *sample;
    int wassfid, wasfn, wasps;
} PI, DI;

static struct printdefaults {
    Encoding *last_cs;
    enum printtype pt;
    int pointsize;
    unichar_t *text;
} pdefs[] = { { &custom, pt_fontdisplay }, { &custom, pt_chars }, { &custom, pt_fontdisplay }};
/* defaults for print from fontview, charview, metricsview */

/* ************************************************************************** */
/* ***************************** Printing Stuff ***************************** */
/* ************************************************************************** */

static void pdf_addobject(PI *pi) {
    if ( pi->next_object==0 ) {
	pi->max_object = 100;
	pi->object_offsets = galloc(pi->max_object*sizeof(int));
	pi->object_offsets[pi->next_object++] = 0;	/* Object 0 is magic */
    } else if ( pi->next_object>=pi->max_object ) {
	pi->max_object += 100;
	pi->object_offsets = grealloc(pi->object_offsets,pi->max_object*sizeof(int));
    }
    pi->object_offsets[pi->next_object] = ftell(pi->out);
    fprintf( pi->out, "%d 0 obj\n", pi->next_object++ );
}

static void pdf_addpage(PI *pi) {
    if ( pi->next_page==0 ) {
	pi->max_page = 100;
	pi->page_objects = galloc(pi->max_page*sizeof(int));
    } else if ( pi->next_page>=pi->max_page ) {
	pi->max_page += 100;
	pi->page_objects = grealloc(pi->page_objects,pi->max_page*sizeof(int));
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

    forever {
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
    if ( base!=0 )
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
    if ( base!=0 ) {
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

    sfbit->our_font_objs = galloc((sfbit->map->enccount/256+1)*sizeof(int *));
    sfbit->fonts = galloc((sfbit->map->enccount/256+1)*sizeof(int *));
    for ( i=0; i<sfbit->map->enccount; i += 256 ) {
	sfbit->fonts[i/256] = -1;
	dump_pfb_encoding(pi,sfid,i,fd_obj);
    }
    sfbit->twobyte = false;
}

static int pdf_charproc(PI *pi, SplineChar *sc) {
    int ret = pi->next_object;
#ifdef FONTFORGE_CONFIG_TYPE3
    long streamstart, streamlength;
    int i;

    pdf_addobject(pi);
    fprintf( pi->out, "<< /Length %d 0 R >>\n", pi->next_object );
    fprintf( pi->out, "stream\n" );
    streamstart = ftell(pi->out);

    /* In addition to drawing the glyph, we must provide some metrics */
    for ( i=ly_fore; i<sc->layer_cnt; ++i )
	if ( (sc->layers[i].dofill && sc->layers[i].fill_brush.col!=COLOR_INHERITED) ||
		(sc->layers[i].dostroke && sc->layers[i].stroke_pen.brush.col!=COLOR_INHERITED))
    break;
    if ( i==sc->layer_cnt ) {
	/* We never set the color, use d1 to specify metrics */
	DBounds b;
	SplineCharFindBounds(sc,&b);
	fprintf( pi->out, "%d 0 %g %g %g %g d1\n",
		sc->width,
		b.minx, b.miny, b.maxx, b.maxy );
    } else
	fprintf( pi->out, "%d 0 d0\n", sc->width );

    SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,true);

    streamlength = ftell(pi->out)-streamstart;
    fprintf( pi->out, "\nendstream\n" );
    fprintf( pi->out, "endobj\n" );

    pdf_addobject(pi);
    fprintf( pi->out, " %ld\n", streamlength );
    fprintf( pi->out, "endobj\n\n" );
#else
    IError("This should never get called");
#endif
return( ret );
}

static void dump_pdf3_encoding(PI *pi,int sfid, int base,DBounds *bb, int notdefproc) {
    int i, first=-1, last, gid;
    int charprocs[256];
    struct sfbits *sfbit = &pi->sfbits[sfid];
    SplineFont *sf = sfbit->sf;
    EncMap *map = sfbit->map;

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
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );

    /* Widths array */
    pdf_addobject(pi);
    fprintf( pi->out, "  [\n" );
    for ( i=base+first; i<base+last; ++i )
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
    for ( i=base+first; i<base+last; ++i )
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
    for ( i=base+first; i<base+last; ++i )
	if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    fprintf( pi->out, "\t/%s %d 0 R\n", sf->glyphs[gid]->name, charprocs[i-base] );
    fprintf( pi->out, "  >>\n" );
    fprintf( pi->out, "endobj\n" );
}

static void pdf_gen_type3(PI *pi,int sfid) {
    int i, notdefproc;
    DBounds bb;
    SplineChar sc;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif
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
#ifdef FONTFORGE_CONFIG_TYPE3
	memset(layers,0,sizeof(layers));
	sc.layers = layers;
#endif
	notdefproc = pdf_charproc(pi, &sc);
    }

    SplineFontFindBounds(sf,&bb);
    sfbit->our_font_objs = galloc((map->enccount/256+1)*sizeof(int *));
    sfbit->fonts = galloc((map->enccount/256+1)*sizeof(int *));
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

    widths = galloc(cidmax*sizeof(uint16));

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
    sfbit->our_font_objs = galloc(sizeof(int));
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
    fprintf( pi->out, "  /CreationDate (D:%4d%02d%02d%02d%02d",
	    tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min );
    if ( timezone==0 )
	fprintf( pi->out, "Z)\n" );
    else 
	fprintf( pi->out, "%+02d')\n", (int) timezone/3600 );	/* doesn't handle half-hour zones */
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
    /* In case we have a type3 font, include the images */
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
    free( pi->sfbits );
    free(pi->object_offsets);
    free(pi->page_objects);
}

static void DumpIdentCMap(PI *pi, int sfid) {
    struct sfbits *sfbit = &pi->sfbits[pi->sfid];
    SplineFont *sf = sfbit->sf;
    int i, j, k, max;

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
    fprintf( pi->out, "%%%%Title: (Noop %s %s %d)\n", sf->cidregistry, sf->ordering, sf->supplement );
    fprintf( pi->out, "%%%%EndComments\n" );

    fprintf( pi->out, "/CIDInit /ProcSet findresource begin\n" );

    fprintf( pi->out, "12 dict begin\n" );

    fprintf( pi->out, "begincmap\n" );

    fprintf( pi->out, "/CIDSystemInfo 3 dict dup begin\n" );
    fprintf( pi->out, "  /Registry (%s) def\n", sf->cidregistry );
    fprintf( pi->out, "  /Ordering (%s) def\n", sf->ordering );
    fprintf( pi->out, "  /Supplement %d def\n", sf->supplement );
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
    fprintf( pi->out, "%%%%BoundingBox: 40 20 %d %d\n", pi->pagewidth-30, pi->pageheight-20 );
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
	    if ( pi->printtype!=pt_fontsample ) {
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
			if ( pi->printtype!=pt_fontsample )
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
			if ( pi->printtype!=pt_fontsample )
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
    sfbit->istype42cid = sf->order2;
    sfbit->iscid = sf->subfontcnt!=0 || sfbit->istype42cid;
    if ( pi->pointsize==0 )
	pi->pointsize = sfbit->iscid && !sfbit->istype42cid?18:20;		/* 18 fits 20 across, 20 fits 16 */

    sfbit->fontfile = tmpfile();
    if ( sfbit->fontfile==NULL ) {
	gwwv_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return(false);
    }
    if ( pi->sfid==0 )
	gwwv_progress_start_indicator(10,_("Printing Font"),_("Printing Font"),
		_("Generating Postscript Font"),sf->glyphcnt,1);
    else
	gwwv_progress_reset();
    gwwv_progress_enable_stop(false);
    if ( pi->printtype==pt_pdf && sf->multilayer ) {
	/* These need to be done in line as pdf objects */
	/* I leave fontfile open as a flag, even though we don't use it */
    } else if ( pi->printtype==pt_pdf && sfbit->iscid ) {
	if ( !_WriteTTFFont(sfbit->fontfile,sf,
		sfbit->istype42cid?ff_type42cid:ff_cffcid,NULL,bf_none,
		ps_flag_nocffsugar,map))
	    error = true;
    } else if ( !_WritePSFont(sfbit->fontfile,sf,
		pi->printtype==pt_pdf ? ff_pfb :
		sf->multilayer?ff_ptype3:
		is_mm?ff_mma:
		sfbit->istype42cid?ff_type42cid:
		sfbit->iscid?ff_cid:
		sfbit->twobyte?ff_ptype0:
		ff_pfa,ps_flag_identitycidmap,map,NULL))
	error = true;

    gwwv_progress_end_indicator();

    if ( error ) {
	gwwv_post_error(_("Failed to generate postscript font"),_("Failed to generate postscript font") );
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
	SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,false);
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
	SC_PSDump((void (*)(int,void *)) fputc,pi->out,sc,true,true);
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    else {
	for ( i=0; i<pi->mv->glyphcnt; ++i )
	    if ( SCWorthOutputting(pi->mv->glyphs[i].sc))
		SCPrintPage(pi,pi->mv->glyphs[i].sc);
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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
	fprintf( pi->out, "BT\n  /FTB 12 Tf\n  80 %d Td\n", pageheight-84 );
	if ( pi->pt==pt_fontsample )
	    fprintf(pi->out,"(Sample Text from %s) Tj\n", sfbit->sf->fullname );
	else {
	    fprintf(pi->out,"(Sample Sizes of %s) Tj\n", sfbit->sf->fullname );
	    fprintf(pi->out,"ET\nq 1 0 0 1 40 %d cm\n", pi->pageheight-34-
		    pi->pointsize*sfbit->sf->ascent/(sfbit->sf->ascent+sfbit->sf->descent) );
	}
	pi->lastfont = -1;
    } else {
	fprintf(pi->out,"%%%%Page: %d %d\n", pi->page, pi->page );
	fprintf(pi->out,"%%%%PageResources: font %s\n", sfbit->sf->fontname );
	fprintf(pi->out,"save mark\n" );
	fprintf(pi->out,"Times-Bold__12 setfont\n" );
	if ( pi->pt==pt_fontsample )
	    fprintf(pi->out,"(Sample Text from %s) 80 %d n_show\n", sfbit->sf->fullname, pageheight-84 );
	else {
	    fprintf(pi->out,"(Sample Sizes of %s) 80 %d n_show\n", sfbit->sf->fullname, pageheight-84 );
	    fprintf(pi->out,"40 %d translate\n", pi->pageheight-34-
		    pi->pointsize*sfbit->sf->ascent/(sfbit->sf->ascent+sfbit->sf->descent) );
	}
	if ( sfbit->iscid )
	    fprintf(pi->out,"%s setfont\n", sfbit->psfontname );
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
	    fprintf(pi->out,"/F%d-%d %d Tf\n  <", sfid, fn, fd->pointsize);
	    pi->wassfid = sfid; pi->wasfn = fn; pi->wasps = fd->pointsize;
	} else
	    putc('<',pi->out);
	fprintf(pi->out, "%g %g Td ",
		(x+osc->vr.xoff)*pi->scale,
		(baseline+osc->vr.yoff)*pi->scale );
	outputchar(pi,sfid,sc);
	fprintf( pi->out, "> Tj\n" );
    } else {
	int fn = 0;
	fprintf(pi->out, "%g %g moveto ",
		(x+osc->vr.xoff)*pi->scale,
		(baseline+osc->vr.yoff)*pi->scale );
	if ( (sfbit->twobyte && enc>0xffff) || (!sfbit->twobyte && enc>0xff) )
	    fn = enc>>8;
	if ( pi->wassfid!=sfid || fn!=pi->wasfn || fd->pointsize!=pi->wasps ) {
	    if ( (sfbit->twobyte && enc>0xffff) || (!sfbit->twobyte && enc>0xff) )
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
    SFTextArea *st = pi->sample;
    struct opentype_str **line;
    int i,j;

    pi->pointsize = 12;		/* no longer meaningful */
    pi->extravspace = pi->pointsize/6;
    pi->scale = 72.0/st->dpi;
    pi->lastfont = -1; pi->intext = false;
    pi->wassfid = -1;

    for ( cnt=0, sfmaps = st->sfmaps; sfmaps!=NULL; sfmaps = sfmaps->next, ++cnt ) {
	pi->sfid = cnt;
	sfmaps->sfbit_id = cnt;
	pi->sfbits[cnt].sfmap = sfmaps;
	if ( !PIDownloadFont(pi,sfmaps->sf,sfmaps->map))
return;
    }
    dump_prologue(pi);

    samplestartpage(pi);
    y = top = rint((pageheight - 96)/pi->scale);	/* In dpi units */
    bottom = rint(36/pi->scale);			/* multiply by scale to get ps points */

    for ( i=0; i<st->lcnt; ++i ) {
	if ( y - st->lineheights[i].fh < bottom ) {
	    samplestartpage(pi);
	    y = top;
	}
	x = rint(36/pi->scale);
	baseline = y - st->lineheights[i].as;
	y -= st->lineheights[i].fh;
	line = st->lines[i];
	for ( j=0; line[j]!=NULL; ++j ) {
	    outputotchar(pi,line[j],x,baseline);
	    x += line[j]->advance_width + line[j]->vr.h_adv_off;
	}
    }
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    else {
	for ( i=0; i<pi->mv->glyphcnt; ++i )
	    if ( SCWorthOutputting(pi->mv->glyphs[i].sc))
		SCPrintSizes(pi,pi->mv->glyphs[i].sc);
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    dump_trailer(pi);
}

/* ************************************************************************** */
/* *********************** Code for Page Setup dialog *********************** */
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#define CID_lp		1001
#define CID_lpr		1002
#define	CID_ghostview	1003
#define CID_File	1004
#define CID_Other	1005
#define CID_OtherCmd	1006
#define	CID_Pagesize	1007
#define CID_CopiesLab	1008
#define CID_Copies	1009
#define CID_PrinterLab	1010
#define CID_Printer	1011
#define CID_PDFFile	1012

static void PG_SetEnabled(PI *pi) {
    int enable;

    enable = GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lp)) ||
	    GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lpr));

    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_CopiesLab),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_Copies),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_PrinterLab),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_Printer),enable);

    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_OtherCmd),
	    GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other)));
}

static int PG_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret;
	int err=false;
	int copies, pgwidth, pgheight;

	copies = GetInt8(pi->setup,CID_Copies,_("_Copies:"),&err);
	if ( err )
return(true);

	if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other)) &&
		*_GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_OtherCmd))=='\0' ) {
	    gwwv_post_error(_("No Command Specified"),_("No Command Specified"));
return(true);
	}

	ret = _GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_Pagesize));
	if ( uc_strstr(ret,"Letter")!=NULL ) {
	    pgwidth = 612; pgheight = 792;
	} else if ( uc_strstr(ret,"Legal")!=NULL ) {
	    pgwidth = 612; pgheight = 1008;
	} else if ( uc_strstr(ret,"A4")!=NULL ) {
	    pgwidth = 595; pgheight = 842;
	} else if ( uc_strstr(ret,"A3")!=NULL ) {
	    pgwidth = 842; pgheight = 1191;
	} else if ( uc_strstr(ret,"B4")!=NULL ) {
	    pgwidth = 708; pgheight = 1000;
	} else if ( uc_strstr(ret,"B5")!=NULL ) {
	    pgwidth = 516; pgheight = 728;
	} else {
	    char *cret = cu_copy(ret), *pt;
	    float pw,ph, scale;
	    if ( sscanf(cret,"%gx%g",&pw,&ph)!=2 ) {
		IError("Bad Pagesize must be a known name or <num>x<num><units>\nWhere <units> is one of pt (points), mm, cm, in" );
return( true );
	    }
	    pt = cret+strlen(cret)-1;
	    while ( isspace(*pt) ) --pt;
	    if ( strncmp(pt-2,"in",2)==0)
		scale = 72;
	    else if ( strncmp(pt-2,"cm",2)==0 )
		scale = 72/2.54;
	    else if ( strncmp(pt-2,"mm",2)==0 )
		scale = 72/25.4;
	    else if ( strncmp(pt-2,"pt",2)==0 )
		scale = 1;
	    else {
		IError("Bad Pagesize units are unknown\nMust be one of pt (points), mm, cm, in" );
return( true );
	    }
	    pgwidth = pw*scale; pgheight = ph*scale;
	    free(cret);
	}

	ret = _GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_Printer));
	if ( uc_strcmp(ret,"<default>")==0 || *ret=='\0' )
	    ret = NULL;
	pi->printer = cu_copy(ret);
	pi->pagewidth = pgwidth; pi->pageheight = pgheight;
	pi->copies = copies;

	if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lp)))
	    pi->printtype = pt_lp;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lpr)))
	    pi->printtype = pt_lpr;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_ghostview)))
	    pi->printtype = pt_ghostview;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_PDFFile)))
	    pi->printtype = pt_pdf;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other))) {
	    pi->printtype = pt_other;
	    printcommand = cu_copy(_GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_OtherCmd)));
	} else
	    pi->printtype = pt_file;

	printtype = pi->printtype;
	free(printlazyprinter); printlazyprinter = copy(pi->printer);
	pagewidth = pgwidth; pageheight = pgheight;

	pi->done = true;
	SavePrefs();
    }
return( true );
}

static int PG_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	pi->done = true;
    }
return( true );
}

static int PG_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PG_SetEnabled(pi);
    }
return( true );
}

static int pg_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI *pi = GDrawGetUserData(gw);
	pi->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("print.html");
return( true );
	}
return( false );
    }
return( true );
}

static GTextInfo *PrinterList() {
    char line[400];
    FILE *printcap;
    GTextInfo *tis=NULL;
    int cnt;
    char *bpt, *cpt;

    printcap = fopen("/etc/printcap","r");
    if ( printcap==NULL ) {
	tis = gcalloc(2,sizeof(GTextInfo));
	tis[0].text = uc_copy("<default>");
return( tis );
    }

    while ( 1 ) {
	cnt=1;		/* leave room for default printer */
	while ( fgets(line,sizeof(line),printcap)!=NULL ) {
	    if ( !isspace(*line) && *line!='#' ) {
		if ( tis!=NULL ) {
		    bpt = strchr(line,'|');
		    cpt = strchr(line,':');
		    if ( cpt==NULL && bpt==NULL )
			cpt = line+strlen(line)-1;
		    else if ( cpt!=NULL && bpt!=NULL && bpt<cpt )
			cpt = bpt;
		    else if ( cpt==NULL )
			cpt = bpt;
		    tis[cnt].text = uc_copyn(line,cpt-line);
		}
		++cnt;
	    }
	}
	if ( tis!=NULL ) {
	    fclose(printcap);
return( tis );
	}
	tis = gcalloc((cnt+1),sizeof(GTextInfo));
	tis[0].text = uc_copy("<default>");
	rewind(printcap);
    }
}

static int progexists(char *prog) {
    char buffer[1025];

return( ProgramExists(prog,buffer)!=NULL );
}

static int PageSetup(PI *pi) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    char buf[10], pb[30];
    int pt;
    /* Don't translate these. we compare against the text */
    static GTextInfo pagesizes[] = {
	{ (unichar_t *) "US Letter", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ (unichar_t *) "US Legal", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ (unichar_t *) "A3", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ (unichar_t *) "A4", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ (unichar_t *) "B4", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ NULL }
    };

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Page Setup");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,250));
    pos.height = GDrawPointsToPixels(NULL,174);
    pi->setup = GDrawCreateTopWindow(NULL,&pos,pg_e_h,pi,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

/* program names also don't get translated */
    label[0].text = (unichar_t *) "lp";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.mnemonic = 'l';
    gcd[0].gd.pos.x = 40; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = progexists("lp")? (gg_visible | gg_enabled):gg_visible;
    gcd[0].gd.cid = CID_lp;
    gcd[0].gd.handle_controlevent = PG_RadioSet;
    gcd[0].creator = GRadioCreate;

    label[1].text = (unichar_t *) "lpr";
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.mnemonic = 'r';
    gcd[1].gd.pos.x = gcd[0].gd.pos.x; gcd[1].gd.pos.y = 18+gcd[0].gd.pos.y; 
    gcd[1].gd.flags = progexists("lpr")? (gg_visible | gg_enabled):gg_visible;
    gcd[1].gd.cid = CID_lpr;
    gcd[1].gd.handle_controlevent = PG_RadioSet;
    gcd[1].creator = GRadioCreate;

    use_gv = false;
    label[2].text = (unichar_t *) "ghostview";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'g';
    gcd[2].gd.pos.x = gcd[0].gd.pos.x+50; gcd[2].gd.pos.y = gcd[0].gd.pos.y;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    if ( !progexists("ghostview") ) {
	if ( progexists("gv") ) {
	    label[2].text = (unichar_t *) "gv";
	    use_gv = true;
	} else
	    gcd[2].gd.flags = gg_visible;
    }
    gcd[2].gd.cid = CID_ghostview;
    gcd[2].gd.handle_controlevent = PG_RadioSet;
    gcd[2].creator = GRadioCreate;

    label[3].text = (unichar_t *) _("To _File");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'F';
    gcd[3].gd.pos.x = gcd[2].gd.pos.x; gcd[3].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[3].gd.flags = gg_visible | gg_enabled;
    gcd[3].gd.cid = CID_File;
    gcd[3].gd.handle_controlevent = PG_RadioSet;
    gcd[3].creator = GRadioCreate;

    label[4].text = (unichar_t *) _("To P_DF File");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'F';
    gcd[4].gd.pos.x = gcd[2].gd.pos.x+70; gcd[4].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].gd.cid = CID_PDFFile;
    gcd[4].gd.handle_controlevent = PG_RadioSet;
    gcd[4].creator = GRadioCreate;

    label[5].text = (unichar_t *) _("_Other");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = 22+gcd[1].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[5].gd.cid = CID_Other;
    gcd[5].gd.handle_controlevent = PG_RadioSet;
    gcd[5].gd.popup_msg = (unichar_t *) _("Any other command with all its arguments.\nThe command must expect to deal with a postscript\nfile which it will find by reading its standard input.");
    gcd[5].creator = GRadioCreate;

    if ( (pt=pi->printtype)==pt_unknown ) pt = pt_lp;
    if ( pt==pt_pdf ) pt = 4;		/* These two are out of order */
    else if ( pt==pt_other ) pt = 5;
    if ( !(gcd[pt].gd.flags&gg_enabled) ) pt = pt_file;		/* always enabled */
    gcd[pt].gd.flags |= gg_cb_on;

    label[6].text = (unichar_t *) (printcommand?printcommand:"");
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'O';
    gcd[6].gd.pos.x = gcd[2].gd.pos.x; gcd[6].gd.pos.y = gcd[5].gd.pos.y-4;
    gcd[6].gd.pos.width = 120;
    gcd[6].gd.flags = gg_visible | gg_enabled;
    gcd[6].gd.cid = CID_OtherCmd;
    gcd[6].creator = GTextFieldCreate;

    label[7].text = (unichar_t *) _("Page_Size:");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.mnemonic = 'S';
    gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 24+gcd[5].gd.pos.y+6; 
    gcd[7].gd.flags = gg_visible | gg_enabled;
    gcd[7].creator = GLabelCreate;

    if ( pi->pagewidth==595 && pi->pageheight==792 )
	strcpy(pb,"US Letter");		/* Pick a name, this is the default case */
    else if ( pi->pagewidth==612 && pi->pageheight==792 )
	strcpy(pb,"US Letter");
    else if ( pi->pagewidth==612 && pi->pageheight==1008 )
	strcpy(pb,"US Legal");
    else if ( pi->pagewidth==595 && pi->pageheight==842 )
	strcpy(pb,"A4");
    else if ( pi->pagewidth==842 && pi->pageheight==1191 )
	strcpy(pb,"A3");
    else if ( pi->pagewidth==708 && pi->pageheight==1000 )
	strcpy(pb,"B4");
    else
	sprintf(pb,"%dx%d mm", (int) (pi->pagewidth*25.4/72),(int) (pi->pageheight*25.4/72));
    label[8].text = (unichar_t *) pb;
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'S';
    gcd[8].gd.pos.x = 60; gcd[8].gd.pos.y = gcd[7].gd.pos.y-6;
    gcd[8].gd.pos.width = 90;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.cid = CID_Pagesize;
    gcd[8].gd.u.list = pagesizes;
    gcd[8].creator = GListFieldCreate;


    label[9].text = (unichar_t *) _("_Copies:");
    label[9].text_is_1byte = true;
    label[9].text_in_resource = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.mnemonic = 'C';
    gcd[9].gd.pos.x = 160; gcd[9].gd.pos.y = gcd[7].gd.pos.y; 
    gcd[9].gd.flags = gg_visible | gg_enabled;
    gcd[9].gd.cid = CID_CopiesLab;
    gcd[9].creator = GLabelCreate;

    sprintf(buf,"%d",pi->copies);
    label[10].text = (unichar_t *) buf;
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.mnemonic = 'C';
    gcd[10].gd.pos.x = 200; gcd[10].gd.pos.y = gcd[8].gd.pos.y;
    gcd[10].gd.pos.width = 40;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    gcd[10].gd.cid = CID_Copies;
    gcd[10].creator = GTextFieldCreate;


    label[11].text = (unichar_t *) _("_Printer:");
    label[11].text_is_1byte = true;
    label[11].text_in_resource = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.mnemonic = 'P';
    gcd[11].gd.pos.x = 5; gcd[11].gd.pos.y = 30+gcd[7].gd.pos.y+6; 
    gcd[11].gd.flags = gg_visible | gg_enabled;
    gcd[11].gd.cid = CID_PrinterLab;
    gcd[11].creator = GLabelCreate;

    label[12].text = (unichar_t *) pi->printer;
    label[12].text_is_1byte = true;
    if ( pi->printer!=NULL )
	gcd[12].gd.label = &label[12];
    gcd[12].gd.mnemonic = 'P';
    gcd[12].gd.pos.x = 60; gcd[12].gd.pos.y = gcd[11].gd.pos.y-6;
    gcd[12].gd.pos.width = 90;
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].gd.cid = CID_Printer;
    gcd[12].gd.u.list = PrinterList();
    gcd[12].creator = GListFieldCreate;


    gcd[13].gd.pos.x = 30-3; gcd[13].gd.pos.y = gcd[12].gd.pos.y+36;
    gcd[13].gd.pos.width = -1; gcd[13].gd.pos.height = 0;
    gcd[13].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[13].text = (unichar_t *) _("_OK");
    label[13].text_is_1byte = true;
    label[13].text_in_resource = true;
    gcd[13].gd.mnemonic = 'O';
    gcd[13].gd.label = &label[13];
    gcd[13].gd.handle_controlevent = PG_OK;
    gcd[13].creator = GButtonCreate;

    gcd[14].gd.pos.x = -30; gcd[14].gd.pos.y = gcd[13].gd.pos.y+3;
    gcd[14].gd.pos.width = -1; gcd[14].gd.pos.height = 0;
    gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[14].text = (unichar_t *) _("_Cancel");
    label[14].text_is_1byte = true;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.mnemonic = 'C';
    gcd[14].gd.handle_controlevent = PG_Cancel;
    gcd[14].creator = GButtonCreate;

    gcd[15].gd.pos.x = 2; gcd[15].gd.pos.y = 2;
    gcd[15].gd.pos.width = pos.width-4; gcd[15].gd.pos.height = pos.height-2;
    gcd[15].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[15].creator = GGroupCreate;

    GGadgetsCreate(pi->setup,gcd);
    GTextInfoListFree(gcd[12].gd.u.list);
    PG_SetEnabled(pi);
    GDrawSetVisible(pi->setup,true);
    while ( !pi->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(pi->setup);
    pi->done = false;
return( pi->printtype!=pt_unknown );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/* ************************************************************************** */
/* ************************* Code for Print dialog ************************** */
/* ************************************************************************** */

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* Slightly different from one in fontview */
static int FVSelCount(FontView *fv) {
    int i, cnt=0, gid;
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]))
	    ++cnt;
return( cnt);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void QueueIt(PI *pi) {
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
}

static void DoPrinting(PI *pi,char *filename) {
    int sfmax=1;

    if ( pi->pt==pt_fontsample ) {
	struct sfmaps *sfmap;
	for ( sfmap=pi->sample->sfmaps, sfmax=0; sfmap!=NULL; sfmap=sfmap->next, ++sfmax );
	if ( sfmax==0 ) sfmax=1;
    }
    pi->sfmax = sfmax;
    pi->sfbits = gcalloc(sfmax,sizeof(struct sfbits));
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
	gwwv_post_error(_("Print Failed"),_("Failed to generate postscript in file %s"),
		filename==NULL?"temporary":filename );
    if ( pi->printtype!=pt_file && pi->printtype!=pt_pdf )
	QueueIt(pi);
    if ( fclose(pi->out)!=0 )
	gwwv_post_error(_("Print Failed"),_("Failed to generate postscript in file %s"),
		filename==NULL?"temporary":filename );
    free(pi->sfbits);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    /* CIDs for Print */
#define CID_TabSet	1000
#define CID_Display	1001
#define CID_Chars	1002
#define	CID_MultiSize	1003
#define CID_PSLab	1005
#define	CID_PointSize	1006
#define CID_OK		1009
#define CID_Cancel	1010
#define CID_Setup	1010

    /* CIDs for display */
#define CID_Font	2001
#define CID_AA		2002
#define CID_SizeLab	2003
#define CID_Size	2004
#define CID_pfb		2005
#define CID_ttf		2006
#define CID_otf		2007
#define CID_bitmap	2009
#define CID_pfaedit	2010
#define CID_SampleText	2011
#define CID_ScriptLang	2022
#define CID_Features	2023
#define CID_DPI		2024

static void PRT_SetEnabled(PI *pi) {
    int enable_ps;

    enable_ps = !GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Chars));

    GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_PSLab),enable_ps);
    GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_PointSize),enable_ps);
}

static int PRT_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	int err = false;
	int di = pi->fv!=NULL?0:pi->mv!=NULL?2:1;
	char *ret;
	char *file;
	char buf[100];

	pi->pt = GTabSetGetSel(GWidgetGetControl(pi->gw,CID_TabSet))==0 ? pt_fontsample :
		GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Chars))? pt_chars:
		GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_MultiSize))? pt_multisize:
		pt_fontdisplay;
	if ( pi->pt==pt_fontdisplay ) {
	    pi->pointsize = GetInt8(pi->gw,CID_PointSize,_("_Pointsize:"),&err);
	    if ( err )
return(true);
	    if ( pi->pointsize<1 || pi->pointsize>200 ) {
		gwwv_post_error(_("Invalid point size"),_("Invalid point size"));
return(true);
	    }
	}
	if ( pi->printtype==pt_unknown )
	    if ( !PageSetup(pi))
return(true);

	if ( pi->printtype==pt_file || pi->printtype==pt_pdf ) {
	    sprintf(buf,"pr-%.90s.%s", pi->mainsf->fontname,
		    pi->printtype==pt_file?"ps":"pdf");
	    ret = gwwv_save_filename(_("Print To File..."),buf,
		    pi->printtype==pt_pdf?"*.pdf":"*.ps");
	    if ( ret==NULL )
return(true);
	    file = utf82def_copy(ret);
	    free(ret);
	    pi->out = fopen(file,"wb");
	    if ( pi->out==NULL ) {
		gwwv_post_error(_("Print Failed"),_("Failed to open file %s for output"), file);
		free(file);
return(true);
	    }
	} else {
	    file = NULL;
	    pi->out = tmpfile();
	    if ( pi->out==NULL ) {
		gwwv_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return(true);
	    }
	}

	pdefs[di].last_cs = pi->mainmap->enc;
	pdefs[di].pt = pi->pt;
	pdefs[di].pointsize = pi->pointsize;

	if ( pi->pt==pt_fontsample ) {
	    pi->sample = SFTFConvertToPrint(GWidgetGetControl(pi->gw,CID_SampleText),
		    (pagewidth-1*72)*printdpi/72,
		    (pageheight-1*72)*printdpi/72,
		    printdpi);
	}

	DoPrinting(pi,file);
	free(file);
	if ( pi->pt==pt_fontsample )
	    GGadgetDestroy(&pi->sample->g);

	pi->done = true;
    }
return( true );
}

static int PRT_Setup(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PageSetup(pi);
    }
return( true );
}

static int PRT_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PI *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PRT_SetEnabled(pi);
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
//* Slovaka (from http://shiar.net/misc/txt/pangram.en) */
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
    NULL
};
static uint32 _simplecyrilliclangs[] = {
    CHR('R','U','S',' '),
    CHR('R','U','S',' ')
};
/* Russian */
static char *_annakarenena[] = {
    " Всѣ счастливыя семьи похожи другъ на друга, каждая несчастливая семья несчастлива по-своему.",
    " Все смѣшалось въ домѣ Облонскихъ. Жена узнала, что мужъ былъ связи съ бывшею въ ихъ домѣ француженкою-гувернанткой, и объявила мужу, что не можетъ жить съ нимъ въ одномъ домѣ.",
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
/* Renaisance English with period ligatures */
static char *_muchado[] = {
    " But till all graces be in one woman, one womã ſhal not com in my grace: rich ſhe ſhall be thats certain, wiſe, or ile none, vertuous, or ile neuer cheapen her.",
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
    " ПРЕДМЕТ И ЗАДАЧИ НА ФОНЕТИКАТА Думата фонетика произлиза от гръцката дума рнопе, която означава „звук“, „глас“, „тон“.",
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
/* Laautzyy */
static char *_YihKing[] = {
    "道可道非常道，",
    "名可名非常名。",
    NULL
};
static char *_LiiBair[] = {
    "將進酒",
    "",
    "君不見 黃河之水天上來 奔流到海不復回",
    "君不見 高堂明鏡悲白髮 朝如青絲暮成雪",
    "人生得意須盡歡 莫使金樽空對⺝",
    "天生我材必有用 千金散盡還復來",
    "烹羊宰牛且為樂 會須一飲三百杯",
    "岑夫子 丹丘生 將進酒 君莫停",
    "與君歌一曲 請君為我側耳聽",
    "",
    "鐘鼓饌玉不足貴 但願長醉不願醒",
    "古來聖賢皆寂寞 惟有飲者留其名",
    "陳王昔時宴平樂 斗酒十千恣讙謔",
    "主人何為言少錢 徑須沽取對君酌",
    "五花馬 千金裘 呼兒將出換美酒",
    "與爾同消萬古愁",
    NULL
};
static char *_LiiBairShort[] = {
    "將進酒",
    "",
    "君不見 黃河之水天上來 奔流到海不復回",
    "君不見 高堂明鏡悲白髮 朝如青絲暮成雪",
    NULL
};
/* Japanese */
static char *_Genji[] = {
    "源⽒物語（紫式部）：いづれの御時にか、⼥御更⾐∂またさぶらひたまひける中に、いとやむごとなき際には∂らぬが、すぐれて時めきたまふ∂りけり",
    NULL
};
static char *_IAmACat[] = {
    "吾輩は猫で∂る（夏ｭﾚ漱⽯）：吾輩は猫で∂る",
    NULL
};

/* The following translations of the gospel according to John are all from */
/*  Compendium of the world's languages. 1991 Routledge. by George L. Campbell*/

/* Belorussian */
static char *_belorussianjohn[] = {
    "Вначале было Слово, и Слово было у Бога, и Слово было Бог.",
    "Оно было в начале у Бога.",
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
/* norwegian */
static char *_norwegianjohn[] = {
    "I begynnelsen var Ordet, Ordet var hos Gud, og Ordet var Gud.",
    "Han var i begynnelsen hos Gud.",
    "Alt er blitt til ved ham; uten ham er ikke noe blitt til av alt som er til.",
    NULL
};
/* ?old? norwegian */
static char *_nnorwegianjohn[] = {
    "I opphavet var Ordet, og Ordet var hjå Gud, og Ordet var Gud.",
    "Han var i opphavet hjå Gud.",
    NULL
};
/* old church slavonic */
static char *_churchjohn[] = {
    "Въ началѣ бѣ Слово ƶ Слово Слово къ Бг҃,",
    "г҃ ъбѣ Слово .",
    "в҃ Сей бѣ искони къ Б.",
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

/* I've omitted cornish. no interesting letters. no current speakers */

#if 1 /* http://www.ethnologue.com/iso639/codes.asp */
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
    { _debello, "la", sc_latin, CHR('l','a','t','n'), CHR('L','A','T',' ')},
    { _hebrew, "he", sc_hebrew, CHR('h','e','b','r'), CHR('I','W','R',' ') },
    { _arabic, "ar", sc_arabic, CHR('a','r','a','b'), CHR('A','R','A',' ')},
    { _hangulsijo, "ko", sc_hangul, CHR('h','a','n','g'), CHR('K','O','R',' ')},
    { _YihKing, "zh", sc_chinesetrad, CHR('h','a','n','i'), CHR('Z','H','T',' ')},
    { _LiiBair, "zh", sc_chinesetrad, CHR('h','a','n','i'), CHR('Z','H','T',' ')},
    { _Genji, "ja", sc_kanji, CHR('h','a','n','i'), CHR('J','A','N',' ')},
    { _IAmACat, "ja", sc_kanji, CHR('h','a','n','i'), CHR('J','A','N',' ')},
    { _donquixote, "es", sc_latin, CHR('l','a','t','n'), CHR('E','S','P',' ')},
    { _inferno, "it", sc_latin, CHR('l','a','t','n'), CHR('I','T','A',' ')},
    { _beorwulf, "enm", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ')},		/* 639-2 name for middle english */
    { _muchado, "eng", sc_latin, CHR('l','a','t','n'), CHR('E','N','G',' ')},		/* 639-2 name for modern english */
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
    { NULL }
};
#else
static char **sample[] = { _simple, _simplecyrill, _faust, _pheadra, _antigone,
	_annakarenena, debello, hebrew, arabic, hangulsijo, YihKing, LiiBair, Genji,
	_IAmACat, donquixote, inferno, beorwulf, muchado, PippiGarOmBord,
	_mabinogion, goodsoldier, macedonian, bulgarian, belorussianjohn,
	_churchjohn,
	_lithuanian, _polish, slovene, irishjohn, basquejohn, portjohn,
	_icelandicjohn, _danishjohn, swedishjohn, norwegianjohn, nnorwegianjohn,
	_dutchjohn, _finnishjohn,
	_cherokeejohn, _thaijohn, georgianjohn, swahilijohn,
	NULL };
#endif

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
    else if ( strcmp(langbuf,"pl")==0 )
	simple_pos = 10;
    else if ( strcmp(langbuf,"cz")==0 )
	simple_pos = 11;
    else
	simple_pos = rand()&3;
    _simple[0] = _simplelatnchoices[simple_pos];
    sample[0].lang = _simplelatnlangs[simple_pos];

    for ( j=0; _simplecyrillchoices[j]!=NULL; ++j );
    simple_pos = rand()%j;
    _simplecyrill[0] = _simplecyrillchoices[simple_pos];
    sample[1].lang = _simplecyrilliclangs[simple_pos];
}

static int AllChars( SplineFont *sf, const char *str) {
    int i, ch;

    if ( sf->subfontcnt==0 ) {
	while ( (ch = utf8_ildb(&str))!='\0' ) {
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		if ( sf->glyphs[i]->unicodeenc == ch )
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

unichar_t *PrtBuildDef( SplineFont *sf, void *tf,
	void (*langsyscallback)(void *tf, int end, uint32 script, uint32 lang) ) {
    int i, j, gotem, len, any=0;
    unichar_t *ret=NULL;
    char **cur;

    OrderSampleByLang();

    while ( 1 ) {
	len = any = 0;
	for ( i=0; sample[i].sample!=NULL; ++i ) {
	    gotem = true;
	    cur = sample[i].sample;
	    for ( j=0; cur[j]!=NULL && gotem; ++j )
		gotem = AllChars(sf,cur[j]);
	    if ( !gotem && sample[i].sample==_simple ) {
		gotem = true;
		_simple[0] = _simplelatnchoices[1];
	    } else if ( !gotem && sample[i].sample==_LiiBair ) {
		cur = _LiiBairShort;
		gotem = true;
		for ( j=0; cur[j]!=NULL && gotem; ++j )
		    gotem = AllChars(sf,cur[j]);
	    }
	    if ( gotem ) {
		++any;
		for ( j=0; cur[j]!=NULL; ++j ) {
		    if ( ret )
			utf82u_strcpy(ret+len,cur[j]);
		    len += utf8_strlen(cur[j]);
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

	/* If no matches then put in "the quick brown...", in russian too if the encoding suggests it... */
	if ( !any ) {
	    for ( j=0; _simple[j]!=NULL; ++j ) {
		if ( ret )
		    utf82u_strcpy(ret+len,_simple[j]);
		len += utf8_strlen(_simple[j]);
		if ( ret )
		    ret[len] = '\n';
		++len;
		if ( ret && langsyscallback!=NULL )
		    (langsyscallback)(tf,len,CHR('l','a','t','n'),CHR('E','N','G',' '));
	    }
	    if ( SFGetChar(sf,0x411,NULL)!=NULL ) {
		if ( ret )
		    ret[len] = '\n';
		++len;
		for ( j=0; _simplecyrill[j]!=NULL; ++j ) {
		    if ( ret )
		    utf82u_strcpy(ret+len,_simplecyrill[j]);
		    len += utf8_strlen(_simplecyrill[j]);
		    if ( ret )
			ret[len] = '\n';
		    ++len;
		    if ( ret && langsyscallback!=NULL )
			(langsyscallback)(tf,len,CHR('c','y','r','l'),CHR('R','U','S',' '));
		}
	    }
	}

	if ( ret ) {
	    ret[len-1]='\0';
return( ret );
	}
	if ( len == 0 ) {
	    /* Er... We didn't find anything?! */
return( gcalloc(1,sizeof(unichar_t)));
	}
	ret = galloc((len+1)*sizeof(unichar_t));
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void PIInit(PI *pi,FontView *fv,SplineChar *sc,MetricsView *mv) {
#else
static void PIInit(PI *pi,FontView *fv,SplineChar *sc,void *mv) {
#endif
    int di = fv!=NULL?0:sc!=NULL?1:2;

    memset(pi,'\0',sizeof(*pi));
    pi->fv = fv;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    pi->mv = mv;
#endif
    pi->sc = sc;
    if ( fv!=NULL ) {
	pi->mainsf = fv->sf;
	pi->mainmap = fv->map;
    } else if ( sc!=NULL ) {
	pi->mainsf = sc->parent;
	pi->mainmap = pi->mainsf->fv->map;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    } else {
	pi->mainsf = mv->fv->sf;
	pi->mainmap = mv->fv->map;
#endif
    }
    if ( pi->mainsf->cidmaster )
	pi->mainsf = pi->mainsf->cidmaster;
    PIGetPrinterDefs(pi);
    pi->pointsize = pdefs[di].pointsize;
    if ( pi->pointsize==0 )
	pi->pointsize = pi->mainsf->subfontcnt!=0?18:20;		/* 18 fits 20 across, 20 fits 16 */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

/* ************************************************************************** */
/* ************************ Code for Display dialog ************************* */
/* ************************************************************************** */

static void TextInfoDataFree(GTextInfo *ti) {
    int i;

    if ( ti==NULL )
return;
    for ( i=0; ti[i].text!=NULL || ti[i].line ; ++i )
	free(ti[i].userdata);
    GTextInfoListFree(ti);
}

static GTextInfo *FontNames(SplineFont *cur_sf) {
    int cnt;
    FontView *fv;
    SplineFont *sf;
    GTextInfo *ti;

    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next )
	if ( fv->nextsame==NULL )
	    ++cnt;
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next )
	if ( fv->nextsame==NULL ) {
	    sf = fv->sf;
	    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	    ti[cnt].text = uc_copy(sf->fontname);
	    ti[cnt].userdata = sf;
	    if ( sf==cur_sf )
		ti[cnt].selected = true;
	    ++cnt;
	}
return( ti );
}

static BDFFont *DSP_BestMatch(SplineFont *sf,int aa,int size) {
    BDFFont *bdf, *sizem=NULL;
    int a;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( bdf->clut==NULL && !aa )
	    a = 4;
	else if ( bdf->clut!=NULL && aa ) {
	    if ( bdf->clut->clut_len==256 )
		a = 4;
	    else if ( bdf->clut->clut_len==16 )
		a = 3;
	    else
		a = 2;
	} else
	    a = 1;
	if ( bdf->pixelsize==size && a==4 )
return( bdf );
	if ( sizem==NULL )
	    sizem = bdf;
	else {
	    int sdnew = bdf->pixelsize-size, sdold = sizem->pixelsize-size;
	    if ( sdnew<0 ) sdnew = -sdnew;
	    if ( sdold<0 ) sdold = -sdold;
	    if ( sdnew<sdold )
		sizem = bdf;
	    else if ( sdnew==sdold ) {
		int olda;
		if ( sizem->clut==NULL && !aa )
		    olda = 4;
		else if ( sizem->clut!=NULL && aa ) {
		    if ( sizem->clut->clut_len==256 )
			olda = 4;
		    else if ( sizem->clut->clut_len==16 )
			olda = 3;
		    else
			olda = 2;
		} else
		    olda = 1;
		if ( a>olda )
		    sizem = bdf;
	    }
	}
    }
return( sizem );	
}

static BDFFont *DSP_BestMatchDlg(DI *di) {
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int val;
    unichar_t *end;

    if ( sel==NULL )
return( NULL );
    sf = sel->userdata;
    val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    if ( *end!='\0' || val<4 )
return( NULL );

return( DSP_BestMatch(sf,GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)),val) );
}

static enum sftf_fonttype DSP_FontType(DI *di) {
    int type;
    type = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfb))? sftf_pfb :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_ttf))? sftf_ttf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_otf))? sftf_otf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfaedit))? sftf_pfaedit :
	   sftf_bitmap;
return( type );
}

static void DSP_SetFont(DI *di,int doall) {
    unichar_t *end;
    int size = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
    int type;

    if ( sel==NULL || *end )
return;
    sf = sel->userdata;

    type = DSP_FontType(di);

    if ( !SFTFSetFontData(GWidgetGetControl(di->gw,CID_SampleText),doall?0:-1,-1,
	    sf,type,size,aa))
	gwwv_post_error(_("Bad Font"),_("Bad Font"));
}

static void DSP_ChangeFontCallback(void *context,SplineFont *sf,enum sftf_fonttype type,
	int size, int aa, uint32 script, uint32 lang, uint32 *feats) {
    DI *di = context;
    char buf[16];
    int i,j,cnt;
    uint32 *tags;
    GTextInfo **ti;

    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),aa);

    sprintf(buf,"%d",size);
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_Size),buf);

    {
	GTextInfo **ti;
	int i,set; int32 len;
	ti = GGadgetGetList(GWidgetGetControl(di->gw,CID_Font),&len);
	for ( i=0; i<len; ++i )
	    if ( ti[i]->userdata == sf )
	break;
	if ( i<len ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(di->gw,CID_Font),i);
	    /*GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Font),ti[i]->text);*/
	}
	set = hasFreeType() && !sf->onlybitmaps && sf->subfontcnt==0;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfb),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_ttf),set);
	set = hasFreeType() && !sf->onlybitmaps;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfaedit),!sf->onlybitmaps);
    }

    if ( type==sftf_pfb )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
    else if ( type==sftf_ttf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_ttf),true);
    else if ( type==sftf_otf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
    else if ( type==sftf_pfaedit )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfaedit),true);
    else
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);

    buf[0] = script>>24; buf[1] = script>>16; buf[2] = script>>8; buf[3] = script;
    buf[4] = '{';
    buf[5] = lang>>24; buf[6] = lang>>16; buf[7] = lang>>8; buf[8] = lang;
    buf[9] = '}';
    buf[10] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ScriptLang),buf);

    tags = SFFeaturesInScriptLang(sf,-2,script,lang);
    if ( tags[0]==0 ) {
	free(tags);
	tags = SFFeaturesInScriptLang(sf,-2,script,DEFAULT_LANG);
    }
    for ( cnt=0; tags[cnt]!=0; ++cnt );
    for ( i=0; feats[i]!=0; ++i ) {
	for ( j=0; tags[j]!=0; ++j ) {
	    if ( feats[i]==tags[j] )
	break;
	}
	if ( tags[j]==0 )
	    ++cnt;
    }
    ti = galloc((cnt+2)*sizeof(GTextInfo *));
    for ( i=0; tags[i]!=0; ++i ) {
	ti[i] = gcalloc( 1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	if ( (tags[i]>>24)<' ' || (tags[i]>>24)>0x7e )
	    sprintf( buf, "<%d,%d>", tags[i]>>16, tags[i]&0xffff );
	else {
	    buf[0] = tags[i]>>24; buf[1] = tags[i]>>16; buf[2] = tags[i]>>8; buf[3] = tags[i]; buf[4] = 0;
	}
	ti[i]->text = uc_copy(buf);
	ti[i]->userdata = (void *) (intpt) tags[i];
	for ( j=0; feats[j]!=0; ++j ) {
	    if ( feats[j] == tags[i] ) {
		ti[i]->selected = true;
	break;
	    }
	}
    }
    cnt = i;
    for ( i=0; feats[i]!=0; ++i ) {
	for ( j=0; tags[j]!=0; ++j ) {
	    if ( feats[i]==tags[j] )
	break;
	}
	if ( tags[j]==0 ) {
	    ti[cnt] = gcalloc( 1,sizeof(GTextInfo));
	    ti[cnt]->bg = COLOR_DEFAULT;
	    ti[cnt]->fg = COLOR_CREATE(0x70,0x70,0x70);
	    ti[cnt]->selected = true;
	    buf[0] = feats[i]>>24; buf[1] = feats[i]>>16; buf[2] = feats[i]>>8; buf[3] = feats[i]; buf[4] = 0;
	    ti[cnt]->text = uc_copy(buf);
	    ti[cnt++]->userdata = (void *) (intpt) feats[i];
	}
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    /* These will become ordered because the list widget will do that */
    GGadgetSetList(GWidgetGetControl(di->gw,CID_Features),ti,false);
    free(tags);
}

static int DSP_AAState(SplineFont *sf,BDFFont *bestbdf) {
    /* What should AntiAlias look like given the current set of bitmaps */
    int anyaa=0, anybit=0;
    BDFFont *bdf;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	if ( bdf->clut==NULL )
	    anybit = true;
	else
	    anyaa = true;
    if ( anybit && anyaa )
return( gg_visible | gg_enabled | (bestbdf!=NULL && bestbdf->clut!=NULL ? gg_cb_on : 0) );
    else if ( anybit )
return( gg_visible );
    else if ( anyaa )
return( gg_visible | gg_cb_on );

return( gg_visible );
}

static int DSP_FontChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *sel = GGadgetGetListItemSelected(g);
	SplineFont *sf;
	BDFFont *best;
	int flags, pick = 0, i;
	char size[12]; unichar_t usize[12];
	uint16 cnt;

	if ( sel==NULL )
return( true );
	sf = sel->userdata;

	TextInfoDataFree(di->scriptlangs);
	di->scriptlangs = SLOfFont(sf);
	GGadgetSetList(GWidgetGetControl(di->gw,CID_ScriptLang),
		GTextInfoArrayFromList(di->scriptlangs,&cnt),false);

	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) && sf->bitmaps==NULL )
	    pick = true;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	if ( !hasFreeType() || sf->onlybitmaps ) {
	    best = DSP_BestMatchDlg(di);
	    flags = DSP_AAState(sf,best);
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
	    if ( best!=NULL ) {
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    pick = true;
	} else if ( sf->subfontcnt!=0 ) {
	    for ( i=CID_pfb; i<CID_otf; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
		if ( GGadgetIsChecked(GWidgetGetControl(di->gw,i)) )
		    pick = true;
	    }
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
	} else {
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
	}
	if ( pick )
	    DSP_SetFont(di,false);
	else
	    SFTFSetFont(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,sf);
    }
return( true );
}

static int DSP_AAChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    int val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),NULL,10);
	    int bestdiff = 8000, bdfdiff;
	    BDFFont *bdf, *best=NULL;
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( (aa && bdf->clut) || (!aa && bdf->clut==NULL)) {
		    if ((bdfdiff = bdf->pixelsize-val)<0 ) bdfdiff = -bdfdiff;
		    if ( bdfdiff<bestdiff ) {
			best = bdf;
			bestdiff = bdfdiff;
			if ( bdfdiff==0 )
	    break;
		    }
		}
	    }
	    if ( best!=NULL ) {
		char size[12]; unichar_t usize[12];
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    DSP_SetFont(di,false);
	} else
	    SFTFSetSize(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)));
    }
return( true );
}

static int DSP_SizeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int size = GetInt8(di->gw,CID_Size,_("_Size:"),&err);
	if ( err || size<4 )
return( true );
	if ( GWidgetGetControl(di->gw,CID_SampleText)==NULL )
return( true );		/* Happens during startup */
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    BDFFont *bdf, *best=NULL;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( bdf->pixelsize==size ) {
		    if (( bdf->clut && aa ) || ( bdf->clut==NULL && !aa )) {
			best = bdf;
	    break;
		    }
		    best = bdf;
		}
	    }
	    if ( best==NULL ) {
		char buf[100], *pt=buf, *end=buf+sizeof(buf)-10;
		unichar_t ubuf[12];
		int lastsize = 0;
		for ( bdf=sf->bitmaps; bdf!=NULL && pt<end; bdf=bdf->next ) {
		    if ( bdf->pixelsize!=lastsize ) {
			sprintf( pt, "%d,", bdf->pixelsize );
			lastsize = bdf->pixelsize;
			pt += strlen(pt);
		    }
		}
		if ( pt!=buf )
		    pt[-1] = '\0';
		gwwv_post_error(_("Bad Size"),_("Requested bitmap size not available in font. Font supports %s"),buf);
		best = DSP_BestMatchDlg(di);
		if ( best!=NULL ) {
		    sprintf( buf, "%d", best->pixelsize);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),ubuf);
		    size = best->pixelsize;
		}
	    }
	    if ( best==NULL )
return(true);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),best->clut!=NULL );
	    DSP_SetFont(di,false);
	} else
	    SFTFSetSize(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,size);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->sizechanged!=NULL )
	    GDrawCancelTimer(di->sizechanged);
	di->sizechanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_SizeChangedTimer(DI *di) {
    GEvent e;

    di->sizechanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_SizeChanged(e.u.control.g,&e);
}


static int DSP_DpiChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int dpi = GetInt8(di->gw,CID_DPI,_("_DPI:"),&err);
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	if ( err || dpi<20 || dpi>300 )
return( true );
	if ( sample==NULL )
return( true );		/* Happens during startup */
	SFTFSetDPI(sample,dpi);
	GGadgetRedraw(sample);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->dpichanged!=NULL )
	    GDrawCancelTimer(di->dpichanged);
	di->dpichanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_DpiChangedTimer(DI *di) {
    GEvent e;

    di->dpichanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_DpiChanged(e.u.control.g,&e);
}

static int DSP_Refresh(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	/* DI *di = GDrawGetUserData(GGadgetGetWindow(g)); */
	/* !!!! */
    }
return( true );
}

static int DSP_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    BDFFont *best = DSP_BestMatchDlg(di);
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int flags;
	    char size[12]; unichar_t usize[12];

	    if ( sel!=NULL ) {
		sf = sel->userdata;
		flags = DSP_AAState(sf,best);
		GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
		if ( best!=NULL ) {
		    sprintf( size, "%d", best->pixelsize );
		    uc_strcpy(usize,size);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
		}
	    }
	    DSP_SetFont(di,false);
	} else {
	    /*GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),true);*/
	    SFTFSetFontType(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    DSP_FontType(di));
	}
    }
return( true );
}

static int DSP_ScriptLangChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *sstr = _GGadgetGetTitle(g);
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32 script, lang;

	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    GGadgetSetTitle8(g,di->scriptlangs[e->u.control.u.tf_changed.from_pulldown].userdata );
	    sstr = _GGadgetGetTitle(g);
	} else {
	    if ( u_strlen(sstr)<4 || !isalpha(sstr[0]) || !isalnum(sstr[1]) /*|| !isalnum(sstr[2]) || !isalnum(sstr[3])*/ )
return( true );
	    if ( u_strlen(sstr)==4 )
		/* No language, we'll use default */;
	    else if ( u_strlen(sstr)!=10 || sstr[4]!='{' || sstr[9]!='}' ||
		    !isalpha(sstr[5]) || !isalpha(sstr[6]) || !isalpha(sstr[7])  )
return( true );
	}
	script = DEFAULT_SCRIPT;
	lang = DEFAULT_LANG;
	if ( u_strlen(sstr)>=4 )
	    script = (sstr[0]<<24) | (sstr[1]<<16) | (sstr[2]<<8) | sstr[3];
	if ( sstr[4]=='{' && u_strlen(sstr)>=9 )
	    lang = (sstr[5]<<24) | (sstr[6]<<16) | (sstr[7]<<8) | sstr[8];
	SFTFSetScriptLang(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,script,lang);
    }
return( true );
}

static int DSP_FeaturesChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32 *feats;
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i,cnt;

	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected ) ++cnt;
	feats = galloc((cnt+1)*sizeof(uint32));
	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected )
		feats[cnt++] = (intpt) ti[i]->userdata;
	feats[cnt] = 0;
	/* These will be ordered because the list widget will do that */
	SFTFSetFeatures(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,feats);
    }
return( true );
}

static int DSP_TextChanged(GGadget *g, GEvent *e) {
return( true );
}

static int DSP_Done(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	DI *di = GDrawGetUserData(gw);
	di->done = true;
    }
return( true );
}

static int dsp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	DI *di = GDrawGetUserData(gw);
	di->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("display.html");
return( true );
    } else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	if ( event->u.chr.state&ksm_shift ) {
	    DI *di = GDrawGetUserData(gw);
	    di->done = true;
	} else
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    } else if ( event->type==et_timer ) {
	DI *di = GDrawGetUserData(gw);
	if ( event->u.timer.timer==di->sizechanged )
	    DSP_SizeChangedTimer(di);
	else if ( event->u.timer.timer==di->dpichanged )
	    DSP_DpiChangedTimer(di);
    }
return( true );
}
    
void PrintDlg(FontView *fv,SplineChar *sc,MetricsView *mv) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17], boxes[15], mgcd[5], pgcd[8],
	    *harray[8], *farray[7], *barray[4],
	    *barray2[8], *varray[9], *varray2[9], *harray2[4],
	    *varray3[4][4], *ptarray[4], *varray4[4], *varray5[4][2],
	    *regenarray[6];
    GTextInfo label[17], mlabel[5], plabel[8];
    GTabInfo aspects[3];
    DI di;
    int dpi;
    char buf[12], dpibuf[12], sizebuf[12];
    SplineFont *sf = fv!=NULL ? fv->sf : sc!=NULL ? sc->parent : mv->fv->sf;
    int hasfreetype = hasFreeType();
    BDFFont *bestbdf=DSP_BestMatch(sf,true,12);
    unichar_t *temp;
    int cnt;
    int fromwindow = fv!=NULL?0:sc!=NULL?1:2;

    if ( sf->cidmaster )
	sf = sf->cidmaster;

    PIInit(&di,fv,sc,mv);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Print...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,410));
    pos.height = GDrawPointsToPixels(NULL,330);
    di.gw = GDrawCreateTopWindow(NULL,&pos,dsp_e_h,&di,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&mlabel,0,sizeof(mlabel));
    memset(&plabel,0,sizeof(plabel));
    memset(&gcd,0,sizeof(gcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&pgcd,0,sizeof(pgcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) sf->fontname;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.popup_msg = (unichar_t *) _("Select some text, then use this list to change the\nfont in which those characters are displayed.");
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.pos.width = 150;
    gcd[0].gd.flags = (fv_list==NULL || fv_list->next==NULL) ?
	    (gg_visible | gg_utf8_popup):
	    (gg_visible | gg_enabled | gg_utf8_popup);
    gcd[0].gd.cid = CID_Font;
    gcd[0].gd.u.list = FontNames(sf);
    gcd[0].gd.handle_controlevent = DSP_FontChanged;
    gcd[0].creator = GListButtonCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    label[2].text = (unichar_t *) _("_Size:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 210; gcd[2].gd.pos.y = gcd[0].gd.pos.y+6; 
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[2].gd.cid = CID_SizeLab;
    gcd[2].creator = GLabelCreate;
    harray[0] = &gcd[2];

    if ( bestbdf !=NULL && ( !hasfreetype || sf->onlybitmaps ))
	sprintf( buf, "%d", bestbdf->pixelsize );
    else
	strcpy(buf,"12");
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 240; gcd[3].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[3].gd.cid = CID_Size;
    gcd[3].gd.handle_controlevent = DSP_SizeChanged;
    gcd[3].gd.popup_msg = (unichar_t *) _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[3].creator = GNumericFieldCreate;
    harray[1] = &gcd[3]; harray[2] = GCD_HPad10;

    label[1].text = (unichar_t *) _("_AA");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 170; gcd[1].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup;
    if ( sf->bitmaps!=NULL && ( !hasfreetype || sf->onlybitmaps ))
	gcd[1].gd.flags = DSP_AAState(sf,bestbdf);
    gcd[1].gd.popup_msg = (unichar_t *) _("Select some text, this controls whether those characters will be\nAntiAlias (greymap) characters, or bitmap characters");
    gcd[1].gd.handle_controlevent = DSP_AAChange;
    gcd[1].gd.cid = CID_AA;
    gcd[1].creator = GCheckBoxCreate;
    harray[3] = &gcd[1]; harray[4] = GCD_Glue; harray[5] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    label[4].text = (unichar_t *) "pfb";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = gcd[0].gd.pos.x; gcd[4].gd.pos.y = 24+gcd[3].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[4].gd.cid = CID_pfb;
    gcd[4].gd.handle_controlevent = DSP_RadioSet;
    gcd[4].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[4].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps ) gcd[4].gd.flags = gg_visible;
    farray[0] = &gcd[4];

    label[5].text = (unichar_t *) "ttf";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = 46; gcd[5].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[5].gd.cid = CID_ttf;
    gcd[5].gd.handle_controlevent = DSP_RadioSet;
    gcd[5].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[5].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps ) gcd[5].gd.flags = gg_visible;
    else if ( sf->order2 ) gcd[5].gd.flags |= gg_cb_on;
    farray[1] = &gcd[5];

    label[6].text = (unichar_t *) "otf";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 114; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[6].gd.cid = CID_otf;
    gcd[6].gd.handle_controlevent = DSP_RadioSet;
    gcd[6].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[6].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps ) gcd[6].gd.flags = gg_visible;
    else if ( sf->subfontcnt!=0 || !sf->order2 ) gcd[6].gd.flags |= gg_cb_on;
    farray[2] = &gcd[6];
    

    label[7].text = (unichar_t *) "bitmap";
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 148; gcd[7].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[7].gd.cid = CID_bitmap;
    gcd[7].gd.handle_controlevent = DSP_RadioSet;
    gcd[7].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[7].creator = GRadioCreate;
    if ( sf->bitmaps==NULL ) gcd[7].gd.flags = gg_visible;
    else if ( !hasfreetype || sf->onlybitmaps ) gcd[7].gd.flags |= gg_cb_on;
    farray[3] = &gcd[7];

    label[8].text = (unichar_t *) "FontForge";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 200; gcd[8].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[8].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[8].gd.cid = CID_pfaedit;
    gcd[8].gd.handle_controlevent = DSP_RadioSet;
    gcd[8].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[8].creator = GRadioCreate;
    if ( !hasfreetype && sf->bitmaps==NULL ) gcd[8].gd.flags |= gg_cb_on;
    else if ( sf->onlybitmaps ) gcd[8].gd.flags = gg_visible;
    farray[4] = &gcd[8]; farray[5] = GCD_Glue; farray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = farray;
    boxes[3].creator = GHBoxCreate;
    varray[4] = &boxes[3]; varray[5] = NULL;

    label[9].text = (unichar_t *) "DFLT{dflt}";
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.popup_msg = (unichar_t *) _("Select some text, then use this list to specify\nthe current script & language.");
    gcd[9].gd.pos.x = 12; gcd[9].gd.pos.y = 6; 
    gcd[9].gd.pos.width = 150;
    gcd[9].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[9].gd.cid = CID_ScriptLang;
    gcd[9].gd.u.list = di.scriptlangs = SLOfFont(sf);
    gcd[9].gd.handle_controlevent = DSP_ScriptLangChanged;
    gcd[9].creator = GListFieldCreate;
    varray[6] = &gcd[9]; varray[7] = NULL; varray[8] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = varray;
    boxes[4].creator = GHVBoxCreate;
    harray2[1] = &boxes[4]; harray2[2] = GCD_Glue; harray2[3] = NULL;

    gcd[10].gd.popup_msg = (unichar_t *) _("Select some text, then use this list to specify\nactive features.");
    gcd[10].gd.pos.width = 50;
    gcd[10].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | gg_list_alphabetic | gg_list_multiplesel;
    gcd[10].gd.cid = CID_Features;
    gcd[10].gd.handle_controlevent = DSP_FeaturesChanged;
    gcd[10].creator = GListCreate;
    harray2[0] = &gcd[10];

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray2;
    boxes[5].creator = GHBoxCreate;
    varray2[0] = &boxes[5]; varray2[1] = NULL;


    gcd[11].gd.pos.x = 5; gcd[11].gd.pos.y = 20+gcd[11].gd.pos.y; 
    gcd[11].gd.pos.width = 400; gcd[11].gd.pos.height = 236; 
    gcd[11].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    gcd[11].gd.handle_controlevent = DSP_TextChanged;
    gcd[11].gd.cid = CID_SampleText;
    gcd[11].creator = SFTextAreaCreate;
    varray2[2] = &gcd[11]; varray2[3] = NULL;

    gcd[12].gd.flags = gg_visible | gg_enabled ;
    gcd[12].creator = GLineCreate;
    varray2[4] = &gcd[12]; varray2[5] = NULL;

    label[13].text = (unichar_t *) "DPI:";
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[13].gd.popup_msg = (unichar_t *) _("Specifies screen dots per inch");
    gcd[13].creator = GLabelCreate;

    dpi = GDrawPointsToPixels(NULL,72);
    sprintf( dpibuf, "%d", dpi );
    label[14].text = (unichar_t *) dpibuf;
    label[14].text_is_1byte = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.width = 50;
    gcd[14].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[14].gd.popup_msg = (unichar_t *) _("Specifies screen dots per inch");
    gcd[14].gd.cid = CID_DPI;
    gcd[14].gd.handle_controlevent = DSP_DpiChanged;
    gcd[14].creator = GNumericFieldCreate;

    gcd[15].gd.flags = gg_visible /*| gg_enabled*/ | gg_utf8_popup ;
    gcd[15].gd.popup_msg = (unichar_t *) _("FontForge does not update this window when a change is made to the font.\nIf a font has changed press the button to force an update");
    label[15].text = (unichar_t *) _("_Refresh");
    label[15].text_is_1byte = true;
    label[15].text_in_resource = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.handle_controlevent = DSP_Refresh;
    gcd[15].creator = GButtonCreate;

    regenarray[0] = &gcd[13]; regenarray[1] = &gcd[14]; regenarray[2] = GCD_Glue;
    regenarray[3] = &gcd[15]; regenarray[4] = NULL;

    boxes[12].gd.flags = gg_enabled|gg_visible;
    boxes[12].gd.u.boxelements = regenarray;
    boxes[12].creator = GHBoxCreate;
    varray2[6] = &boxes[12]; varray2[7] = NULL; varray2[8] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = varray2;
    boxes[6].creator = GHVBoxCreate;

    memset(aspects,0,sizeof(aspects));
    aspects[0].text = (unichar_t *) _("Display");
    aspects[0].text_is_1byte = true;
    aspects[0].gcd = &boxes[6];

    plabel[0].text = (unichar_t *) _("_Full Font Display");
    plabel[0].text_is_1byte = true;
    plabel[0].text_in_resource = true;
    pgcd[0].gd.label = &plabel[0];
    pgcd[0].gd.pos.x = 12; pgcd[0].gd.pos.y = 6; 
    pgcd[0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    pgcd[0].gd.cid = CID_Display;
    pgcd[0].gd.handle_controlevent = PRT_RadioSet;
    pgcd[0].gd.popup_msg = (unichar_t *) _("Displays all the glyphs in the font on a rectangular grid at the given point size");
    pgcd[0].creator = GRadioCreate;
    varray3[0][0] = GCD_HPad10; varray3[0][1] = &pgcd[0]; varray3[0][2] = GCD_Glue; varray3[0][3] = NULL;

    cnt = 1;
    if ( fv!=NULL )
	cnt = FVSelCount(fv);
    else if ( mv!=NULL )
	cnt = mv->glyphcnt;
    plabel[1].text = (unichar_t *) (cnt==1?_("Full Pa_ge Glyph"):_("Full Pa_ge Glyphs"));
    plabel[1].text_is_1byte = true;
    plabel[1].text_in_resource = true;
    pgcd[1].gd.label = &plabel[1];
    pgcd[1].gd.flags = (cnt==0 ? (gg_visible | gg_utf8_popup ): (gg_visible | gg_enabled | gg_utf8_popup));
    pgcd[1].gd.cid = CID_Chars;
    pgcd[1].gd.handle_controlevent = PRT_RadioSet;
    pgcd[1].gd.popup_msg = (unichar_t *) _("Displays all the selected characters, each on its own page, at an extremely large point size");
    pgcd[1].creator = GRadioCreate;
    varray3[1][0] = GCD_HPad10; varray3[1][1] = &pgcd[1]; varray3[1][2] = GCD_Glue; varray3[1][3] = NULL;

    plabel[2].text = (unichar_t *) (cnt==1?_("_Multi Size Glyph"):_("_Multi Size Glyphs"));
    plabel[2].text_is_1byte = true;
    plabel[2].text_in_resource = true;
    pgcd[2].gd.label = &plabel[2];
    pgcd[2].gd.flags = pgcd[1].gd.flags;
    pgcd[2].gd.cid = CID_MultiSize;
    pgcd[2].gd.handle_controlevent = PRT_RadioSet;
    pgcd[2].gd.popup_msg = (unichar_t *) _("Displays all the selected characters, at several different point sizes");
    pgcd[2].creator = GRadioCreate;

    if ( pdefs[fromwindow].pt==pt_chars && cnt==0 )
	pdefs[fromwindow].pt = pt_fontdisplay;
    pgcd[pdefs[fromwindow].pt].gd.flags |= gg_cb_on;

    varray3[2][0] = GCD_HPad10; varray3[2][1] = &pgcd[2]; varray3[2][2] = GCD_Glue; varray3[2][3] = NULL;
    varray3[3][0] = NULL;

    boxes[13].gd.flags = gg_enabled|gg_visible;
    boxes[13].gd.u.boxelements = varray3[0];
    boxes[13].creator = GHVBoxCreate;

    plabel[3].text = (unichar_t *) _("_Pointsize:");
    plabel[3].text_is_1byte = true;
    plabel[3].text_in_resource = true;
    pgcd[3].gd.label = &plabel[3];
    pgcd[3].gd.flags = gg_visible | gg_enabled;
    pgcd[3].gd.cid = CID_PSLab;
    pgcd[3].creator = GLabelCreate;
    ptarray[0] = &pgcd[3];

    sprintf(sizebuf,"%d",di.pointsize);
    plabel[4].text = (unichar_t *) sizebuf;
    plabel[4].text_is_1byte = true;
    pgcd[4].gd.label = &plabel[4];
    pgcd[4].gd.pos.width = 60;
    pgcd[4].gd.flags = gg_visible | gg_enabled;
    pgcd[4].gd.cid = CID_PointSize;
    pgcd[4].creator = GNumericFieldCreate;
    ptarray[1] = &pgcd[4]; ptarray[2] = GCD_Glue; ptarray[3] = NULL;

    boxes[8].gd.flags = gg_enabled|gg_visible;
    boxes[8].gd.u.boxelements = ptarray;
    boxes[8].creator = GHBoxCreate;

    varray4[0] = &boxes[13]; varray4[1] = &boxes[8]; varray4[2] = GCD_Glue; varray4[3] = NULL;
    boxes[9].gd.flags = gg_enabled|gg_visible;
    boxes[9].gd.u.boxelements = varray4;
    boxes[9].creator = GVBoxCreate;

    aspects[1].text = (unichar_t *) _("Print");
    aspects[1].text_is_1byte = true;
    aspects[1].gcd = &boxes[9];

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.cid = CID_TabSet;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled ;
    mlabel[1].text = (unichar_t *) _("S_etup");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = PRT_Setup;
    mgcd[1].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &mgcd[1]; barray[2] = GCD_Glue; barray[3] = NULL;

    boxes[14].gd.flags = gg_enabled|gg_visible;
    boxes[14].gd.u.boxelements = barray;
    boxes[14].creator = GHBoxCreate;

    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[2].text = (unichar_t *) _("_Print");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.mnemonic = 'O';
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = PRT_OK;
    mgcd[2].gd.cid = CID_OK;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.width = -1; mgcd[3].gd.pos.height = 0;
    mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[3].text = (unichar_t *) _("_Done");
    mlabel[3].text_is_1byte = true;
    mlabel[3].text_in_resource = true;
    mgcd[3].gd.label = &mlabel[3];
    mgcd[3].gd.cid = CID_Cancel;
    mgcd[3].gd.handle_controlevent = DSP_Done;
    mgcd[3].creator = GButtonCreate;
    barray2[0] = GCD_Glue; barray2[1] = &mgcd[2]; barray2[2] = GCD_Glue;
    barray2[3] = GCD_Glue; barray2[4] = &mgcd[3]; barray2[5] = GCD_Glue; barray2[6] = NULL;

    boxes[11].gd.flags = gg_enabled|gg_visible;
    boxes[11].gd.u.boxelements = barray2;
    boxes[11].creator = GHBoxCreate;

    varray5[0][0] = &mgcd[0]; varray5[0][1] = NULL;
    varray5[1][0] = &boxes[14]; varray5[1][1] = NULL;
    varray5[2][0] = &boxes[11]; varray5[2][1] = NULL;
    varray5[3][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray5[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(di.gw,boxes);
    GListSetSBAlwaysVisible(gcd[10].ret,true);
    GListSetPopupCallback(gcd[10].ret,MV_FriendlyFeatures);

    GTextInfoListFree(gcd[0].gd.u.list);
    DSP_SetFont(&di,true);
    SFTFSetDPI(gcd[11].ret,dpi);
    temp = PrtBuildDef(sf,gcd[11].ret,
	    (void (*)(void *, int, uint32, uint32))SFTFInitLangSys);
    GGadgetSetTitle(gcd[11].ret, temp);
    free(temp);
    SFTFRegisterCallback(gcd[11].ret,&di,DSP_ChangeFontCallback);
    SFTFProvokeCallback(gcd[11].ret);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[6].ret,1);
    GHVBoxSetExpandableCol(boxes[13].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[8].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[9].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[14].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[11].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[12].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);
    
    GDrawSetVisible(di.gw,true);
    while ( !di.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(di.gw);
    TextInfoDataFree(di.scriptlangs);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
    space = upt = galloc((max+1)*sizeof(unichar_t));
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

void ScriptPrint(FontView *fv,int type,int32 *pointsizes,char *samplefile,
	unichar_t *sample, char *outputfile) {
    PI pi;
    char buf[100];
    SFTextArea *st;

    PIInit(&pi,fv,NULL,NULL);
    if ( pointsizes!=NULL ) {
	pi.pointsizes = pointsizes;
	pi.pointsize = pointsizes[0];
    }
    pi.pt = type;
    if ( type==pt_fontsample ) {
	st = gcalloc(1,sizeof(SFTextArea));
	st->multi_line = true;
	st->accepts_returns = true;
	st->wrap = true;
	st->dpi = printdpi;
	st->ps = -1;
	st->g.inner.width = st->g.inner.width = (pagewidth-1*72)*printdpi/72;
	st->g.funcs = &sftextarea_funcs;
	SFMapOfSF(st,fv->sf);

	if ( samplefile!=NULL && *samplefile!='\0' )
	    sample = FileToUString(samplefile,65536);
	if ( sample==NULL )
	    sample = PrtBuildDef(pi.mainsf,st,(void (*)(void *, int, uint32, uint32))SFTFInitLangSys);
	else
	    SFTFInitLangSys(&st->g,u_strlen(sample),DEFAULT_SCRIPT,DEFAULT_LANG);
	GGadgetSetTitle(&st->g, sample);
	pi.sample = st;
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
	    gwwv_post_error(_("Print Failed"),_("Failed to open file %s for output"), outputfile);
return;
	}
    } else {
	outputfile = NULL;
	pi.out = tmpfile();
	if ( pi.out==NULL ) {
	    gwwv_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return;
	}
    }

    DoPrinting(&pi,outputfile);

    if ( pi.pt==pt_fontsample )
	GGadgetDestroy(&pi.sample->g);

}
