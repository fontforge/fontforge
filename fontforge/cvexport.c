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

#include "cvexport.h"

#include "autohint.h"
#include "bvedit.h"
#include "dumppfa.h"
#include "fontforgevw.h"
#include "spiro.h"
#include <math.h>
#include <locale.h>
#include <string.h>
#include "gfile.h"
#include <time.h>
#include "ustring.h"
#include "gio.h"
#include "gicons.h"
#include "print.h"	/* For pdf output routines */
#include <utype.h>

static void EpsGeneratePreview(FILE *eps,SplineChar *sc,int layer,DBounds *b) {
    double scale, temp;
    int pixelsize, depth;
    BDFChar *bdfc;
    int i,j;

    /* Try for a preview that fits within a 72x72 box */
    if ( b->maxx==b->minx || b->maxy==b->miny )
return;
    scale = 72.0/(b->maxx-b->minx);
    temp = 72.0/(b->maxy-b->miny);
    if ( temp<scale ) scale = temp;
    pixelsize = rint((sc->parent->ascent+sc->parent->descent)*scale);
    scale = pixelsize/(double) (sc->parent->ascent+sc->parent->descent);

    depth = 4;
    bdfc = SplineCharFreeTypeRasterizeNoHints(sc,layer,pixelsize,72,4);
    if ( bdfc==NULL )
	bdfc = SplineCharAntiAlias(sc,layer,pixelsize,4);
    if ( bdfc==NULL )
return;

    fprintf(eps,"%%%%BeginPreview: %d %d %d %d\n",
	    bdfc->xmax-bdfc->xmin+1, bdfc->ymax-bdfc->ymin+1, depth, bdfc->ymax-bdfc->ymin+1 );
    for ( i=0; i<=bdfc->ymax-bdfc->ymin; ++i ) {
	putc('%',eps);
	for ( j=0; j<=bdfc->xmax-bdfc->xmin; ++j )
	    fprintf(eps,"%X",bdfc->bitmap[i*bdfc->bytes_per_line+j]);
	if ( !((bdfc->xmax-bdfc->xmin)&1) )
	    putc('0',eps);
	putc('\n',eps);
    }
    BDFCharFree(bdfc);
    fprintf(eps,"%%%%EndPreview\n" );
}

