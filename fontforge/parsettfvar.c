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

#include "parsettfvar.h"

#include "fontforge.h"
#include "fvfonts.h"
#include "mem.h"
#include "parsettf.h"
#include "splineutil.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "ttf.h"

/***************************************************************************/
/*                                                                         */
/* Apple documents these tables at                                         */
/*    http:// developer.apple.com/fonts/TTRefMan/RM06/Chap6[fgca]var.html  */
/* The documentation for fvar is inconsistant. At one point it says that   */
/*  countSizePairs should be 3, at another point 2. It should be 2.        */
/* The documentation for gvar is not intelligable, cvar refers you to gvar */
/*  and is thus also incomprehensible                                      */
/* The documentation for avar appears correct, but as Apple has no fonts   */
/*  with an 'avar' table, so it's hard to test.                            */
/*                                                                         */
/***************************************************************************/

/* Parse Apple's variation tables (font, glyph, cvt and axis) */
/* To be interesting a font must have an fvar and a gvar */
/* If it's hinted it better have a cvar */
/* It may have an avar if it wants to be complicated */

void VariationFree(struct ttfinfo *info) {
    int i,j;
    struct variations *variation = info->variations;

    if ( variation==NULL )
return;
    if ( variation->axes!=NULL ) {
	for ( i=0; i<variation->axis_count; ++i ) {
	    free(variation->axes[i].mapfrom);
	    free(variation->axes[i].mapto);
	}
	free(variation->axes);
    }
    if ( variation->instances!=NULL ) {
	for ( i=0; i<variation->instance_count; ++i ) {
	    free(variation->instances[i].coords);
	}
	free(variation->instances);
    }
    if ( variation->tuples!=NULL ) {
	for ( i=0; i<variation->tuple_count; ++i ) {
	    free(variation->tuples[i].coords);
	    if ( variation->tuples[i].chars!=NULL )
		for ( j=0; j<info->glyph_cnt; ++j )
		    SplineCharFree(variation->tuples[i].chars[j]);
	    free(variation->tuples[i].chars);
	    KernClassListFree(variation->tuples[i].khead);
	    KernClassListFree(variation->tuples[i].vkhead);
	}
	free(variation->tuples);
    }
    free(variation);
    info->variations = NULL;
}

static void parsefvar(struct ttfinfo *info, FILE *ttf) {
    int data_off, axis_count, instance_count, cnt;
    int i,j;

    fseek(ttf,info->fvar_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 )	/* I only understand version 1 */
return;
    data_off = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt>2 )
	LogError( _("Hmm, this 'fvar' table has more count/size pairs than I expect\n") );
    else if ( cnt<2 ) {
	LogError( _("Hmm, this 'fvar' table has too few count/size pairs, I shan't parse it\n") );
return;
    }
    axis_count = getushort(ttf);
    if ( axis_count==0 || axis_count>4 ) {
	if ( axis_count==0 )
	    LogError( _("Hmm, this 'fvar' table has no axes, that doesn't make sense.\n") );
	else
	    LogError( _("Hmm, this 'fvar' table has more axes than FontForge can handle.\n") );
return;
    }
    if ( getushort(ttf)!=20 ) {
	LogError( _("Hmm, this 'fvar' table has an unexpected size for an axis, I shan't parse it\n") );
return;
    }
    instance_count = getushort(ttf);
    if ( getushort(ttf)!=4+4*axis_count ) {
	LogError( _("Hmm, this 'fvar' table has an unexpected size for an instance, I shan't parse it\n") );
return;
    }
    if ( data_off+axis_count*20+instance_count*(4+4*axis_count)> info->fvar_len ) {
	LogError( _("Hmm, this 'fvar' table is too short\n") );
return;
    }

    info->variations = calloc(1,sizeof(struct variations));
    info->variations->axis_count = axis_count;
    info->variations->instance_count = instance_count;
    info->variations->axes = calloc(axis_count,sizeof(struct taxis));
    info->variations->instances = calloc(instance_count,sizeof(struct tinstance));
    for ( i=0; i<instance_count; ++i )
	info->variations->instances[i].coords = malloc(axis_count*sizeof(real));

    fseek(ttf,info->fvar_start+data_off,SEEK_SET);
    for ( i=0; i<axis_count; ++i ) {
	struct taxis *a = &info->variations->axes[i];
	a->tag = getlong(ttf);
	a->min = getlong(ttf)/65536.0;
	a->def = getlong(ttf)/65536.0;
	a->max = getlong(ttf)/65536.0;
	/* flags = */ getushort(ttf);
	a->nameid = getushort(ttf);
    }
    for ( i=0; i<instance_count; ++i ) {
	struct tinstance *ti = &info->variations->instances[i];
	ti->nameid = getushort(ttf);
	/* flags = */ getushort(ttf);
	for ( j=0 ; j<axis_count; ++j )
	    ti->coords[j] = getlong(ttf)/65536.0;
    }
}

