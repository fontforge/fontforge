/* Copyright (C) 2007-2012 by George Williams */
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

/* Access to Raph Levien's spiro splines */
/* See http://www.levien.com/spiro/ */

#ifdef _NO_LIBSPIRO

static int has_spiro = false;

SplineSet *SpiroCP2SplineSet(spiro_cp *spiros) {
return( NULL );
}

spiro_cp *SplineSet2SpiroCP(SplineSet *ss,uint16 *cnt) {
return( NULL );
}

#else /* ! _NO_LIBSPIRO */

#  include "bezctx_ff.h"

static int has_spiro = true;

SplineSet *SpiroCP2SplineSet(spiro_cp *spiros) {
    int n;
    int any = 0;
    spiro_cp *nspiros;
    SplineSet *ss;
    int lastty = 0;

    if ( spiros==NULL )
return( NULL );
    for ( n=0; spiros[n].ty!=SPIRO_END; ++n )
	if ( SPIRO_SELECTED(&spiros[n]) )
	    ++any;
    if ( n==0 )
return( NULL );
    if ( n==1 ) {
	ss = chunkalloc(sizeof(SplineSet));
	ss->first = ss->last = SplinePointCreate(spiros[0].x,spiros[0].y);
    } else {
	bezctx *bc = new_bezctx_ff();
	if ( (spiros[0].ty&0x7f)=='{' ) {
	    lastty = spiros[n-1].ty;
	    spiros[n-1].ty = '}';
	}

	if ( !any ) {
#if _LIBSPIRO_FUN
	    if ( TaggedSpiroCPsToBezier0(spiros,bc)==0 ) {
		return( NULL );
	    }
#else
	    TaggedSpiroCPsToBezier(spiros,bc);
#endif
	} else {
	    nspiros = galloc((n+1)*sizeof(spiro_cp));
	    memcpy(nspiros,spiros,(n+1)*sizeof(spiro_cp));
	    for ( n=0; nspiros[n].ty!=SPIRO_END; ++n )
		nspiros[n].ty &= ~0x80;
#if _LIBSPIRO_FUN
	    if ( TaggedSpiroCPsToBezier0(nspiros,bc)==0 ) {
		free(nspiros);
		return( NULL );
	    }
#else
	    TaggedSpiroCPsToBezier(nspiros,bc);
#endif
	    free(nspiros);
	}
	ss = bezctx_ff_close(bc);

	if ( (spiros[0].ty&0x7f)=='{' )
	    spiros[n-1].ty = lastty;
    }
    ss->spiros = spiros;
    ss->spiro_cnt = ss->spiro_max = n+1;
    SPLCatagorizePoints(ss);
return( ss );
}

