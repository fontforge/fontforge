/* Copyright (C) 2003-2012 by George Williams */
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
#include "autohint.h"
#include "fontforgevw.h"
#include <unistd.h>
#include <math.h>
#include <locale.h>
#include <utype.h>
#include <chardata.h>
#include <ustring.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sd.h"

/* ************************************************************************** */
/* ****************************    SVG Output    **************************** */
/* ************************************************************************** */

static void latin1ToUtf8Out(FILE *file,char *str) {
    /* beware of characters above 0x80, also &, <, > (things that are magic for xml) */
    while ( *str ) {
	if ( *str=='&' || *str=='<' || *str=='>' || (*str&0x80) )
	    fprintf( file, "&#%d;", (uint8) *str);
	else
	    putc(*str,file);
	++str;
    }
}

static int svg_outfontheader(FILE *file, SplineFont *sf,int layer) {
    int defwid = SFFigureDefWidth(sf,NULL);
    struct pfminfo info;
    static const char *condexp[] = { "squinchy", "ultra-condensed", "extra-condensed",
	"condensed", "semi-condensed", "normal", "semi-expanded", "expanded",
	"extra-expanded", "ultra-expanded", "broad" };
    DBounds bb;
    BlueData bd;
    char *hash, *hasv, ch;
    int minu, maxu, i;
    const char *author = GetAuthor();

    memset(&info,0,sizeof(info));
    SFDefaultOS2Info(&info,sf,sf->fontname);
    SplineFontLayerFindBounds(sf,layer,&bb);
    QuickBlues(sf,layer,&bd);

    fprintf( file, "<?xml version=\"1.0\" standalone=\"no\"?>\n" );
    fprintf( file, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n" );
    if ( sf->comments!=NULL ) {
	fprintf( file, "<!--\n" );
	latin1ToUtf8Out(file,sf->comments);
	fprintf( file, "\n-->\n" );
    }
    fprintf( file, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\">\n" );
    fprintf( file, "<metadata>\nCreated by FontForge %d at %s",
	    FONTFORGE_VERSIONDATE_RAW, ctime((time_t*)&sf->modificationtime) );
    if ( author!=NULL )
	fprintf(file," By %s\n", author);
    else
	fprintf(file,"\n" );
    if ( sf->copyright!=NULL ) {
	latin1ToUtf8Out(file,sf->copyright);
	putc('\n',file);
    }
    fprintf( file, "</metadata>\n" );
    fprintf( file, "<defs>\n" );
    fprintf( file, "<font id=\"");
    latin1ToUtf8Out(file, sf->fontname);
    fprintf(file, "\" horiz-adv-x=\"%d\" ", defwid );
    if ( sf->hasvmetrics )
	fprintf( file, "vert-adv-y=\"%d\" ", sf->ascent+sf->descent );
    putc('>',file); putc('\n',file);
    fprintf( file, "  <font-face \n" );
    fprintf( file, "    font-family=\"");
    latin1ToUtf8Out(file, sf->familyname_with_timestamp ? sf->familyname_with_timestamp : sf->familyname );
    fprintf( file, "\"\n");
    fprintf( file, "    font-weight=\"%d\"\n", info.weight );
    if ( strstrmatch(sf->fontname,"obli") || strstrmatch(sf->fontname,"slanted") )
	fprintf( file, "    font-style=\"oblique\"\n" );
    else if ( MacStyleCode(sf,NULL)&sf_italic )
	fprintf( file, "    font-style=\"italic\"\n" );
    if ( strstrmatch(sf->fontname,"small") || strstrmatch(sf->fontname,"cap") )
	fprintf( file, "    font-variant=\"small-caps\"\n" );
    fprintf( file, "    font-stretch=\"%s\"\n", condexp[info.width]);
    fprintf( file, "    units-per-em=\"%d\"\n", sf->ascent+sf->descent );
    fprintf( file, "    panose-1=\"%d %d %d %d %d %d %d %d %d %d\"\n", info.panose[0],
	info.panose[1], info.panose[2], info.panose[3], info.panose[4], info.panose[5],
	info.panose[6], info.panose[7], info.panose[8], info.panose[9]);
    fprintf( file, "    ascent=\"%d\"\n", sf->ascent );
    fprintf( file, "    descent=\"%d\"\n", -sf->descent );
    if ( bd.xheight>0 )
	fprintf( file, "    x-height=\"%g\"\n", (double) bd.xheight );
    if ( bd.caph>0 )
	fprintf( file, "    cap-height=\"%g\"\n", (double) bd.caph );
    fprintf( file, "    bbox=\"%g %g %g %g\"\n", (double) bb.minx, (double) bb.miny, (double) bb.maxx, (double) bb.maxy );
    fprintf( file, "    underline-thickness=\"%g\"\n", (double) sf->uwidth );
    fprintf( file, "    underline-position=\"%g\"\n", (double) sf->upos );
    if ( sf->italicangle!=0 )
	fprintf(file, "    slope=\"%g\"\n", (double) sf->italicangle );
    hash = PSDictHasEntry(sf->private,"StdHW");
    hasv = PSDictHasEntry(sf->private,"StdVW");
    if ( hash!=NULL ) {
	if ( *hash=='[' ) ++hash;
	ch = hash[strlen(hash)-1];
	if ( ch==']' ) hash[strlen(hash)-1] = '\0';
	fprintf(file, "    stemh=\"%s\"\n", hash );
	if ( ch==']' ) hash[strlen(hash)] = ch;
    }
    if ( hasv!=NULL ) {
	if ( *hasv=='[' ) ++hasv;
	ch = hasv[strlen(hasv)-1];
	if ( ch==']' ) hasv[strlen(hasv)-1] = '\0';
	fprintf(file, "    stemv=\"%s\"\n", hasv );
	if ( ch==']' ) hasv[strlen(hasv)] = ch;
    }
    minu = 0x7fffff; maxu = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->unicodeenc>0 ) {
	if ( sf->glyphs[i]->unicodeenc<minu ) minu = sf->glyphs[i]->unicodeenc;
	if ( sf->glyphs[i]->unicodeenc>maxu ) maxu = sf->glyphs[i]->unicodeenc;
    }
    if ( maxu!=0 )
	fprintf(file, "    unicode-range=\"U+%04X-%04X\"\n", minu, maxu );
    fprintf( file, "  />\n" );
return( defwid );
}

static int svg_pathdump(FILE *file, SplineSet *spl, int lineout,
	int forceclosed, int do_clips) {
    BasePoint last;
    char buffer[85];
    int closed=false;
    Spline *sp, *first;
    /* as I see it there is nothing to be gained by optimizing out the */
    /* command characters, since they just have to be replaced by spaces */
    /* so I don't bother to */

    last.x = last.y = 0;
    while ( spl!=NULL ) {
      if ( (do_clips && spl->is_clip_path) || (!do_clips && !spl->is_clip_path)) {
	sprintf( buffer, "M%g %g", (double) spl->first->me.x, (double) spl->first->me.y );
	if ( lineout+strlen(buffer)>=255 ) { putc('\n',file); lineout = 0; }
	fputs( buffer,file );
	lineout += strlen( buffer );
	last = spl->first->me;
	closed = false;

	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    if ( first==NULL ) first=sp;
	    if ( sp->knownlinear ) {
		if ( sp->to->me.x==sp->from->me.x )
		    sprintf( buffer,"v%g", (double) (sp->to->me.y-last.y) );
		else if ( sp->to->me.y==sp->from->me.y )
		    sprintf( buffer,"h%g", (double) (sp->to->me.x-last.x) );
		else if ( sp->to->next==first ) {
		    strcpy( buffer, "z");
		    closed = true;
		} else
		    sprintf( buffer,"l%g %g", (double) (sp->to->me.x-last.x), (double) (sp->to->me.y-last.y) );
	    } else if ( sp->order2 ) {
		if ( sp->from->prev!=NULL && sp->from!=spl->first &&
			sp->from->me.x-sp->from->prevcp.x == sp->from->nextcp.x-sp->from->me.x &&
			sp->from->me.y-sp->from->prevcp.y == sp->from->nextcp.y-sp->from->me.y )
		    sprintf( buffer,"t%g %g", (double) (sp->to->me.x-last.x), (double) (sp->to->me.y-last.y) );
		else
		    sprintf( buffer,"q%g %g %g %g",
			    (double) (sp->to->prevcp.x-last.x), (double) (sp->to->prevcp.y-last.y),
			    (double) (sp->to->me.x-last.x),(double) (sp->to->me.y-last.y));
	    } else {
		if ( sp->from->prev!=NULL && sp->from!=spl->first &&
			sp->from->me.x-sp->from->prevcp.x == sp->from->nextcp.x-sp->from->me.x &&
			sp->from->me.y-sp->from->prevcp.y == sp->from->nextcp.y-sp->from->me.y )
		    sprintf( buffer,"s%g %g %g %g",
			    (double) (sp->to->prevcp.x-last.x), (double) (sp->to->prevcp.y-last.y),
			    (double) (sp->to->me.x-last.x),(double) (sp->to->me.y-last.y));
		else
		    sprintf( buffer,"c%g %g %g %g %g %g",
			    (double) (sp->from->nextcp.x-last.x), (double) (sp->from->nextcp.y-last.y),
			    (double) (sp->to->prevcp.x-last.x), (double) (sp->to->prevcp.y-last.y),
			    (double) (sp->to->me.x-last.x),(double) (sp->to->me.y-last.y));
	    }
	    if ( lineout+strlen(buffer)>=255 ) { putc('\n',file); lineout = 0; }
	    fputs( buffer,file );
	    lineout += strlen( buffer );
	    last = sp->to->me;
	}
	if ( !closed && (forceclosed || spl->first->prev!=NULL) ) {
	    if ( lineout>=254 ) { putc('\n',file); lineout=0; }
	    putc('z',file);
	    ++lineout;
	}
      }
      spl = spl->next;
    }
return( lineout );
}

static void svg_dumpstroke(FILE *file, struct pen *cpen, struct pen *fallback,
	const char *scname, SplineChar *nested, int layer, int istop) {
    static const char *joins[] = { "miter", "round", "bevel", "inherit", NULL };
    static const char *caps[] = { "butt", "round", "square", "inherit", NULL };
    struct pen pen;

    pen = *cpen;
    if ( fallback!=NULL ) {
	if ( pen.brush.col == COLOR_INHERITED ) pen.brush.col = fallback->brush.col;
	if ( pen.brush.opacity <0 ) pen.brush.opacity = fallback->brush.opacity;
	if ( pen.width == WIDTH_INHERITED ) pen.width = fallback->width;
	if ( pen.linecap == lc_inherited ) pen.linecap = fallback->linecap;
	if ( pen.linejoin == lj_inherited ) pen.linejoin = fallback->linejoin;
	if ( pen.dashes[0]==0 && pen.dashes[1]==DASH_INHERITED )
	    memcpy(pen.dashes,fallback->dashes,sizeof(pen.dashes));
    }

    if ( pen.brush.gradient!=NULL ) {
	fprintf( file, "stroke=\"url(#%s", scname );
	if ( nested!=NULL )
	    fprintf( file, "-%s", nested->name );
	fprintf( file, "-ly%d-stroke-grad)\" ", layer );
    } else if ( pen.brush.pattern!=NULL && istop ) {
	fprintf( file, "stroke=\"url(#%s", scname );
	if ( nested!=NULL )
	    fprintf( file, "-%s", nested->name );
	fprintf( file, "-ly%d-stroke-pattern)\" ", layer );
    } else {
	if ( pen.brush.col!=COLOR_INHERITED )
	    fprintf( file, "stroke=\"#%02x%02x%02x\" ",
		    COLOR_RED(pen.brush.col), COLOR_GREEN(pen.brush.col), COLOR_BLUE(pen.brush.col));
	else
	    fprintf( file, "stroke=\"currentColor\" " );
	if ( pen.brush.opacity>=0 )
	    fprintf( file, "stroke-opacity=\"%g\" ", (double)pen.brush.opacity);
    }
    if ( pen.width!=WIDTH_INHERITED )
	fprintf( file, "stroke-width=\"%g\" ", (double)pen.width );
    if ( pen.linecap!=lc_inherited )
	fprintf( file, "stroke-linecap=\"%s\" ", caps[pen.linecap] );
    if ( pen.linejoin!=lc_inherited )
	fprintf( file, "stroke-linejoin=\"%s\" ", joins[pen.linejoin] );
/* the current transformation matrix will not affect the fill, but it will */
/*  affect the way stroke looks. So we must include it here. BUT the spline */
/*  set has already been transformed, so we must apply the inverse transform */
/*  to the splineset before outputting it, so that applying the transform */
/*  will give us the splines we desire. */
    if ( pen.trans[0]!=1.0 || pen.trans[3]!=1.0 || pen.trans[1]!=0 || pen.trans[2]!=0 )
	fprintf( file, "transform=\"matrix(%g, %g, %g, %g, 0, 0)\" ",
		(double) pen.trans[0], (double) pen.trans[1], (double) pen.trans[2], (double) pen.trans[3] );
    if ( pen.dashes[0]==0 && pen.dashes[1]==DASH_INHERITED ) {
	fprintf( file, "stroke-dasharray=\"inherit\" " );
    } else if ( pen.dashes[0]!=0 ) {
	int i;
	fprintf( file, "stroke-dasharray=\"" );
	for ( i=0; i<DASH_MAX && pen.dashes[i]!=0; ++i )
	    fprintf( file, "%d ", pen.dashes[i]);
	fprintf( file,"\" ");
    } else {
        /* That's the default, don't need to say it */
	/* fprintf( file, "stroke-dasharray=\"none\" " )*/
	;
    }
}

static void svg_dumpfill(FILE *file, struct brush *cbrush, struct brush *fallback,
	int dofill, const char *scname, SplineChar *nested, int layer, int istop ) {
    struct brush brush;

    if ( !dofill ) {
	fprintf( file, "fill=\"none\" " );
return;
    }

    brush = *cbrush;
    if ( fallback!=NULL ) {
	if ( brush.col==COLOR_INHERITED ) brush.col = fallback->col;
	if ( brush.opacity<0 ) brush.opacity = fallback->opacity;
    }

    if ( brush.gradient!=NULL ) {
	fprintf( file, "fill=\"url(#%s", scname );
	if ( nested!=NULL )
	    fprintf( file, "-%s", nested->name );
	fprintf( file, "-ly%d-fill-grad)\" ", layer );
    } else if ( brush.pattern!=NULL && istop ) {
	fprintf( file, "fill=\"url(#%s", scname );
	if ( nested!=NULL )
	    fprintf( file, "-%s", nested->name );
	fprintf( file, "-ly%d-fill-pattern)\" ", layer );
    } else {
	if ( brush.col!=COLOR_INHERITED )
	    fprintf( file, "fill=\"#%02x%02x%02x\" ",
		    COLOR_RED(brush.col), COLOR_GREEN(brush.col), COLOR_BLUE(brush.col));
	else
	    fprintf( file, "fill=\"currentColor\" " );
	if ( brush.opacity>=0 )
	    fprintf( file, "fill-opacity=\"%g\" ", (double)brush.opacity);
    }
}

static SplineSet *TransBy(SplineSet *ss, real trans[4] ) {
    real inversetrans[6], transform[6];

    if ( trans[0]==1.0 && trans[3]==1.0 && trans[1]==0 && trans[2]==0 )
return( ss );
    memcpy(transform,trans,4*sizeof(real));
    transform[4] = transform[5] = 0;
    MatInverse(inversetrans,transform);
return( SplinePointListTransform(SplinePointListCopy(
		    ss),inversetrans,tpt_AllPoints));
}

static int svg_sc_any(SplineChar *sc,int layer) {
    int i,j;
    int any;
    RefChar *ref;
    int first, last;

    first = last = layer;
    if ( sc->parent->multilayer )
	last = sc->layer_cnt-1;
    any = false;
    for ( i=first; i<=last && !any; ++i ) {
	any = sc->layers[i].splines!=NULL || sc->layers[i].images!=NULL;
	for ( ref=sc->layers[i].refs ; ref!=NULL && !any; ref = ref->next )
	    for ( j=0; j<ref->layer_cnt && !any; ++j )
		any = ref->layers[j].splines!=NULL;
    }
return( any );
}

static int base64tab[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+', '/'
};

static void DataURI_ImageDump(FILE *file,struct gimage *img) {
    const char *mimetype=NULL;
    FILE *imgf;
    int done = false;
    int threechars[3], fourchars[4], i, ch, ch_on_line;
#if !defined( _NO_LIBJPEG)
    struct _GImage *base = img->list_len==0 ? img->u.image : img->u.images[0];
#endif

    /* Technically we can only put a file into an URI if the whole thing is */
    /*  less than 1024 bytes long. But I shall ignore that issue */
    imgf = tmpfile();
#if !defined(_NO_LIBJPEG)
    if ( base->image_type == it_true ) {
	done = GImageWrite_Jpeg(img,imgf,78,false);
	mimetype = "image/jpeg";
    }
#endif
#ifndef _NO_LIBPNG
    if ( !done ) {
	done = GImageWrite_Png(img,imgf,false);
	mimetype = "image/png";
    }
#endif
    if ( !done ) {
	GImageWrite_Bmp(img,imgf);
	mimetype = "image/bmp";
    }

    fprintf( file,"%s;base64,", mimetype );
    rewind(imgf);

    /* Now do base64 output conversion */

    rewind(imgf);
    ch = getc(imgf);
    ch_on_line = 0;
    while ( ch!=EOF ) {
	threechars[0] = threechars[1] = threechars[2] = 0;
	for ( i=0; i<3 && ch!=EOF ; ++i ) {
	    threechars[i] = ch;
	    ch = getc(imgf);
	}
	if ( i>0 ) {
	    fourchars[0] = base64tab[threechars[0]>>2];
	    fourchars[1] = base64tab[((threechars[0]&0x3)<<4)|(threechars[1]>>4)];
	    fourchars[2] = base64tab[((threechars[1]&0xf)<<2)|(threechars[2]>>6)];
	    fourchars[3] = base64tab[threechars[2]&0x3f];
	    if ( i<3 )
		fourchars[3] = '=';
	    if ( i<2 )
		fourchars[2] = '=';
	    putc(fourchars[0],file);
	    putc(fourchars[1],file);
	    putc(fourchars[2],file);
	    putc(fourchars[3],file);
	    ch_on_line += 4;
	    if ( ch_on_line>=72 ) {
		putc('\n',file);
		ch_on_line = 0;
	    }
	}
    }
    fclose(imgf);
}