int _ExportEPS(FILE *eps,SplineChar *sc, int layer, int preview) {
    DBounds b;
    time_t now;
    struct tm *tm;
    int ret;
    const char *author = GetAuthor();

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.

    fprintf( eps, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
    SplineCharLayerFindBounds(sc,layer,&b);
    fprintf( eps, "%%%%BoundingBox: %g %g %g %g\n", (double) b.minx, (double) b.miny, (double) b.maxx, (double) b.maxy );
    fprintf( eps, "%%%%Pages: 0\n" );
    fprintf( eps, "%%%%Title: %s from %s\n", sc->name, sc->parent->fontname );
    fprintf( eps, "%%%%Creator: FontForge\n" );
    if ( author!=NULL )
	fprintf( eps, "%%%%Author: %s\n", author);
    time(&now);
    tm = localtime(&now);
    fprintf( eps, "%%%%CreationDate: %d:%02d %d-%d-%d\n", tm->tm_hour, tm->tm_min,
	    tm->tm_mday, tm->tm_mon+1, 1900+tm->tm_year );
    if ( sc->parent->multilayer ) {
	int ly, had_grad=0, had_pat=0;
	for ( ly=ly_fore; ly<sc->layer_cnt; ++ly ) {
	    if ( sc->layers[ly].fill_brush.gradient!=NULL || sc->layers[ly].stroke_pen.brush.gradient!=NULL ) {
		had_grad = true;
	break;
	    }
	    if ( sc->layers[ly].fill_brush.gradient!=NULL || sc->layers[ly].stroke_pen.brush.gradient!=NULL )
		had_pat = true;
	}
	if ( had_grad )
	    fprintf( eps, "%%%%LanguageLevel: 3\n" );
	else if ( had_pat )
	    fprintf( eps, "%%%%LanguageLevel: 2\n" );
    }
    fprintf( eps, "%%%%EndComments\n" );
    if ( preview )
	EpsGeneratePreview(eps,sc,layer,&b);
    fprintf( eps, "%%%%EndProlog\n" );
    fprintf( eps, "%%%%Page \"%s\" 1\n", sc->name );

    fprintf( eps, "gsave newpath\n" );
    SC_PSDump((void (*)(int,void *)) fputc,eps,sc,true,false,layer);
    if ( sc->parent->multilayer )
	fprintf( eps, "grestore\n" );
    else
    if ( sc->parent->strokedfont )
	fprintf( eps, "%g setlinewidth stroke grestore\n", (double) sc->parent->strokewidth );
    else
	fprintf( eps, "fill grestore\n" );
    fprintf( eps, "%%%%EOF\n" );
    ret = !ferror(eps);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( ret );
}

int ExportEPS(char *filename,SplineChar *sc,int layer) {
    FILE *eps;
    int ret;

    eps = fopen(filename,"w");
    if ( eps==NULL ) {
return(0);
    }
    ret = _ExportEPS(eps,sc,layer,true);
    fclose(eps);
return( ret );
}

int _ExportPDF(FILE *pdf,SplineChar *sc,int layer) {
/* TODO: Note, maybe this routine can be combined with print.c dump_pdfprologue() */
    DBounds b;
    time_t now;
    struct tm *tm;
    int ret;
    char oldloc[24];
    int _objlocs[8], xrefloc, streamstart, streamlength, resid = 0, nextobj;
    int *objlocs = _objlocs;
    const char *author = GetAuthor();
    int i;

    SFUntickAll(sc->parent);
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.

    fprintf( pdf, "%%PDF-1.4\n%%\201\342\202\203\n" );	/* Header comment + binary comment */
    /* Every document contains a catalog which points to a page tree, which */
    /*  in our case, points to a single page */
    objlocs[1] = ftell(pdf);
    fprintf( pdf, "1 0 obj\n << /Type /Catalog\n    /Pages 2 0 R\n    /PageMode /UseNone\n >>\nendobj\n" );
    objlocs[2] = ftell(pdf);
    fprintf( pdf, "2 0 obj\n << /Type /Pages\n    /Kids [ 3 0 R ]\n    /Count 1\n >>\nendobj\n" );
    /* And our single page points to its contents */
    objlocs[3] = ftell(pdf);
    fprintf( pdf, "3 0 obj\n" );
    fprintf( pdf, " << /Type /Page\n" );
    fprintf( pdf, "    /Parent 2 0 R\n" );
    fprintf( pdf, "    /Resources " );
    if ( sc->parent->multilayer ) {
	resid = ftell(pdf);
	fprintf( pdf, "000000 0 R\n" );
    } else
	fprintf( pdf, "<< >>\n" );
    SplineCharLayerFindBounds(sc,layer,&b);
    fprintf( pdf, "    /MediaBox [%g %g %g %g]\n", (double) b.minx, (double) b.miny, (double) b.maxx, (double) b.maxy );
    fprintf( pdf, "    /Contents 4 0 R\n" );
    fprintf( pdf, " >>\n" );
    fprintf( pdf, "endobj\n" );
    /* And the contents are the interesting stuff */
    objlocs[4] = ftell(pdf);
    fprintf( pdf, "4 0 obj\n" );
    fprintf( pdf, " << /Length 5 0 R >> \n" );
    fprintf( pdf, " stream \n" );
    streamstart = ftell(pdf);
    SC_PSDump((void (*)(int,void *)) fputc,pdf,sc,true,true,layer);
    if ( sc->parent->multilayer )
	/* Already filled or stroked */;
    else if ( sc->parent->strokedfont )
	fprintf( pdf, "%g w S\n", (double) sc->parent->strokewidth );
    else
	fprintf( pdf, "f\n" );
    streamlength = ftell(pdf)-streamstart;
    fprintf( pdf, " endstream\n" );
    fprintf( pdf, "endobj\n" );
    objlocs[5] = ftell(pdf);
    fprintf( pdf, "5 0 obj\n" );
    fprintf( pdf, " %d\n", (int) streamlength );
    fprintf( pdf, "endobj\n" );

    /* Optional Info dict */
    objlocs[6] = ftell(pdf);
    fprintf( pdf, "6 0 obj\n" );
    fprintf( pdf, " <<\n" );
    fprintf( pdf, "    /Creator (FontForge)\n" );
    time(&now);
    tm = localtime(&now);
    fprintf( pdf, "    /CreationDate (D:%04d%02d%02d%02d%02d%02d",
	    1900+tm->tm_year, tm->tm_mon+1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec );
#ifdef _NO_TZSET
    fprintf( pdf, "Z)\n" );
#else
    tzset();
    if ( timezone==0 )
	fprintf( pdf, "Z)\n" );
    else {
	if ( timezone<0 ) /* fprintf bug - this is a kludge to print +/- in front of a %02d-padded value */
	    fprintf( pdf, "-" );
	else
	    fprintf( pdf, "+" );
	fprintf( pdf, "%02d'%02d')\n", (int)(timezone/3600),(int)(timezone/60-(timezone/3600)*60) );
    }
#endif
    fprintf( pdf, "    /Title (%s from %s)\n", sc->name, sc->parent->fontname );
    if ( author!=NULL )
	fprintf( pdf, "    /Author (%s)\n", author );
    fprintf( pdf, " >>\n" );

    nextobj = 7;
    if ( sc->parent->multilayer ) {
	PI pi;
	int resobj;
	memset(&pi,0,sizeof(pi));
	pi.out = pdf;
	pi.max_object = 100;
	pi.object_offsets = malloc(pi.max_object*sizeof(int));
	memcpy(pi.object_offsets,objlocs,nextobj*sizeof(int));
	pi.next_object = nextobj;
	resobj = PdfDumpGlyphResources(&pi,sc);
	nextobj = pi.next_object;
	objlocs = pi.object_offsets;
	fseek(pdf,resid,SEEK_SET);
	fprintf(pdf,"%06d", resobj );
	fseek(pdf,0,SEEK_END);
    }

    xrefloc = ftell(pdf);
    fprintf( pdf, "xref\n" );
    fprintf( pdf, " 0 %d\n", nextobj );
    fprintf( pdf, "0000000000 65535 f \n" );
    for ( i=1; i<nextobj; ++i )
	fprintf( pdf, "%010d %05d n \n", (int) objlocs[i], 0 );
    fprintf( pdf, "trailer\n" );
    fprintf( pdf, " <<\n" );
    fprintf( pdf, "    /Size %d\n", nextobj );
    fprintf( pdf, "    /Root 1 0 R\n" );
    fprintf( pdf, "    /Info 6 0 R\n" );
    fprintf( pdf, " >>\n" );
    fprintf( pdf, "startxref\n" );
    fprintf( pdf, "%d\n", (int) xrefloc );
    fprintf( pdf, "%%%%EOF\n" );

    if ( objlocs!=_objlocs )
	free(objlocs);

    ret = !ferror(pdf);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( ret );
}

int ExportPDF(char *filename,SplineChar *sc,int layer) {
    FILE *eps;
    int ret;

    eps = fopen(filename,"w");
    if ( eps==NULL ) {
return(0);
    }
    ret = _ExportPDF(eps,sc,layer);
    fclose(eps);
return( ret );
}


int _ExportPlate(FILE *plate,SplineChar *sc,int layer) {
    char oldloc[24];
    int do_open;
    SplineSet *ss;
    spiro_cp *spiros;
    int i, ret;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    /* Output closed contours first, then open. Plate files can only handle */
    /*  one open contour (I think) and it must be at the end */
    fprintf( plate, "(plate\n" );
    for ( do_open=0; do_open<2; ++do_open ) {
	for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	    if ( ss->first->prev==NULL ) {
		if ( !do_open || ss->first->next==NULL )
	continue;
	    } else {
		if ( do_open )
	continue;
	    }
	    spiros = ss->spiros;
	    if ( ss->spiro_cnt==0 )
		spiros = SplineSet2SpiroCP(ss,NULL);
	    for ( i=0; spiros[i].ty!='z'; ++i ) {
		if ( spiros[i].ty==SPIRO_OPEN_CONTOUR )
		    fprintf( plate, "  (o " );
		else
		    fprintf( plate, "  (%c ", spiros[i].ty&~0x80 );
		/* Raph's plate files have the baseline way up in the air */
		fprintf( plate, "%g %g)\n", spiros[i].x, 800.-spiros[i].y );
	    }
	    if ( ss->first->prev!=NULL )
		fprintf( plate, "  (z)\n" );
	    if ( spiros!=ss->spiros )
		free(spiros);
	}
    }
    fprintf(plate, ")\n");
    ret = !ferror(plate);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( ret );
}