static void parseavar(struct ttfinfo *info, FILE *ttf) {
    int axis_count, pair_count;
    int i,j;

    if ( info->variations==NULL || info->avar_start==0 || info->avar_len==0 )
return;

    fseek(ttf,info->avar_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 ) {	/* I only understand version 1 */
	VariationFree(info);
return;
    }
    axis_count = getlong(ttf);
    if ( axis_count!=info->variations->axis_count ) {
	LogError( _("Hmm, the axis count in the 'avar' table is different from that in the 'fvar' table.\n") );
	VariationFree(info);
return;
    }
    for ( i=0; i<axis_count; ++i ) {
	pair_count = getushort(ttf);
	if ( pair_count!=0 ) {
	    info->variations->axes[i].mapfrom = malloc(pair_count*sizeof(real));
	    info->variations->axes[i].mapto= malloc(pair_count*sizeof(real));
	    for ( j=0; j<pair_count; ++j ) {
		info->variations->axes[i].mapfrom[j] = getushort(ttf)/16384.0;
		info->variations->axes[i].mapto[j] = getushort(ttf)/16384.0;
	    }
	}
    }
    if ( ftell(ttf)-info->avar_start>info->avar_len) {
	LogError( _("Hmm, the 'avar' table is too long.\n") );
	VariationFree(info);
return;
    }
}

static SplineChar **InfoCopyGlyphs(struct ttfinfo *info) {
    SplineChar **chars = malloc(info->glyph_cnt*sizeof(SplineChar *));
    int i;
    RefChar *r;

    for ( i=0; i<info->glyph_cnt; ++i ) {
	if ( info->chars[i]==NULL )
	    chars[i] = NULL;
	else {
	    chars[i] = SplineCharCopy(info->chars[i],NULL,NULL);
	    free(chars[i]->ttf_instrs); chars[i]->ttf_instrs = NULL;
	    chars[i]->ttf_instrs_len = 0;
	    PSTFree(chars[i]->possub); chars[i]->possub = NULL;
	    for ( r=chars[i]->layers[ly_fore].refs; r!=NULL; r=r->next )
		r->sc = NULL;
	    chars[i]->changed = false;
	    chars[i]->ticked = false;
	}
    }

    for ( i=0; i<info->glyph_cnt; ++i )
	ttfFixupRef(chars,i);
return( chars );
}

#define BAD_DELTA	0x10001
static int *readpackeddeltas(FILE *ttf,int n) {
    int *deltas;
    int runcnt, i, j;

    deltas = malloc(n*sizeof(int));

    i = 0;
    while ( i<n ) {
	runcnt = getc(ttf);
	if ( runcnt&0x80 ) {
	    /* runcnt zeros get added */
	    for ( j=0; j<=(runcnt&0x3f) && i<n ; ++j )
		deltas[i++] = 0;
	} else if ( runcnt&0x40 ) {
	    /* runcnt shorts from the stack */
	    for ( j=0 ; j<=(runcnt&0x3f) && i<n ; ++j )
		deltas[i++] = (int16) getushort(ttf);
	} else {
	    /* runcnt signed bytes from the stack */
	    for ( j=0; j<=(runcnt&0x3f) && i<n; ++j )
		deltas[i++] = (int8) getc(ttf);
	}
	if ( j<=(runcnt&0x3f) ) {
	    if ( n>0 )
		deltas[0] = BAD_DELTA;
	}
    }
return( deltas );
}