static void svg_dumpgradient(FILE *file,struct gradient *gradient,
	const char *scname,SplineChar *nested,int layer,int is_fill) {
    int i;
    Color csame; float osame;

    fprintf( file, "    <%s ", gradient->radius==0 ? "linearGradient" : "radialGradient" );
    if ( nested==NULL )
	fprintf( file, " id=\"%s-ly%d-%s-grad\"", scname, layer, is_fill ? "fill" : "stroke" );
    else
	fprintf( file, " id=\"%s-%s-ly%d-%s-grad\"", scname, nested->name, layer, is_fill ? "fill" : "stroke" );
    fprintf(file, "\n\tgradientUnits=\"userSpaceOnUse\"" );
    if ( gradient->radius==0 ) {
	fprintf( file, "\n\tx1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\"",
		(double) gradient->start.x, (double) gradient->start.y,
		(double) gradient->stop.x, (double) gradient->stop.y );
    } else {
	if ( gradient->start.x==gradient->stop.x && gradient->start.y==gradient->stop.y )
	    fprintf( file, "\n\tcx=\"%g\" cy=\"%g\" r=\"%g\"",
		(double) gradient->stop.x, (double) gradient->stop.y,
		(double) gradient->radius );
	else
	    fprintf( file, "\n\tfx=\"%g\" fy=\"%g\" cx=\"%g\" cy=\"%g\" r=\"%g\"",
		(double) gradient->start.x, (double) gradient->start.y,
		(double) gradient->stop.x, (double) gradient->stop.y,
		(double) gradient->radius );
    }
    fprintf(file, "\n\tspreadMethod=\"%s\">\n",
		gradient->sm == sm_pad ? "pad" :
		gradient->sm == sm_reflect ? "reflect" :
		"repeat" );

    csame = (Color)-1; osame = -1;
    for ( i=0; i<gradient->stop_cnt; ++i ) {
	if ( csame==(Color)-1 )
	    csame = gradient->grad_stops[i].col;
	else if ( csame!=gradient->grad_stops[i].col )
	    csame = (Color)-2;
	if ( osame== -1 )
	    osame = gradient->grad_stops[i].opacity;
	else if ( (double)osame!=gradient->grad_stops[i].opacity )
	    osame = -2;
    }
    for ( i=0; i<gradient->stop_cnt; ++i ) {
	fprintf( file, "      <stop offset=\"%g\"", (double) gradient->grad_stops[i].offset );
	if ( csame==(Color)-2 ) {
	    if ( gradient->grad_stops[i].col==COLOR_INHERITED )
		fprintf( file, " stop-color=\"inherit\"" );
	    else
		fprintf( file, " stop-color=\"#%06x\"", gradient->grad_stops[i].col );
	}
	if ( osame<0 ) {
	    if ( gradient->grad_stops[i].opacity==COLOR_INHERITED )
		fprintf( file, " stop-opacity=\"inherit\"" );
	    else
		fprintf( file, " stop-opacity=\"%g\"", (double) gradient->grad_stops[i].opacity );
	}
	fprintf( file, "/>\n" );
    }
    fprintf( file, "    </%s>\n", gradient->radius==0 ? "linearGradient" : "radialGradient" );
}

static void svg_dumpscdefs(FILE *file,SplineChar *sc,const char *name,int istop);
static void svg_dumptype3(FILE *file,SplineChar *sc,const char *name,int istop);

static void svg_dumppattern(FILE *file,struct pattern *pattern,
	const char *scname, SplineChar *base,SplineChar *nested,int layer,int is_fill) {
    SplineChar *pattern_sc = SFGetChar(base->parent,-1,pattern->pattern);
    char *patsubname = NULL;

    if ( pattern_sc!=NULL ) {
	patsubname = strconcat3(scname,"-",pattern->pattern);
	svg_dumpscdefs(file,pattern_sc,patsubname,false);
    } else
	LogError(_("No glyph named %s, used as a pattern in %s\n"), pattern->pattern, scname);

    fprintf( file, "    <pattern " );
    if ( nested==NULL )
	fprintf( file, " id=\"%s-ly%d-%s-pattern\"", scname, layer, is_fill ? "fill" : "stroke" );
    else
	fprintf( file, " id=\"%s-%s-ly%d-%s-pattern\"", scname, nested->name, layer, is_fill ? "fill" : "stroke" );
    fprintf(file, "\n\tpatternUnits=\"userSpaceOnUse\"" );
    if ( pattern_sc!=NULL ) {
	DBounds b;
	PatternSCBounds(pattern_sc,&b);
	fprintf( file, "\n\tviewBox=\"%g %g %g %g\"",
		(double) b.minx, (double) b.miny,
		(double) (b.maxx-b.minx), (double) (b.maxy-b.miny) );
    }
    fprintf( file, "\n\twidth=\"%g\" height=\"%g\"",
	    (double) pattern->width, (double) pattern->height );
    if ( pattern->transform[0]!=1 || pattern->transform[1]!=0 ||
	    pattern->transform[2]!=0 || pattern->transform[3]!=1 ||
	    pattern->transform[4]!=0 || pattern->transform[5]!=0 ) {
	fprintf( file, "\n\tpatternTransform=\"matrix(%g %g %g %g %g %g)\"",
		(double) pattern->transform[0], (double) pattern->transform[1],
		(double) pattern->transform[2], (double) pattern->transform[3],
		(double) pattern->transform[4], (double) pattern->transform[5] );
    }
    if ( pattern_sc!=NULL )
	svg_dumpscdefs(file,pattern_sc,patsubname,false);
    fprintf( file, "    </pattern>\n" );
    free(patsubname);
}

static void svg_layer_defs(FILE *file, SplineSet *splines,struct brush *fill_brush,struct pen *stroke_pen,
	SplineChar *sc, const char *scname, SplineChar *nested, int layer, int istop ) {
    if ( SSHasClip(splines)) {
	if ( nested==NULL )
	    fprintf( file, "    <clipPath id=\"%s-ly%d-clip\">\n", scname, layer );
	else
	    fprintf( file, "    <clipPath id=\"%s-%s-ly%d-clip\">\n", scname, nested->name, layer );
	fprintf(file, "      <path d=\"\n");
	svg_pathdump(file,sc->layers[layer].splines,16,true,true);
	fprintf(file, "\"/>\n" );
	fprintf( file, "    </clipPath>\n" );
    }
    if ( fill_brush->gradient!=NULL )
	svg_dumpgradient(file,fill_brush->gradient,scname,nested,layer,true);
    else if ( fill_brush->pattern!=NULL && istop )
	svg_dumppattern(file,fill_brush->pattern,scname,sc,nested,layer,true);
    if ( stroke_pen->brush.gradient!=NULL )
	svg_dumpgradient(file,stroke_pen->brush.gradient,scname,nested,layer,false);
    else if ( stroke_pen->brush.pattern!=NULL && istop )
	svg_dumppattern(file,stroke_pen->brush.pattern,scname,sc,nested,layer,false);
}

static void svg_dumpscdefs(FILE *file,SplineChar *sc,const char *name,int istop) {
    int i, j;
    RefChar *ref;

    for ( i=ly_fore; i<sc->layer_cnt ; ++i ) {
	svg_layer_defs(file,sc->layers[i].splines,&sc->layers[i].fill_brush,&sc->layers[i].stroke_pen,
		sc,name,NULL,i,istop);
	for ( ref=sc->layers[i].refs ; ref!=NULL; ref = ref->next ) {
	    for ( j=0; j<ref->layer_cnt; ++j ) if ( ref->layers[j].splines!=NULL ) {
		svg_layer_defs(file,ref->layers[j].splines,&ref->layers[j].fill_brush,&ref->layers[j].stroke_pen,
			sc,name,ref->sc,j,istop);
	    }
	}
    }
}

static void svg_dumptype3(FILE *file,SplineChar *sc,const char *name,int istop) {
    int i, j;
    RefChar *ref;
    ImageList *images;
    SplineSet *transed;

    for ( i=ly_fore; i<sc->layer_cnt ; ++i ) {
	if ( SSHasDrawn(sc->layers[i].splines) ) {
	    fprintf(file, "  <g " );
	    if ( SSHasClip(sc->layers[i].splines))
		fprintf( file, "clip-path=\"url(#%s-ly%d-clip)\" ", name, i );
	    transed = sc->layers[i].splines;
	    if ( sc->layers[i].dostroke ) {
		svg_dumpstroke(file,&sc->layers[i].stroke_pen,NULL,name,NULL,i,istop);
		transed = TransBy(transed,sc->layers[i].stroke_pen.trans);
	    }
	    svg_dumpfill(file,&sc->layers[i].fill_brush,NULL,sc->layers[i].dofill,name,NULL,i,istop);
	    fprintf( file, ">\n" );
	    fprintf(file, "    <path d=\"\n");
	    svg_pathdump(file,transed,12,!sc->layers[i].dostroke,false);
	    fprintf(file, "\"/>\n" );
	    if ( transed!=sc->layers[i].splines )
		SplinePointListsFree(transed);
	    fprintf(file, "  </g>\n" );
	}
	for ( ref=sc->layers[i].refs ; ref!=NULL; ref = ref->next ) {
	    for ( j=0; j<ref->layer_cnt; ++j ) if ( ref->layers[j].splines!=NULL ) {
		fprintf(file, "   <g " );
		transed = ref->layers[j].splines;
		if ( SSHasClip(transed))
		    fprintf( file, "clip-path=\"url(#%s-%s-ly%d-clip)\" ", name, ref->sc->name, j );
		if ( ref->layers[j].dostroke ) {
		    svg_dumpstroke(file,&ref->layers[j].stroke_pen,&sc->layers[i].stroke_pen,sc->name,ref->sc,j,istop);
		    transed = TransBy(transed,ref->layers[j].stroke_pen.trans);
		}
		svg_dumpfill(file,&ref->layers[j].fill_brush,&sc->layers[i].fill_brush,ref->layers[j].dofill,sc->name,ref->sc,j,istop);
		fprintf( file, ">\n" );
		fprintf(file, "  <path d=\"\n");
		svg_pathdump(file,transed,12,!ref->layers[j].dostroke,false);
		fprintf(file, "\"/>\n" );
		if ( transed!=ref->layers[j].splines )
		    SplinePointListsFree(transed);
		fprintf(file, "   </g>\n" );
	    }
	}
	for ( images=sc->layers[i].images ; images!=NULL; images = images->next ) {
	    struct _GImage *base;
	    fprintf(file, "      <image\n" );
	    base = images->image->list_len==0 ? images->image->u.image :
		    images->image->u.images[0];
	    fprintf(file, "\twidth=\"%g\"\n\theight=\"%g\"\n",
		    (double) (base->width*images->xscale), (double) (base->height*images->yscale) );
	    fprintf(file, "\tx=\"%g\"\n\ty=\"%g\"\n", (double) images->xoff, (double) images->yoff );
	    fprintf(file, "\txlink:href=\"data:" );
	    DataURI_ImageDump(file,images->image);
	    fprintf(file, "\" />\n" );
	}
    }
}

static void svg_scpathdump(FILE *file, SplineChar *sc,const char *endpath,int layer) {
    RefChar *ref;
    int lineout;
    int i,j;
    int needs_defs=0;

    if ( !svg_sc_any(sc,layer) ) {
	/* I think a space is represented by leaving out the d (path) entirely*/
	/*  rather than having d="" */
	fputs(" />\n",file);
    } else if ( sc->parent->strokedfont ) {
	/* Can't be done with a path, requires nested elements (I think) */
	fprintf(file,">\n  <g stroke=\"currentColor\" stroke-width=\"%g\" fill=\"none\">\n", (double) sc->parent->strokewidth );
	fprintf( file,"    <path d=\"");
	lineout = svg_pathdump(file,sc->layers[layer].splines,3,false,false);
	for ( ref= sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    lineout = svg_pathdump(file,ref->layers[0].splines,lineout,false,false);
	if ( lineout>=255-4 ) putc('\n',file );
	putc('"',file);
	fputs(" />\n  </g>\n",file);
	fputs(endpath,file);
    } else if ( !sc->parent->multilayer ) {
	fprintf( file,"d=\"");
	lineout = svg_pathdump(file,sc->layers[layer].splines,3,true,false);
	for ( ref= sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    lineout = svg_pathdump(file,ref->layers[0].splines,lineout,true,false);
	if ( lineout>=255-4 ) putc('\n',file );
	putc('"',file);
	fputs(" />\n",file);
    } else {
	fputs(">\n",file);
	for ( i=ly_fore; i<sc->layer_cnt && !needs_defs ; ++i ) {
	    if ( SSHasClip(sc->layers[i].splines))
		needs_defs = true;
	    else if ( sc->layers[i].fill_brush.pattern!=NULL ||
		    sc->layers[i].fill_brush.gradient!=NULL ||
		    sc->layers[i].stroke_pen.brush.pattern!=NULL ||
		    sc->layers[i].stroke_pen.brush.gradient!=NULL )
		needs_defs = true;
	    for ( ref=sc->layers[i].refs ; ref!=NULL; ref = ref->next ) {
		for ( j=0; j<ref->layer_cnt; ++j ) if ( ref->layers[j].splines!=NULL ) {
		    if ( SSHasClip(ref->layers[j].splines))
			needs_defs = true;
		    else if ( ref->layers[j].fill_brush.pattern!=NULL ||
			    ref->layers[j].fill_brush.gradient!=NULL ||
			    ref->layers[j].stroke_pen.brush.pattern!=NULL ||
			    ref->layers[j].stroke_pen.brush.gradient!=NULL )
			needs_defs = true;
		}
	    }
	}
	if ( needs_defs ) {
	    fprintf(file, "  <defs>\n" );
	    svg_dumpscdefs(file,sc,sc->name,true);
	    fprintf(file, "  </defs>\n" );
	}
	svg_dumptype3(file,sc,sc->name,true);
	fputs(endpath,file);
    }
}

static int LigCnt(SplineFont *sf,PST *lig,int32 *univals,int max) {
    char *pt, *end;
    int c=0;
    SplineChar *sc;

    if ( lig->type!=pst_ligature )
return( 0 );
    else if ( !lig->subtable->lookup->store_in_afm )
return( 0 );
    pt = lig->u.lig.components;
    for (;;) {
	end = strchr(pt,' ');
	if ( end!=NULL ) *end='\0';
	sc = SFGetChar(sf,-1,pt);
	if ( end!=NULL ) *end=' ';
	if ( sc==NULL || sc->unicodeenc==-1 )
return( 0 );
	if ( c>=max )
return( 0 );
	univals[c++] = sc->unicodeenc;
	if ( end==NULL )
return( c );
	pt = end+1;
	while ( *pt==' ' ) ++pt;
    }
}

static PST *HasLigature(SplineChar *sc) {
    PST *pst, *best=NULL;
    int bestc=0,c;
    int32 univals[50];

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type==pst_ligature ) {
	    c = LigCnt(sc->parent,pst,univals,sizeof(univals)/sizeof(univals[0]));
	    if ( c>1 && c>bestc ) {
		c = bestc;
		best = pst;
	    }
	}
    }
return( best );
}

SplineChar *SCHasSubs(SplineChar *sc, uint32 tag) {
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type==pst_substitution &&
		FeatureTagInFeatureScriptList(tag,pst->subtable->lookup->features) )
return( SFGetChar(sc->parent,-1,pst->u.subs.variant));
    }
return( NULL );
}