int ExportPlate(char *filename,SplineChar *sc,int layer) {
    FILE *plate;
    int ret;

    plate = fopen(filename,"w");
    if ( plate==NULL ) {
return(0);
    }
    ret = _ExportPlate(plate,sc,layer);
    fclose(plate);
return( ret );
}

int ExportSVG(char *filename,SplineChar *sc,int layer) {
    FILE *svg;
    int ret;

    svg = fopen(filename,"w");
    if ( svg==NULL ) {
return(0);
    }
    ret = _ExportSVG(svg,sc,layer);
    fclose(svg);
return( ret );
}

int ExportGlif(char *filename,SplineChar *sc,int layer) {
    FILE *glif;
    int ret;

    glif = fopen(filename,"w");
    if ( glif==NULL ) {
return(0);
    }
    ret = _ExportGlif(glif,sc,layer);
return( ret );
}

static void FigDumpPt(FILE *fig,BasePoint *me,real scale,real ascent) {
    fprintf( fig, "%d %d ", (int) rint(me->x*scale), (int) rint(ascent-me->y*scale));
}

static void FigSplineSet(FILE *fig,SplineSet *spl,int spmax, int asc) {
    SplinePoint *sp;
    int cnt;
    real scale = 7*1200.0/spmax;
    real ascent = 11*1200*asc/spmax;

    while ( spl!=NULL ) {
	/* type=3, SPline; sub_type=3, closed interpreted; linestyle=0(solid); thickness=1*/
	/*  colors (pen,fill)=0, black; depth=0(stacking order); pen_style(unused)*/
	/*  area_fill=-1 (no fill) style_val (0.0 no dashes), capstyle=0 (don't care) */
	/*  forward arrow=0 (none) backarrow (none); point count */
	cnt=0;
	sp = spl->first;
	while ( 1 ) {
	    ++cnt;
	    if ( !sp->noprevcp && sp->prev!=NULL ) ++cnt;
	    if ( !sp->nonextcp && sp->next!=NULL ) ++cnt;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    ++cnt;
	}
	fprintf( fig, "3 %d 0 1 0 0 0 0 -1 0.0 0 0 0 %d\n",
		spl->first->prev==NULL?4:5, cnt );
	/* line of coordinates pairs */
	sp = spl->first;
	putc('\t',fig);
	while ( 1 ) {
	    if ( !sp->noprevcp && sp->prev!=NULL && sp!=spl->first )
		FigDumpPt(fig,&sp->prevcp,scale,ascent);
	    FigDumpPt(fig,&sp->me,scale,ascent);
	    if ( !sp->nonextcp && sp->next!=NULL )
		FigDumpPt(fig,&sp->nextcp,scale,ascent);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    if ( !sp->noprevcp && sp->prev!=NULL )
		FigDumpPt(fig,&sp->prevcp,scale,ascent);
	    FigDumpPt(fig,&sp->me,scale,ascent);
	}
	/* line of "shape factors", 0=> corner */
	putc('\n',fig);
	sp = spl->first;
	putc('\t',fig);
	while ( 1 ) {
	    if ( !sp->noprevcp && sp->prev!=NULL && sp!=spl->first )
		fprintf(fig,"1 ");
	    if (( sp->noprevcp && sp->nonextcp ) || sp->pointtype==pt_corner )
		fprintf(fig,"0 ");
	    else
		fprintf(fig,"-1 ");
	    if ( !sp->nonextcp && sp->next!=NULL )
		fprintf(fig,"1 ");
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    if ( !sp->noprevcp && sp->prev!=NULL )
		fprintf(fig,"1 ");
	    if (( sp->noprevcp && sp->nonextcp ) || sp->pointtype==pt_corner )
		fprintf(fig,"0 ");
	    else
		fprintf(fig,"-1 ");
	}
	putc('\n',fig);
	spl=spl->next;
    }
}

