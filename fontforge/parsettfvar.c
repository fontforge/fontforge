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
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "ttf.h"

/* Parse Apple's variation tables (font, glyph, cvt and axis) */
/* To be interesting a font must have an fvar and a gvar */
/* If it's hinted it better have a cvar */
/* It may have an avar if it wants to be complicated */

static void VariationFree(struct ttfinfo *info) {
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
	    for ( j=0; j<info->glyph_cnt; ++j )
		SplineCharFree(info->chars[j]);
	    free(info->chars);
	    KernClassListFree(info->khead);
	    KernClassListFree(info->vkhead);
	}
	free(variation->tuples);
    }
    free(variation);
    info->variations = NULL;
}

static void parsefvar(struct ttfinfo *info, FILE *ttf) {
    int data_off, axis_count, instance_count;
    int i,j;

    fseek(ttf,info->fvar_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 )	/* I only understand version 1 */
return;
    data_off = getushort(ttf);
    if ( getushort(ttf)>2 )
	fprintf( stderr, "Hmm, this 'fvar' table has more count/size pairs than I expect\n" );
    else if ( getushort(ttf)<2 ) {
	fprintf( stderr, "Hmm, this 'fvar' table has too few count/size pairs, I shan't parse it\n" );
return;
    }
    axis_count = getushort(ttf);
    if ( axis_count==0 || axis_count>4 ) {
	if ( axis_count==0 )
	    fprintf( stderr, "Hmm, this 'fvar' table has no axes, that doesn't make sense.\n" );
	else
	    fprintf( stderr, "Hmm, this 'fvar' table has more axes than FontForge can handle.\n" );
return;
    }
    if ( getushort(ttf)!=20 ) {
	fprintf( stderr, "Hmm, this 'fvar' table has an unexpected size for an axis, I shan't parse it\n" );
return;
    }
    instance_count = getushort(ttf);
    if ( getushort(ttf)!=4+4*axis_count ) {
	fprintf( stderr, "Hmm, this 'fvar' table has an unexpected size for an instance, I shan't parse it\n" );
return;
    }
    if ( data_off+axis_count*20+instance_count*(4+4*axis_count)> info->fvar_len ) {
	fprintf( stderr, "Hmm, this 'fvar' table is too short\n" );
return;
    }

    info->variations = gcalloc(1,sizeof(struct variations));
    info->variations->axis_count = axis_count;
    info->variations->instance_count = instance_count;
    info->variations->axes = gcalloc(axis_count,sizeof(struct taxis));
    info->variations->instances = gcalloc(axis_count,sizeof(struct tinstance));
    for ( i=0; i<instance_count; ++i )
	info->variations->instances[i].coords = galloc(axis_count*sizeof(real));

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
	fprintf( stderr, "Hmm, the axis count in the 'avar' table is different from that in the 'fvar' table.\n" );
	VariationFree(info);
return;
    }
    for ( i=0; i<axis_count; ++i ) {
	pair_count = getushort(ttf);
	if ( pair_count!=0 ) {
	    info->variations->axes[i].mapfrom = galloc(pair_count*sizeof(real));
	    info->variations->axes[i].mapto= galloc(pair_count*sizeof(real));
	    for ( j=0; j<pair_count; ++j ) {
		info->variations->axes[i].mapfrom[j] = getushort(ttf)/16384.0;
		info->variations->axes[i].mapto[j] = getushort(ttf)/16384.0;
	    }
	}
    }
    if ( ftell(ttf)-info->avar_start>info->avar_len) {
	fprintf( stderr, "Hmm, the the 'avar' table is too long.\n" );
	VariationFree(info);
return;
    }
}

static SplineChar **InfoCopyGlyphs(struct ttfinfo *info) {
    SplineChar **chars = galloc(info->glyph_cnt*sizeof(SplineChar *));
    int i;

    for ( i=0; i<info->glyph_cnt; ++i )
	chars[i] = ( info->chars[i]!=NULL ) ? SplineCharCopy(info->chars[i],NULL) : NULL;
return( chars );
}