static void svg_scdump(FILE *file, SplineChar *sc,int defwid, int encuni, int vs,int layer) {
    PST *best=NULL;
    const unichar_t *alt;
    int32 univals[50];
    int i, c;

    best = HasLigature(sc);
    if ( sc->comment!=NULL ) {
	fprintf( file, "\n<!--\n%s\n-->\n",sc->comment );
    }
    fprintf(file,"    <glyph glyph-name=\"%s\" ",sc->name );
    if ( best!=NULL ) {
	c = LigCnt(sc->parent,best,univals,sizeof(univals)/sizeof(univals[0]));
	fputs("unicode=\"",file);
	for ( i=0; i<c; ++i )
	    if ( univals[i]>='A' && univals[i]<='z' )
		putc(univals[i],file);
	    else
		fprintf(file,"&#x%x;", (unsigned int) univals[i]);
	fputs("\" ",file);
    } else if ( encuni!=-1 && encuni<0x110000 ) {
	if ( encuni!=0x9 &&
		encuni!=0xa &&
		encuni!=0xd &&
		!(encuni>=0x20 && encuni<=0xd7ff) &&
		!(encuni>=0xe000 && encuni<=0xfffd) &&
		!(encuni>=0x10000 && encuni<=0x10ffff) )
	    /* Not allowed in XML */;
	else if ( (encuni>=0x7f && encuni<=0x84) ||
		  (encuni>=0x86 && encuni<=0x9f) ||
		  (encuni>=0xfdd0 && encuni<=0xfddf) ||
		  (encuni&0xffff)==0xfffe ||
		  (encuni&0xffff)==0xffff )
	    /* Not recommended in XML */;
	else if ( encuni>=32 && encuni<127 &&
		encuni!='"' && encuni!='&' &&
		encuni!='<' && encuni!='>' )
	    fprintf( file, "unicode=\"%c\" ", encuni);
	else if ( encuni<0x10000 &&
		( isarabisolated(encuni) || isarabinitial(encuni) || isarabmedial(encuni) || isarabfinal(encuni) ) &&
		unicode_alternates[encuni>>8]!=NULL &&
		(alt = unicode_alternates[encuni>>8][encuni&0xff])!=NULL &&
		alt[1]=='\0' )
	    /* For arabic forms use the base representation in the 0600 block */
	    fprintf( file, "unicode=\"&#x%x;\" ", alt[0]);
	else
	    fprintf( file, "unicode=\"&#x%x;\" ", encuni);
	if ( vs!=-1 )
	    fprintf( file, "unicode=\"&#x%x;\" ", vs);
    }
    if ( sc->width!=defwid )
	fprintf( file, "horiz-adv-x=\"%d\" ", sc->width );
    if ( sc->parent->hasvmetrics && sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	fprintf( file, "vert-adv-y=\"%d\" ", sc->vwidth );
    if ( strstr(sc->name,".vert")!=NULL || strstr(sc->name,".vrt2")!=NULL )
	fprintf( file, "orientation=\"v\" " );
    if ( encuni!=-1 && encuni<0x10000 ) {
	if ( isarabinitial(encuni))
	    fprintf( file,"arabic-form=\"initial\" " );
	else if ( isarabmedial(encuni))
	    fprintf( file,"arabic-form=\"medial\" ");
	else if ( isarabfinal(encuni))
	    fprintf( file,"arabic-form=\"final\" ");
	else if ( isarabisolated(encuni))
	    fprintf( file,"arabic-form=\"isolated\" ");
    }
    putc('\n',file);
    svg_scpathdump(file,sc," </glyph>\n",layer);
    sc->ticked = true;
}

static void svg_notdefdump(FILE *file, SplineFont *sf,int defwid,int layer) {
    int notdefpos;

    notdefpos = SFFindNotdef(sf,-2);

    if ( notdefpos!=-1 ) {
	SplineChar *sc = sf->glyphs[notdefpos];

	fprintf(file, "<missing-glyph ");
	if ( sc->width!=defwid )
	    fprintf( file, "horiz-adv-x=\"%d\" ", sc->width );
	if ( sc->parent->hasvmetrics && sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	    fprintf( file, "vert-adv-y=\"%d\" ", sc->vwidth );
	putc('\n',file);
	svg_scpathdump(file,sc," </glyph>\n",layer);
    } else {
	/* We'll let both the horiz and vert advances default to the values */
	/*  specified by the font, and I think a space is done by omitting */
	/*  d (the path) altogether */
	fprintf(file,"    <missing-glyph />\n");	/* Is this a blank space? */
    }
}

static void fputkerns( FILE *file, char *names) {
    while ( *names ) {
	if ( *names==' ' ) {
	    putc(',',file);
	    while ( names[1]==' ' ) ++names;
	} else
	    putc(*names,file);
	++names;
    }
}

static void svg_dumpkerns(FILE *file,SplineFont *sf,int isv) {
    int i,j;
    KernPair *kp;
    KernClass *kc;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
	for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns;
		kp!=NULL; kp = kp->next )
	    if ( kp->off!=0 && SCWorthOutputting(kp->sc)) {
		fprintf( file, isv ? "    <vkern " : "    <hkern " );
		if ( sf->glyphs[i]->unicodeenc==-1 || HasLigature(sf->glyphs[i]))
		    fprintf( file, "g1=\"%s\" ", sf->glyphs[i]->name );
		else if ( sf->glyphs[i]->unicodeenc>='A' && sf->glyphs[i]->unicodeenc<='z' )
		    fprintf( file, "u1=\"%c\" ", sf->glyphs[i]->unicodeenc );
		else
		    fprintf( file, "u1=\"&#x%x;\" ", sf->glyphs[i]->unicodeenc );
		if ( kp->sc->unicodeenc==-1 || HasLigature(kp->sc))
		    fprintf( file, "g2=\"%s\" ", kp->sc->name );
		else if ( kp->sc->unicodeenc>='A' && kp->sc->unicodeenc<='z' )
		    fprintf( file, "u2=\"%c\" ", kp->sc->unicodeenc );
		else
		    fprintf( file, "u2=\"&#x%x;\" ", kp->sc->unicodeenc );
		fprintf( file, "k=\"%d\" />\n", -kp->off );
	    }
    }

    for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) {
        for ( i=0; i<kc->first_cnt; ++i ) {
            if ( kc->firsts[i] && *kc->firsts[i]!='\0' ) {
                for ( j=0; j<kc->second_cnt; ++j ) {
                    if ( kc->seconds[j] && *kc->seconds[j]!='\0' &&
                         kc->offsets[i*kc->second_cnt+j]!=0 ) {
                        fprintf( file, isv ? "    <vkern g1=\"" : "    <hkern g1=\"" );
                        fputkerns( file, kc->firsts[i]);
                        fprintf( file, "\"\n\tg2=\"" );
                        fputkerns( file, kc->seconds[j]);
                        fprintf( file, "\"\n\tk=\"%d\" />\n",
                                -kc->offsets[i*kc->second_cnt+j]);
                    }
                }
            }
        }
    }
}

static void svg_outfonttrailer(FILE *file) {
    fprintf(file,"  </font>\n");
    fprintf(file,"</defs></svg>\n" );
}

static int AnyArabicForm( SplineChar *sc ) {
    struct altuni *altuni;

    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		(isarabinitial(sc->unicodeenc) ||
		 isarabmedial(sc->unicodeenc) ||
		 isarabfinal(sc->unicodeenc) ||
		 isarabisolated(sc->unicodeenc)))
return( sc->unicodeenc );
    for ( altuni = sc->altuni; altuni!=NULL; altuni = altuni->next )
	if ( altuni->unienc!=-1 && altuni->unienc<0x10000 &&
		altuni->vs==-1 && altuni->fid==0 &&
		(isarabinitial(altuni->unienc) ||
		 isarabmedial(altuni->unienc) ||
		 isarabfinal(altuni->unienc) ||
		 isarabisolated(altuni->unienc)))
return( altuni->unienc );

return( -1 );
}

static int UnformedUni(int uni) {
return( uni==-1 || uni>=0x10000 ||
		!(isarabinitial(uni) ||
		 isarabmedial(uni) ||
		 isarabfinal(uni) ||
		 isarabisolated(uni)));
}

static void svg_sfdump(FILE *file,SplineFont *sf,int layer) {
    int defwid, i, formeduni;
    struct altuni *altuni;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;

    defwid = svg_outfontheader(file,sf,layer);
    svg_notdefdump(file,sf,defwid,layer);

    /* Ligatures must be output before their initial components */
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( HasLigature(sf->glyphs[i]))
		svg_scdump(file, sf->glyphs[i],defwid,sf->glyphs[i]->unicodeenc,-1,layer);
	    /* Variation selectors should probably be treated as ligatures */
	    for ( altuni = sf->glyphs[i]->altuni; altuni!=NULL; altuni = altuni->next )
		if ( altuni->vs!=-1 && altuni->fid==0 )
		    svg_scdump(file, sf->glyphs[i],defwid,altuni->unienc,altuni->vs,layer);
	}
    }
    /* And formed arabic before unformed */
    for ( i=0; i<sf->glyphcnt; ++i ) {
	SplineChar *sc = sf->glyphs[i];
	if ( SCWorthOutputting(sc) && !sc->ticked ) {
	    if ( (formeduni = AnyArabicForm(sc))!=-1 )
		svg_scdump(file, sc,defwid,formeduni,-1,layer);
	    else if ( sc->unicodeenc>=0x0600 && sc->unicodeenc<=0x06ff ) {
		/* The conventions now (as I understand them) suggest that */
		/*  fonts not use the unicode encodings for formed arabic */
		/*  but should use simple substitutions instead */
		int arab_off = sc->unicodeenc-0x600;
		SplineChar *formed;
		formed = SCHasSubs(sc,CHR('i','n','i','t'));
		if ( SCWorthOutputting(formed) && formed->unicodeenc==-1 &&
			!formed->ticked && ArabicForms[arab_off].initial!=0 )
		    svg_scdump(file,formed,defwid,ArabicForms[arab_off].initial,-1,layer);
		formed = SCHasSubs(sc,CHR('m','e','d','i'));
		if ( SCWorthOutputting(formed) && formed->unicodeenc==-1 &&
			!formed->ticked && ArabicForms[arab_off].medial!=0 )
		    svg_scdump(file,formed,defwid,ArabicForms[arab_off].medial,-1,layer);
		formed = SCHasSubs(sc,CHR('f','i','n','a'));
		if ( SCWorthOutputting(formed) && formed->unicodeenc==-1 &&
			!formed->ticked && ArabicForms[arab_off].final!=0 )
		    svg_scdump(file,formed,defwid,ArabicForms[arab_off].final,-1,layer);
		formed = SCHasSubs(sc,CHR('i','s','o','l'));
		if ( SCWorthOutputting(formed) && formed->unicodeenc==-1 &&
			!formed->ticked && ArabicForms[arab_off].isolated!=0 )
		    svg_scdump(file,formed,defwid,ArabicForms[arab_off].isolated,-1,layer);
	    }
	}
    }
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( SCWorthOutputting(sf->glyphs[i]) && !sf->glyphs[i]->ticked ) {
	    if ( UnformedUni(sf->glyphs[i]->unicodeenc) )
		svg_scdump(file, sf->glyphs[i],defwid,sf->glyphs[i]->unicodeenc,-1,layer);
	    for ( altuni = sf->glyphs[i]->altuni; altuni!=NULL; altuni = altuni->next )
		if ( altuni->vs==-1 && altuni->fid==0 )
		    svg_scdump(file, sf->glyphs[i],defwid,altuni->unienc,altuni->vs,layer);
	}
    }
    svg_dumpkerns(file,sf,false);
    svg_dumpkerns(file,sf,true);
    svg_outfonttrailer(file);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
}

int _WriteSVGFont(FILE *file,SplineFont *sf,int flags,
	EncMap *map,int layer) {
    int ret;

    svg_sfdump(file,sf,layer);
    ret = true;
    if ( ferror(file))
	ret = false;
return( ret );
}

int WriteSVGFont(const char *fontname,SplineFont *sf,enum fontformat format,int flags,
	EncMap *map,int layer) {
    FILE *file;
    int ret;

    if ( strstr(fontname,"://")!=NULL ) {
	if (( file = tmpfile())==NULL )
return( 0 );
    } else {
	if (( file=fopen(fontname,"w+"))==NULL )
return( 0 );
    }
    svg_sfdump(file,sf,layer);
    ret = true;
    if ( ferror(file))
	ret = false;
    if ( strstr(fontname,"://")!=NULL && ret )
	ret = URLFromFile(fontname,file);
    if ( fclose(file)==-1 )
return( 0 );
return( ret );
}