#define ALL_POINTS	0x10001
#define END_OF_POINTS	0x10000

static int *readpackedpoints(FILE *ttf) {
    int *points;
    int n, runcnt, i, j, first;

    n = getc(ttf);
    if ( n==EOF )
	n = 0;
    if ( n&0x80 )
	n = getc(ttf)|((n&0x7f)<<8);
    points = malloc((n+1)*sizeof(int));
    if ( n==0 )
	points[0] = ALL_POINTS;
    else {
	i = 0;
	while ( i<n ) {
	    runcnt = getc(ttf);
	    if ( runcnt&0x80 ) {
		runcnt = (runcnt&0x7f);
		points[i++] = first = getushort(ttf);
		/* first point not included in runcount */
		for ( j=0; j<runcnt && i<n; ++j )
		    points[i++] = (first += getushort(ttf));
	    } else {
		points[i++] = first = getc(ttf);
		for ( j=0; j<runcnt && i<n; ++j )
		    points[i++] = (first += getc(ttf));
	    }
	}
	points[n] = END_OF_POINTS;
    }
return( points );
}

static int TuplesMatch(struct variations *v, int vtest, int dbase) {
    /* variations for [0,1] make up part of design [1,1], but not the other */
    /* way round */
    int i;

    if ( dbase>=v->tuple_count )
return( false );

    if ( vtest==dbase )
return( true );
    for ( i=0; i<v->axis_count; ++i ) {
	if ( v->tuples[vtest].coords[i]==0 && v->tuples[dbase].coords[i]!=0 )
return( false );
	if ( v->tuples[dbase].coords[i]!=0 &&
		v->tuples[dbase].coords[i]!=v->tuples[vtest].coords[i] )
return( false );
    }
return( true );
}

static int PointCount(SplineChar *sc) {
    int i;
    RefChar *ref;
    SplineSet *ss;
    SplinePoint *sp;

    if ( sc->layers[ly_fore].refs!=NULL )
	for ( i=0, ref=sc->layers[ly_fore].refs; ref!=NULL; ++i, ref=ref->next );
    else {
	for ( i=0, ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; sp!=NULL ; ) {
		if ( sp->ttfindex!=0xffff && sp->ttfindex!=0xfffe )
		    ++i;
		if ( sp->nextcpindex!=0xffff && sp->nextcpindex!=0xfffe )
		    ++i;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
    }
return( i );
}

static void SCShiftAllBy(SplineChar *sc,int xd, int yd) {
    /* If they change the left/right side-bearing, I think that means everything */
    /*  should be shifted over */
    SplineSet *ss;
    SplinePoint *sp;
    RefChar *ref;

    if ( xd==0 && yd==0 )
return;

    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; sp!=NULL ; ) {
	    if ( sp->ttfindex!=0xffff && sp->ttfindex!=0xfffe ) {
		sp->me.x += xd;
		sp->me.y += yd;
	    }
	    if ( sp->nextcpindex!=0xffff && sp->nextcpindex!=0xfffe ) {
		sp->nextcp.x += xd;
		sp->nextcp.y += yd;
		if ( sp->next!=NULL )
		    sp->next->to->prevcp = sp->nextcp;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
    }
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	ref->transform[4] += xd;
	ref->transform[5] += yd;
	SCReinstanciateRefChar(sc,ref,ly_fore);
    }
}