static int *readpackeddeltas(FILE *ttf,int n) {
    int *deltas;
    int runcnt, i, j;

    deltas = galloc(n*sizeof(int));

    i = 0;
    while ( i<n ) {
	runcnt = getc(ttf);
	if ( runcnt&0x80 ) {
	    /* runcnt zeros get added */
	    for ( j=0; j<=(runcnt&0x3f); ++j )
		deltas[i++] = 0;
	} else if ( runcnt&0x40 ) {
	    /* runcnt shorts from the stack *;
	    runcnt = (runcnt&0x7f);
	    for ( j=0 ; j<=(runcnt&0x3f); ++j )
		deltas[i++] = (int16) getushort(ttf);
	} else {
	    /* runcnt signed bytes from the stack */
	    for ( j=0; j<runcnt; ++j )
		deltas[i++] = (int8) getc(ttf);
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
    if ( n&0x80 )
	n = getc(ttf)|((n&0x7f)<<8);
    points = galloc((n+1)*sizeof(int));
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
		for ( j=0; j<runcnt; ++j )
		    points[i++] = (first += getushort(ttf));
	    } else {
		points[i++] = first = getc(ttf);
		for ( j=0; j<runcnt; ++j )
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

    if ( vtest==dbase )
return( true );
    for ( i=0; i<v->axis_count; ++i ) {
	if ( v->tuples[dbase].coords[i]==0 && v->tuples[vtest].coords[i]!=0 )
return( false );
	if ( v->tuples[vtest].coords[i]!=0 &&
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
	    for ( sp=ss->first; sp!=NULL ; ++i ) {
		if ( sp->ttfindex!=0xffff && sp->ttfindex!=0xfffe )
		    ++i;
		if ( sp->nextcpindex!=0xffff && sp->nextcpindex!=0xfffe )
		    ++i;
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
    Spline *s, *first;
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
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    SplineRefigure(s);
	    if ( first==NULL ) first = s;
	}
    }
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	ref->transform[4] += xd;
	ref->transform[5] += yd;
	SCReinstanciateRefChar(sc,ref);
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
		    SCReinstanciateRefChar(sc,ref);
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
		first = NULL;
		for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
		    SplineRefigure(s);
		    if ( first==NULL ) first = s;
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
			SCReinstanciateRefChar(sc,ref);
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
	if ( points[j]==i )
	    SCShiftAllBy(sc,-xdeltas[j++],0);
	if ( points[j]==i+1 )
	    sc->width += xdeltas[j++];
	if ( points[j]==i+2 )
	    SCShiftAllBy(sc,0,-ydeltas[j++]);
	if ( points[j]==i+3 )
	    sc->vwidth += ydeltas[j++];
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
    if ( points[0]==ALL_POINTS )
	pcnt = PointCount(info->chars[gnum])+4;
    else {
	for ( pcnt=0; points[pcnt]!=END_OF_POINTS; ++pcnt );
    }
    xdeltas = readpackeddeltas(ttf,pcnt);
    ydeltas = readpackeddeltas(ttf,pcnt);
    for ( tc = 0; tc<v->tuple_count; ++tc )
	if ( TuplesMatch(v,tc,tupleIndex))
	    VaryGlyph(v->tuples[tc].chars[gnum],points,xdeltas,ydeltas,pcnt);
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
    axiscount = getlong(ttf);
    if ( axiscount!=info->variations->axis_count ) {
	fprintf( stderr, "Hmm, the axis count in the 'gvar' table is different from that in the 'fvar' table.\n" );
	VariationFree(info);
return;
    }
    globaltc = getushort(ttf);
    tupoff = getlong(ttf) + info->gvar_start;
    gc = getushort(ttf);
    gvarflags = getushort(ttf);
    dataoff = getlong(ttf) + info->gvar_start;
    if ( globaltc==0 || globaltc>100 ) {
	if ( globaltc==0 )
	    fprintf( stderr, "Hmm, no global tuples specified in the 'gvar' table.\n" );
	else
	    fprintf( stderr, "Hmm, too many global tuples specified in the 'gvar' table.\n" );
	VariationFree(info);
return;
    }
    if ( gc>info->glyph_cnt ) {
	fprintf( stderr, "Hmm, more glyph variation data specified than there are glyphs in font.\n" );
	VariationFree(info);
return;
    }

    gvars = galloc((gc+1)*sizeof(uint32));
    if ( gvarflags&1 ) {	/* 32 bit data */
	for ( i=0; i<=gc; ++i )
	    gvars[i] = getlong(ttf)+dataoff;
    } else {
	for ( i=0; i<=gc; ++i )
	    gvars[i] = getushort(ttf)*2 +dataoff;	/* Undocumented *2 */
    }

    v->tuple_count = globaltc;
    v->tuples = gcalloc(globaltc,sizeof(struct tuples));
    fseek(ttf,tupoff,SEEK_SET);
    for ( i=0; i<globaltc; ++i ) {
	v->tuples[i].coords = galloc(axiscount*sizeof(float));
	for ( j=0; j<axiscount; ++j )
	    v->tuples[i].coords[j] = ((short) getushort(ttf))/16384.0;
	v->tuples[i].chars = InfoCopyGlyphs(info);
    }

    for ( g=0; g<gc; ++g ) if ( gvars[g]!=gvars[g+1] ) {
	int tc;
	uint32 datoff;
	int *sharedpoints=NULL;
	tc = getushort(ttf);
	datoff = gvars[g]+getushort(ttf);
	if ( tc&0x8000 ) {
	    uint32 here = ftell(ttf);
	    fseek(ttf,dataoff,SEEK_SET);
	    sharedpoints = readpackedpoints(ttf);
	    dataoff = ftell(ttf);
	    fseek(ttf,here,SEEK_SET);
	}
	for ( i=0; i<(tc&0xfff); ++i ) {
	    int tupleDataSize, tupleIndex;
	    tupleDataSize = getushort(ttf);
	    tupleIndex = getushort(ttf);
	    if ( tupleIndex&0xc000 ) {
		if ( !warned )
		    fprintf( stderr, "Warning: Glyph %d contains either private or intermediate tuple data.\n FontForge supports neither.\n",
			    g);
		warned = true;
		if ( tupleIndex&0x8000 )
		    fseek(ttf,2*axiscount,SEEK_CUR);
		if ( tupleIndex&0x4000 )
		    fseek(ttf,4*axiscount,SEEK_CUR);
	    } else {
		int *localpoints=NULL;
		uint32 here = ftell(ttf);
		fseek(ttf,dataoff,SEEK_SET);
		if ( tupleIndex&0x2000 )
		    localpoints = readpackedpoints(ttf);
		VaryGlyphs(info,tupleIndex&0xfff,g,
			(tupleIndex&0x2000)?sharedpoints:localpoints,ttf);
		free(localpoints);
		fseek(ttf,here,SEEK_SET);
	    }
	    dataoff += tupleDataSize;
	}
	free(sharedpoints);
    }
    free(gvars);
}

static void parsecvar(struct ttfinfo *info, FILE *ttf) {
}

void readttfvariations(struct ttfinfo *info, FILE *ttf) {
    if ( info->gvar_start==0 || info->gvar_len==0 || info->fvar_start==0 || info->fvar_len==0 )
return;

    parsefvar(info,ttf);
    if ( info->variations!=NULL && info->avar_start!=0 )
	parseavar(info,ttf);
    if ( info->variations!=NULL )
	parsegvar(info,ttf);
    if ( info->variations!=NULL && info->cvar_start!=0 )
	parsecvar(info,ttf);
}