int ExportFig(char *filename,SplineChar *sc,int layer) {
    FILE *fig;
    RefChar *rf;
    int ret;
    int spmax = sc->parent->ascent+sc->parent->descent;
    /* This is by no means perfect. but it is a reasonable approximation */

    fig = fopen(filename,"w");
    if ( fig==NULL ) {
return(0);
    }

    fprintf( fig, "#FIG 3.2\n" );
    fprintf( fig, "Portrait\n" );
    fprintf( fig, "Center\n" );
    fprintf( fig, "Inches\n" );
    fprintf( fig, "Letter\n" );
    fprintf( fig, "100.00\n" );
    fprintf( fig, "Single\n" );
    fprintf( fig, "-2\n" );
    fprintf( fig, "1200 2\n" );
    FigSplineSet(fig,sc->layers[layer].splines,spmax,sc->parent->ascent);
    for ( rf=sc->layers[layer].refs; rf!=NULL; rf=rf->next )
	FigSplineSet(fig,rf->layers[0].splines, spmax, sc->parent->ascent );
    ret = !ferror(fig);
    fclose(fig);
return( ret );
}

int ExportImage(char *filename,SplineChar *sc, int layer, int format, int pixelsize, int bitsperpixel) {
/* 0=*.xbm, 1=*.bmp, 2=*.png, 3=*.xpm, 4=*.c(fontforge-internal) */
    struct _GImage base;
    GImage gi;
    GClut clut;
    BDFChar *bdfc;
    int ret;
    int tot, i;
    uint8 *pt, *end;
    int scale;
    void *freetypecontext;
    double emsize = sc->parent->ascent+sc->parent->descent;

    if ( autohint_before_generate && sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);

    memset(&gi,'\0', sizeof(gi));
    memset(&base,'\0', sizeof(base));
    memset(&clut,'\0', sizeof(clut));
    gi.u.image = &base;

    if ( bitsperpixel==1 ) {
	if ( (freetypecontext = FreeTypeFontContext(sc->parent,sc,NULL,layer))==NULL )
	    bdfc = SplineCharRasterize(sc,layer,pixelsize);
	else {
	    bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,pixelsize,72,1);
	    FreeTypeFreeContext(freetypecontext);
	}
	BCRegularizeBitmap(bdfc);
	/* People don't seem to like having a minimal bounding box for their */
	/*  images. */
	BCExpandBitmapToEmBox(bdfc,
		0,
		(int) rint(sc->parent->ascent*pixelsize/emsize)-pixelsize,
		(int) rint(sc->width*pixelsize/emsize),
		(int) rint(sc->parent->ascent*pixelsize/emsize));

	/* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
	tot = bdfc->bytes_per_line*(bdfc->ymax-bdfc->ymin+1);
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base.image_type = it_mono;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.trans = -1;
	if ( format==0 )
	    ret = !GImageWriteXbm(&gi,filename);
#ifndef _NO_LIBPNG
	else if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
#endif
	else if ( format==3 )
	    ret = !GImageWriteXpm(&gi,filename);
	else if ( format==4 )
	    ret = !GImageWriteGImage(&gi,filename);
	else
	    ret = GImageWriteBmp(&gi,filename);
	BDFCharFree(bdfc);
    } else {
	if ( (freetypecontext = FreeTypeFontContext(sc->parent,sc,NULL,layer))==NULL )
	    bdfc = SplineCharAntiAlias(sc,pixelsize,layer,(1<<(bitsperpixel/2)));
	else {
	    bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,pixelsize,72,bitsperpixel);
	    FreeTypeFreeContext(freetypecontext);
	}
	BCRegularizeGreymap(bdfc);
	BCExpandBitmapToEmBox(bdfc,
		0,
		(int) rint(sc->parent->ascent*pixelsize/emsize) - pixelsize,
		(int) rint(sc->width*pixelsize/emsize),
		(int) rint(sc->parent->ascent*pixelsize/emsize));
	base.image_type = it_index;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.clut = &clut;
	base.trans = -1;
	clut.clut_len = 1<<bitsperpixel;
	clut.is_grey = true;
	clut.trans_index = -1;
	scale = 255/((1<<bitsperpixel)-1);
	scale = COLOR_CREATE(scale,scale,scale);
	for ( i=0; i< 1<<bitsperpixel; ++i )
	    clut.clut[(1<<bitsperpixel)-1 - i] = i*scale;
#ifndef _NO_LIBPNG
	if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
	else
#endif
	    ret = GImageWriteBmp(&gi,filename);
	BDFCharFree(bdfc);
    }
return( ret );
}

int BCExportXBM(char *filename,BDFChar *bdfc, int format) {
    struct _GImage base;
    GImage gi;
    GClut clut;
    int ret;
    int tot;
    int scale, i;
    uint8 *pt, *end;

    memset(&gi,'\0', sizeof(gi));
    memset(&base,'\0', sizeof(base));
    gi.u.image = &base;

    if ( !bdfc->byte_data ) {
	BCRegularizeBitmap(bdfc);
	/* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
	tot = bdfc->bytes_per_line*(bdfc->ymax-bdfc->ymin+1);
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base.image_type = it_mono;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.trans = -1;
	if ( format==0 )
	    ret = !GImageWriteXbm(&gi,filename);
#ifndef _NO_LIBPNG
	else if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
#endif
	else if ( format==3 )
	    ret = !GImageWriteXpm(&gi,filename);
	else if ( format==4 )
	    ret = !GImageWriteGImage(&gi,filename);
	else
	    ret = GImageWriteBmp(&gi,filename);
	/* And back to normal */
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );
    } else {
	BCRegularizeGreymap(bdfc);
	base.image_type = it_index;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.clut = &clut;
	clut.clut_len = 1<<bdfc->depth;
	clut.is_grey = true;
	clut.trans_index = base.trans = -1;
	scale = 255/((1<<bdfc->depth)-1);
	scale = COLOR_CREATE(scale,scale,scale);
	for ( i=0; i< 1<<bdfc->depth; ++i )
	    clut.clut[(1<<bdfc->depth)-1 - i] = i*scale;
#ifndef _NO_LIBPNG
	if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
	else
#endif
	    ret = GImageWriteBmp(&gi,filename);
    }