spiro_cp *SplineSet2SpiroCP(SplineSet *ss,uint16 *_cnt) {
    /* I don't know a good way to do this. I hope including a couple of */
    /*  mid-points on every spline will do a reasonable job */
    SplinePoint *sp;
    Spline *s;
    int cnt;
    spiro_cp *ret;

    for ( cnt=0, sp=ss->first; ; ) {
	++cnt;
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp == ss->first )
    break;
    }

    ret = galloc((3*cnt+1)*sizeof(spiro_cp));

    for ( cnt=0, sp=ss->first; ; ) {
	ret[cnt].x = sp->me.x;
	ret[cnt].y = sp->me.y;
	ret[cnt].ty = sp->pointtype == pt_corner ? SPIRO_CORNER :
		      sp->pointtype == pt_tangent ? SPIRO_LEFT :
						    SPIRO_G4;
	if ( sp->pointtype == pt_tangent && sp->prev!=NULL && sp->next!=NULL ) {
	    if ( (sp->next->knownlinear && sp->prev->knownlinear) ||
		    (!sp->next->knownlinear && !sp->prev->knownlinear ))
		ret[cnt].ty = SPIRO_CORNER;
	    else if ( sp->prev->knownlinear && !sp->nonextcp )
		ret[cnt].ty = SPIRO_RIGHT;
	    else if ( sp->next->knownlinear && !sp->noprevcp )
		ret[cnt].ty = SPIRO_LEFT;
	} else if ( sp->pointtype==pt_curve && sp->prev!=NULL && sp->prev->knownlinear &&
		!sp->nonextcp && sp->prev->from->pointtype == pt_corner )
	    ret[cnt].ty = SPIRO_LEFT;
	else if ( sp->pointtype==pt_curve && sp->next!=NULL && sp->next->knownlinear &&
		!sp->noprevcp && sp->next->to->pointtype == pt_corner )
	    ret[cnt].ty = SPIRO_RIGHT;
	++cnt;
	if ( sp->next==NULL )
    break;
	s = sp->next;
	if ( s->isquadratic ) {
	    ret[cnt].x = s->splines[0].d + .5*(s->splines[0].c+.5*(s->splines[0].b + .5*s->splines[0].a));
	    ret[cnt].y = s->splines[1].d + .5*(s->splines[1].c+.5*(s->splines[1].b + .5*s->splines[1].a));
	    ret[cnt++].ty = SPIRO_G4;
	} else if ( !s->knownlinear ) {
	    ret[cnt].x = s->splines[0].d + .333*(s->splines[0].c+.333*(s->splines[0].b + .333*s->splines[0].a));
	    ret[cnt].y = s->splines[1].d + .333*(s->splines[1].c+.333*(s->splines[1].b + .333*s->splines[1].a));
	    ret[cnt++].ty = SPIRO_G4;
	    ret[cnt].x = s->splines[0].d + .667*(s->splines[0].c+.667*(s->splines[0].b + .667*s->splines[0].a));
	    ret[cnt].y = s->splines[1].d + .667*(s->splines[1].c+.667*(s->splines[1].b + .667*s->splines[1].a));
	    ret[cnt++].ty = SPIRO_G4;
	}
	sp = sp->next->to;
	if ( sp == ss->first )
    break;
    }
    ret[cnt].x = ret[cnt].y = 0;
    ret[cnt++].ty = SPIRO_END;
    if ( ss->first->prev==NULL )
	ret[0].ty = SPIRO_OPEN_CONTOUR;
    if ( _cnt!=NULL ) *_cnt = cnt;
return( ret );
}

#endif /* ! _NO_LIBSPIRO */

int hasspiro(void) {
    return has_spiro;
}

spiro_cp *SpiroCPCopy(spiro_cp *spiros,uint16 *_cnt) {
    int n;
    spiro_cp *nspiros;

    if ( spiros==NULL )
return( NULL );
    for ( n=0; spiros[n].ty!='z'; ++n );
    nspiros = galloc((n+1)*sizeof(spiro_cp));
    memcpy(nspiros,spiros,(n+1)*sizeof(spiro_cp));
    if ( _cnt != NULL ) *_cnt = n+1;
return( nspiros );
}

void SSRegenerateFromSpiros(SplineSet *spl) {
    SplineSet *temp;

    if ( spl->spiro_cnt<=1 )
return;
	if ( !has_spiro)
return;

    SplineSetBeziersClear(spl);
    temp = SpiroCP2SplineSet(spl->spiros);
    if ( temp!=NULL ) {
	spl->first = temp->first;
	spl->last = temp->last;
	chunkfree(temp,sizeof(SplineSet));
    } else {
	/* didn't converge... or something */
	int i;
	SplinePoint *sp, *last;
	last = spl->first = SplinePointCreate(spl->spiros[0].x, spl->spiros[0].y);
	for ( i=1; i<spl->spiro_cnt; ++i ) {
	    sp = SplinePointCreate(spl->spiros[i].x, spl->spiros[i].y);
	    SplineMake3(last,sp);
	    last = sp;
	}
	if ( SPIRO_SPL_OPEN(spl))
	    spl->last = last;
	else {
	    SplineMake3(last,spl->first);
	    spl->last = spl->first;
	}
    }
    spl->beziers_need_optimizer = true;
}
