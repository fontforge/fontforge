/* Copyright (C) 2003 by George Williams */
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
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <chardata.h>
#include <ustring.h>

static int svg_outfontheader(FILE *file, SplineFont *sf) {
    char *pt;
    int defwid = SFFigureDefWidth(sf,NULL);
    struct pfminfo info;
    static const char *condexp[] = { "squinchy", "ultra-condensed", "extra-condensed",
	"condensed", "semi-condensed", "normal", "semi-expanded", "expanded",
	"extra-expanded", "ultra-expanded", "broad" };
    DBounds bb;
    BlueData bd;

    info = sf->pfminfo;
    SFDefaultOS2Info(&info,sf,sf->fontname);
    SplineFontFindBounds(sf,&bb);
    QuickBlues(sf,&bd);

    fprintf( file, "<?xml version=\"1.0\" standalone=\"no\"?>\n" );
    fprintf( file, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg11.dtd\" >\n" );
    if ( sf->comments!=NULL ) {
	for ( pt=sf->comments; *pt!='\0'; ) {
	    if ( *pt!='\n' ) {
		fprintf( file, "<!-- " );
		while ( *pt!='\n' && *pt!='\0' ) {
		    putc(*pt,file);
		    ++pt;
		}
		fprintf( file, " --!>" );
	    }
	    putc('\n',file);
	    if ( *pt=='\n' ) ++pt;
	}
    }
    fprintf( file, "<svg>\n<defs>\n" );
    fprintf( file, "<font id=\"%s\" horiz-adv-x=\"%d\" ", sf->fontname, defwid );
    if ( sf->hasvmetrics )
	fprintf( file, "vert-adv-y=%d ", sf->ascent+sf->descent );
    putc('>',file); putc('\n',file);
    fprintf( file, "  <font-face \n" );
    fprintf( file, "    font-family=\"%s\"\n", sf->familyname );
    fprintf( file, "    font-weigth=\"%d\"\n", info.weight );
    if ( strstrmatch(sf->fontname,"obli") || strstrmatch(sf->fontname,"slanted") )
	fprintf( file, "    font-style=\"oblique\"\n" );
    else if ( MacStyleCode(sf,NULL)&sf_italic )
	fprintf( file, "    font-style=\"italic\"\n" );
    if ( strstrmatch(sf->fontname,"small") || strstrmatch(sf->fontname,"cap") )
	fprintf( file, "    font-variant=small-caps\n" );
    fprintf(file, "    font-stretch=\"%s\"\n", condexp[info.width]);
    fprintf( file, "    units-per-em=\"%d\"\n", sf->ascent+sf->descent );
    fprintf( file, "    panose-1=\"%d %d %d %d %d %d %d %d %d %d\"\n", info.panose[0],
	info.panose[1], info.panose[2], info.panose[3], info.panose[4], info.panose[5],
	info.panose[6], info.panose[7], info.panose[8], info.panose[9]);
    fprintf( file, "    ascent=\"%d\"\n", sf->ascent );
    fprintf( file, "    descent=\"%d\"\n", sf->descent );
    fprintf( file, "    x-height=\"%d\"\n", bd.xheight );
    fprintf( file, "    cap-height=\"%d\"\n", bd.caph );
    fprintf( file, "    bbox=\"%d %d %d %d\"\n", bb.minx, bb.miny, bb.maxx, bb.maxy );
/* !!!! Check bounding box format */
    fprintf( file, "    underline-thickness=\"%d\"\n", sf->upos );
    fprintf( file, "    underline-position=\"%d\"\n", sf->uwidth );
    if ( sf->italicangle!=0 )
	fprintf(file, "    slope=\"%d\"\n", sf->italicangle );
/* !!!! Check slope format */
    fprintf( file, "  />\n" );
return( defwid );
}

