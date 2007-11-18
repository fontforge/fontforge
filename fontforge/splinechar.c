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

#include "pfaedit.h"
#include <math.h>
#include <locale.h>
#ifndef FONTFORGE_CONFIG_GTK
# include <ustring.h>
# include <utype.h>
# include <gresource.h>
# ifdef FONTFORGE_CONFIG_GDRAW
extern int _GScrollBar_Width;
#  include <gkeysym.h>
# endif
#endif
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

void SCClearRounds(SplineChar *sc) {
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->roundx = sp->roundy = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}

void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl) {
    /* Replace all the old points with the ones in rpl in the minimum distance hints */
    SplinePoint *osp, *rsp;
    MinimumDistance *test;

    if ( md==NULL )
return;

    while ( old!=NULL && rpl!=NULL ) {
	osp = old->first; rsp = rpl->first;
	while ( 1 ) {
	    for ( test=md; test!=NULL ; test=test->next ) {
		if ( test->sp1==osp )
		    test->sp1 = rsp;
		if ( test->sp2==osp )
		    test->sp2 = rsp;
	    }
	    if ( osp->next==NULL || rsp->next==NULL )
	break;
	    osp = osp->next->to;
	    rsp = rsp->next->to;
	    if ( osp==old->first )
	break;
	}
	old = old->next;
	rpl = rpl->next;
    }
}
