/* Copyright (C) 2009-2012 by George Williams */
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
#include "fontforgevw.h"
#include "fffreetype.h"
#include "delta.h"
#include "splineutil.h"
#include <math.h>

static int NearestPt(Spline *s,BasePoint *bp) {
    double distance=1e20, test, offx, offy;
    int pt= -1;

    if ( s->from->ttfindex!=0xffff ) {
	offx = s->from->me.x-bp->x;
	offy = s->from->me.y-bp->y;
	distance = offx*offx + offy*offy;
	pt = s->from->ttfindex;
    } else if ( s->from->prev!=NULL && s->from->prev->from->nextcpindex!=0xffff ) {
	offx = s->from->prev->from->nextcp.x-bp->x;
	offy = s->from->prev->from->nextcp.y-bp->y;
	distance = offx*offx + offy*offy;
	pt = s->from->prev->from->nextcpindex;
    }
    if ( s->to->ttfindex!=0xffff ) {
	offx = s->to->me.x-bp->x;
	offy = s->to->me.y-bp->y;
	test = offx*offx + offy*offy;
	if ( test<distance ) {
	    distance=test;
	    pt = s->to->ttfindex;
	}
    } else if ( s->to->next!=NULL && s->to->nextcpindex!=0xffff ) {
	offx = s->to->nextcp.x-bp->x;
	offy = s->to->nextcp.y-bp->y;
	test = offx*offx + offy*offy;
	if ( test<distance ) {
	    distance=test;
	    pt = s->to->nextcpindex;
	}
    }
    if ( s->from->nextcpindex!=0xffff ) {
	offx = s->from->nextcp.x-bp->x;
	offy = s->from->nextcp.y-bp->y;
	test = offx*offx + offy*offy;
	if ( test<distance ) {
	    distance=test;
	    pt = s->from->nextcpindex;
	}
    }
return( pt );
}

static int Duplicate(struct qg_data *data,int pt,int x,int y,double dist) {
    int i;

    for ( i=data->glyph_start; i<data->cur; ++i ) {
	if ( data->qg[i].nearestpt == pt &&
		data->qg[i].x == x &&
		data->qg[i].y == y ) {
	    if ( dist<data->qg[i].distance )
		data->qg[i].distance = dist;
return( true );
	}
    }
return( false );
}

static void SplineFindQuestionablePoints(struct qg_data *data,Spline *s) {
    DBounds b;
    BasePoint bp;
    real bottomy;
    QuestionableGrid *qg;

    /* Find the bounding box of the spline. It's quadratic, so only one cp */
    b.minx = b.maxx = s->from->me.x;
    b.miny = b.maxy = s->from->me.y;
    if ( s->from->nextcp.x > b.maxx ) b.maxx = s->from->nextcp.x;
    else if ( s->from->nextcp.x < b.minx ) b.minx = s->from->nextcp.x;
    if ( s->from->nextcp.y > b.maxy ) b.maxy = s->from->nextcp.y;
    else if ( s->from->nextcp.y < b.miny ) b.miny = s->from->nextcp.y;
    if ( s->to->me.x > b.maxx ) b.maxx = s->to->me.x;
    else if ( s->to->me.x < b.minx ) b.minx = s->to->me.x;
    if ( s->to->me.y > b.maxy ) b.maxy = s->to->me.y;
    else if ( s->to->me.y < b.miny ) b.miny = s->to->me.y;

    b.minx -= data->within; b.maxx += data->within;
    b.miny -= data->within; b.maxy += data->within;

    bp.x = floor(b.minx)+.5;
    if ( bp.x<b.minx ) ++bp.x;
    bottomy = floor(b.miny)+.5;
    if ( bottomy<b.miny ) ++bottomy;
    for ( ; bp.x<=b.maxx; ++bp.x ) {
	for ( bp.y = bottomy; bp.y<=b.maxy; ++bp.y ) {
	    bigreal dist = SplineMinDistanceToPoint(s,&bp);
	    if ( dist<data->within ) {
		int pt = NearestPt(s,&bp);
		if ( !Duplicate(data,pt,floor(bp.x),floor(bp.y),dist)) {
		    if ( data->cur>=data->max )
			data->qg = realloc(data->qg,(data->max += 100) * sizeof(QuestionableGrid));
		    qg = &data->qg[data->cur];
		    qg->sc = data->sc;
		    qg->size = data->cur_size;
		    qg->nearestpt = pt;
		    qg->x = floor(bp.x); qg->y = floor(bp.y);
		    qg->distance = dist;
		    ++data->cur;
		}
	    }
	}
    }
}

static void SCFindQuestionablePoints(struct qg_data *data) {
    uint16 width;
    SplineSet *gridfit = FreeType_GridFitChar(data->freetype_context,
	    data->sc->orig_pos, data->cur_size, data->cur_size,
	    data->dpi, &width, data->sc, data->depth, false);
    SplineSet *spl;
    Spline *s, *first;

    data->glyph_start = data->cur;
    for ( spl = gridfit; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( s=spl->first->next; s!=first && s!=NULL; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    SplineFindQuestionablePoints(data,s);
	}
    }
    SplinePointListsFree(gridfit);
}

void TopFindQuestionablePoints(struct qg_data *data) {
    char *pt, *end;
    int low, high;

    if ( data->fv!=NULL )
	data->freetype_context = _FreeTypeFontContext(data->fv->sf,NULL,data->fv,data->layer,
	    ff_ttf,0,NULL);
    else
	data->freetype_context = _FreeTypeFontContext(data->sc->parent,data->sc,NULL,data->layer,
	    ff_ttf,0,NULL);
    if ( data->freetype_context==NULL ) {
	data->error = qg_nofont;
return;
    }
    data->qg = NULL;
    data->cur = data->max = 0;
    data->error = qg_ok;

    for ( pt=data->pixelsizes; *pt; pt=end ) {
	low = strtol(pt,&end,10);
	if ( pt==end ) {
	    data->error = qg_notnumber;
return;
	}
	while ( *end==' ' ) ++end;
	if ( *end=='-' ) {
	    pt = end+1;
	    high = strtol(pt,&end,10);
	    if ( pt==end ) {
		data->error = qg_notnumber;
return;
	    }
	    if ( high<low ) {
		data->error = qg_badrange;
return;
	    }
	} else
	    high = low;
	if ( low<2 || low>4096 || high<2 || high>4096 ) {
	    data->error = qg_badnumber;
return;
	}
	while ( *end==' ' ) ++end;
	if ( *end==',' ) ++end;
	for ( data->cur_size = low; data->cur_size <= high; ++ data->cur_size ) {
	    if ( data->fv!=NULL ) {
		int enc, gid;
		for ( enc=0; enc<data->fv->map->enccount; ++enc ) {
		    if ( data->fv->selected[enc] && (gid = data->fv->map->map[enc])!=-1 &&
			    (data->sc = data->fv->sf->glyphs[gid])!=NULL )
			SCFindQuestionablePoints(data);
		}
	    } else {
		SCFindQuestionablePoints(data);
	    }
	}
    }
    FreeTypeFreeContext(data->freetype_context);
    data->freetype_context = NULL;
}