static void VaryGlyph(SplineChar *sc,int *points, int *xdeltas, int *ydeltas,
	int pcnt) {
    /* A character contains either composites or contours */
    int i,j;
    RefChar *ref;
    SplineSet *ss;
    SplinePoint *sp;
    Spline *s, *first;

    if ( points[0]==ALL_POINTS ) {
	if ( sc->layers[ly_fore].refs!=NULL ) {
	    for ( i=0, ref=sc->layers[ly_fore].refs; ref!=NULL; ++i, ref=ref->next ) {
		if ( xdeltas[i]!=0 || ydeltas[i]!=0 ) {
		    ref->transform[4] += xdeltas[i];
		    ref->transform[5] += ydeltas[i];
		    SCReinstanciateRefChar(sc,ref,ly_fore);
		}
	    }
	} else {
	    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first; sp!=NULL ; ) {
		    if ( sp->ttfindex!=0xffff && sp->ttfindex!=0xfffe ) {
			sp->me.x += xdeltas[sp->ttfindex];
			sp->me.y += ydeltas[sp->ttfindex];
		    }
		    if ( sp->nextcpindex!=0xffff && sp->nextcpindex!=0xfffe ) {
			sp->nextcp.x += xdeltas[sp->nextcpindex];
			sp->nextcp.y += ydeltas[sp->nextcpindex];
			if ( sp->next!=NULL )
			    sp->next->to->prevcp = sp->nextcp;
		    }
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp == ss->first )
		break;
		}
	    }
	}
	SCShiftAllBy(sc,-xdeltas[pcnt-4],0);
	SCShiftAllBy(sc,0,-ydeltas[pcnt-2]);
	sc->width += xdeltas[pcnt-3];
	sc->vwidth += ydeltas[pcnt-1];
    } else {
	j = 0;
	if ( sc->layers[ly_fore].refs!=NULL ) {
	    for ( i=0, ref=sc->layers[ly_fore].refs; ref!=NULL; ++i, ref=ref->next ) {
		if ( points[j]==i ) {
		    if ( xdeltas[j]!=0 || ydeltas[j]!=0 ) {
			ref->transform[4] += xdeltas[j];
			ref->transform[5] += ydeltas[j];
			SCReinstanciateRefChar(sc,ref,ly_fore);
		    }
		    ++j;
		}
	    }
	} else {
	    for ( i=0, ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		if ( ss->first->prev!=NULL && ss->first->prev->from->nextcpindex==points[j] ) {
		    ss->first->prevcp.x += xdeltas[j];
		    ss->first->prevcp.y += ydeltas[j];
		    ss->first->prev->from->nextcp = ss->first->prevcp;
		    ++j;
		}
		for ( sp=ss->first; sp!=NULL ; ++i ) {
		    if ( sp->ttfindex!=0xffff && sp->ttfindex!=0xfffe )
			++i;
		    if ( sp->ttfindex==points[j] ) {
			sp->me.x += xdeltas[j];
			sp->me.y += ydeltas[j++];
		    }
		    if ( sp->nextcpindex!=0xffff && sp->nextcpindex!=0xfffe )
			++i;
		    if ( sp->nextcpindex==points[j] ) {
			sp->nextcp.x += xdeltas[j];
			sp->nextcp.y += ydeltas[j++];
			if ( sp->next!=NULL )
			    sp->next->to->prevcp = sp->nextcp;
		    } else if ( sp->nonextcp ) {
			sp->nextcp = sp->me;
		    }
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp == ss->first )
		break;
		}
		first = NULL;
	    }
	}
	if ( points[j]==i )
	    SCShiftAllBy(sc,-xdeltas[j++],0);
	if ( points[j]==i+1 )
	    sc->width += xdeltas[j++];
	if ( points[j]==i+2 )
	    SCShiftAllBy(sc,0,-ydeltas[j++]);
	if ( points[j]==i+3 )
	    sc->vwidth += ydeltas[j++];
    }
    if ( sc->layers[ly_fore].refs==NULL ) {
	for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; sp!=NULL ; ++i ) {
		if ( sp->ttfindex==0xffff ) {
		    sp->me.x = ( sp->nextcp.x + sp->prevcp.x )/2;
		    sp->me.y = ( sp->nextcp.y + sp->prevcp.y )/2;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp == ss->first )
	    break;
	    }

	    first = NULL;
	    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
		SplineRefigure(s);
		if ( first==NULL ) first = s;
	    }
	}
    }
}

