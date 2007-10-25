/* Copyright (C) 2007 by George Williams */
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

/* Access to Raph Levien's spiro splines */
/* See http://www.levien.com/spiro/ */

#ifdef NODYNAMIC
int hasspiro(void) {
return(false);
}

SplineSet *SpiroCP2SplineSet(spiro_cp *spiros) {
return( NULL );
}

spiro_cp *SplineSet2SpiroCP(SplineSet *ss,int *cnt) {
return( NULL );
}
#else
#  include <dynamic.h>
#  include "bezctx_ff.h"
#  include "spiro_exports.h"

static DL_CONST void *libspiro;
static spiro_seg *(*_run_spiro)(const spiro_cp *src, int n);
static void (*_free_spiro)(spiro_seg *s);
static void (*_spiro_to_bpath)(const spiro_seg *s, int n, bezctx *bc);
static int inited = false, has_spiro = false;

static void initSpiro(void) {
    if ( inited )
return;

    libspiro = dlopen("libspiro" SO_EXT,RTLD_LAZY);
    if ( libspiro==NULL )
#ifdef LIBDIR
	libspiro = dlopen(LIBDIR "/libspiro" SO_EXT,RTLD_LAZY);
#else
	libspiro = dlopen("/usr/local/bin" "/libspiro" SO_EXT,RTLD_LAZY);
#endif
	
    inited = true;

    if ( libspiro==NULL ) {
#ifndef __Mac
	fprintf( stderr, "%s\n", dlerror());
#endif
return;
    }

    _run_spiro = (spiro_seg *(*)(const spiro_cp *src, int n)) dlsym(libspiro,"run_spiro");
    _free_spiro = (void (*)(spiro_seg *s)) dlsym(libspiro,"free_spiro");
    _spiro_to_bpath = (void (*)(const spiro_seg *s, int n, bezctx *bc)) dlsym(libspiro,"spiro_to_bpath");
    if ( _run_spiro==NULL || _free_spiro==NULL || _spiro_to_bpath==NULL ) {
	LogError("Hmm. This system has a libspiro, but it doesn't contain the entry points\nfontforge needs. Must be something else.\n");
    } else
	has_spiro = true;
}

int hasspiro(void) {
    initSpiro();
return(has_spiro);
}

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
	spiro_seg *s;
	bezctx *bc = new_bezctx_ff();
	if ( spiros[0].ty=='{' ) {
	    lastty = spiros[n-1].ty;
	    spiros[n-1].ty = '}';
	}

	if ( !any )
	    s = _run_spiro(spiros,n);
	else {
	    nspiros = galloc((n+1)*sizeof(spiro_cp));
	    memcpy(nspiros,spiros,(n+1)*sizeof(spiro_cp));
	    for ( n=0; nspiros[n].ty!=SPIRO_END; ++n )
		nspiros[n].ty &= ~0x80;
	    s = _run_spiro(nspiros,n);
	    free(nspiros);
	}
	_spiro_to_bpath(s,n,bc);
	_free_spiro(s);
	ss = bezctx_ff_close(bc);

	if ( spiros[0].ty=='{' )
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
    BasePoint pdir, ndir;
    int is_right;

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
	    else if ( sp->prev->knownlinear && !sp->nonextcp ) {
		pdir.x = sp->me.x-sp->prev->from->me.x;
		pdir.y = sp->me.y-sp->prev->from->me.y;
		ndir.x = sp->nextcp.x - sp->me.x;
		ndir.y = sp->nextcp.y - sp->me.y;
		is_right = (-pdir.y*ndir.x + pdir.x*ndir.y)<0;
		ret[cnt].ty = is_right ? SPIRO_RIGHT : SPIRO_LEFT;
	    } else if ( sp->next->knownlinear && !sp->noprevcp ) {
		pdir.x = sp->me.x-sp->next->to->me.x;
		pdir.y = sp->me.y-sp->next->to->me.y;
		ndir.x = sp->prevcp.x - sp->me.x;
		ndir.y = sp->prevcp.y - sp->me.y;
		is_right = (-pdir.y*ndir.x + pdir.x*ndir.y)<0;
		ret[cnt].ty = is_right ? SPIRO_RIGHT : SPIRO_LEFT;
	    }
	}
	++cnt;
	if ( sp->next==NULL )
    break;
	s = sp->next;
	if ( !s->knownlinear ) {
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
	ret[0].ty = SPIRO_OPEN_CONTOUR;		/* I'm guessing this means an open contour to raph */
    if ( _cnt!=NULL ) *_cnt = cnt+1;
return( ret );
}
#endif

spiro_cp *SpiroCPCopy(spiro_cp *spiros,uint16 *_cnt) {
    int n;
    spiro_cp *nspiros;

    if ( spiros==NULL )
return( NULL );
    for ( n=0; spiros[n].ty!='z'; ++n );
    nspiros = galloc((n+1)*sizeof(spiro_cp));
    memcpy(nspiros,spiros,(n+1)*sizeof(spiro_cp));
    for ( n=0; nspiros[n].ty!='z'; ++n )
	nspiros[n].ty &= ~0x80;
    if ( _cnt != NULL ) *_cnt = n+1;
return( nspiros );
}

void SSRegenerateFromSpiros(SplineSet *spl) {
    SplineSet *temp;
    SplineSetBeziersClear(spl);
    temp = SpiroCP2SplineSet(spl->spiros);
    spl->first = temp->first;
    spl->last = temp->last;
    chunkfree(temp,sizeof(SplineSet));
}