int _ExportSVG(FILE *svg,SplineChar *sc,int layer) {
    char *end;
    int em_size;
    DBounds b;

    SplineCharLayerFindBounds(sc,layer,&b);
    em_size = sc->parent->ascent+sc->parent->descent;
    if ( b.minx>0 ) b.minx=0;
    if ( b.miny>-sc->parent->descent ) b.miny = -sc->parent->descent;
    if ( b.maxy<em_size ) b.maxy = em_size;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    fprintf(svg, "<?xml version=\"1.0\" standalone=\"no\"?>\n" );
    fprintf(svg, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n" );
    fprintf(svg, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"%d %d %d %d\">\n",
	    (int) floor(b.minx), (int) floor(b.miny),
	    (int) ceil(sc->width), (int) ceil(em_size));
    fprintf(svg, "  <g transform=\"matrix(1 0 0 -1 0 %d)\">\n",
	    sc->parent->ascent );
    if ( sc->parent->multilayer || sc->parent->strokedfont || !svg_sc_any(sc,layer)) {
	fprintf(svg, "   <g ");
	end = "   </g>\n";
    } else {
	fprintf(svg, "   <path fill=\"currentColor\"\n");
	end = "   </path>\n";
    }
    svg_scpathdump(svg,sc,end,layer);
    fprintf(svg, "  </g>\n\n" );
    fprintf(svg, "</svg>\n" );

    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( !ferror(svg));
}

/* ************************************************************************** */
/* *****************************    SVG Input    **************************** */
/* ************************************************************************** */

#undef extended			/* used in xlink.h */
#include <libxml/parser.h>

static int libxml_init_base() {
return( true );
}

/* Find a node with the given id */
static xmlNodePtr XmlFindID(xmlNodePtr xml, char *name) {
    xmlChar *id;
    xmlNodePtr child, ret;

    id = xmlGetProp(xml,(xmlChar *) "id");
    if ( id!=NULL && xmlStrcmp(id,(xmlChar *) name)==0 ) {
	xmlFree(id);
return( xml );
    }
    if ( id!=NULL )
	xmlFree(id);

    for ( child = xml->children; child!=NULL; child=child->next ) {
	ret = XmlFindID(child,name);
	if ( ret!=NULL )
return( ret );
    }
return( NULL );
}

static xmlNodePtr XmlFindURI(xmlNodePtr xml, char *name) {
    xmlNodePtr ret;
    char *pt, ch;

    if ( strncmp(name,"url(#",5)!=0 )
return( NULL );
    name += 5;
    for ( pt=name; *pt!=')' && *pt!='\0'; ++pt );
    ch = *pt; *pt = '\0';
    ret = XmlFindID(xml,name);
    *pt = ch;
return( ret );
}

/* We want to look for "font" nodes within "svg" nodes. Since "svg" nodes may */
/*  be embedded within another xml/html document there may be several of them */
/*  and there may be several fonts within each */
static int _FindSVGFontNodes(xmlNodePtr node,xmlNodePtr *fonts,int cnt, int max,
	char *nodename) {
    if ( xmlStrcmp(node->name,(const xmlChar *) nodename)==0 ) {
	if ( strcmp(nodename,"svg")==0 )
	    nodename = "font";
	else {
	    fonts[cnt++] = node;
	    if ( cnt>=max )
return( cnt );
	}
    }

    for ( node=node->children; node!=NULL; node=node->next ) {
	cnt = _FindSVGFontNodes(node,fonts,cnt,max,nodename);
	if ( cnt>=max )
return( cnt );
    }
return( cnt );
}

static xmlNodePtr *FindSVGFontNodes(xmlDocPtr doc) {
    xmlNodePtr *fonts=NULL;
    int cnt;

    fonts = calloc(100,sizeof(xmlNodePtr));	/* If the file has more than 100 fonts in it then it's foolish to expect the user to pick out one, so let's limit ourselves to 100 */
    cnt = _FindSVGFontNodes(xmlDocGetRootElement(doc),fonts,0,100,"svg");
    if ( cnt==0 ) {
	free(fonts);
return( NULL );
    }
return( fonts );
}

static xmlNodePtr SVGPickFont(xmlNodePtr *fonts,char *filename) {
    int cnt;
    char **names;
    xmlChar *name;
    char *pt, *lparen;
    int choice;

    for ( cnt=0; fonts[cnt]!=NULL; ++cnt);
    names = malloc((cnt+1)*sizeof(char *));
    for ( cnt=0; fonts[cnt]!=NULL; ++cnt) {
	name = xmlGetProp(fonts[cnt],(xmlChar *) "id");
	if ( name==NULL ) {
	    names[cnt] = copy("nameless-font");
	} else {
	    names[cnt] = copy((char *) name);
	    xmlFree(name);
	}
    }
    names[cnt] = NULL;

    choice = -1;
    pt = NULL;
    if ( filename!=NULL )
	pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( pt!=NULL && (lparen = strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	char *find = copy(lparen+1);
	pt = strchr(find,')');
	if ( pt!=NULL ) *pt='\0';
	for ( choice=cnt-1; choice>=0; --choice )
	    if ( strcmp(names[choice],find)==0 )
	break;
	if ( choice==-1 ) {
	    char *fn = copy(filename);
	    fn[lparen-filename] = '\0';
	    ff_post_error(_("Not in Collection"),_("%s is not in %.100s"),find,fn);
	    free(fn);
	}
	free(find);
    } else if ( no_windowing_ui )
	choice = 0;
    else
	choice = ff_choose(_("Pick a font, any font..."),(const char **) names,cnt,0,_("There are multiple fonts in this file, pick one"));
    for ( cnt=0; names[cnt]!=NULL; ++cnt )
	free(names[cnt]);
    free(names);
    if ( choice!=-1 )
return( fonts[choice] );

return( NULL );
}

#define PI	3.1415926535897932

/* I don't see where the spec says that the seperator between numbers is */
/*  comma or whitespace (both is ok too) */
/* But the style sheet spec says it, so I probably just missed it */
static char *skipcomma(char *pt) {
    while ( isspace(*pt))++pt;
    if ( *pt==',' ) ++pt;
return( pt );
}

static void SVGTraceArc(SplineSet *cur,BasePoint *current,
	double x,double y,double rx,double ry,double axisrot,
	int large_arc,int sweep) {
    double cosr, sinr;
    double x1p, y1p;
    double lambda, factor;
    double cxp, cyp, cx, cy;
    double tmpx, tmpy, t2x, t2y;
    double startangle, delta, a;
    SplinePoint *final, *sp;
    BasePoint arcp[4], prevcp[4], nextcp[4], firstcp[2];
    int i, j, ia, firstia;
    static double sines[] = { 0, 1, 0, -1, 0, 1, 0, -1, 0, 1, 0, -1 };
    static double cosines[]={ 1, 0, -1, 0, 1, 0, -1, 0, 1, 0, -1, 0 };

    final = SplinePointCreate(x,y);
    if ( rx < 0 ) rx = -rx;
    if ( ry < 0 ) ry = -ry;
    if ( rx!=0 && ry!=0 ) {
	/* Page 647 in the SVG 1.1 spec describes how to do this */
	/* This is Appendix F (Implementation notes) section 6.5 */
	cosr = cos(axisrot); sinr = sin(axisrot);
	x1p = cosr*(current->x-x)/2 + sinr*(current->y-y)/2;
	y1p =-sinr*(current->x-x)/2 + cosr*(current->y-y)/2;
	/* Correct for bad radii */
	lambda = x1p*x1p/(rx*rx) + y1p*y1p/(ry*ry);
	if ( lambda>1 ) {
	   lambda = sqrt(lambda);
	   rx *= lambda;
	   ry *= lambda;
	}
	factor = rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p;
	if ( RealNear(factor,0))
	    factor = 0;		/* Avoid rounding errors that lead to small negative values */
	else
	    factor = sqrt(factor/(rx*rx*y1p*y1p+ry*ry*x1p*x1p));
	if ( large_arc==sweep )
	    factor = -factor;
	cxp = factor*(rx*y1p)/ry;
	cyp =-factor*(ry*x1p)/rx;
	cx = cosr*cxp - sinr*cyp + (current->x+x)/2;
	cy = sinr*cxp + cosr*cyp + (current->y+y)/2;

	tmpx = (x1p-cxp)/rx; tmpy = (y1p-cyp)/ry;
	startangle = acos(tmpx/sqrt(tmpx*tmpx+tmpy*tmpy));
	if ( tmpy<0 )
	    startangle = -startangle;
	t2x = (-x1p-cxp)/rx; t2y = (-y1p-cyp)/ry;
	delta = (tmpx*t2x+tmpy*t2y)/
		  sqrt((tmpx*tmpx+tmpy*tmpy)*(t2x*t2x+t2y*t2y));
	/* We occasionally got rounding errors near -1 */
	if ( delta<=-1 )
	    delta = 3.1415926535897932;
	else if ( delta>=1 )
	    delta = 0;
	else
	    delta = acos(delta);
	if ( tmpx*t2y-tmpy*t2x<0 )
	    delta = -delta;
	if ( sweep==0 && delta>0 )
	    delta -= 2*PI;
	if ( sweep && delta<0 )
	    delta += 2*PI;

	if ( delta>0 ) {
	    i = 0;
	    ia = firstia = floor(startangle/(PI/2))+1;
	    for ( a=ia*(PI/2), ia+=4; a<startangle+delta && !RealNear(a,startangle+delta); a += PI/2, ++i, ++ia ) {
		t2x = rx*cosines[ia]; t2y = ry*sines[ia];
		arcp[i].x = cosr*t2x - sinr*t2y + cx;
		arcp[i].y = sinr*t2x + cosr*t2y + cy;
		if ( t2x==0 ) {
		    t2x = rx*cosines[ia+1]; t2y = 0;
		} else {
		    t2x = 0; t2y = ry*sines[ia+1];
		}
		prevcp[i].x = arcp[i].x - .552*(cosr*t2x - sinr*t2y);
		prevcp[i].y = arcp[i].y - .552*(sinr*t2x + cosr*t2y);
		nextcp[i].x = arcp[i].x + .552*(cosr*t2x - sinr*t2y);
		nextcp[i].y = arcp[i].y + .552*(sinr*t2x + cosr*t2y);
	    }
	} else {
	    i = 0;
	    ia = firstia = ceil(startangle/(PI/2))-1;
	    for ( a=ia*(PI/2), ia += 8; a>startangle+delta && !RealNear(a,startangle+delta); a -= PI/2, ++i, --ia ) {
		t2x = rx*cosines[ia]; t2y = ry*sines[ia];
		arcp[i].x = cosr*t2x - sinr*t2y + cx;
		arcp[i].y = sinr*t2x + cosr*t2y + cy;
		if ( t2x==0 ) {
		    t2x = rx*cosines[ia+1]; t2y = 0;
		} else {
		    t2x = 0; t2y = ry*sines[ia+1];
		}
		prevcp[i].x = arcp[i].x + .552*(cosr*t2x - sinr*t2y);
		prevcp[i].y = arcp[i].y + .552*(sinr*t2x + cosr*t2y);
		nextcp[i].x = arcp[i].x - .552*(cosr*t2x - sinr*t2y);
		nextcp[i].y = arcp[i].y - .552*(sinr*t2x + cosr*t2y);
	    }
	}
	if ( i!=0 ) {
	    double firsta=firstia*PI/2;
	    double d = (firsta-startangle)/2;
	    double th = startangle+d;
	    double hypot = 1/cos(d);
	    BasePoint temp;
	    t2x = rx*cos(th)*hypot; t2y = ry*sin(th)*hypot;
	    temp.x = cosr*t2x - sinr*t2y + cx;
	    temp.y = sinr*t2x + cosr*t2y + cy;
	    firstcp[0].x = cur->last->me.x + .552*(temp.x-cur->last->me.x);
	    firstcp[0].y = cur->last->me.y + .552*(temp.y-cur->last->me.y);
	    firstcp[1].x = arcp[0].x + .552*(temp.x-arcp[0].x);
	    firstcp[1].y = arcp[0].y + .552*(temp.y-arcp[0].y);
	}
	for ( j=0; j<i; ++j ) {
	    sp = SplinePointCreate(arcp[j].x,arcp[j].y);
	    if ( j!=0 ) {
		sp->prevcp = prevcp[j];
		cur->last->nextcp = nextcp[j-1];
	    } else {
		sp->prevcp = firstcp[1];
		cur->last->nextcp = firstcp[0];
	    }
	    sp->noprevcp = cur->last->nonextcp = false;
	    SplineMake(cur->last,sp,false);
	    cur->last = sp;
	}
	{ double hypot, c, s;
	BasePoint temp;
	if ( i==0 ) {
	    double th = startangle+delta/2;
	    hypot = 1.0/cos(delta/2);
	    c = cos(th); s=sin(th);
	} else {
	    double lasta = delta<0 ? a+PI/2 : a-PI/2;
	    double d = (startangle+delta-lasta);
	    double th = lasta+d/2;
	    hypot = 1.0/cos(d/2);
	    c = cos(th); s=sin(th);
	}
	t2x = rx*c*hypot; t2y = ry*s*hypot;
	temp.x = cosr*t2x - sinr*t2y + cx;
	temp.y = sinr*t2x + cosr*t2y + cy;
	cur->last->nextcp.x = cur->last->me.x + .552*(temp.x-cur->last->me.x);
	cur->last->nextcp.y = cur->last->me.y + .552*(temp.y-cur->last->me.y);
	final->prevcp.x = final->me.x + .552*(temp.x-final->me.x);
	final->prevcp.y = final->me.y + .552*(temp.y-final->me.y);
	cur->last->nonextcp = final->noprevcp = false;
	}
    }
    *current = final->me;
    SplineMake(cur->last,final,false);
    cur->last = final;
}

static SplineSet *SVGParsePath(xmlChar *path) {
    BasePoint current;
    SplineSet *head=NULL, *last=NULL, *cur=NULL;
    SplinePoint *sp;
    int type = 'M';
    double x1,x2,x,y1,y2,y,rx,ry,axisrot;
    int large_arc,sweep;
    int order2 = 0;
    char *end;

    current.x = current.y = 0;

    while ( *path ) {
	while ( *path==' ' ) ++path;
        if ( isalpha(*path)) {
            type = *path++;
        }
	if ( *path=='\0' && type!='z' && type!='Z' )
    break;
	if ( type=='m' || type=='M' ) {
	    if ( cur!=NULL && cur->last!=cur->first ) {
		if ( RealNear(cur->last->me.x,cur->first->me.x) &&
			RealNear(cur->last->me.y,cur->first->me.y) ) {
		    cur->first->prevcp = cur->last->prevcp;
		    cur->first->noprevcp = cur->last->noprevcp;
		    cur->first->prev = cur->last->prev;
		    cur->first->prev->to = cur->first;
		    SplinePointFree(cur->last);
		}
		cur->last = cur->first;
	    }
	    x = strtod((char *) path,&end);
	    end = skipcomma(end);
	    y = strtod(end,&end);
	    if ( type=='m' ) {
		x += current.x; y += current.y;
	    }
	    sp = SplinePointCreate(x,y);
	    current = sp->me;
	    cur = chunkalloc(sizeof(SplineSet));
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	    cur->first = cur->last = sp;
	    /* If you omit a command after a moveto then it defaults to lineto */
	    if ( type=='m' ) type='l';
	    else type = 'L';
	} else if ( type=='z' || type=='Z' ) {
	    if ( cur!=NULL && cur->last!=cur->first ) {
		if ( RealNear(cur->last->me.x,cur->first->me.x) &&
			RealNear(cur->last->me.y,cur->first->me.y) ) {
		    cur->first->prevcp = cur->last->prevcp;
		    cur->first->noprevcp = cur->last->noprevcp;
		    cur->first->prev = cur->last->prev;
		    cur->first->prev->to = cur->first;
		    SplinePointFree(cur->last);
		} else
		    SplineMake(cur->last,cur->first,order2);
		cur->last = cur->first;
		current = cur->first->me;
	    }
	    type = ' ';
	    end = (char *) path;
	} else {
	    if ( cur==NULL ) {
		sp = SplinePointCreate(current.x,current.y);
		cur = chunkalloc(sizeof(SplineSet));
		if ( head==NULL )
		    head = cur;
		else
		    last->next = cur;
		last = cur;
		cur->first = cur->last = sp;
	    }
	    switch ( type ) {
	      case 'l': case'L':
		x = strtod((char *) path,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='l' ) {
		    x += current.x; y += current.y;
		}
		sp = SplinePointCreate(x,y);
		current = sp->me;
		SplineMake(cur->last,sp,order2);
		cur->last = sp;
	      break;
	      case 'h': case'H':
		x = strtod((char *) path,&end);
		y = current.y;
		if ( type=='h' ) {
		    x += current.x;
		}
		sp = SplinePointCreate(x,y);
		current = sp->me;
		SplineMake(cur->last,sp,order2);
		cur->last = sp;
	      break;
	      case 'v': case 'V':
		x = current.x;
		y = strtod((char *) path,&end);
		if ( type=='v' ) {
		    y += current.y;
		}
		sp = SplinePointCreate(x,y);
		current = sp->me;
		SplineMake(cur->last,sp,order2);
		cur->last = sp;
	      break;
	      case 'c': case 'C':
		x1 = strtod((char *) path,&end);
		end = skipcomma(end);
		y1 = strtod(end,&end);
		end = skipcomma(end);
		x2 = strtod(end,&end);
		end = skipcomma(end);
		y2 = strtod(end,&end);
		end = skipcomma(end);
		x = strtod(end,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='c' ) {
		    x1 += current.x; y1 += current.y;
		    x2 += current.x; y2 += current.y;
		    x += current.x; y += current.y;
		}
		sp = SplinePointCreate(x,y);
		sp->prevcp.x = x2; sp->prevcp.y = y2; sp->noprevcp = false;
		cur->last->nextcp.x = x1; cur->last->nextcp.y = y1; cur->last->nonextcp = false;
		current = sp->me;
		SplineMake(cur->last,sp,false);
		cur->last = sp;
	      break;
	      case 's': case 'S':
		x1 = 2*cur->last->me.x - cur->last->prevcp.x;
		y1 = 2*cur->last->me.y - cur->last->prevcp.y;
		x2 = strtod((char *) path,&end);
		end = skipcomma(end);
		y2 = strtod(end,&end);
		end = skipcomma(end);
		x = strtod(end,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='s' ) {
		    x2 += current.x; y2 += current.y;
		    x += current.x; y += current.y;
		}
		sp = SplinePointCreate(x,y);
		sp->prevcp.x = x2; sp->prevcp.y = y2; sp->noprevcp = false;
		cur->last->nextcp.x = x1; cur->last->nextcp.y = y1; cur->last->nonextcp = false;
		current = sp->me;
		SplineMake(cur->last,sp,false);
		cur->last = sp;
	      break;
	      case 'Q': case 'q':
		x1 = strtod((char *) path,&end);
		end = skipcomma(end);
		y1 = strtod(end,&end);
		end = skipcomma(end);
		x = strtod(end,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='q' ) {
		    x1 += current.x; y1 += current.y;
		    x += current.x; y += current.y;
		}
		sp = SplinePointCreate(x,y);
		sp->prevcp.x = x1; sp->prevcp.y = y1; sp->noprevcp = false;
		cur->last->nextcp.x = x1; cur->last->nextcp.y = y1; cur->last->nonextcp = false;
		current = sp->me;
		SplineMake(cur->last,sp,true);
		cur->last = sp;
		order2 = true;
	      break;
	      case 'T': case 't':
		x = strtod((char *) path,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='t' ) {
		    x += current.x; y += current.y;
		}
		x1 = 2*cur->last->me.x - cur->last->prevcp.x;
		y1 = 2*cur->last->me.y - cur->last->prevcp.y;
		sp = SplinePointCreate(x,y);
		sp->prevcp.x = x1; sp->prevcp.y = y1; sp->noprevcp = false;
		cur->last->nextcp.x = x1; cur->last->nextcp.y = y1; cur->last->nonextcp = false;
		current = sp->me;
		SplineMake(cur->last,sp,true);
		cur->last = sp;
		order2 = true;
	      break;
	      case 'A': case 'a':
		rx = strtod((char *) path,&end);
		end = skipcomma(end);
		ry = strtod(end,&end);
		end = skipcomma(end);
		axisrot = strtod(end,&end)*3.1415926535897932/180;
		end = skipcomma(end);
		large_arc = strtol(end,&end,10);
		end = skipcomma(end);
		sweep = strtol(end,&end,10);
		end = skipcomma(end);
		x = strtod(end,&end);
		end = skipcomma(end);
		y = strtod(end,&end);
		if ( type=='a' ) {
		    x += current.x; y += current.y;
		}
		if ( x!=current.x || y!=current.y )
		    SVGTraceArc(cur,&current,x,y,rx,ry,axisrot,large_arc,sweep);
	      break;
	      default:
		LogError( _("Unknown type '%c' found in path specification\n"), type );
	      break;
	    }
	}
	path = (xmlChar *) skipcomma(end);
    }
return( head );
}

static SplineSet *SVGParseExtendedPath(xmlNodePtr svg, xmlNodePtr top) {
    /* Inkscape exends paths by allowing a sprio representation */
    /* But their representation looks nothing like spiros and I can't guess at it */
    xmlChar *outline/*, *effect, *spirooutline*/;
    SplineSet *head = NULL;

    outline = xmlGetProp(svg,(xmlChar *) "d");
    if ( outline!=NULL ) {
	head = SVGParsePath(outline);
	xmlFree(outline);
    }
return( head );
}

static SplineSet *SVGParseRect(xmlNodePtr rect) {
	/* x,y,width,height,rx,ry */
    double x,y,width,height,rx,ry;
    char *num;
    SplinePoint *sp;
    SplineSet *cur;

    num = (char *) xmlGetProp(rect,(xmlChar *) "x");
    if ( num!=NULL ) {
	x = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	x = 0;
    num = (char *) xmlGetProp(rect,(xmlChar *) "width");
    if ( num!=NULL ) {
	width = strtod((char *) num,NULL);
	xmlFree(num);
    } else
return( NULL );
    num = (char *) xmlGetProp(rect,(xmlChar *) "y");
    if ( num!=NULL ) {
	y = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	y = 0;
    num = (char *) xmlGetProp(rect,(xmlChar *) "height");
    if ( num!=NULL ) {
	height = strtod((char *) num,NULL);
	xmlFree(num);
    } else
return( NULL );

    rx = ry = 0;
    num = (char *) xmlGetProp(rect,(xmlChar *) "rx");
    if ( num!=NULL ) {
	ry = rx = strtod((char *) num,NULL);
	xmlFree(num);
    }
    num = (char *) xmlGetProp(rect,(xmlChar *) "ry");
    if ( num!=NULL ) {
	ry = strtod((char *) num,NULL);
	if ( rx==0 ) ry = rx;
	xmlFree(num);
    }

    if ( 2*rx>width ) rx = width/2;
    if ( 2*ry>height ) ry = height/2;

    cur = chunkalloc(sizeof(SplineSet));
    if ( rx==0 ) {
	cur->first = SplinePointCreate(x,y+height);
	cur->last = SplinePointCreate(x+width,y+height);
	SplineMake(cur->first,cur->last,true);
	sp = SplinePointCreate(x+width,y);
	SplineMake(cur->last,sp,true);
	cur->last = sp;
	sp = SplinePointCreate(x,y);
	SplineMake(cur->last,sp,true);
	SplineMake(sp,cur->first,true);
	cur->last = cur->first;
return( cur );
    } else {
	cur->first = SplinePointCreate(x,y+height-ry);
	cur->last = SplinePointCreate(x+rx,y+height);
	cur->first->nextcp.x = x; cur->first->nextcp.y = y+height;
	cur->last->prevcp = cur->first->nextcp;
	cur->first->nonextcp = cur->last->noprevcp = false;
	SplineMake(cur->first,cur->last,false);
	if ( rx<2*width ) {
	    sp = SplinePointCreate(x+width-rx,y+height);
	    SplineMake(cur->last,sp,true);
	    cur->last = sp;
	}
	sp = SplinePointCreate(x+width,y+height-ry);
	sp->prevcp.x = x+width; sp->prevcp.y = y+height;
	cur->last->nextcp = sp->prevcp;
	cur->last->nonextcp = sp->noprevcp = false;
	SplineMake(cur->last,sp,false);
	cur->last = sp;
	if ( ry<2*height ) {
	    sp = SplinePointCreate(x+width,y+ry);
	    SplineMake(cur->last,sp,false);
	    cur->last = sp;
	}
	sp = SplinePointCreate(x+width-rx,y);
	sp->prevcp.x = x+width; sp->prevcp.y = y;
	cur->last->nextcp = sp->prevcp;
	cur->last->nonextcp = sp->noprevcp = false;
	SplineMake(cur->last,sp,false);
	cur->last = sp;
	if ( rx<2*width ) {
	    sp = SplinePointCreate(x+rx,y);
	    SplineMake(cur->last,sp,false);
	    cur->last = sp;
	}
	cur->last->nextcp.x = x; cur->last->nextcp.y = y;
	cur->last->nonextcp = false;
	if ( ry>=2*height ) {
	    cur->first->prevcp = cur->last->nextcp;
	    cur->first->noprevcp = false;
	} else {
	    sp = SplinePointCreate(x,y+ry);
	    sp->prevcp = cur->last->nextcp;
	    sp->noprevcp = false;
	    SplineMake(cur->last,sp,false);
	    cur->last = sp;
	}
	SplineMake(cur->last,cur->first,false);
	cur->first = cur->last;
return( cur );
    }
}

static SplineSet *SVGParseLine(xmlNodePtr line) {
	/* x1,y1, x2,y2 */
    double x,y, x2,y2;
    char *num;
    SplinePoint *sp1, *sp2;
    SplineSet *cur;

    num = (char *) xmlGetProp(line,(xmlChar *) "x1");
    if ( num!=NULL ) {
	x = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	x = 0;
    num = (char *) xmlGetProp(line,(xmlChar *) "x2");
    if ( num!=NULL ) {
	x2 = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	x2 = 0;
    num = (char *) xmlGetProp(line,(xmlChar *) "y1");
    if ( num!=NULL ) {
	y = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	y = 0;
    num = (char *) xmlGetProp(line,(xmlChar *) "y2");
    if ( num!=NULL ) {
	y2 = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	y2 = 0;

    sp1 = SplinePointCreate(x,y);
    sp2 = SplinePointCreate(x2,y2);
    SplineMake(sp1,sp2,false);
    cur = chunkalloc(sizeof(SplineSet));
    cur->first = sp1;
    cur->last = sp2;
return( cur );
}

static SplineSet *SVGParseEllipse(xmlNodePtr ellipse, int iscircle) {
	/* cx,cy,rx,ry */
	/* cx,cy,r */
    double cx,cy,rx,ry;
    char *num;
    SplinePoint *sp;
    SplineSet *cur;

    num = (char *) xmlGetProp(ellipse,(xmlChar *) "cx");
    if ( num!=NULL ) {
	cx = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	cx = 0;
    num = (char *) xmlGetProp(ellipse,(xmlChar *) "cy");
    if ( num!=NULL ) {
	cy = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	cy = 0;
    if ( iscircle ) {
	num = (char *) xmlGetProp(ellipse,(xmlChar *) "r");
	if ( num!=NULL ) {
	    rx = ry = strtod((char *) num,NULL);
	    xmlFree(num);
	} else
return( NULL );
    } else {
	num = (char *) xmlGetProp(ellipse,(xmlChar *) "rx");
	if ( num!=NULL ) {
	    rx = strtod((char *) num,NULL);
	    xmlFree(num);
	} else
return( NULL );
	num = (char *) xmlGetProp(ellipse,(xmlChar *) "ry");
	if ( num!=NULL ) {
	    ry = strtod((char *) num,NULL);
	    xmlFree(num);
	} else
return( NULL );
    }
    if ( rx<0 ) rx = -rx;
    if ( ry<0 ) ry = -ry;

    /* a magic number to make cubic beziers approximate ellipses    */
    /*            4/3 * ( sqrt(2) - 1 ) = 0.55228...                */
    /*   also     4/3 * tan(t/4)   where t = 90 b/c we do 4 curves  */
    double magic = 0.5522847498307933984022516322796;
    /* offset from on-curve point to control points                 */
    double drx = rx * magic;
    double dry = ry * magic;
    cur = chunkalloc(sizeof(SplineSet));
    cur->first = SplinePointCreate(cx-rx,cy);
    cur->first->nextcp.x = cx-rx; cur->first->nextcp.y = cy+dry;
    cur->first->prevcp.x = cx-rx; cur->first->prevcp.y = cy-dry;
    cur->first->noprevcp = cur->first->nonextcp = false;
    cur->last = SplinePointCreate(cx,cy+ry);
    cur->last->prevcp.x = cx-drx; cur->last->prevcp.y = cy+ry;
    cur->last->nextcp.x = cx+drx; cur->last->nextcp.y = cy+ry;
    cur->last->noprevcp = cur->last->nonextcp = false;
    SplineMake(cur->first,cur->last,false);
    sp = SplinePointCreate(cx+rx,cy);
    sp->prevcp.x = cx+rx; sp->prevcp.y = cy+dry;
    sp->nextcp.x = cx+rx; sp->nextcp.y = cy-dry;
    sp->nonextcp = sp->noprevcp = false;
    SplineMake(cur->last,sp,false);
    cur->last = sp;
    sp = SplinePointCreate(cx,cy-ry);
    sp->prevcp.x = cx+drx; sp->prevcp.y = cy-ry;
    sp->nextcp.x = cx-drx; sp->nextcp.y = cy-ry;
    sp->nonextcp = sp->noprevcp = false;
    SplineMake(cur->last,sp,false);
    SplineMake(sp,cur->first,false);
    cur->last = cur->first;
    return( cur );
}

static SplineSet *SVGParsePoly(xmlNodePtr poly, int isgon) {
	/* points */
    double x,y;
    char *pts, *end;
    SplinePoint *sp;
    SplineSet *cur;

    pts = (char *) xmlGetProp(poly,(xmlChar *) "points");
    if ( pts==NULL )
return( NULL );

    x = strtod(pts,&end);
    while ( isspace(*end) || *end==',' ) ++end;
    y = strtod(end,&end);
    while ( isspace(*end)) ++end;

    cur = chunkalloc(sizeof(SplineSet));
    cur->first = cur->last = SplinePointCreate(x,y);
    while ( *end ) {
	x = strtod(end,&end);
	while ( isspace(*end) || *end==',' ) ++end;
	y = strtod(end,&end);
	while ( isspace(*end)) ++end;
	sp = SplinePointCreate(x,y);
	SplineMake(cur->last,sp,false);
	cur->last = sp;
    }
    if ( isgon ) {
	if ( RealNear(cur->last->me.x,cur->first->me.x) &&
		RealNear(cur->last->me.y,cur->first->me.y) ) {
	    cur->first->prev = cur->last->prev;
	    cur->first->prev->to = cur->first;
	    SplinePointFree(cur->last);
	} else
	    SplineMake(cur->last,cur->first,false);
	cur->last = cur->first;
    }
return( cur );
}

struct svg_state {
    double linewidth;
    int dofill, dostroke;
    uint32 fillcol, strokecol;
    float fillopacity, strokeopacity;
    int isvisible;
    enum linecap lc;
    enum linejoin lj;
    real transform[6];
    DashType dashes[DASH_MAX];
    SplineSet *clippath;
    uint8 free_clip;
    uint32 currentColor;
    uint32 stopColor;
    float stopOpacity;
};

static void SVGFigureTransform(struct svg_state *st,char *name) {
    real trans[6], res[6];
    double a, cx,cy;
    char *pt, *paren, *end;
	/* matrix(a,b,c,d,e,f)
	   rotate(theta[,cx,cy])
	   scale(sx[,sy])
	   translate(x,y)
	   skewX(theta)
	   skewY(theta)
	  */

    for ( pt = (char *)name; isspace(*pt); ++pt );
    while ( *pt ) {
	paren = strchr(pt,'(');
	if ( paren==NULL )
    break;
	if ( strncmp(pt,"matrix",paren-pt)==0 ) {
	    trans[0] = strtod(paren+1,&end);
	    trans[1] = strtod(skipcomma(end),&end);
	    trans[2] = strtod(skipcomma(end),&end);
	    trans[3] = strtod(skipcomma(end),&end);
	    trans[4] = strtod(skipcomma(end),&end);
	    trans[5] = strtod(skipcomma(end),&end);
	} else if ( strncmp(pt,"rotate",paren-pt)==0 ) {
	    trans[4] = trans[5] = 0;
	    a = strtod(paren+1,&end)*3.1415926535897932/180;
	    trans[0] = trans[3] = cos(a);
	    trans[1] = sin(a);
	    trans[2] = -trans[1];
	    while ( isspace(*end)) ++end;
	    if ( *end!=')' ) {
		cx = strtod(skipcomma(end),&end);
		cy = strtod(skipcomma(end),&end);
		res[0] = res[3] = 1;
		res[1] = res[2] = 0;
		res[4] = cx; res[5] = cy;
		MatMultiply(res,trans,res);
		trans[0] = trans[3] = 1;
		trans[1] = trans[2] = 0;
		trans[4] = -cx; trans[5] = -cy;
		MatMultiply(res,trans,res);
		memcpy(trans,res,sizeof(res));
	    }
	} else if ( strncmp(pt,"scale",paren-pt)==0 ) {
	    trans[1] = trans[2] = trans[4] = trans[5] = 0;
	    trans[0] = trans[3] = strtod(paren+1,&end);
	    while ( isspace(*end)) ++end;
	    if ( *end!=')' )
		trans[3] = strtod(skipcomma(end),&end);
	} else if ( strncmp(pt,"translate",paren-pt)==0 ) {
	    trans[0] = trans[3] = 1;
	    trans[1] = trans[2] = trans[5] = 0;
	    trans[4] = strtod(paren+1,&end);
	    while ( isspace(*end)) ++end;
	    if ( *end!=')' )
		trans[5] = strtod(skipcomma(end),&end);
	} else if ( strncmp(pt,"skewX",paren-pt)==0 ) {
	    trans[0] = trans[3] = 1;
	    trans[1] = trans[2] = trans[4] = trans[5] = 0;
	    trans[2] = tan(strtod(paren+1,&end)*3.1415926535897932/180);
	} else if ( strncmp(pt,"skewY",paren-pt)==0 ) {
	    trans[0] = trans[3] = 1;
	    trans[1] = trans[2] = trans[4] = trans[5] = 0;
	    trans[1] = tan(strtod(paren+1,&end)*3.1415926535897932/180);
	} else
    break;
	while ( isspace(*end)) ++end;
	if ( *end!=')')
    break;
	MatMultiply(trans,st->transform,st->transform);
	pt = end+1;
	while ( isspace(*pt)) ++pt;
    }
}

static void SVGuseTransform(struct svg_state *st,xmlNodePtr use, xmlNodePtr symbol) {
    double x,y,uwid,uheight,swid,sheight;
    char *num, *end;
    real trans[6];

    num = (char *) xmlGetProp(use,(xmlChar *) "x");
    if ( num!=NULL ) {
	x = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	x = 0;
    num = (char *) xmlGetProp(use,(xmlChar *) "y");
    if ( num!=NULL ) {
	y = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	y = 0;
    if ( x!=0 || y!=0 ) {
	trans[0] = trans[3] = 1;
	trans[1] = trans[2] = 0;
	trans[4] = x; trans[5] = y;
	MatMultiply(trans,st->transform,st->transform);
    }
    num = (char *) xmlGetProp(use,(xmlChar *) "width");
    if ( num!=NULL ) {
	uwid = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	uwid = 0;
    num = (char *) xmlGetProp(use,(xmlChar *) "height");
    if ( num!=NULL ) {
	uheight = strtod((char *) num,NULL);
	xmlFree(num);
    } else
	uheight = 0;
    num = (char *) xmlGetProp(symbol,(xmlChar *) "viewBox");
    if ( num!=NULL ) {
	x = strtod((char *) num,&end);
	y = strtod((char *) end+1,&end);
	swid = strtod((char *) end+1,&end);
	sheight = strtod((char *) end+1,&end);
	xmlFree(num);
    } else
return;
    if ( uwid != 0 || uheight != 0 ) {
	trans[0] = trans[3] = 1;
	trans[1] = trans[2] = trans[4] = trans[5] = 0;
	if ( uwid != 0 && swid!=0 ) trans[0] = uwid/swid;
	if ( uheight != 0 && sheight!=0 ) trans[3] = uheight/sheight;
	MatMultiply(trans,st->transform,st->transform);
    }
}

static real parseGCoord(xmlChar *prop,int bb_units,real bb_low, real bb_high) {
    char *end;
    double val = strtod((char *) prop,&end);

    if ( *end=='%' )
	val /= 100.0;

    if ( bb_units )
	val = bb_low + val*(bb_high-bb_low);
return( val );
}

static int xmlParseColor(xmlChar *name,uint32 *color, char **url,struct svg_state *st);

static void xmlParseColorSource(xmlNodePtr top,char *name,DBounds *bbox,
	struct svg_state *st,struct gradient **_grad,struct epattern **_epat) {
    xmlNodePtr colour_source = XmlFindURI(top,name);
    int islinear;
    xmlChar *prop;
    xmlNodePtr kid;
    int scnt;

    *_grad = NULL; *_epat = NULL;
    if ( colour_source==NULL )
	LogError(_("Could not find Color Source with id %s."), name );
    else if ( (islinear = (xmlStrcmp(colour_source->name,(xmlChar *) "linearGradient")==0)) ||
	    xmlStrcmp(colour_source->name,(xmlChar *) "radialGradient")==0 ) {
	struct gradient *grad = chunkalloc(sizeof(struct gradient));
	int bbox_units;
	*_grad = grad;

	prop = xmlGetProp(colour_source,(xmlChar *) "gradientUnits");
	if ( prop!=NULL ) {
	    bbox_units = xmlStrcmp(prop,(xmlChar *) "userSpaceOnUse")!=0;
	    xmlFree(prop);
	} else
	    bbox_units = true;

	prop = xmlGetProp(colour_source,(xmlChar *) "gradientTransform");
	/* I don't support this currently */
	if ( prop!=NULL )
	    xmlFree(prop);

	grad->sm = sm_pad;
	prop = xmlGetProp(colour_source,(xmlChar *) "spreadMethod");
	if ( prop!=NULL ) {
	    if ( xmlStrcmp(prop,(xmlChar *) "reflect")==0 )
		grad->sm = sm_reflect;
	    else if ( xmlStrcmp(prop,(xmlChar *) "repeat")==0 )
		grad->sm = sm_repeat;
	    xmlFree(prop);
	}

	if ( islinear ) {
	    grad->start.x = bbox->minx; grad->start.y = bbox->miny;
	    grad->stop.x  = bbox->maxx; grad->stop.y  = bbox->maxy;

	    prop = xmlGetProp(colour_source,(xmlChar *) "x1");
	    if ( prop!=NULL ) {
		grad->start.x = parseGCoord( prop,bbox_units,bbox->minx,bbox->maxx);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "x2");
	    if ( prop!=NULL ) {
		grad->stop.x   = parseGCoord( prop,bbox_units,bbox->minx,bbox->maxx);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "y1");
	    if ( prop!=NULL ) {
		grad->start.y = parseGCoord( prop,bbox_units,bbox->miny,bbox->maxy);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "y2");
	    if ( prop!=NULL ) {
		grad->stop.y   = parseGCoord( prop,bbox_units,bbox->miny,bbox->maxy);
		xmlFree(prop);
	    }

	    grad->radius = 0;
	} else {
	    double offx = (bbox->maxx-bbox->minx)/2;
	    double offy = (bbox->maxy-bbox->miny)/2;
	    grad->stop.x = (bbox->minx+bbox->maxx)/2;
	    grad->stop.y = (bbox->minx+bbox->maxy)/2;
	    grad->radius = sqrt(offx*offx + offy*offy);

	    prop = xmlGetProp(colour_source,(xmlChar *) "cx");
	    if ( prop!=NULL ) {
		grad->stop.x = parseGCoord( prop,bbox_units,bbox->minx,bbox->maxx);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "cy");
	    if ( prop!=NULL ) {
		grad->stop.y = parseGCoord( prop,bbox_units,bbox->miny,bbox->maxy);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "radius");
	    if ( prop!=NULL ) {
		grad->radius = parseGCoord( prop,bbox_units,0,sqrt(4*(offx*offx + offy*offy)));
		xmlFree(prop);
	    }

	    grad->start = grad->stop;
	    prop = xmlGetProp(colour_source,(xmlChar *) "fx");
	    if ( prop!=NULL ) {
		grad->start.x = parseGCoord( prop,bbox_units,bbox->minx,bbox->maxx);
		xmlFree(prop);
	    }

	    prop = xmlGetProp(colour_source,(xmlChar *) "fy");
	    if ( prop!=NULL ) {
		grad->start.y = parseGCoord( prop,bbox_units,bbox->miny,bbox->maxy);
		xmlFree(prop);
	    }
	}

	scnt = 0;
	for ( kid = colour_source->children; kid!=NULL; kid=kid->next ) if ( xmlStrcmp(kid->name,(xmlChar *) "stop")==0 )
	    ++scnt;

	if ( scnt==0 ) {
	    /* I'm not sure how to use the style stop-color, but I'm guessing */
	    /*  this might be it */
	    grad->stop_cnt = 1;
	    grad->grad_stops = calloc(1,sizeof(struct grad_stops));
	    grad->grad_stops[scnt].offset = 1;
	    grad->grad_stops[scnt].col = st->stopColor;
	    grad->grad_stops[scnt].opacity = st->stopOpacity;
	    ++scnt;
	} else {
	    grad->stop_cnt = scnt;
	    grad->grad_stops = calloc(scnt,sizeof(struct grad_stops));
	    scnt = 0;
	    for ( kid = colour_source->children; kid!=NULL; kid=kid->next ) if ( xmlStrcmp(kid->name,(xmlChar *) "stop")==0 ) {
		grad->grad_stops[scnt].col = st->stopColor;
		grad->grad_stops[scnt].opacity = st->stopOpacity;

		prop = xmlGetProp(kid,(xmlChar *) "offset");
		if ( prop!=NULL ) {
		    grad->grad_stops[scnt].offset = parseGCoord( prop,false,0,1.0);
		    xmlFree(prop);
		}

		prop = xmlGetProp(kid,(xmlChar *) "stop-color");
		if ( prop!=NULL ) {
		    xmlParseColor(prop, &grad->grad_stops[scnt].col, NULL, st);
		    xmlFree(prop);
		}

		prop = xmlGetProp(kid,(xmlChar *) "stop-opacity");
		if ( prop!=NULL ) {
		    grad->grad_stops[scnt].opacity = strtod((char *) prop,NULL);
		    xmlFree(prop);
		} else
		    grad->grad_stops[scnt].opacity = 1.0;

		++scnt;
	    }
	}
    } else if ( xmlStrcmp(colour_source->name,(xmlChar *) "pattern")==0 ) {
	LogError(_("FontForge does not currently parse pattern Color Sources (%s)."),
		name );
    } else {
	LogError(_("Color Source with id %s had an unexpected type %s."),
		name, (char *) colour_source->name );
    }
}

/* Annoyingly we can't parse the colour source until we have parsed the contents */
/*  because the colour source needs to know the bounding box of the total item */
/*  This means that for something like <g> we must now go back and figure out */
/*  which components get this gradient/pattern, and which have their own source*/
/*  that overrides the inherited one */
static void xmlApplyColourSources(xmlNodePtr top,Entity *head,
	struct svg_state *st,
	char *fill_colour_source,char *stroke_colour_source) {
    DBounds b, ssb;
    Entity *e;
    struct gradient *grad;
    struct epattern *epat;

    memset(&b,0,sizeof(b));
    for ( e=head; e!=NULL; e=e->next ) if ( e->type==et_splines ) {
	SplineSetFindBounds(e->u.splines.splines,&ssb);
	if ( b.minx==0 && b.miny==0 && b.maxx==0 && b.maxy==0 )
	    b = ssb;
	else {
	    if ( b.minx>ssb.minx ) b.minx = ssb.minx;
	    if ( b.maxx>ssb.maxx ) b.maxx = ssb.maxx;
	    if ( b.miny>ssb.miny ) b.miny = ssb.miny;
	    if ( b.maxy>ssb.maxy ) b.maxy = ssb.maxy;
	}
    }
    if ( b.minx==b.maxx ) b.maxx = b.minx+1;
    if ( b.miny==b.maxy ) b.maxy = b.maxy+1;

    if ( fill_colour_source!=NULL ) {
	xmlParseColorSource(top,fill_colour_source,&b,st,&grad,&epat);
	free(fill_colour_source);
	for ( e=head; e!=NULL; e=e->next ) if ( e->type==et_splines ) {
	    if ( e->u.splines.fill.grad==NULL && e->u.splines.fill.tile==NULL &&
		    e->u.splines.fill.col == COLOR_INHERITED ) {
		e->u.splines.fill.grad = GradientCopy(grad,NULL);
		/*e->u.splines.fill.tile = EPatternCopy(epat,NULL);*/
	    }
	}
	GradientFree(grad);
	/*EPatternFree(epat);*/
    }

    if ( stroke_colour_source!=NULL ) {
	xmlParseColorSource(top,stroke_colour_source,&b,st,&grad,&epat);
	free(stroke_colour_source);
	for ( e=head; e!=NULL; e=e->next ) if ( e->type==et_splines ) {
	    if ( e->u.splines.stroke.grad==NULL && e->u.splines.stroke.tile==NULL &&
		    e->u.splines.stroke.col == COLOR_INHERITED ) {
		e->u.splines.stroke.grad = GradientCopy(grad,NULL);
		/*e->u.splines.stroke.tile = EPatternCopy(epat,NULL);*/
	    }
	}
	GradientFree(grad);
	/*EPatternFree(epat);*/
    }
}

static int xmlParseColor(xmlChar *name,uint32 *color, char **url,struct svg_state *st) {
    int doit, i;
    static struct { char *name; uint32 col; } stdcols[] = {
	{ "red", 0xff0000 },
	{ "green", 0x008000 },
	{ "blue", 0x0000ff },
	{ "cyan", 0x00ffff },
	{ "magenta", 0xff00ff },
	{ "yellow", 0xffff00 },
	{ "black", 0x000000 },
	{ "darkgray", 0x404040 },
	{ "darkgrey", 0x404040 },
	{ "gray", 0x808080 },
	{ "grey", 0x808080 },
	{ "lightgray", 0xc0c0c0 },
	{ "lightgrey", 0xc0c0c0 },
	{ "white", 0xffffff },
	{ "maroon", 0x800000 },
	{ "olive", 0x808000 },
	{ "navy", 0x000080 },
	{ "purple", 0x800080 },
	{ "lime", 0x00ff00 },
	{ "aqua", 0x00ffff },
	{ "teal", 0x008080 },
	{ "fuchsia", 0xff0080 },
	{ "silver", 0xc0c0c0 },
	{ NULL, 0 }
    };

    doit = xmlStrcmp(name,(xmlChar *) "none")!=0;
    if ( doit ) {
	for ( i=0; stdcols[i].name!=NULL; ++i )
	    if ( xmlStrcmp(name,(xmlChar *) stdcols[i].name)==0 )
	break;
	if ( stdcols[i].name!=NULL )
	    *color = stdcols[i].col;
	else if ( xmlStrcmp(name,(xmlChar *) "currentColor")==0 )
	    *color = st->currentColor;
	else if ( name[0]=='#' ) {
	    unsigned int temp=0;
	    if ( sscanf( (char *) name, "#%x", &temp )!=1 )
		LogError( _("Bad hex color spec: %s\n"), (char *) name );
	    if ( strlen( (char *) name)==4 ) {
		*color = (((temp&0xf00)*0x11)<<8) |
			 (((temp&0x0f0)*0x11)<<4) |
			 (((temp&0x00f)*0x11)   );
	    } else if ( strlen( (char *) name)==7 ) {
		*color = temp;
	    } else
		*color = COLOR_INHERITED;
	} else if ( strncmp( (char *) name, "rgb(",4)==0 ) {
	    float r=0,g=0,b=0;
	    if ( sscanf((char *)name + 4, "%g,%g,%g", &r, &g, &b )!=3 )
		LogError( _("Bad RGB color spec: %s\n"), (char *) name );
	    if ( strchr((char *) name,'.')!=NULL ) {
		if ( r>=1 ) r = 1; else if ( r<0 ) r=0;
		if ( g>=1 ) g = 1; else if ( g<0 ) g=0;
		if ( b>=1 ) b = 1; else if ( b<0 ) b=0;
		*color = ( ((int) rint(r*255))<<16 ) |
			 ( ((int) rint(g*255))<<8  ) |
			 ( ((int) rint(b*255))     );
	    } else {
		if ( r>=255 ) r = 255; else if ( r<0 ) r=0;
		if ( g>=255 ) g = 255; else if ( g<0 ) g=0;
		if ( b>=255 ) b = 255; else if ( b<0 ) b=0;
		*color = ( ((int) r)<<16 ) |
			 ( ((int) g)<<8  ) |
			 ( ((int) b)     );
	    }
	} else if ( url!=NULL && strncmp( (char *) name, "url(#",5)==0 ) {
	    *url = copy( (char *) name);
	    *color = COLOR_INHERITED;
	} else {
	    LogError( _("Failed to parse color %s\n"), (char *) name );
	    *color = COLOR_INHERITED;
	}
    }
return( doit );
}

static int base64ch(int ch) {
    if ( ch>='A' && ch<='Z' )
return( ch-'A' );
    if ( ch>='a' && ch<='z' )
return( ch-'a'+26 );
    if ( ch>='0' && ch<='9' )
return( ch-'0'+52 );
    if ( ch=='+' )
return( 62 );
    if ( ch=='/' )
return( 63 );
    if ( ch=='=' )
return( 64 );

return( -1 );
}

static void DecodeBase64ToFile(FILE *tmp,char *str) {
    char fourchars[4];
    int i;

    while ( *str ) {
	fourchars[0] = fourchars[1] = fourchars[2] = fourchars[3] = 64;
	for ( i=0; i<4; ++i ) {
	    while ( isspace(*str) || base64ch(*str)==-1 ) ++str;
	    if ( *str=='\0' )
	break;
	    fourchars[i] = base64ch(*str++);
	}
	if ( fourchars[0]<64 && fourchars[1]<64 ) {
	    putc((fourchars[0]<<2)|(fourchars[1]>>4), tmp);
	    if ( fourchars[2]<64 ) {
		putc((fourchars[1]<<4)|(fourchars[2]>>2), tmp);
		if ( fourchars[3]<64 )
		    putc((fourchars[2]<<6)|fourchars[3], tmp);
	    }
	}
    }
}

static GImage *GImageFromDataURI(char *uri) {
    char *mimetype;
    int is_base64=false, ch;
    FILE *tmp;
    GImage *img;

    if ( uri==NULL )
return( NULL );
    if ( strncmp(uri,"data:",5)!=0 )
return( NULL );
    uri += 5;

    mimetype = uri;
    while ( *uri!=',' && *uri!=';' && *uri!='\0' ) ++uri;
    if ( *uri=='\0' )
return( NULL );
    ch = *uri;
    *uri='\0';
    if ( ch==';' && strncmp(uri+1,"base64,",7)==0 ) {
	is_base64=true;
	uri += 6;
	ch = ',';
    } else if ( ch==';' )		/* Can't deal with other encoding methods */
return( NULL );

    ++uri;
    if ( strcmp(mimetype,"image/png")==0 ||
	    strcmp(mimetype,"image/jpeg")==0 ||
	    strcmp(mimetype,"image/bmp")==0 )
	/* These we support (if we've got the libraries) */;
    else {
	LogError(_("Unsupported mime type in data URI: %s\n"), mimetype );
return( NULL );
    }
    tmp = tmpfile();
    if ( is_base64 )
	DecodeBase64ToFile(tmp,uri);
    else {
	while ( *uri ) {
	    putc(*uri,tmp);
	    ++uri;
	}
    }
    rewind(tmp);
#ifndef _NO_LIBPNG
    if ( strcmp(mimetype,"image/png")==0 )
	img = GImageRead_Png(tmp);
    else
#endif
#ifndef _NO_LIBJPEG
    if ( strcmp(mimetype,"image/jpeg")==0 )
	img = GImageRead_Jpeg(tmp);
    else
#endif
    if ( strcmp(mimetype,"image/bmp")==0 )
	img = GImageRead_Bmp(tmp);
    else
	img = NULL;
    fclose(tmp);
return( img );
}

static Entity *SVGParseImage(xmlNodePtr svg) {
    double x=0,y=0,width=1,height=1;
    GImage *img;
    struct _GImage *base;
    Entity *ent;
    xmlChar *val;

    val = xmlGetProp(svg,(xmlChar *) "x");
    if ( val!=NULL ) {
	x = strtod((char *) val,NULL);
	free(val);
    }
    val = xmlGetProp(svg,(xmlChar *) "y");
    if ( val!=NULL ) {
	y = strtod((char *) val,NULL);
	free(val);
    }

    val = xmlGetProp(svg,(xmlChar *) "width");
    if ( val!=NULL ) {
	width = strtod((char *) val,NULL);
	free(val);
    }
    val = xmlGetProp(svg,(xmlChar *) "height");
    if ( val!=NULL ) {
	height = strtod((char *) val,NULL);
	free(val);
    }

    val = xmlGetProp(svg,(xmlChar *) /*"xlink:href"*/ "href");
    if ( val==NULL )
return( NULL );
    if ( strncmp((char *) val,"data:",5)!=0 ) {
	LogError(_("FontForge only supports embedded images in data: URIs\n"));
	free(val);
return( NULL );		/* I can only handle data URIs */
    }
    img = GImageFromDataURI((char *) val);
    free(val);
    if ( img==NULL )
return( NULL );
    base = img->list_len==0 ? img->u.image : img->u.images[0];

    ent = chunkalloc(sizeof(Entity));
    ent->type = et_image;
    ent->u.image.image = img;
    ent->u.image.transform[1] = ent->u.image.transform[2] = 0;
    ent->u.image.transform[0] = width/base->width;
    ent->u.image.transform[3] = height/base->height;
    ent->u.image.transform[4] = x;
    ent->u.image.transform[5] = y;
    ent->u.image.col = 0xffffffff;
return( ent );
}

static Entity *EntityCreate(SplinePointList *head,struct svg_state *state) {
    Entity *ent = calloc(1,sizeof(Entity));
    ent->type = et_splines;
    ent->u.splines.splines = head;
    ent->u.splines.cap = state->lc;
    ent->u.splines.join = state->lj;
    ent->u.splines.stroke_width = state->linewidth;
    ent->u.splines.fill.col = state->dofill ? state->fillcol : state->dostroke ? 0xffffffff : COLOR_INHERITED;
    ent->u.splines.stroke.col = state->dostroke ? state->strokecol : 0xffffffff;
    ent->u.splines.fill.opacity = state->fillopacity;
    ent->u.splines.stroke.opacity = state->strokeopacity;
    memcpy(ent->u.splines.transform,state->transform,6*sizeof(real));
    ent->clippath = SplinePointListCopy(state->clippath);
return( ent );
}

static void SvgStateFree(struct svg_state *st) {
    if ( st->free_clip )
	SplinePointListFree(st->clippath);
}

static void SVGFigureStyle(struct svg_state *st,char *name,
	char **fill_colour_source,char **stroke_colour_source) {
    char *pt;
    char namebuf[200], propbuf[400];

    for (;;) {
	while ( isspace(*name)) ++name;
	if ( *name==':' ) {
	    /* Missing prop name, skip the value */
	    while ( *name!=';' && *name!='\0' ) ++name;
	    if ( *name==';' ) ++name;
	} else if ( *name!='\0' && *name!=';' ) {
	    pt = namebuf;
	    while ( *name!='\0' && *name!=':' && *name!=';' && !isspace(*name) ) {
		if ( pt<namebuf+sizeof(namebuf)-1 )
		    *pt++ = *name;
		++name;
	    }
	    *pt = '\0';
	    while ( *name!=':' && *name!=';' && *name!='\0' ) ++name;
	    if ( *name==':' ) ++name;

	    /* fetch prop value */
	    while ( isspace(*name) ) ++name;
	    propbuf[0] = '\0';
	    if ( *name!='\0' && *name!=';' ) {
		pt = propbuf;
		while ( *name!='\0' && *name!=';' && !isspace(*name) ) {
		    if ( pt<propbuf+sizeof(propbuf)-1 )
			*pt++ = *name;
		    ++name;
		}
		*pt = '\0';
	    }
	    while ( *name!=';' && *name!='\0' ) ++name;
	    if ( *name==';' ) ++name;

	    if ( strcmp(namebuf,"color")==0 )
		xmlParseColor(propbuf,&st->currentColor,NULL,st);
	    else if ( strcmp(namebuf,"fill")==0 )
		st->dofill = xmlParseColor(propbuf,&st->fillcol,fill_colour_source,st);
	    else if ( strcmp(namebuf,"visibility")==0 )
		st->isvisible = strcmp(propbuf,"hidden")!=0 &&
			strcmp(propbuf,"colapse")!=0;
	    else if ( strcmp(namebuf,"fill-opacity")==0 )
		st->fillopacity = strtod(propbuf,NULL);
	    else if ( strcmp(namebuf,"stroke")==0 )
		st->dostroke = xmlParseColor(propbuf,&st->strokecol,stroke_colour_source,st);
	    else if ( strcmp(namebuf,"stroke-opacity")==0 )
		st->strokeopacity = strtod((char *)propbuf,NULL);
	    else if ( strcmp(namebuf,"stroke-width")==0 )
		st->linewidth = strtod((char *)propbuf,NULL);
	    else if ( strcmp(namebuf,"stroke-linecap")==0 )
		st->lc = strcmp(propbuf,"butt") ? lc_butt :
			     strcmp(propbuf,"round") ? lc_round :
			     lc_square;
	    else if ( strcmp(namebuf,"stroke-linejoin")==0 )
		st->lj = strcmp(propbuf,"miter") ? lj_miter :
			     strcmp(propbuf,"round") ? lj_round :
			     lj_bevel;
	    else if ( strcmp(namebuf,"stroke-dasharray")==0 ) {
		if ( strcmp(propbuf,"inherit") ) {
		    st->dashes[0] = 0; st->dashes[1] = DASH_INHERITED;
		} else if ( strcmp(propbuf,"none") ) {
		    st->dashes[0] = 0; st->dashes[1] = 0;
		} else {
		    int i;
		    char *pt, *end;
		    pt = propbuf;
		    for ( i=0; i<DASH_MAX && *pt!='\0'; ++i ) {
			st->dashes[i] = strtol( pt, &end,10);
			pt = end;
		    }
		    if ( i<DASH_MAX ) st->dashes[i] = 0;
		}
	    } else if ( strcmp(namebuf,"stop-color")==0 )
		xmlParseColor(propbuf,&st->stopColor,NULL,st);
	    else if ( strcmp(namebuf,"stop-opacity")==0 )
		st->stopOpacity = strtod(propbuf,NULL);
	    else {
		/* Lots of props we ignore */
		;
            }
	} else if ( *name==';' )
	    ++name;
	if ( *name=='\0' )
    break;
    }
}

static Entity *_SVGParseSVG(xmlNodePtr svg, xmlNodePtr top,
	struct svg_state *inherit) {
    struct svg_state st;
    xmlChar *name;
    xmlNodePtr kid;
    Entity *ehead, *elast, *eret;
    SplineSet *head;
    int treat_symbol_as_g = false;
    char *fill_colour_source = NULL, *stroke_colour_source = NULL;

    if ( svg==NULL )
return( NULL );

    st = *inherit;
    st.free_clip = false;
  tail_recurse:
    name = xmlGetProp(svg,(xmlChar *) "display");
    if ( name!=NULL ) {
	int hide = xmlStrcmp(name,(xmlChar *) "none")==0;
	xmlFree(name);
	if ( hide )
return( NULL );
    }
    name = xmlGetProp(svg,(xmlChar *) "visibility");
    if ( name!=NULL ) {
	st.isvisible = xmlStrcmp(name,(xmlChar *) "hidden")!=0 &&
		xmlStrcmp(name,(xmlChar *) "colapse")!=0;
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "fill");
    if ( name!=NULL ) {
	st.dofill = xmlParseColor(name,&st.fillcol,&fill_colour_source,&st);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "fill-opacity");
    if ( name!=NULL ) {
	st.fillopacity = strtod((char *)name,NULL);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke");
    if ( name!=NULL ) {
	st.dostroke = xmlParseColor(name,&st.strokecol,&stroke_colour_source,&st);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke-opacity");
    if ( name!=NULL ) {
	st.strokeopacity = strtod((char *)name,NULL);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke-width");
    if ( name!=NULL ) {
	st.linewidth = strtod((char *)name,NULL);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke-linecap");
    if ( name!=NULL ) {
	st.lc = xmlStrcmp(name,(xmlChar *) "butt") ? lc_butt :
		     xmlStrcmp(name,(xmlChar *) "round") ? lc_round :
		     lc_square;
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke-linejoin");
    if ( name!=NULL ) {
	st.lj = xmlStrcmp(name,(xmlChar *) "miter") ? lj_miter :
		     xmlStrcmp(name,(xmlChar *) "round") ? lj_round :
		     lj_bevel;
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "stroke-dasharray");
    if ( name!=NULL ) {
	if ( xmlStrcmp(name,(xmlChar *) "inherit") ) {
	    st.dashes[0] = 0; st.dashes[1] = DASH_INHERITED;
	} else if ( xmlStrcmp(name,(xmlChar *) "none") ) {
	    st.dashes[0] = 0; st.dashes[1] = 0;
	} else {
	    int i;
	    xmlChar *pt, *end;
	    pt = name;
	    for ( i=0; i<DASH_MAX && *pt!='\0'; ++i ) {
		st.dashes[i] = strtol((char *) pt,(char **) &end,10);
		pt = end;
	    }
	    if ( i<DASH_MAX ) st.dashes[i] = 0;
	}
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "style");
    if ( name!=NULL ) {
	SVGFigureStyle(&st,(char *) name, &fill_colour_source, &stroke_colour_source);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "transform");
    if ( name!=NULL ) {
	SVGFigureTransform(&st,(char *) name);
	xmlFree(name);
    }
    name = xmlGetProp(svg,(xmlChar *) "clip-path");
    if ( name!=NULL ) {
	xmlNodePtr clip = XmlFindURI(top,(char *) name);
	if ( clip!=NULL && xmlStrcmp(clip->name,(xmlChar *) "clipPath")==0) {
	    const xmlChar *temp = clip->name;
	    struct svg_state null_state;
	    memset(&null_state,0,sizeof(null_state));
	    null_state.isvisible = true;
	    null_state.transform[0] = null_state.transform[3] = 1;
	    clip->name = (xmlChar *) "g";
	    eret = _SVGParseSVG(clip,top,&null_state);
	    clip->name = temp;
	    if ( eret!=NULL && eret->type == et_splines ) {
		st.clippath = eret->u.splines.splines;
		eret->u.splines.splines = NULL;
	    }
	    free(eret);
	} else
	    LogError(_("Could not find clippath named %s."), name );
	xmlFree(name);
    }

    if ( (treat_symbol_as_g && xmlStrcmp(svg->name,(xmlChar *) "symbol")==0) ||
	    xmlStrcmp(svg->name,(xmlChar *) "svg")==0 ||
	    xmlStrcmp(svg->name,(xmlChar *) "glyph")==0 ||
	    xmlStrcmp(svg->name,(xmlChar *) "pattern")==0 ||
	    xmlStrcmp(svg->name,(xmlChar *) "g")==0 ) {
	ehead = elast = NULL;
	for ( kid = svg->children; kid!=NULL; kid=kid->next ) {
	    eret = _SVGParseSVG(kid,top,&st);
	    if ( eret!=NULL ) {
		if ( elast==NULL )
		    ehead = eret;
		else
		    elast->next = eret;
		while ( eret->next!=NULL ) eret = eret->next;
		elast = eret;
	    }
	}
	SvgStateFree(&st);
	if ( fill_colour_source!=NULL || stroke_colour_source!=NULL )
	    xmlApplyColourSources(top,ehead,&st,fill_colour_source,stroke_colour_source);
return( ehead );
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "use")==0 ) {
	name = xmlGetProp(svg,(xmlChar *) "href");
	kid = NULL;
	if ( name!=NULL && *name=='#' ) {	/* Within this file */
	    kid = XmlFindID(top,(char *) name+1);
	    treat_symbol_as_g = true;
	}
	SVGuseTransform(&st,svg,kid);
	svg = kid;
	if ( name!=NULL )
	    xmlFree(name);
	if ( svg!=NULL )
  goto tail_recurse;
	SvgStateFree(&st);
return( NULL );
    }

    if ( !st.isvisible ) {
	SvgStateFree(&st);
return( NULL );
    }

    /* basic shapes */
    head = NULL;
    if ( xmlStrcmp(svg->name,(xmlChar *) "path")==0 ) {
	head = SVGParseExtendedPath(svg,top);
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "rect")==0 ) {
	head = SVGParseRect(svg);		/* x,y,width,height,rx,ry */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "circle")==0 ) {
	head = SVGParseEllipse(svg,true);	/* cx,cy, r */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "ellipse")==0 ) {
	head = SVGParseEllipse(svg,false);	/* cx,cy, rx,ry */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "line")==0 ) {
	head = SVGParseLine(svg);		/* x1,y1, x2,y2 */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "polyline")==0 ) {
	head = SVGParsePoly(svg,0);		/* points */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "polygon")==0 ) {
	head = SVGParsePoly(svg,1);		/* points */
    } else if ( xmlStrcmp(svg->name,(xmlChar *) "image")==0 ) {
	eret = SVGParseImage(svg);
	if (eret!=NULL)
	    eret->clippath = st.clippath;
return( eret );
    } else
return( NULL );
    if ( head==NULL )
return( NULL );

    SPLCategorizePoints(head);

    eret = EntityCreate(SplinePointListTransform(head,st.transform,tpt_AllPoints), &st);
    if ( fill_colour_source!=NULL || stroke_colour_source!=NULL )
	xmlApplyColourSources(top,eret,&st,fill_colour_source,stroke_colour_source);
    SvgStateFree(&st);
return( eret );
}

static Entity *SVGParseSVG(xmlNodePtr svg,int em_size,int ascent) {
    struct svg_state st;
    char *num, *end;
    double swidth,sheight,width=1,height=1;

    memset(&st,0,sizeof(st));
    st.lc = lc_inherited;
    st.lj = lj_inherited;
    st.linewidth = WIDTH_INHERITED;
    st.fillcol = COLOR_INHERITED;
    st.strokecol = COLOR_INHERITED;
    st.currentColor = COLOR_INHERITED;
    st.stopColor = COLOR_INHERITED;
    st.isvisible = true;
    st.transform[0] = 1;
    st.transform[3] = -1;	/* The SVG coord system has y increasing down */
    				/*  Font coords have y increasing up */
    st.transform[5] = ascent;
    st.strokeopacity = st.fillopacity = 1.0;

    num = (char *) xmlGetProp(svg,(xmlChar *) "width");
    if ( num!=NULL ) {
	width = strtod(num,NULL);
	xmlFree(num);
    }
    num = (char *) xmlGetProp(svg,(xmlChar *) "height");
    if ( num!=NULL ) {
	height = strtod(num,NULL);
	xmlFree(num);
    }
    if ( height<=0 ) height = 1;
    if ( width<=0 ) width = 1;
    num = (char *) xmlGetProp(svg,(xmlChar *) "viewBox");
    if ( num!=NULL ) {
	/* x = */strtod((char *) num,&end);
	/* y = */strtod((char *) end+1,&end);
	swidth = strtod((char *) end+1,&end);
	sheight = strtod((char *) end+1,&end);
	xmlFree(num);
	if ( width>height ) {
	    if ( swidth!=0 ) {
		st.transform[0] *= em_size/swidth;
		st.transform[3] *= em_size/swidth;
	    }
	} else {
	    if ( sheight!=0 ) {
		st.transform[0] *= em_size/sheight;
		st.transform[3] *= em_size/sheight;
	    }
	}
    }
return( _SVGParseSVG(svg,svg,&st));
}

static void SVGParseGlyphBody(SplineChar *sc, xmlNodePtr glyph,int *flags) {
    xmlChar *path;

    path = xmlGetProp(glyph,(xmlChar *) "d");
    if ( path!=NULL ) {
	sc->layers[ly_fore].splines = SVGParseExtendedPath(glyph,glyph);
	xmlFree(path);
    } else {
	Entity *ent = SVGParseSVG(glyph,sc->parent->ascent+sc->parent->descent,
		sc->parent->ascent);
	sc->layer_cnt = 1;
	SCAppendEntityLayers(sc,ent);
	if ( sc->layer_cnt==1 ) ++sc->layer_cnt;
	else sc->parent->multilayer = true;
    }

    SCCategorizePoints(sc);
}

static SplineChar *SVGParseGlyphArgs(xmlNodePtr glyph,int defh, int defv,
	SplineFont *sf) {
    SplineChar *sc = SFSplineCharCreate(sf);
    xmlChar *name, *form, *glyphname, *unicode, *orientation;
    uint32 *u;
    char buffer[100];

    name = xmlGetProp(glyph,(xmlChar *) "horiz-adv-x");
    if ( name!=NULL ) {
	sc->width = strtod((char *) name,NULL);
        sc->widthset = true;
	xmlFree(name);
    } else
	sc->width = defh;
    name = xmlGetProp(glyph,(xmlChar *) "vert-adv-y");
    if ( name!=NULL ) {
	sc->vwidth = strtod((char *) name,NULL);
	xmlFree(name);
    } else
	sc->vwidth = defv;
    name = xmlGetProp(glyph,(xmlChar *) "vert-adv-y");
    if ( name!=NULL ) {
	sc->vwidth = strtod((char *) name,NULL);
	xmlFree(name);
    } else
	sc->vwidth = defv;

    form = xmlGetProp(glyph,(xmlChar *) "arabic-form");
    unicode = xmlGetProp(glyph,(xmlChar *) "unicode");
    glyphname = xmlGetProp(glyph,(xmlChar *) "glyph-name");
    orientation = xmlGetProp(glyph,(xmlChar *) "orientation");
    if ( unicode!=NULL ) {

	u = utf82u_copy((char *) unicode);
	xmlFree(unicode);
	if ( u[1]=='\0' ) {
	    sc->unicodeenc = u[0];
	    if ( form!=NULL && u[0]>=0x600 && u[0]<=0x6ff ) {
		if ( xmlStrcmp(form,(xmlChar *) "initial")==0 )
		    sc->unicodeenc = ArabicForms[u[0]-0x600].initial;
		else if ( xmlStrcmp(form,(xmlChar *) "medial")==0 )
		    sc->unicodeenc = ArabicForms[u[0]-0x600].medial;
		else if ( xmlStrcmp(form,(xmlChar *) "final")==0 )
		    sc->unicodeenc = ArabicForms[u[0]-0x600].final;
		else if ( xmlStrcmp(form,(xmlChar *) "isolated")==0 )
		    sc->unicodeenc = ArabicForms[u[0]-0x600].isolated;
	    }
	}
	free(u);
    }
    if ( glyphname!=NULL ) {
	if ( sc->unicodeenc==-1 )
	    sc->unicodeenc = UniFromName((char *) glyphname,ui_none,&custom);
	sc->name = copy((char *) glyphname);
	xmlFree(glyphname);
    } else if ( orientation!=NULL && *orientation=='v' && sc->unicodeenc!=-1 ) {
	if ( sc->unicodeenc<0x10000 )
	    sprintf( buffer, "uni%04X.vert", sc->unicodeenc );
	else
	    sprintf( buffer, "u%04X.vert", sc->unicodeenc );
	sc->name = copy( buffer );
    }
    /* we finish off defaulting the glyph name in the parseglyph routine */
    if ( form!=NULL )
	xmlFree(form);
    if ( orientation!=NULL )
	xmlFree(orientation);
return( sc );
}

static SplineChar *SVGParseMissing(SplineFont *sf,xmlNodePtr notdef,int defh, int defv, int enc, int *flags) {
    SplineChar *sc = SVGParseGlyphArgs(notdef,defh,defv,sf);
    sc->parent = sf;
    sc->name = copy(".notdef");
    sc->unicodeenc = 0;
    SVGParseGlyphBody(sc,notdef,flags);
return( sc );
}

static SplineChar *SVGParseGlyph(SplineFont *sf,xmlNodePtr glyph,int defh, int defv, int enc, int *flags) {
    char buffer[400];
    SplineChar *sc = SVGParseGlyphArgs(glyph,defh,defv,sf);
    sc->parent = sf;
    if ( sc->name==NULL ) {
	if ( sc->unicodeenc==-1 ) {
	    sprintf( buffer, "glyph%d", enc);
	    sc->name = copy(buffer);
	} else
	    sc->name = copy(StdGlyphName(buffer,sc->unicodeenc,ui_none,NULL));
    }
    SVGParseGlyphBody(sc,glyph,flags);
return( sc );
}

static void SVGLigatureFixupCheck(SplineChar *sc,xmlNodePtr glyph) {
    xmlChar *unicode;
    uint32 *u;
    int len, len2;
    SplineChar **chars, *any = NULL;
    char *comp, *pt;

    unicode = xmlGetProp(glyph,(xmlChar *) "unicode");
    if ( unicode!=NULL ) {
	u = utf82u_copy((char *) unicode);
	xmlFree(unicode);
	if ( u[1]!='\0' && u[2]=='\0' &&
		((u[1]>=0x180B && u[1]<=0x180D) ||	/* Mongolian VS */
		 (u[1]>=0xfe00 && u[1]<=0xfe0f) ||	/* First VS block */
		 (u[1]>=0xE0100 && u[1]<=0xE01EF)) ) {	/* Second VS block */
	    /* Problably a variant glyph marked with a variation selector */
	    /* ... not a true ligature at all */
	    /* http://babelstone.blogspot.com/2007/06/secret-life-of-variation-selectors.html */
	    struct altuni *altuni = chunkalloc(sizeof(struct altuni));
	    altuni->unienc = u[0];
	    altuni->vs = u[1];
	    altuni->fid = 0;
	    altuni->next = sc->altuni;
	    sc->altuni = altuni;
	} else if ( u[1]!='\0' ) {
	    /* Normal ligature */
	    for ( len=0; u[len]!=0; ++len );
	    chars = malloc(len*sizeof(SplineChar *));
	    for ( len=len2=0; u[len]!=0; ++len ) {
		chars[len] = SFGetChar(sc->parent,u[len],NULL);
		if ( chars[len]==NULL )
		    len2 += 9;
		else {
		    len2 += strlen(chars[len]->name)+1;
		    if ( any==NULL ) any = chars[len];
		}
	    }
	    if ( any==NULL ) any=sc;
	    comp = pt = malloc(len2+1);
	    *pt = '\0';
	    for ( len=0; u[len]!=0; ++len ) {
		if ( chars[len]!=NULL )
		    strcpy(pt,chars[len]->name);
		else if ( u[len]<0x10000 )
		    sprintf(pt,"uni%04X",  (unsigned int) u[len]);
		else
		    sprintf(pt,"u%04X",  (unsigned int) u[len]);
		pt += strlen(pt);
		if ( u[len+1]!='\0' )
		    *pt++ = ' ';
	    }
	    SubsNew(sc,pst_ligature,CHR('l','i','g','a'),comp,any);
		/* Understand the unicode latin ligatures. There are too many */
		/*  arabic ones */
	    if ( u[0]=='f' ) {
		if ( u[1]=='f' && u[2]==0 )
		    sc->unicodeenc = 0xfb00;
		else if ( u[1]=='i' && u[2]==0 )
		    sc->unicodeenc = 0xfb01;
		else if ( u[1]=='l' && u[2]==0 )
		    sc->unicodeenc = 0xfb02;
		else if ( u[1]=='f' && u[2]=='i' && u[3]==0 )
		    sc->unicodeenc = 0xfb03;
		else if ( u[1]=='f' && u[2]=='l' && u[3]==0 )
		    sc->unicodeenc = 0xfb04;
		else if ( u[1]=='t' && u[2]==0 )
		    sc->unicodeenc = 0xfb05;
	    } else if ( u[0]=='s' && u[1]=='t' && u[2]==0 )
		sc->unicodeenc = 0xfb06;
	    if ( strncmp(sc->name,"glyph",5)==0 && isdigit(sc->name[5])) {
		/* It's a default name, we can do better */
		free(sc->name);
		sc->name = copy(comp);
		for ( pt = sc->name; *pt; ++pt )
		    if ( *pt==' ' ) *pt = '_';
	    }
	}
	free(u);
    }
}

static char *SVGGetNames(SplineFont *sf,xmlChar *g,xmlChar *utf8,SplineChar **sc) {
    uint32 *u=NULL;
    char *names;
    int len, i, ch;
    SplineChar *temp;
    char *pt, *gpt;

    *sc = NULL;
    len = 0;
    if ( utf8!=NULL ) {
	u = utf82u_copy((char *) utf8);
	for ( i=0; u[i]!=0; ++i ) {
	    temp = SFGetChar(sf,u[i],NULL);
	    if ( temp!=NULL ) {
		if ( *sc==NULL ) *sc = temp;
		len += strlen(temp->name)+1;
	    }
	}
    }
    names = pt = malloc(len+(g!=NULL?strlen((char *)g):0)+1);
    if ( utf8!=NULL ) {
	for ( i=0; u[i]!=0; ++i ) {
	    temp = SFGetChar(sf,u[i],NULL);
	    if ( temp!=NULL ) {
		strcpy(pt,temp->name);
		pt += strlen( pt );
		*pt++ = ' ';
	    }
	}
	free(u);
    }
    if ( g!=NULL ) {
	for ( gpt=(char *) g; *gpt; ) {
	    if ( *gpt==',' || isspace(*gpt)) {
		while ( *gpt==',' || isspace(*gpt)) ++gpt;
		*pt++ = ' ';
	    } else {
		*pt++ = *gpt++;
	    }
	}
	if ( *sc==NULL ) {
	    for ( gpt = (char *) g; *gpt!='\0' && *gpt!=',' && !isspace(*gpt); ++gpt );
	    ch = *gpt; *gpt = '\0';
	    *sc = SFGetChar(sf,-1,(char *) g);
	    *gpt = ch;
	}
    }
    if ( pt>names && pt[-1]==' ' ) --pt;
    *pt = '\0';
return( names );
}

static void SVGParseKern(SplineFont *sf,xmlNodePtr kern,int isv) {
    xmlChar *k, *g1, *u1, *g2, *u2;
    double off;
    char *c1, *c2;
    char *pt1, *pt2, *end1, *end2;
    SplineChar *sc1, *sc2;
    uint32 script;
    struct lookup_subtable *subtable;

    k = xmlGetProp(kern,(xmlChar *) "k");
    if ( k==NULL )
return;
    off = -strtod((char *)k, NULL);
    xmlFree(k);
    if ( off==0 )
return;

    g1 = xmlGetProp(kern,(xmlChar *) "g1");
    u1 = xmlGetProp(kern,(xmlChar *) "u1");
    if ( g1==NULL && u1==NULL )
return;
    c1 = SVGGetNames(sf,g1,u1,&sc1);
    if ( g1!=NULL ) xmlFree(g1);
    if ( u1!=NULL ) xmlFree(u1);

    g2 = xmlGetProp(kern,(xmlChar *) "g2");
    u2 = xmlGetProp(kern,(xmlChar *) "u2");
    if ( g2==NULL && u2==NULL ) {
	free(c1);
return;
    }
    c2 = SVGGetNames(sf,g2,u2,&sc2);
    if ( g2!=NULL ) xmlFree(g2);
    if ( u2!=NULL ) xmlFree(u2);

    script = DEFAULT_SCRIPT;
    if ( sc1!=NULL )
	script = SCScriptFromUnicode(sc1);
    if ( script==DEFAULT_SCRIPT && sc2!=NULL )
	script = SCScriptFromUnicode(sc2);
/* It is tempting to use a kern class... but it doesn't work. No guarantees */
/*  that either of our two "classes" will ever show again. We could do the */
/*  complex shinanigans we do in parsing feature files, but it's a lot easier */
/*  just to make kernpairs. */
    subtable = SFSubTableFindOrMake(sf,
		isv?CHR('v','k','r','n'):CHR('k','e','r','n'),
		script, gpos_pair);
    subtable->lookup->lookup_flags = ((sc1!=NULL && SCRightToLeft(sc1)) ||
		    (sc1==NULL && sc2!=NULL && SCRightToLeft(sc2)))? pst_r2l : 0;
    for ( pt1=c1 ; ; ) {
	while ( *pt1==' ' ) ++pt1;
	if ( *pt1=='\0' )
    break;
	end1 = strchr(pt1,' ');
	if ( end1!=NULL ) *end1 = '\0';
	sc1 = SFGetChar(sf,-1,pt1);
	if ( sc1!=NULL ) for ( pt2=c2 ; ; ) {
	    while ( *pt2==' ' ) ++pt2;
	    if ( *pt2=='\0' )
	break;
	    end2 = strchr(pt2,' ');
	    if ( end2!=NULL ) *end2 = '\0';
	    sc2 = SFGetChar(sf,-1,pt2);
	    if ( sc2!=NULL ) {
		KernPair *kp = chunkalloc(sizeof(KernPair));
		kp->sc = sc2;
		kp->off = off;
		if ( isv ) {
		    kp->next = sc1->vkerns;
		    sc1->vkerns = kp;
		} else {
		    kp->next = sc1->kerns;
		    sc1->kerns = kp;
		}
		kp->subtable = subtable;
	    }
	    if ( end2==NULL )
	break;
	    *end2 = ' ';
	    pt2 = end2+1;
	}
	if ( end1==NULL )
    break;
	*end1 = ' ';
	pt1 = end1+1;
    }
    free(c1); free(c2);
}

static SplineFont *SVGParseFont(xmlNodePtr font) {
    int cnt, flags = -1;
    xmlNodePtr kids;
    int defh=0, defv=0;
    int has_font_face = false;
    xmlChar *name;
    SplineFont *sf;
    EncMap *map;
    int i;

    sf = SplineFontEmpty();
    name = xmlGetProp(font,(xmlChar *) "horiz-adv-x");
    if ( name!=NULL ) {
	defh = strtod((char *) name,NULL);
	xmlFree(name);
    }
    name = xmlGetProp(font,(xmlChar *) "vert-adv-y");
    if ( name!=NULL ) {
	defv = strtod((char *) name,NULL);
	xmlFree(name);
	sf->hasvmetrics = true;
    }
    name = xmlGetProp(font,(xmlChar *) "id");
    if ( name!=NULL ) {
	sf->fontname = copy( (char *) name);
	xmlFree(name);
    }

    cnt = 0;
    for ( kids = font->children; kids!=NULL; kids=kids->next ) {
	int ascent=0, descent=0;
	if ( xmlStrcmp(kids->name,(const xmlChar *) "font-face")==0 ) {
	    has_font_face = true;
	    name = xmlGetProp(kids,(xmlChar *) "units-per-em");
	    if ( name!=NULL ) {
		int val = rint(strtod((char *) name,NULL));
		xmlFree(name);
		if ( val<0 ) val = 0;
		sf->ascent = val*800/1000;
		sf->descent = val - sf->ascent;
		if ( defv==0 ) defv = val;
		if ( defh==0 ) defh = val;
		SFDefaultOS2Simple(&sf->pfminfo,sf);
	    } else {
		LogError( _("This font does not specify units-per-em\n") );
		SplineFontFree(sf);
return( NULL );
	    }
	    name = xmlGetProp(kids,(xmlChar *) "font-family");
	    if ( name!=NULL ) {
		if ( strchr((char *) name,',')!=NULL )
		    *strchr((char *) name,',') ='\0';
		sf->familyname = copy( (char *) name);
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "font-weight");
	    if ( name!=NULL ) {
		if ( strnmatch((char *) name,"normal",6)==0 ) {
		    sf->pfminfo.weight = 400;
		    sf->weight = copy("Regular");
		    sf->pfminfo.panose[2] = 5;
		} else if ( strnmatch((char *) name,"bold",4)==0 ) {
		    sf->pfminfo.weight = 700;
		    sf->weight = copy("Bold");
		    sf->pfminfo.panose[2] = 8;
		} else {
		    sf->pfminfo.weight = strtod((char *) name,NULL);
		    if ( sf->pfminfo.weight <= 100 ) {
			sf->weight = copy("Thin");
			sf->pfminfo.panose[2] = 2;
		    } else if ( sf->pfminfo.weight <= 200 ) {
			sf->weight = copy("Extra-Light");
			sf->pfminfo.panose[2] = 3;
		    } else if ( sf->pfminfo.weight <= 300 ) {
			sf->weight = copy("Light");
			sf->pfminfo.panose[2] = 4;
		    } else if ( sf->pfminfo.weight <= 400 ) {
			sf->weight = copy("Regular");
			sf->pfminfo.panose[2] = 5;
		    } else if ( sf->pfminfo.weight <= 500 ) {
			sf->weight = copy("Medium");
			sf->pfminfo.panose[2] = 6;
		    } else if ( sf->pfminfo.weight <= 600 ) {
			sf->weight = copy("DemiBold");
			sf->pfminfo.panose[2] = 7;
		    } else if ( sf->pfminfo.weight <= 700 ) {
			sf->weight = copy("Bold");
			sf->pfminfo.panose[2] = 8;
		    } else if ( sf->pfminfo.weight <= 800 ) {
			sf->weight = copy("Heavy");
			sf->pfminfo.panose[2] = 9;
		    } else {
			sf->weight = copy("Black");
			sf->pfminfo.panose[2] = 10;
		    }
		}
		sf->pfminfo.panose_set = true;
		sf->pfminfo.pfmset = true;
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "font-stretch");
	    if ( name!=NULL ) {
		if ( strnmatch((char *) name,"normal",6)==0 ) {
		    sf->pfminfo.panose[3] = 3;
		    sf->pfminfo.width = 5;
		} else if ( strmatch((char *) name,"ultra-condensed")==0 ) {
		    sf->pfminfo.panose[3] = 8;
		    sf->pfminfo.width = 1;
		} else if ( strmatch((char *) name,"extra-condensed")==0 ) {
		    sf->pfminfo.panose[3] = 8;
		    sf->pfminfo.width = 2;
		} else if ( strmatch((char *) name,"condensed")==0 ) {
		    sf->pfminfo.panose[3] = 6;
		    sf->pfminfo.width = 3;
		} else if ( strmatch((char *) name,"semi-condensed")==0 ) {
		    sf->pfminfo.panose[3] = 6;
		    sf->pfminfo.width = 4;
		} else if ( strmatch((char *) name,"ultra-expanded")==0 ) {
		    sf->pfminfo.panose[3] = 7;
		    sf->pfminfo.width = 9;
		} else if ( strmatch((char *) name,"extra-expanded")==0 ) {
		    sf->pfminfo.panose[3] = 7;
		    sf->pfminfo.width = 8;
		} else if ( strmatch((char *) name,"expanded")==0 ) {
		    sf->pfminfo.panose[3] = 5;
		    sf->pfminfo.width = 7;
		} else if ( strmatch((char *) name,"semi-expanded")==0 ) {
		    sf->pfminfo.panose[3] = 5;
		    sf->pfminfo.width = 6;
		}
		sf->pfminfo.panose_set = true;
		sf->pfminfo.pfmset = true;
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "panose-1");
	    if ( name!=NULL ) {
		char *pt, *end;
		int i;
		for ( i=0, pt=(char *) name; i<10 && *pt; pt = end, ++i ) {
		    sf->pfminfo.panose[i] = strtol(pt,&end,10);
		}
		sf->pfminfo.panose_set = true;
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "slope");
	    if ( name!=NULL ) {
		sf->italicangle = strtod((char *) name,NULL);
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "underline-position");
	    if ( name!=NULL ) {
		sf->upos = strtod((char *) name,NULL);
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "underline-thickness");
	    if ( name!=NULL ) {
		sf->uwidth = strtod((char *) name,NULL);
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "ascent");
	    if ( name!=NULL ) {
		ascent = strtod((char *) name,NULL);
		xmlFree(name);
	    }
	    name = xmlGetProp(kids,(xmlChar *) "descent");
	    if ( name!=NULL ) {
		descent = strtod((char *) name,NULL);
		xmlFree(name);
	    }
	    if ( ascent-descent==sf->ascent+sf->descent ) {
		sf->ascent = ascent;
		sf->descent = -descent;
	    }
	    sf->pfminfo.pfmset = true;
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "glyph")==0 ||
		xmlStrcmp(kids->name,(const xmlChar *) "missing-glyph")==0 )
	    ++cnt;
    }
    if ( !has_font_face ) {
	LogError( _("This font does not specify font-face\n") );
	SplineFontFree(sf);
return( NULL );
    }
    if ( sf->weight==NULL )
	sf->weight = copy("Regular");
    if ( sf->fontname==NULL && sf->familyname==NULL )
	sf->fontname = GetNextUntitledName();
    if ( sf->familyname==NULL )
	sf->familyname = copy(sf->fontname);
    if ( sf->fontname==NULL )
	sf->fontname = EnforcePostScriptName(sf->familyname);
    sf->fullname = copy(sf->fontname);

    /* Give ourselves an xuid, just in case they want to convert to PostScript*/
    if ( xuid!=NULL ) {
	sf->xuid = malloc(strlen(xuid)+20);
	sprintf(sf->xuid,"[%s %d]", xuid, (rand()&0xffffff));
    }

    ff_progress_change_total(cnt);
    sf->glyphcnt = sf->glyphmax = cnt;
    sf->glyphs = malloc(cnt*sizeof(SplineChar *));

    cnt = 0;
    for ( kids = font->children; kids!=NULL; kids=kids->next ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) "missing-glyph")==0 ) {
	    sf->glyphs[cnt] = SVGParseMissing(sf,kids,defh,defv,cnt,&flags);
	    if ( sf->glyphs[cnt]!=NULL ) {
		sf->glyphs[cnt]->orig_pos = cnt;
		cnt++;
	    }
	    ff_progress_next();
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "glyph")==0 ) {
	    sf->glyphs[cnt] = SVGParseGlyph(sf,kids,defh,defv,cnt,&flags);
	    if ( sf->glyphs[cnt]!=NULL ) {
		sf->glyphs[cnt]->orig_pos = cnt;
		cnt++;
	    }
	    ff_progress_next();
	}
    }
    cnt = 0;
    for ( kids = font->children; kids!=NULL; kids=kids->next ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) "hkern")==0 ) {
	    SVGParseKern(sf,kids,false);
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "vkern")==0 ) {
	    SVGParseKern(sf,kids,true);
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "glyph")==0 ) {
	    SVGLigatureFixupCheck(sf->glyphs[cnt++],kids);
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "missing-glyph")==0 ) {
	    ++cnt;
	}
    }

    map = chunkalloc(sizeof(EncMap));
    map->enccount = map->encmax = map->backmax = sf->glyphcnt;
    map->enc = FindOrMakeEncoding("Original");
    map->map = malloc(sf->glyphcnt*sizeof(int));
    map->backmap = malloc(sf->glyphcnt*sizeof(int));
    for ( i=0; i<sf->glyphcnt; ++i )
	map->map[i] = map->backmap[i] = i;
    sf->map = map;

return( sf );
}