static void VaryGlyphs(struct ttfinfo *info,int tupleIndex,int gnum,
	int *points, FILE *ttf ) {
    /* one annoying thing about gvar, is that the variations do not describe */
    /*  designs. well variations for [0,1] describes that design, but the */
    /*  design for [1,1] includes the variations [0,1], [1,0], and [1,1] */
    int pcnt, tc;
    int *xdeltas, *ydeltas;
    struct variations *v = info->variations;

    if ( info->chars[gnum]==NULL )	/* Apple doesn't support ttc so this */
return;					/*  can't happen */
    if ( points==NULL ) {
	LogError( _("Mismatched local and shared tuple flags.\n") );
return;
    }

    if ( points[0]==ALL_POINTS )
	pcnt = PointCount(info->chars[gnum])+4;
    else {
	for ( pcnt=0; points[pcnt]!=END_OF_POINTS; ++pcnt );
    }
    xdeltas = readpackeddeltas(ttf,pcnt);
    ydeltas = readpackeddeltas(ttf,pcnt);
    if ( xdeltas[0]!=BAD_DELTA && ydeltas[0]!=BAD_DELTA )
	for ( tc = 0; tc<v->tuple_count; ++tc ) {
	    if ( TuplesMatch(v,tc,tupleIndex))
		VaryGlyph(v->tuples[tc].chars[gnum],points,xdeltas,ydeltas,pcnt);
    } else {
	static int warned = false;
	if ( !warned )
	    LogError( _("Incorrect number of deltas in glyph %d (%s)\n"), gnum,
		    info->chars[gnum]->name!=NULL?info->chars[gnum]->name:"<Nameless>" );
	warned = true;
    }
    free(xdeltas);
    free(ydeltas);
}

static void parsegvar(struct ttfinfo *info, FILE *ttf) {
    /* I'm only going to support a subset of the gvar. Only the global tuples */
    int axiscount, globaltc, gvarflags, gc, i,j,g;
    uint32 tupoff, dataoff, *gvars;
    struct variations *v = info->variations;
    int warned=false;

    fseek(ttf,info->gvar_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 ) {	/* I only understand version 1 */
	VariationFree(info);
return;
    }
    axiscount = getushort(ttf);
    if ( axiscount!=info->variations->axis_count ) {
	LogError( _("Hmm, the axis count in the 'gvar' table is different from that in the 'fvar' table.\n") );
	VariationFree(info);
return;
    }
    globaltc = getushort(ttf);
    tupoff = getlong(ttf) + info->gvar_start;
    gc = getushort(ttf);
    gvarflags = getushort(ttf);
    dataoff = getlong(ttf) + info->gvar_start;
    if ( globaltc==0 || globaltc>AppleMmMax ) {
	if ( globaltc==0 )
	    LogError( _("Hmm, no global tuples specified in the 'gvar' table.\n") );
	else
	    LogError( _("Hmm, too many global tuples specified in the 'gvar' table.\n FontForge only supports %d\n"), AppleMmMax );
	VariationFree(info);
return;
    }
    if ( gc>info->glyph_cnt ) {
	LogError( _("Hmm, more glyph variation data specified than there are glyphs in font.\n") );
	VariationFree(info);
return;
    }

    gvars = malloc((gc+1)*sizeof(uint32));
    if ( gvarflags&1 ) {	/* 32 bit data */
	for ( i=0; i<=gc; ++i )
	    gvars[i] = getlong(ttf)+dataoff;
    } else {
	for ( i=0; i<=gc; ++i )
	    gvars[i] = getushort(ttf)*2 +dataoff;	/* Undocumented *2 */
    }

    v->tuple_count = globaltc;
    v->tuples = calloc(globaltc,sizeof(struct tuples));
    fseek(ttf,tupoff,SEEK_SET);
    for ( i=0; i<globaltc; ++i ) {
	v->tuples[i].coords = malloc(axiscount*sizeof(float));
	for ( j=0; j<axiscount; ++j )
	    v->tuples[i].coords[j] = ((short) getushort(ttf))/16384.0;
	v->tuples[i].chars = InfoCopyGlyphs(info);
    }

    for ( g=0; g<gc; ++g ) if ( gvars[g]!=gvars[g+1] ) {
	int tc;
	uint32 datoff;
	int *sharedpoints=NULL;
	fseek(ttf,gvars[g],SEEK_SET);
	tc = getushort(ttf);
	datoff = gvars[g]+getushort(ttf);
	if ( tc&0x8000 ) {
	    uint32 here = ftell(ttf);
	    fseek(ttf,datoff,SEEK_SET);
	    sharedpoints = readpackedpoints(ttf);
	    datoff = ftell(ttf);
	    fseek(ttf,here,SEEK_SET);
	}
	for ( i=0; i<(tc&0xfff); ++i ) {
	    int tupleDataSize, tupleIndex;
	    tupleDataSize = getushort(ttf);
	    tupleIndex = getushort(ttf);
	    if ( tupleIndex&0xc000 ) {
		if ( !warned )
		    LogError( _("Warning: Glyph %d contains either private or intermediate tuple data.\n FontForge supports neither.\n"),
			    g);
		warned = true;
		if ( tupleIndex&0x8000 )
		    fseek(ttf,2*axiscount,SEEK_CUR);
		if ( tupleIndex&0x4000 )
		    fseek(ttf,4*axiscount,SEEK_CUR);
	    } else {
		int *localpoints=NULL;
		uint32 here = ftell(ttf);
		fseek(ttf,datoff,SEEK_SET);
		if ( tupleIndex&0x2000 )
		    localpoints = readpackedpoints(ttf);
		VaryGlyphs(info,tupleIndex&0xfff,g,
			(tupleIndex&0x2000)?localpoints:sharedpoints,ttf);
		free(localpoints);
		fseek(ttf,here,SEEK_SET);
	    }
	    datoff += tupleDataSize;
	}
	free(sharedpoints);
    }
    free(gvars);
}