static int svg_pathdump(FILE *file, SplineSet *spl, int lineout) {
    BasePoint last;
    char buffer[60];
    int closed=false;
    Spline *sp, *first;
    /* as I see it there is nothing to be gained by optimizing out the */
    /* command characters, since they just have to be replaced by spaces */
    /* so I don't bother to */

    last.x = last.y = 0;
    while ( spl!=NULL ) {
	sprintf( buffer, "M%g %g", spl->first->me.x, spl->first->me.y );
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
		    sprintf( buffer,"v%g", sp->from->me.y-last.y );
		else if ( sp->to->me.y==sp->from->me.y )
		    sprintf( buffer,"h%g", sp->from->me.x-last.x );
		else if ( sp->to->next==first ) {
		    strcpy( buffer, "z");
		    closed = true;
		} else
		    sprintf( buffer,"l%g %g", sp->to->me.x-last.x, sp->to->me.y-last.y );
	    } else if ( sp->order2 ) {
		if ( sp->from->prev!=NULL &&
			sp->from->me.x-sp->from->prevcp.x == sp->from->nextcp.x-sp->from->me.x &&
			sp->from->me.y-sp->from->prevcp.y == sp->from->nextcp.y-sp->from->me.y )
		    sprintf( buffer,"t%g %g", sp->to->me.x-last.x, sp->to->me.y-last.y );
		else
		    sprintf( buffer,"q%g %g %g %g",
			    sp->to->prevcp.x-last.x, sp->to->prevcp.y-last.y,
			    sp->to->me.x-sp->to->prevcp.x,sp->to->me.y-sp->to->prevcp.y);
	    } else {
		if ( sp->from->prev!=NULL &&
			sp->from->me.x-sp->from->prevcp.x == sp->from->nextcp.x-sp->from->me.x &&
			sp->from->me.y-sp->from->prevcp.y == sp->from->nextcp.y-sp->from->me.y )
		    sprintf( buffer,"s%g %g %g %g",
			    sp->to->prevcp.x-last.x, sp->to->prevcp.y-last.y,
			    sp->to->me.x-sp->to->prevcp.x,sp->to->me.y-sp->to->prevcp.y);
		else
		    sprintf( buffer,"c%g %g %g %g %g %g",
			    sp->from->nextcp.x-last.x, sp->from->nextcp.y-last.y,
			    sp->to->prevcp.x-sp->from->nextcp.x, sp->to->prevcp.y-sp->from->nextcp.y,
			    sp->to->me.x-sp->to->prevcp.x,sp->to->me.y-sp->to->prevcp.y);
	    }
	    if ( lineout+strlen(buffer)>=255 ) { putc('\n',file); lineout = 0; }
	    fputs( buffer,file );
	    lineout += strlen( buffer );
	    last = sp->to->me;
	}
	if ( !closed ) {
	    if ( lineout>=254 ) { putc('\n',file); lineout=0; }
	    putc('z',file);
	    ++lineout;
	}
	spl = spl->next;
    }
return( lineout );
}

static void svg_scpathdump(FILE *file, SplineChar *sc) {
    RefChar *ref;
    int lineout;

    fprintf( file,"d=\"");
    lineout = svg_pathdump(file,sc->splines,3);
    for ( ref= sc->refs; ref!=NULL; ref=ref->next )
	lineout = svg_pathdump(file,ref->splines,lineout);
    if ( lineout>=255-4 ) putc('\n',file );
    fputs("\" />\n",file);
}