return( ret );
}

static void MakeExportName(char *buffer, int blen,char *format_spec,
	SplineChar *sc, EncMap *map) {
    char *end = buffer+blen-3;
    char *pt, *bend;
    char unicode[8];
    int ch;

    while ( *format_spec && buffer<end ) {
	if ( *format_spec!='%' )
	    *buffer++ = *format_spec++;
	else {
	    ++format_spec;
	    ch = *format_spec++;
	    if ((bend = buffer+40)>end ) bend = end;
	    if ( ch=='n' ) {
#if defined( __CygWin ) || defined(__Mac)
		/* Windows file systems are not case conscious */
		/*  nor is the default mac filesystem */
		for ( pt=sc->name; *pt!='\0' && buffer<bend; ) {
		    if ( isupper( *pt ))
			*buffer++ = '$';
		    *buffer++ = *pt++;
		}
#else
		for ( pt=sc->name; *pt!='\0' && buffer<bend; )
		    *buffer++ = *pt++;
#endif
	    } else if ( ch=='f' ) {
		for ( pt=sc->parent->fontname; *pt!='\0' && buffer<bend; )
		    *buffer++ = *pt++;
	    } else if ( ch=='u' || ch=='U' ) {
		if ( sc->unicodeenc == -1 )
		    strcpy(unicode,"xxxx");
		else
		    sprintf( unicode,ch=='u' ? "%04x" : "%04X", sc->unicodeenc );
		for ( pt=unicode; *pt!='\0' && buffer<bend; )
		    *buffer++ = *pt++;
	    } else if ( ch=='e' ) {
		sprintf( unicode,"%d", (int) map->backmap[sc->orig_pos] );
		for ( pt=unicode; *pt!='\0' && buffer<bend; )
		    *buffer++ = *pt++;
	    } else
		*buffer++ = ch;
	}
    }
    *buffer = '\0';
}

void ScriptExport(SplineFont *sf, BDFFont *bdf, int format, int gid,
	char *format_spec,EncMap *map) {
    char buffer[100];
    SplineChar *sc = sf->glyphs[gid];
    BDFChar *bc = bdf!=NULL ? bdf->glyphs[gid] : NULL;
    int good=true;

    if ( sc==NULL )
return;

    MakeExportName(buffer,sizeof(buffer),format_spec,sc,map);

    if ( format==0 )
	good = ExportEPS(buffer,sc,ly_fore);
    else if ( format==1 )
	good = ExportFig(buffer,sc,ly_fore);
    else if ( format==2 )
	good = ExportSVG(buffer,sc,ly_fore);
    else if ( format==3 )
	good = ExportGlif(buffer,sc,ly_fore);
    else if ( format==4 )
	good = ExportPDF(buffer,sc,ly_fore);
    else if ( format==5 )
	good = ExportPlate(buffer,sc,ly_fore);
    else if ( bc!=NULL )
	good = BCExportXBM(buffer,bc,format-6);
    if ( !good )
	ff_post_error(_("Save Failed"),_("Save Failed"));
}