static void AlterEntry(struct ttf_table *cvt, int i, int delta ) {
    int val = memushort(cvt->data,cvt->len,2*i);
    memputshort(cvt->data,2*i,val+delta);
}

static void VaryCvt(struct tuples *tuple,int *points, int *deltas,
	int pcnt, struct ttf_table *orig_cvt) {
    struct ttf_table *cvt;
    int i;

    if ( (cvt = tuple->cvt)==NULL ) {
	cvt = tuple->cvt = chunkalloc(sizeof(struct ttf_table));
	cvt->tag = orig_cvt->tag;
	cvt->len = cvt->maxlen = orig_cvt->len;
	cvt->data = malloc(cvt->len);
	memcpy(cvt->data,orig_cvt->data,cvt->len);
    }
    if ( points[0]==ALL_POINTS ) {
	for ( i=0; i<pcnt; ++i )
	    AlterEntry(cvt,i,deltas[i]);
    } else {
	for ( i=0; i<pcnt; ++i )
	    AlterEntry(cvt,points[i],deltas[i]);
    }
}

static void VaryCvts(struct ttfinfo *info,int tupleIndex, int *points, FILE *ttf,
    struct ttf_table *origcvt ) {
    /* one annoying thing about gvar, is that the variations do not describe */
    /*  designs. well variations for [0,1] describes that design, but the */
    /*  design for [1,1] includes the variations [0,1], [1,0], and [1,1] */
    /* And same is true of cvar */
    int pcnt, tc;
    int *deltas;
    struct variations *v = info->variations;

    if ( points[0]==ALL_POINTS )
	pcnt = origcvt->len/sizeof(uint16);
    else {
	for ( pcnt=0; points[pcnt]!=END_OF_POINTS; ++pcnt );
    }
    deltas = readpackeddeltas(ttf,pcnt);
    if ( deltas[0]!=BAD_DELTA )
	for ( tc = 0; tc<v->tuple_count; ++tc ) {
	    if ( TuplesMatch(v,tc,tupleIndex))
		VaryCvt(&v->tuples[tc],points,deltas,pcnt,origcvt);
    } else {
	static int warned = false;
	if ( !warned )
	    LogError( _("Incorrect number of deltas in cvt\n") );
	warned = true;
    }
    free(deltas);
}