static int SPLFindOrder(SplineSet *ss) {
    Spline *s, *first;

    while ( ss!=NULL ) {
	first = NULL;
	for ( s = ss->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( !s->knownlinear )
return( s->order2 );
	}
	ss = ss->next;
    }
return( -1 );
}

static int EntFindOrder(Entity *ent) {
    int ret;

    while ( ent!=NULL ) {
	if ( ent->type == et_splines ) {
	    ret = SPLFindOrder(ent->u.splines.splines);
	    if ( ret!=-1 )
return( ret );
	}
	ent = ent->next;
    }
return( -1 );
}

int SFFindOrder(SplineFont *sf) {
    int i, ret;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	ret = SPLFindOrder(sf->glyphs[i]->layers[ly_fore].splines);
	if ( ret!=-1 )
return( ret );
    }
return( 0 );
}

int SFLFindOrder(SplineFont *sf, int layerdest) {
    int i, ret;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	ret = SPLFindOrder(sf->glyphs[i]->layers[layerdest].splines);
	if ( ret!=-1 )
return( ret );
    }
return( 0 );
}

static void SPLSetOrder(SplineSet *ss,int order2) {
    Spline *s, *first;
    SplinePoint *from, *to;

    while ( ss!=NULL ) {
	first = NULL;
	for ( s = ss->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( s->order2!=order2 ) {
		if ( s->knownlinear ) {
		    s->from->nextcp = s->from->me;
		    s->to->prevcp = s->to->me;
		    s->from->nonextcp = s->to->noprevcp = true;
		    s->order2 = order2;
		} else if ( order2 ) {
		    from = SplineTtfApprox(s);
		    s->from->nextcp = from->nextcp;
		    s->from->nonextcp = from->nonextcp;
		    s->from->next = from->next;
		    from->next->from = s->from;
		    SplinePointFree(from);
		    for ( to = s->from->next->to; to->next!=NULL; to=to->next->to );
		    s->to->prevcp = to->prevcp;
		    s->to->noprevcp = to->noprevcp;
		    s->to->prev = to->prev;
		    to->prev->to = s->to;
		    SplinePointFree(to);
		    to = s->to;
		    from = s->from;
		    SplineFree(s);
		    if ( first==s ) first = from->next;
		    s = to->prev;
		} else {
		    s->from->nextcp.x = s->splines[0].c/3 + s->from->me.x;
		    s->from->nextcp.y = s->splines[1].c/3 + s->from->me.y;
		    s->to->prevcp.x = s->from->nextcp.x+ (s->splines[0].b+s->splines[0].c)/3;
		    s->to->prevcp.y = s->from->nextcp.y+ (s->splines[1].b+s->splines[1].c)/3;
		    s->order2 = false;
		    SplineRefigure(s);
		}
	    }
	}
	ss = ss->next;
    }
}