static int LigCnt(SplineFont *sf,PST *lig,uint32 *univals,int max) {
    char *pt, *end;
    int c=0;
    SplineChar *sc;

    if ( lig->type!=pst_ligature )
return( 0 );
    else if ( lig->tag!=CHR('l','i','g','a') && lig->tag!=CHR('r','l','i','g'))
return( 0 );
    pt = lig->u.lig.components;
    forever {
	end = strchr(pt,' ');
	if ( end!=NULL ) *end='\0';
	sc = SFGetCharDup(sf,-1,pt);
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

static void svg_scdump(FILE *file, SplineChar *sc,int defwid) {
    PST *best=NULL;
    const unichar_t *alt;
    int32 univals[50];
    int i, c, uni;

    best = HasLigature(sc);
    fprintf(file,"    <glyph glyph-name=\"%s\" ",sc->name );
    if ( best!=NULL ) {
	c = LigCnt(sc->parent,best,univals,sizeof(univals)/sizeof(univals[0]));
	fputs("unicode=\"",file);
	for ( i=0; i<c; ++i )
	    if ( univals[i]>=' ' && univals[i]<127 )
		putc(univals[i],file);
	    else
		fprintf(file,"&#x%x;",univals[i]);
	fputs("\" ",file);
    } else if ( sc->unicodeenc!=-1 ) {
	if ( sc->unicodeenc>=32 && sc->unicodeenc<127 && sc->unicodeenc!='"' && sc->unicodeenc!='&')
	    fprintf( file, "unicode=\"%c\" ", sc->unicodeenc);
	else if ( sc->unicodeenc<0x10000 &&
		( isarabisolated(sc->unicodeenc) || isarabinitial(sc->unicodeenc) || isarabmedial(sc->unicodeenc) || isarabfinal(sc->unicodeenc) ) &&
		unicode_alternates[sc->unicodeenc>>8]!=NULL &&
		(alt = unicode_alternates[sc->unicodeenc>>8][uni&0xff])!=NULL &&
		alt[1]=='\0' )
	    /* For arabic forms use the base representation in the 0600 block */
	    fprintf( file, "unicode=\"&#x%x;\" ", alt[0]);
	else
	    fprintf( file, "unicode=\"&#x%x;\" ", sc->unicodeenc);
    }
    if ( sc->width!=defwid )
	fprintf( file, "horiz-adv-x=\"%d\" ", sc->width );
    if ( sc->parent->hasvmetrics && sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	fprintf( file, "vert-adv-y=\"%d\" ", sc->vwidth );
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 ) {
	if ( isarabinitial(sc->unicodeenc))
	    fprintf( file,"arabic-form=initial " );
	else if ( isarabmedial(sc->unicodeenc))
	    fprintf( file,"arabic-form=medial ");
	else if ( isarabfinal(sc->unicodeenc))
	    fprintf( file,"arabic-form=final ");
	else if ( isarabisolated(sc->unicodeenc))
	    fprintf( file,"arabic-form=isolated ");
    }
    putc('\n',file);
    svg_scpathdump(file,sc);
}

static void svg_notdefdump(FILE *file, SplineFont *sf,int defwid) {
    
    if ( SCWorthOutputting(sf->chars[0]) && SCIsNotdef(sf->chars[0],-1 )) {
	SplineChar *sc = sf->chars[0];

	fprintf(file, "<missing-glyph ");
	if ( sc->width!=defwid )
	    fprintf( file, "horiz-adv-x=\"%d\" ", sc->width );
	if ( sc->parent->hasvmetrics && sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	    fprintf( file, "vert-adv-y=\"%d\" ", sc->vwidth );
	putc('\n',file);
	svg_scpathdump(file,sc);
    } else {
	/* We'll let both the horiz and vert advances default to the values */
	/*  specified by the font */
	fprintf(file,"    <missing-glyph d=\"\" />\n");	/* Is this a blank space? */
    }
}

static void svg_dumpkerns(FILE *file,SplineFont *sf) {
    int i,j;
    KernPair *kp;
    KernClass *kc;

    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) ) {
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
	    if ( kp->off!=0 && SCWorthOutputting(kp->sc)) {
		fprintf( file, "    <hkern " );
		if ( sf->chars[i]->unicodeenc!=-1 || HasLigature(sf->chars[i]))
		    fprintf( file, "g1=\"%s\" ", sf->chars[i]->name );
		else if ( sf->chars[i]->unicodeenc>='A' || sf->chars[i]->unicodeenc<='z' )
		    fprintf( file, "u1=\"%c\" ", sf->chars[i]->unicodeenc );
		else
		    fprintf( file, "u1=\"&#x%x\" ", sf->chars[i]->unicodeenc );
		if ( kp->sc->unicodeenc!=-1 || HasLigature(kp->sc))
		    fprintf( file, "g2=\"%s\" ", kp->sc->name );
		else if ( kp->sc->unicodeenc>='A' || kp->sc->unicodeenc<='z' )
		    fprintf( file, "u2=\"%c\" ", kp->sc->unicodeenc );
		else
		    fprintf( file, "u2=\"&#x%x\" ", kp->sc->unicodeenc );
		fprintf( file, "k=\"%d\" />\n", -kp->off );
	    }
    }

    for ( kc=sf->kerns; kc!=NULL; kc=kc->next ) {
	for ( i=1; i<kc->first_cnt; ++i ) for ( j=1; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->first_cnt+j]!=0 ) {
		fprintf( file, "    <hkern g1=\"%s\"\n\tg2=\"%s\"\n\tk=\"%d\" />\n",
			kc->firsts[i], kc->seconds[j], kc->offsets[i*kc->first_cnt+j]);
	    }
	}
    }
}

static void svg_outfonttrailer(FILE *file,SplineFont *sf) {
    fprintf(file,"  </font>\n");
    fprintf(file,"</defs></svg>\n" );
}

static void svg_sfdump(FILE *file,SplineFont *sf) {
    int defwid, i;
    char *oldloc;

    oldloc = setlocale(LC_NUMERIC,"C");

    defwid = svg_outfontheader(file,sf);
    svg_notdefdump(file,sf,defwid);

    /* Ligatures must be output before their initial components */
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) && HasLigature(sf->chars[i]))
	    svg_scdump(file, sf->chars[i],defwid);
    }
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) && !HasLigature(sf->chars[i]))
	    svg_scdump(file, sf->chars[i],defwid);
    }
    svg_dumpkerns(file,sf);
    svg_outfonttrailer(file,sf);
    setlocale(LC_NUMERIC,oldloc);
}

int WriteSVGFont(char *fontname,SplineFont *sf,enum fontformat format,int flags) {
    FILE *file;
    int ret;

    if (( file=fopen(fontname,"w+"))==NULL )
return( 0 );
    svg_sfdump(file,sf);
    ret = true;
    if ( ferror(file))
	ret = false;
    if ( fclose(file)==-1 )
return( 0 );
return( ret );
}

int _ExportSVG(FILE *svg,SplineChar *sc) {
    char *oldloc;

    oldloc = setlocale(LC_NUMERIC,"C");
    fprintf(svg, "<?xml version=\"1.0\" standalone=\"no\"?>\n" );
    fprintf(svg, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg11.dtd\" >\n" ); 
    fprintf(svg, "<svg>\n" );
    fprintf(svg, "  <path fill=\"currentColor\"\n");
    svg_scpathdump(svg,sc);
    fprintf(svg, "</svg>\n" );

    setlocale(LC_NUMERIC,oldloc);
return( !ferror(svg));
}