static void parsecvar(struct ttfinfo *info, FILE *ttf) {
    struct ttf_table *cvt;
    int tuplecount;
    uint32 offset;
    int *sharedpoints=NULL;
    int i;
    int warned = false;

    for ( cvt = info->tabs; cvt!=NULL && cvt->tag!=CHR('c','v','t',' '); cvt=cvt->next );
    if ( cvt==NULL )
return;

    fseek(ttf,info->cvar_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 ) {	/* I only understand version 1 */
	/* I think I can live without cvt variations... */
	/* So I shan't free the structure */
return;
    }

    tuplecount = getushort(ttf);
    offset = info->cvar_start+getushort(ttf);
    /* The documentation implies there are flags packed into the tuplecount */
    /*  but John Jenkins tells me that shared points don't apply to cvar */
    /*  Might as well parse it just in case */
    if ( tuplecount&0x8000 ) {
	uint32 here = ftell(ttf);
	fseek(ttf,offset,SEEK_SET);
	sharedpoints = readpackedpoints(ttf);
	offset = ftell(ttf);
	fseek(ttf,here,SEEK_SET);
    }
    for ( i=0; i<(tuplecount&0xfff); ++i ) {
	int tupleDataSize, tupleIndex;
	tupleDataSize = getushort(ttf);
	tupleIndex = getushort(ttf);
	/* there is no provision here for a global tuple coordinate section */
	/*  so John says there are no tuple indeces. Just embedded tuples */
	if ( tupleIndex&0x4000 ) {
	    if ( !warned )
		LogError( _("Warning: 'cvar' contains intermediate tuple data.\n FontForge doesn't support this.\n") );
	    warned = true;
	    if ( tupleIndex&0x8000 )
		fseek(ttf,2*info->variations->axis_count,SEEK_CUR);
	    if ( tupleIndex&0x4000 )
		fseek(ttf,4*info->variations->axis_count,SEEK_CUR);
	} else {
	    int *localpoints=NULL;
	    uint32 here;
	    int j,k,ti;
	    ti = tupleIndex&0xfff;
	    if ( tupleIndex&0x8000 ) {
		real *coords = malloc(info->variations->axis_count*sizeof(real));
		for ( j=0; j<info->variations->axis_count; ++j )
		    coords[j] = ((int16) getushort(ttf))/16384.0;
		for ( k=0 ; k<info->variations->tuple_count; ++k ) {
		    for ( j=0; j<info->variations->axis_count; ++j )
			if ( coords[j]!=info->variations->tuples[k].coords[j] )
		    break;
		    if ( j==info->variations->axis_count )
		break;
		}
                free(coords);
		ti = -1;
		if ( k!=info->variations->tuple_count )
		    ti = k;
	    }
	    if ( ti!=-1 ) {
		here = ftell(ttf);
		fseek(ttf,offset,SEEK_SET);
		if ( tupleIndex&0x2000 )
		    localpoints = readpackedpoints(ttf);
		VaryCvts(info,ti,(tupleIndex&0x2000)?localpoints:sharedpoints,ttf,cvt);
		free(localpoints);
		fseek(ttf,here,SEEK_SET);
	    }
	}
	offset += tupleDataSize;
    }
    free(sharedpoints);
}

void readttfvariations(struct ttfinfo *info, FILE *ttf) {
    if ( info->gvar_start==0 || info->gvar_len==0 || info->fvar_start==0 || info->fvar_len==0 )
return;

    ff_progress_change_line2(_("Processing Variations"));
    parsefvar(info,ttf);
    if ( info->variations!=NULL && info->avar_start!=0 )
	parseavar(info,ttf);
    if ( info->variations!=NULL )
	parsegvar(info,ttf);
    if ( info->variations!=NULL && info->cvar_start!=0 && info->cvt_start!=0 )
	parsecvar(info,ttf);
}