static void EntSetOrder(Entity *ent,int order2) {
    while ( ent!=NULL ) {
	if ( ent->type == et_splines )
	    SPLSetOrder(ent->u.splines.splines,order2);
	SPLSetOrder(ent->clippath,order2);
	ent = ent->next;
    }
}

void SFSetOrder(SplineFont *sf,int order2) {
    int i,j;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( j=ly_fore; j<sf->glyphs[i]->layer_cnt; ++j ) {
	    SPLSetOrder(sf->glyphs[i]->layers[j].splines,order2);
	    sf->glyphs[i]->layers[j].order2 = order2;
	}
    }
}

void SFLSetOrder(SplineFont *sf, int layerdest, int order2) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i )
      if ( sf->glyphs[i]!=NULL && layerdest < sf->glyphs[i]->layer_cnt) {
	    if (sf->glyphs[i]->layers[layerdest].splines != NULL)
	      SPLSetOrder(sf->glyphs[i]->layers[layerdest].splines,order2);
	    sf->glyphs[i]->layers[layerdest].order2 = order2;
      }
}

static SplineFont *_SFReadSVG(xmlDocPtr doc, char *filename) {
    xmlNodePtr *fonts, font;
    SplineFont *sf;
    char *chosenname = NULL;

    fonts = FindSVGFontNodes(doc);
    if ( fonts==NULL || fonts[0]==NULL ) {
	LogError( _("This file contains no SVG fonts.\n") );
	xmlFreeDoc(doc);
return( NULL );
    }
    font = fonts[0];
    if ( fonts[1]!=NULL ) {
	xmlChar *name;
	font = SVGPickFont(fonts,filename);
	name = xmlGetProp(font,(xmlChar *) "id");
	if ( name!=NULL ) {
	    chosenname = cu_copy(utf82u_copy((char *) name));
	    xmlFree(name);
	}
    }
    free(fonts);
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    sf = SVGParseFont(font);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    xmlFreeDoc(doc);

    if ( sf!=NULL ) {
	struct stat b;
	sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
		SFFindOrder(sf);
	SFSetOrder(sf,sf->layers[ly_fore].order2);
	sf->chosenname = chosenname;
	if ( stat(filename,&b)!=-1 ) {
	    sf->modificationtime = b.st_mtime;
	    sf->creationtime = b.st_mtime;
	}
    }
return( sf );
}

