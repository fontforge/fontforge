/* Copyright (C) 2000,2001 by George Williams */
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
#include <gfile.h>
#include "splinefont.h"
#include "ustring.h"

static void WindowSelect(GWindow base,struct gmenuitem *mi) {
    GDrawRaise(mi->ti.userdata);
}

static void AddMI(GMenuItem *mi,GWindow gw,int changed, int top) {
    mi->ti.userdata = gw;
    mi->ti.bg = changed?0xffffff:GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gw));
    if ( top ) mi->ti.fg = 0x008000;
    mi->invoke = WindowSelect;
    mi->ti.text = GDrawGetWindowTitle(gw);
    if ( u_strlen( mi->ti.text ) > 20 )
	mi->ti.text[20] = '\0';
}

/* Builds up a menu containing the titles of all the major windows */
void WindowMenuBuild(GWindow base,struct gmenuitem *mi) {
    int i, cnt;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    BitmapView *bv;
    GMenuItem *sub;
    BDFFont *bdf;
    extern void GMenuItemArrayFree(GMenuItem *mi);

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    cnt = 0;
    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	++cnt;		/* for the font */
	for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL ) {
	    for ( cv = fv->sf->chars[i]->views; cv!=NULL; cv=cv->next )
		++cnt;		/* for each char view in the font */
	}
	for ( bdf= fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
		for ( bv = bdf->chars[i]->views; bv!=NULL; bv=bv->next )
		    ++cnt;
	    }
	}
	for ( mv=fv->metrics; mv!=NULL; mv=mv->next )
	    ++cnt;
    }
    if ( cnt==0 ) {
	/* This can't happen */
return;
    }
    sub = gcalloc(cnt+1,sizeof(GMenuItem));
    cnt = 0;
    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	AddMI(&sub[cnt++],fv->gw,fv->sf->changed,true);
	for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL ) {
	    for ( cv = fv->sf->chars[i]->views; cv!=NULL; cv=cv->next )
		AddMI(&sub[cnt++],cv->gw,cv->sc->changed,false);
	}
	for ( bdf= fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
		for ( bv = bdf->chars[i]->views; bv!=NULL; bv=bv->next )
		    AddMI(&sub[cnt++],bv->gw,bv->bc->changed,false);
	    }
	}
	for ( mv=fv->metrics; mv!=NULL; mv=mv->next )
	    AddMI(&sub[cnt++],mv->gw,false,false);
    }
    mi->sub = sub;
}

static void RecentSelect(GWindow base,struct gmenuitem *mi) {
    ViewPostscriptFont((char *) (mi->ti.userdata));
}

/* Builds up a menu containing the titles of all the unused recent files */
void MenuRecentBuild(GWindow base,struct gmenuitem *mi) {
    int i, cnt, cnt1;
    FontView *fv;
    extern void GMenuItemArrayFree(struct gmenuitem *mi);
    GMenuItem *sub;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    cnt = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL )
	    ++cnt;
    }
    if ( cnt==0 ) {
	/* This can't happen */
return;
    }
    sub = gcalloc(cnt+1,sizeof(GMenuItem));
    cnt1 = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL ) {
	    GMenuItem *mi = &sub[cnt1++];
	    mi->ti.userdata = RecentFiles[i];
	    mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	    mi->invoke = RecentSelect;
	    mi->ti.text = uc_copy(GFileNameTail(RecentFiles[i]));
	}
    }
    if ( cnt!=cnt1 )
	GDrawIError( "Bad counts in MenuRecentBuild");
    mi->sub = sub;
}

int RecentFilesAny(void) {
    int i;
    FontView *fvl;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fvl=fv_list; fvl!=NULL; fvl=fvl->next )
	    if ( fvl->sf->filename!=NULL && strcmp(fvl->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fvl==NULL )
return( true );
    }
return( false );
}