SplineFont *SFReadSVG(char *filename, int flags) {
    xmlDocPtr doc;
    char *temp=filename, *pt, *lparen;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }

    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (lparen=strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	temp = copy(filename);
	pt = temp + (lparen-filename);
	*pt = '\0';
    }

    doc = xmlParseFile(temp);
    if ( temp!=filename ) free(temp);
    if ( doc==NULL ) {
	/* Can I get an error message from libxml? */
return( NULL );
    }
return( _SFReadSVG(doc,filename));
}

SplineFont *SFReadSVGMem(char *data, int flags) {
    xmlDocPtr doc;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }

    doc = xmlParseMemory(data,strlen(data));
    if ( doc==NULL ) {
	/* Can I get an error message from libxml? */
return( NULL );
    }
return( _SFReadSVG(doc,NULL));
}

char **NamesReadSVG(char *filename) {
    xmlNodePtr *fonts;
    xmlDocPtr doc;
    char **ret=NULL;
    int cnt;
    xmlChar *name;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }

    doc = xmlParseFile(filename);
    if ( doc==NULL ) {
	/* Can I get an error message from libxml? */
return( NULL );
    }

    fonts = FindSVGFontNodes(doc);
    if ( fonts==NULL || fonts[0]==NULL ) {
	xmlFreeDoc(doc);
return( NULL );
    }

    for ( cnt=0; fonts[cnt]!=NULL; ++cnt);
    ret = malloc((cnt+1)*sizeof(char *));
    for ( cnt=0; fonts[cnt]!=NULL; ++cnt) {
	name = xmlGetProp(fonts[cnt],(xmlChar *) "id");
	if ( name==NULL ) {
	    ret[cnt] = copy("nameless-font");
	} else {
	    ret[cnt] = copy((char *) name);
	    xmlFree(name);
	}
    }
    ret[cnt] = NULL;

    free(fonts);
    xmlFreeDoc(doc);

return( ret );
}

Entity *EntityInterpretSVG(char *filename,char *memory, int memlen,int em_size,int ascent) {
    xmlDocPtr doc;
    xmlNodePtr top;
    char oldloc[25];
    Entity *ret;
    int order2;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }
    if ( filename!=NULL )
	doc = xmlParseFile(filename);
    else
	doc = xmlParseMemory(memory,memlen);
    if ( doc==NULL ) {
	/* Can I get an error message from libxml???? */
return( NULL );
    }

    top = xmlDocGetRootElement(doc);
    if ( xmlStrcmp(top->name,(xmlChar *) "svg")!=0 ) {
	LogError( _("%s does not contain an <svg> element at the top\n"), filename);
	xmlFreeDoc(doc);
return( NULL );
    }

    strncpy( oldloc,setlocale(LC_NUMERIC,NULL),24 );
    oldloc[24]=0;
    setlocale(LC_NUMERIC,"C");
    ret = SVGParseSVG(top,em_size,ascent);
    setlocale(LC_NUMERIC,oldloc);
    xmlFreeDoc(doc);

    if ( loaded_fonts_same_as_new )
	order2 = new_fonts_are_order2;
    else
	order2 = EntFindOrder(ret);
    if ( order2==-1 ) order2 = 0;
    EntSetOrder(ret,order2);

return( ret );
}

SplineSet *SplinePointListInterpretSVG(char *filename,char *memory, int memlen,int em_size,int ascent,int is_stroked) {
    Entity *ret = EntityInterpretSVG(filename, memory, memlen, em_size, ascent);
    int flags = -1;
return( SplinesFromEntities(ret,&flags,is_stroked));
}

int HasSVG(void) {
return( libxml_init_base());
}
