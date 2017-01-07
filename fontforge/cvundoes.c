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

#include "cvundoes.h"

#include "autohint.h"
#include "bitmapchar.h"
#include "bvedit.h"
#include "config.h"
#include "cvexport.h"
#include "cvimages.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "namelist.h"
#include "sfd.h"
#include "spiro.h"
#include "splinefill.h"
#include "splineorder2.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "svg.h"
#include "views.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include "inc/basics.h"
#include "inc/gfile.h"
#include "psfont.h"

#ifndef HAVE_EXECINFO_H
// no backtrace available
#else
    #include <execinfo.h>
#endif
#include "collabclient.h"

extern char *coord_sep;

int onlycopydisplayed = 0;
int copymetadata = 0;
int copyttfinstr = 0;
#if defined(__Mac)
int export_clipboard = 0;
#else
int export_clipboard = 1;
#endif


/* ********************************* Undoes ********************************* */

int maxundoes = 120;		/* -1 is infinite */
int preserve_hint_undoes = true;

static uint8 *bmpcopy(uint8 *bitmap,int bytes_per_line, int lines) {
    uint8 *ret = malloc(bytes_per_line*lines);
    memcpy(ret,bitmap,bytes_per_line*lines);
return( ret );
}

RefChar *RefCharsCopyState(SplineChar *sc,int layer) {
    RefChar *head=NULL, *last=NULL, *new, *crefs;

    if ( layer<0 || sc->layers[layer].refs==NULL )
return( NULL );
    for ( crefs = sc->layers[layer].refs; crefs!=NULL; crefs=crefs->next ) {
	new = RefCharCreate();
	free(new->layers);
	*new = *crefs;
	new->layers = calloc(new->layer_cnt,sizeof(struct reflayer));
	new->next = NULL;
	if ( last==NULL )
	    head = last = new;
	else {
	    last->next = new;
	    last = new;
	}
    }
return( head );
}

static SplinePointList *RefCharsCopyUnlinked(SplinePointList *sofar, SplineChar *sc,int layer) {
    RefChar *crefs;
    SplinePointList *last = NULL, *new;
    int l;

    if ( layer<0 || sc->layers[layer].refs==NULL )
return( sofar );
    if ( sofar!=NULL )
	for ( last=sofar; last->next!=NULL; last=last->next );
    for ( crefs = sc->layers[layer].refs; crefs!=NULL; crefs=crefs->next ) {
	for ( l=0; l<crefs->layer_cnt; ++l ) {
	    new = SplinePointListCopy(crefs->layers[l].splines);
	    if ( sofar!=NULL ) {
		last->next = new;
		for ( ; last->next!=NULL; last=last->next );
	    } else {
		sofar = new;
		if ( new!=NULL )
		    for ( last=sofar; last->next!=NULL; last=last->next );
	    }
	}
    }
return( sofar );
}

static int RefCharsMatch(RefChar *urefs,RefChar *crefs) {
    /* I assume they are in the same order */
    while ( urefs!=NULL && crefs!=NULL ) {
	if ( urefs->sc!=crefs->sc ||
		urefs->transform[0]!=crefs->transform[0] ||
		urefs->transform[1]!=crefs->transform[1] ||
		urefs->transform[2]!=crefs->transform[2] ||
		urefs->transform[3]!=crefs->transform[3] ||
		urefs->transform[4]!=crefs->transform[4] ||
		urefs->transform[5]!=crefs->transform[5] )
return( false );
	urefs = urefs->next;
	crefs = crefs->next;
    }
    if ( urefs==NULL && crefs==NULL )
return( true );

return( false );
}

static int ImagesMatch(ImageList *uimgs,ImageList *cimgs) {
    /* I assume they are in the same order */
    while ( uimgs!=NULL && cimgs!=NULL ) {
	if ( uimgs->image!=cimgs->image ||
		uimgs->xoff!=cimgs->xoff ||
		uimgs->yoff!=cimgs->yoff ||
		uimgs->xscale!=cimgs->xscale ||
		uimgs->yscale!=cimgs->yscale  )
return( false );
	uimgs = uimgs->next;
	cimgs = cimgs->next;
    }
    if ( uimgs==NULL && cimgs==NULL )
return( true );

return( false );
}

static RefChar *RefCharInList(RefChar *search,RefChar *list) {
    while ( list!=NULL ) {
	if ( search->sc==list->sc &&
		search->transform[0]==list->transform[0] &&
		search->transform[1]==list->transform[1] &&
		search->transform[2]==list->transform[2] &&
		search->transform[3]==list->transform[3] &&
		search->transform[4]==list->transform[4] &&
		search->transform[5]==list->transform[5] )
return( list );
	list = list->next;
    }
return( NULL );
}

static ImageList *ImageInList(ImageList *search,ImageList *list) {
    while ( list!=NULL ) {
	if ( search->image!=list->image ||
		search->xoff!=list->xoff ||
		search->yoff!=list->yoff ||
		search->xscale!=list->xscale ||
		search->yscale!=list->yscale  )
return( list );
	list = list->next;
    }
return( NULL );
}

static void FixupRefChars(SplineChar *sc,RefChar *urefs,int layer) {
    RefChar *crefs, *cend, *cprev, *unext, *cnext;

    if ( layer==ly_grid )
return;

    crefs = sc->layers[layer].refs;

    cprev = NULL;
    while ( crefs!=NULL && urefs!=NULL ) {
	if ( urefs->sc==crefs->sc &&
		urefs->transform[0]==crefs->transform[0] &&
		urefs->transform[1]==crefs->transform[1] &&
		urefs->transform[2]==crefs->transform[2] &&
		urefs->transform[3]==crefs->transform[3] &&
		urefs->transform[4]==crefs->transform[4] &&
		urefs->transform[5]==crefs->transform[5] ) {
	    unext = urefs->next;
	    crefs->selected = urefs->selected;
	    RefCharFree(urefs);
	    urefs = unext;
	    cprev = crefs;
	    crefs = crefs->next;
	} else if ( (cend = RefCharInList(urefs,crefs->next))!=NULL ) {
	    /* if the undo refchar matches something further down the char's */
	    /*  ref list, then than means we need to delete everything on the */
	    /*  char's list between the two */
	    while ( crefs!=cend ) {
		cnext = crefs->next;
		SCRemoveDependent(sc,crefs,layer);
		crefs = cnext;
	    }
	} else { /* urefs isn't on the list. Add it here */
	    unext = urefs->next;
	    urefs->next = crefs;
	    if ( cprev==NULL )
		sc->layers[layer].refs = urefs;
	    else
		cprev->next = urefs;
	    cprev = urefs;
	    SCReinstanciateRefChar(sc,urefs,layer);
	    SCMakeDependent(sc,urefs->sc);
	    urefs = unext;
	}
    }
    if ( crefs!=NULL ) {
	while ( crefs!=NULL ) {
	    cnext = crefs->next;
	    SCRemoveDependent(sc,crefs,layer);
	    crefs = cnext;
	}
    } else if ( urefs!=NULL ) {
	if ( cprev==NULL )
	    sc->layers[layer].refs = urefs;
	else
	    cprev->next = urefs;
	while ( urefs!=NULL ) {
	    SCReinstanciateRefChar(sc,urefs,layer);
	    SCMakeDependent(sc,urefs->sc);
	    urefs = urefs->next;
	}
    }
}

static int BDFRefCharsMatch(BDFRefChar *urefs,BDFRefChar *crefs) {
    /* I assume they are in the same order */
    while ( urefs!=NULL && crefs!=NULL ) {
	if ( urefs->bdfc != crefs->bdfc ||
		urefs->xoff != crefs->xoff ||
		urefs->yoff != crefs->yoff )
return( false );
	urefs = urefs->next;
	crefs = crefs->next;
    }
    if ( urefs==NULL && crefs==NULL )
return( true );

return( false );
}

static BDFRefChar *BDFRefCharInList( BDFRefChar *search,BDFRefChar *list ) {
    while ( list!=NULL ) {
	if ( search->bdfc == list->bdfc &&
		search->xoff == list->xoff &&
		search->yoff == list->yoff )
return( list );
	list = list->next;
    }
return( NULL );
}

static void FixupBDFRefChars( BDFChar *bc,BDFRefChar *urefs ) {
    BDFRefChar *crefs, *cend, *cprev, *unext, *cnext;

    crefs = bc->refs;
    cprev = NULL;
    while ( crefs!=NULL && urefs!=NULL ) {
	if ( urefs->bdfc == crefs->bdfc &&
		urefs->xoff == crefs->xoff &&
		urefs->yoff == crefs->yoff ) {
	    unext = urefs->next;
	    crefs->selected = urefs->selected;
	    free( urefs );
	    urefs = unext;
	    cprev = crefs;
	    crefs = crefs->next;
	} else if (( cend = BDFRefCharInList( urefs,crefs->next )) != NULL ) {
	    /* if the undo refchar matches something further down the char's */
	    /*  ref list, then that means we need to delete everything on the */
	    /*  char's list between the two */
	    while ( crefs != cend ) {
		cnext = crefs->next;
		BCRemoveDependent( bc,crefs );
		crefs = cnext;
	    }
	} else { /* urefs isn't on the list. Add it here */
	    unext = urefs->next;
	    urefs->next = crefs;
	    if ( cprev==NULL )
		bc->refs = urefs;
	    else
		cprev->next = urefs;
	    cprev = urefs;
	    BCMakeDependent( bc,urefs->bdfc );
	    urefs = unext;
	}
    }
    if ( crefs!=NULL ) {
	while ( crefs!=NULL ) {
	    cnext = crefs->next;
	    BCRemoveDependent( bc,crefs );
	    crefs = cnext;
	}
    } else if ( urefs!=NULL ) {
	if ( cprev==NULL )
	    bc->refs = urefs;
	else
	    cprev->next = urefs;
	while ( urefs!=NULL ) {
	    BCMakeDependent( bc,urefs->bdfc );
	    urefs = urefs->next;
	}
    }
}

static void FixupImages(SplineChar *sc,ImageList *uimgs,int layer) {
    ImageList *cimgs = layer==-1 ? sc->parent->grid.images : sc->layers[layer].images,
	    *cend, *cprev, *unext, *cnext;

    cprev = NULL;
    while ( cimgs!=NULL && uimgs!=NULL ) {
	if ( uimgs->image==cimgs->image &&
		uimgs->xoff==cimgs->xoff &&
		uimgs->yoff==cimgs->yoff &&
		uimgs->xscale==cimgs->xscale &&
		uimgs->yscale==cimgs->yscale  ) {
	    unext = uimgs->next;
	    cimgs->selected = uimgs->selected;
	    free(uimgs);
	    uimgs = unext;
	    cprev = cimgs;
	    cimgs = cimgs->next;
	} else if ( (cend = ImageInList(uimgs,cimgs->next))!=NULL ) {
	    /* if the undo image matches something further down the char's */
	    /*  img list, then than means we need to delete everything on the */
	    /*  char's list between the two */
	    if ( cprev==NULL )
		sc->layers[layer].images = cend;
	    else
		cprev->next = cend;
	    while ( cimgs!=cend ) {
		cnext = cimgs->next;
		free(cimgs);
		cimgs = cnext;
	    }
	} else { /* uimgs isn't on the list. Add it here */
	    unext = uimgs->next;
	    uimgs->next = cimgs;
	    if ( cprev==NULL )
		sc->layers[layer].images = uimgs;
	    else
		cprev->next = uimgs;
	    cprev = uimgs;
	    uimgs = unext;
	}
    }
    if ( cimgs!=NULL ) {
	ImageListsFree(cimgs);
	if ( cprev==NULL )
	    sc->layers[layer].images = NULL;
	else
	    cprev->next = NULL;
    } else if ( uimgs!=NULL ) {
	if ( cprev==NULL )
	    sc->layers[layer].images = uimgs;
	else
	    cprev->next = uimgs;
    }
}

static void UHintListFree(void *hints) {
    StemInfo *h, *t, *p;

    if ( hints==NULL )
return;
    if ( ((StemInfo *) hints)->hinttype==ht_d )
	DStemInfosFree(hints);
    else {
	h = t = hints;
	p = NULL;
	while ( t!=NULL && t->hinttype!=ht_d ) {
	    p = t;
	    t = t->next;
	}
	p->next = NULL;
	StemInfosFree(h);
	DStemInfosFree((DStemInfo *) t);
    }
}

void *UHintCopy(SplineChar *sc,int docopy) {
    StemInfo *h = sc->hstem, *v = sc->vstem, *last=NULL;
    DStemInfo *d = sc->dstem;
    void *ret = NULL;

    if ( docopy ) {
	h = StemInfoCopy(h);
	v = StemInfoCopy(v);
	d = DStemInfoCopy(d);
    } else {
	sc->hstem = NULL;
	sc->vstem = NULL;
	sc->dstem = NULL;
	sc->hconflicts = sc->vconflicts = false;
    }
    ret = h;
    if ( h!=NULL ) {
	h->hinttype = ht_h;
	for ( last=h; last->next!=NULL; last=last->next ) last->next->hinttype = ht_unspecified;
	last->next = v;
    } else
	ret = v;
    if ( v!=NULL ) {
	v->hinttype = ht_v;
	for ( last=v; last->next!=NULL; last=last->next ) last->next->hinttype = ht_unspecified;
    }
    if ( last!=NULL )
	last->next = (StemInfo *) d;
    else
	ret = d;
    if ( d!=NULL ) {
	d->hinttype = ht_d;
	for ( d=d->next; d!=NULL; d=d->next ) d->hinttype = ht_unspecified;
    }
return(ret);
}

void ExtractHints(SplineChar *sc,void *hints,int docopy) {
    StemInfo *h = NULL, *v = NULL, *p;
    DStemInfo *d = NULL;
    StemInfo *pv = NULL, *pd = NULL;

    p = NULL;
    while ( hints!=NULL ) {
	if ( ((StemInfo *) hints)->hinttype == ht_h )
	    h = hints;
	else if ( ((StemInfo *) hints)->hinttype == ht_v ) {
	    v = hints;
	    pv = p;
	} else if ( ((StemInfo *) hints)->hinttype == ht_d ) {
	    d = hints;
	    pd = p;
    break;
	}
	p = hints;
	hints = ((StemInfo *) hints)->next;
    }

    if ( pv!=NULL ) pv->next = NULL;
    if ( pd!=NULL ) pd->next = NULL;
    if ( docopy ) {
	h = StemInfoCopy(h);
	if ( pv!=NULL ) pv->next = v;
	v = StemInfoCopy(v);
	if ( pd!=NULL ) pd->next = (StemInfo *) d;
	d = DStemInfoCopy(d);
    }

    StemInfosFree(sc->hstem);
    StemInfosFree(sc->vstem);
    DStemInfosFree(sc->dstem);
    sc->hstem = h;
    sc->vstem = v;
    sc->dstem = d;
    sc->hconflicts = StemInfoAnyOverlaps(h);
    sc->vconflicts = StemInfoAnyOverlaps(v);
}

void UndoesFreeButRetainFirstN( Undoes** undopp, int retainAmount )
{
    if( !undopp || !*undopp )
        return;
    Undoes* undo = *undopp;
    // wipe them all will change the list header pointer too
    if( !retainAmount ) {
        UndoesFree( undo );
        *undopp = 0;
        return;
    }

    Undoes* undoprev = undo;
    for( ; retainAmount > 0 && undo ; retainAmount-- )
    {
        undoprev = undo;
        undo = undo->next;
    }
    // not enough items to need to do a trim.
    if( retainAmount > 0 )
        return;

    // break off and free the tail
    UndoesFree( undo );
    undoprev->next = 0;
}


void UndoesFree(Undoes *undo) {
    Undoes *unext;
    BDFRefChar *head, *next;

    while ( undo!=NULL ) {
	unext = undo->next;
	switch ( undo->undotype ) {
	  case ut_noop:
	  case ut_width: case ut_vwidth: case ut_lbearing: case ut_rbearing:
	    /* Nothing else to free */;
	  break;
	  case ut_state: case ut_tstate: case ut_statehint: case ut_statename:
	  case ut_hints: case ut_anchors: case ut_statelookup:
	    SplinePointListsFree(undo->u.state.splines);
	    RefCharsFree(undo->u.state.refs);
	    UHintListFree(undo->u.state.hints);
	    free(undo->u.state.instrs);
	    ImageListsFree(undo->u.state.images);
	    if ( undo->undotype==ut_statename ) {
		free( undo->u.state.charname );
		free( undo->u.state.comment );
		PSTFree( undo->u.state.possub );
	    }
	    AnchorPointsFree(undo->u.state.anchor);
	    GradientFree(undo->u.state.fill_brush.gradient);
	    PatternFree(undo->u.state.fill_brush.pattern);
	    GradientFree(undo->u.state.stroke_pen.brush.gradient);
	    PatternFree(undo->u.state.stroke_pen.brush.pattern);
	  break;
	  case ut_bitmap:
	    for ( head=undo->u.bmpstate.refs; head != NULL; ) {
		next = head->next; free( head ); head = next;
	    }
	    free(undo->u.bmpstate.bitmap);
	    BDFFloatFree(undo->u.bmpstate.selection);
	  break;
	  case ut_multiple: case ut_layers:
	    UndoesFree( undo->u.multiple.mult );
	  break;
	  case ut_composit:
	    UndoesFree(undo->u.composit.state);
	    UndoesFree(undo->u.composit.bitmaps);
	  break;
	  default:
	    IError( "Unknown undo type in UndoesFree: %d", undo->undotype );
	  break;
	}
	chunkfree(undo,sizeof(Undoes));
	undo = unext;
    }
}

static Undoes *AddUndo(Undoes *undo,Undoes **uhead,Undoes **rhead) {
    int ucnt;
    Undoes *u, *prev;

    UndoesFree(*rhead);
    *rhead = NULL;
    if ( maxundoes==0 ) maxundoes = 1;		/* Must be at least one or snap to breaks */
    if ( maxundoes>0 ) {
	ucnt = 0;
	prev = NULL;
	for ( u= *uhead; u!=NULL; u=u->next ) {
	    if ( ++ucnt>=maxundoes )
	break;
	    prev = u;
	}
	if ( u!=NULL ) {
	    UndoesFree(u);
	    if ( prev!=NULL )
		prev->next = NULL;
	    else
		*uhead = NULL;
	}
    }
    undo->next = *uhead;
    *uhead = undo;

    return( undo );
}

static Undoes *CVAddUndo(CharViewBase *cv,Undoes *undo) {

    Undoes* ret = AddUndo( undo,
			   &cv->layerheads[cv->drawmode]->undoes,
			   &cv->layerheads[cv->drawmode]->redoes );

    /* Let collab system know that a new undo state */
    /* was pushed now since it last sent a message. */
    collabclient_CVPreserveStateCalled( cv );
    return( ret );
}

int CVLayer(CharViewBase *cv) {
    if ( cv->drawmode==dm_grid )
return( ly_grid );

return( cv->layerheads[cv->drawmode]-cv->sc->layers );
}

Undoes *CVPreserveState(CharViewBase *cv) {
    Undoes *undo;
    int layer = CVLayer(cv);

//    if (!quiet)
//        printf("CVPreserveState() no_windowing_ui:%d maxundoes:%d\n", no_windowing_ui, maxundoes );
    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->layerheads[cv->drawmode]->order2;
    undo->u.state.width = cv->sc->width;
    undo->u.state.vwidth = cv->sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(cv->layerheads[cv->drawmode]->splines);
    undo->u.state.refs = RefCharsCopyState(cv->sc,layer);
    if ( layer==ly_fore ) {
	undo->u.state.anchor = AnchorPointsCopy(cv->sc->anchor);
    }
    undo->u.state.images = ImageListCopy(cv->layerheads[cv->drawmode]->images);
    BrushCopy(&undo->u.state.fill_brush,&cv->layerheads[cv->drawmode]->fill_brush,NULL);
    PenCopy(&undo->u.state.stroke_pen,&cv->layerheads[cv->drawmode]->stroke_pen,NULL);
    undo->u.state.dofill = cv->layerheads[cv->drawmode]->dofill;
    undo->u.state.dostroke = cv->layerheads[cv->drawmode]->dostroke;
    undo->u.state.fillfirst = cv->layerheads[cv->drawmode]->fillfirst;
    undo->layer = cv->drawmode;
//    printf("CVPreserveState() dm:%d layer:%d new undo is at %p\n", cv->drawmode, layer, undo );

    // MIQ: Note, this is the wrong time to call sendRedo as we are
    // currently taking the undo state snapshot, after that the app
    // will modify the local state, and that modification is what we
    // are interested in sending on the wire, not the old undo state.
    // collabclient_sendRedo( cv );

return( CVAddUndo(cv,undo));
}

Undoes *CVPreserveStateHints(CharViewBase *cv) {
    Undoes *undo = CVPreserveState(cv);
    if ( CVLayer(cv)==ly_fore ) {
	undo->undotype = ut_statehint;
	undo->u.state.hints = UHintCopy(cv->sc,true);
	undo->u.state.instrs = (uint8*) copyn((char*) cv->sc->ttf_instrs, cv->sc->ttf_instrs_len);
	undo->u.state.instrs_len = cv->sc->ttf_instrs_len;
    }
return( undo );
}

Undoes *SCPreserveHints(SplineChar *sc,int layer) {
    Undoes *undo;

    if ( layer<0 || layer>=sc->layer_cnt )
        return( NULL );

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);
    if ( !preserve_hint_undoes )
return( NULL );

    undo = chunkalloc(sizeof(Undoes));

    undo->was_modified = sc->changed;
    undo->undotype = ut_hints;
    undo->u.state.hints = UHintCopy(sc,true);
    undo->u.state.instrs = (uint8*) copyn((char *) sc->ttf_instrs, sc->ttf_instrs_len);
    undo->u.state.instrs_len = sc->ttf_instrs_len;
    undo->copied_from = sc->parent;
return( AddUndo(undo,&sc->layers[layer].undoes,&sc->layers[layer].redoes));
}

/* This routine allows for undoes in scripting -- under controlled conditions */
Undoes *_SCPreserveLayer(SplineChar *sc,int layer, int dohints) {
    Undoes *undo;

    if ( maxundoes==0 )
return(NULL);

    if ( layer==ly_grid )
	layer = ly_fore;

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->layer = dm_fore;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->layers[layer].order2;
    undo->u.state.width = sc->width;
    undo->u.state.vwidth = sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(sc->layers[layer].splines);
    undo->u.state.refs = RefCharsCopyState(sc,layer);
    if ( layer==ly_fore ) {
	undo->u.state.anchor = AnchorPointsCopy(sc->anchor);
    }
    if ( dohints ) {
	undo->undotype = ut_statehint;
	undo->u.state.hints = UHintCopy(sc,true);
	undo->u.state.instrs = (uint8 *) copyn((char *) sc->ttf_instrs, sc->ttf_instrs_len);
	undo->u.state.instrs_len = sc->ttf_instrs_len;
	if ( dohints==2 ) {
	    undo->undotype = ut_statename;
	    undo->u.state.unicodeenc = sc->unicodeenc;
	    undo->u.state.charname = copy(sc->name);
	    undo->u.state.comment = copy(sc->comment);
	    undo->u.state.possub = PSTCopy(sc->possub,sc,NULL);
	}
    }
    undo->u.state.images = ImageListCopy(sc->layers[layer].images);
    BrushCopy(&undo->u.state.fill_brush,&sc->layers[layer].fill_brush,NULL);
    PenCopy(&undo->u.state.stroke_pen,&sc->layers[layer].stroke_pen,NULL);
    undo->u.state.dofill = sc->layers[layer].dofill;
    undo->u.state.dostroke = sc->layers[layer].dostroke;
    undo->u.state.fillfirst = sc->layers[layer].fillfirst;
    undo->copied_from = sc->parent;
return( AddUndo(undo,&sc->layers[layer].undoes,&sc->layers[layer].redoes));
}

/* This routine does not allow for undoes in scripting */
Undoes *SCPreserveLayer(SplineChar *sc,int layer, int dohints) {

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);
return( _SCPreserveLayer(sc,layer,dohints));
}


Undoes *SCPreserveState(SplineChar *sc,int dohints) {
    int i;

    if ( sc->parent->multilayer )
	for ( i=ly_fore+1; i<sc->layer_cnt; ++i )
	    SCPreserveLayer(sc,i,false);

    Undoes* ret = SCPreserveLayer( sc, ly_fore, dohints );
    collabclient_SCPreserveStateCalled( sc );
    return( ret );
}

Undoes *SCPreserveBackground(SplineChar *sc) {
return( SCPreserveLayer(sc,ly_back,false));
}

Undoes *_SFPreserveGuide(SplineFont *sf) {
    Undoes *undo;

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->was_modified = sf->changed;
    undo->was_order2 = sf->grid.order2;
    undo->u.state.splines = SplinePointListCopy(sf->grid.splines);
    undo->u.state.images = ImageListCopy(sf->grid.images);
    undo->u.state.fill_brush = sf->grid.fill_brush;
    undo->u.state.stroke_pen = sf->grid.stroke_pen;
    undo->u.state.dofill = sf->grid.dofill;
    undo->u.state.dostroke = sf->grid.dostroke;
    undo->u.state.fillfirst = sf->grid.fillfirst;
    undo->copied_from = sf;
return( AddUndo(undo,&sf->grid.undoes,&sf->grid.redoes));
}

Undoes *SFPreserveGuide(SplineFont *sf) {
    if ( no_windowing_ui || maxundoes==0 )             /* No use for undoes in scripting */
return(NULL);
return( _SFPreserveGuide(sf) );
}

void SCUndoSetLBearingChange(SplineChar *sc,int lbc) {
    Undoes *undo = sc->layers[ly_fore].undoes;

    if ( undo==NULL || undo->undotype != ut_state )
return;
    undo->u.state.lbearingchange = lbc;
}

Undoes *_CVPreserveTState(CharViewBase *cv,PressedOn *p) {
    Undoes *undo;
    RefChar *refs, *urefs;
    int was0 = false, j;

    if ( maxundoes==0 ) {
	was0 = true;
	maxundoes = 1;
    }

    undo = CVPreserveState(cv);
    if ( !p->transany || p->transanyrefs ) {
	for ( refs = cv->layerheads[cv->drawmode]->refs, urefs=undo->u.state.refs; urefs!=NULL; refs=refs->next, urefs=urefs->next )
	    if ( !p->transany || refs->selected )
		for ( j=0; j<urefs->layer_cnt; ++j )
		    urefs->layers[j].splines = SplinePointListCopy(refs->layers[j].splines);
    }
    undo->undotype = ut_tstate;

    if ( was0 )
	maxundoes = 0;

return( undo );
}

Undoes *CVPreserveWidth(CharViewBase *cv,int width) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->layerheads[cv->drawmode]->order2;
    undo->u.width = width;
return( CVAddUndo(cv,undo));
}

Undoes *CVPreserveVWidth(CharViewBase *cv,int vwidth) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->layerheads[cv->drawmode]->order2;
    undo->u.width = vwidth;
return( CVAddUndo(cv,undo));
}

Undoes *SCPreserveWidth(SplineChar *sc) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->layers[ly_fore].order2;
    undo->u.state.width = sc->width;
    undo->layer = dm_fore;

    Undoes* ret = AddUndo(undo,&sc->layers[ly_fore].undoes,&sc->layers[ly_fore].redoes);

    /* Let collab system know that a new undo state */
    /* was pushed now since it last sent a message. */
    collabclient_SCPreserveStateCalled( sc );
    return(ret);
}

Undoes *SCPreserveVWidth(SplineChar *sc) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->layers[ly_fore].order2;
    undo->u.state.width = sc->vwidth;
return( AddUndo(undo,&sc->layers[ly_fore].undoes,&sc->layers[ly_fore].redoes));
}

Undoes *BCPreserveState( BDFChar *bc ) {
    Undoes *undo;
    BDFRefChar *head, *ref, *prev = NULL;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_bitmap;
    undo->u.bmpstate.width = bc->width;
    undo->u.bmpstate.xmin = bc->xmin;
    undo->u.bmpstate.xmax = bc->xmax;
    undo->u.bmpstate.ymin = bc->ymin;
    undo->u.bmpstate.ymax = bc->ymax;
    undo->u.bmpstate.bytes_per_line = bc->bytes_per_line;
    undo->u.bmpstate.bitmap = bmpcopy(bc->bitmap,bc->bytes_per_line,
	    bc->ymax-bc->ymin+1);
    undo->u.bmpstate.selection = BDFFloatCopy(bc->selection);
    for ( head=bc->refs; head!=NULL; head=head->next ) {
	ref = calloc( 1,sizeof( BDFRefChar ));
	memcpy( ref,head,sizeof( BDFRefChar ));
	if ( prev == NULL )
	    undo->u.bmpstate.refs = ref;
	else
	    prev->next = ref;
	prev = ref;
    }
return( AddUndo(undo,&bc->undoes,&bc->redoes));
}

static void SCUndoAct(SplineChar *sc,int layer, Undoes *undo) {

    switch ( undo->undotype ) {
      case ut_noop:
      break;
      case ut_width: {
	int width = sc->width;
	if ( sc->width!=undo->u.width )
	    SCSynchronizeWidth(sc,undo->u.width,width,NULL);
	undo->u.width = width;
      } break;
      case ut_vwidth: {
	int vwidth = sc->vwidth;
	sc->vwidth = undo->u.width;
	undo->u.width = vwidth;
      } break;
      case ut_hints: {
	void *hints = UHintCopy(sc,false);
	uint8 *instrs = sc->ttf_instrs;
	int instrs_len = sc->ttf_instrs_len;
	ExtractHints(sc,undo->u.state.hints,false);
	sc->ttf_instrs = undo->u.state.instrs;
	sc->ttf_instrs_len = undo->u.state.instrs_len;
	undo->u.state.hints = hints;
	undo->u.state.instrs = instrs;
	undo->u.state.instrs_len = instrs_len;
	SCOutOfDateBackground(sc);
      } break;
      case ut_state: case ut_tstate: case ut_statehint: case ut_statename: {
	Layer *head = layer==ly_grid ? &sc->parent->grid : &sc->layers[layer];
	SplinePointList *spl = head->splines;

	printf("SCUndoAct() ut_state case, layer:%d sc:%p scn:%s scw:%d uw:%d\n",
	       layer, sc, sc->name, sc->width, undo->u.state.width );
	
	if ( layer==ly_fore ) {
	    int width = sc->width;
	    int vwidth = sc->vwidth;
	    if ( sc->width!=undo->u.state.width )
	    {
		printf("SCUndoAct() sc:%p scn:%s scw:%d uw:%d\n",
		       sc, sc->name, sc->width, undo->u.state.width );
		SCSynchronizeWidth(sc,undo->u.state.width,width,NULL);
	    }
	    sc->vwidth = undo->u.state.vwidth;
	    undo->u.state.width = width;
	    undo->u.state.vwidth = vwidth;
	}
	head->splines = undo->u.state.splines;
	if ( layer==ly_fore ) {
	    AnchorPoint *ap = sc->anchor;
	    sc->anchor = undo->u.state.anchor;
	    undo->u.state.anchor = ap;
	}
	if ( layer!=ly_grid && !RefCharsMatch(undo->u.state.refs,head->refs)) {
	    RefChar *refs = RefCharsCopyState(sc,layer);
	    FixupRefChars(sc,undo->u.state.refs,layer);
	    undo->u.state.refs = refs;
	}
	if ( layer==ly_fore &&
		(undo->undotype==ut_statehint || undo->undotype==ut_statename)) {
	    void *hints = UHintCopy(sc,false);
	    uint8 *instrs = sc->ttf_instrs;
	    int instrs_len = sc->ttf_instrs_len;
	    ExtractHints(sc,undo->u.state.hints,false);
	    sc->ttf_instrs = undo->u.state.instrs;
	    sc->ttf_instrs_len = undo->u.state.instrs_len;
	    undo->u.state.hints = hints;
	    undo->u.state.instrs = instrs;
	    undo->u.state.instrs_len = instrs_len;
	}
	if ( !ImagesMatch(undo->u.state.images,head->images)) {
	    ImageList *images = ImageListCopy(head->images);
	    FixupImages(sc,undo->u.state.images,layer);
	    undo->u.state.images = images;
	    SCOutOfDateBackground(sc);
	}
	undo->u.state.splines = spl;
	if ( undo->u.state.lbearingchange ) {
	    undo->u.state.lbearingchange = -undo->u.state.lbearingchange;
	    SCSynchronizeLBearing(sc,undo->u.state.lbearingchange,layer);
	}
	if ( layer==ly_fore && undo->undotype==ut_statename ) {
	    char *temp = sc->name;
	    int uni = sc->unicodeenc;
	    PST *possub = sc->possub;
	    char *comment = sc->comment;
	    sc->name = copy(undo->u.state.charname);
	    undo->u.state.charname = temp;
	    sc->unicodeenc = undo->u.state.unicodeenc;
	    undo->u.state.unicodeenc = uni;
	    sc->possub = undo->u.state.possub;
	    undo->u.state.possub = possub;
	    sc->comment = undo->u.state.comment;
	    undo->u.state.comment = comment;
	}
      } break;
      default:
	IError( "Unknown undo type in SCUndoAct: %d", undo->undotype );
      break;
    }
}

void CVDoUndo(CharViewBase *cv) {
    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;

    printf("CVDoUndo() undo:%p u->next:%p\n", undo, ( undo ? undo->next : 0 ) );
    if ( undo==NULL )		/* Shouldn't happen */
	return;

    cv->layerheads[cv->drawmode]->undoes = undo->next;
    undo->next = NULL;

    SCUndoAct(cv->sc,CVLayer(cv),undo);
    undo->next = cv->layerheads[cv->drawmode]->redoes;
    cv->layerheads[cv->drawmode]->redoes = undo;

    if ( !collabclient_generatingUndoForWire(cv) ) {
	_CVCharChangedUpdate(cv,undo->was_modified);
    }
}

void CVDoRedo(CharViewBase *cv) {
    Undoes *undo = cv->layerheads[cv->drawmode]->redoes;

    if ( undo==NULL )		/* Shouldn't happen */
	return;
    cv->layerheads[cv->drawmode]->redoes = undo->next;
    undo->next = NULL;

    SCUndoAct(cv->sc,CVLayer(cv),undo);
    undo->next = cv->layerheads[cv->drawmode]->undoes;
    cv->layerheads[cv->drawmode]->undoes = undo;
    CVCharChangedUpdate(cv);
}

void SCDoUndo(SplineChar *sc,int layer) {
    Undoes *undo = sc->layers[layer].undoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    sc->layers[layer].undoes = undo->next;
    undo->next = NULL;
    SCUndoAct(sc,layer,undo);
    undo->next = sc->layers[layer].redoes;
    sc->layers[layer].redoes = undo;
    _SCCharChangedUpdate(sc,layer,undo->was_modified);
return;
}

void SCDoRedo(SplineChar *sc, int layer) {
    Undoes *undo = sc->layers[layer].redoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    sc->layers[layer].redoes = undo->next;
    undo->next = NULL;
    SCUndoAct(sc,layer,undo);
    undo->next = sc->layers[layer].undoes;
    sc->layers[layer].undoes = undo;
    SCCharChangedUpdate(sc,layer);
return;
}

/* Used when doing incremental transformations. If I just keep doing increments*/
/*  then rounding errors will mount. Instead I go back to the original state */
/*  each time */
void _CVRestoreTOriginalState(CharViewBase *cv,PressedOn *p) {
    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;
    RefChar *ref, *uref;
    ImageList *img, *uimg;
    int j;

    SplinePointListSet(cv->layerheads[cv->drawmode]->splines,undo->u.state.splines);
    if ( !p->anysel || p->transanyrefs ) {
	for ( ref=cv->layerheads[cv->drawmode]->refs, uref=undo->u.state.refs; uref!=NULL; ref=ref->next, uref=uref->next )
	    for ( j=0; j<uref->layer_cnt; ++j )
		if ( uref->layers[j].splines!=NULL ) {
		    SplinePointListSet(ref->layers[j].splines,uref->layers[j].splines);
		    memcpy(&ref->transform,&uref->transform,sizeof(ref->transform));
		}
    }
    for ( img=cv->layerheads[cv->drawmode]->images, uimg=undo->u.state.images; uimg!=NULL;
	    img = img->next, uimg = uimg->next ) {
	img->xoff = uimg->xoff;
	img->yoff = uimg->yoff;
	img->xscale = uimg->xscale;
	img->yscale = uimg->yscale;
    }
}

void _CVUndoCleanup(CharViewBase *cv,PressedOn *p) {
    Undoes * undo = cv->layerheads[cv->drawmode]->undoes;
    RefChar *uref;

    if ( !p->anysel || p->transanyrefs ) {
	for ( uref=undo->u.state.refs; uref!=NULL; uref=uref->next ) {
	    int i;
	    for ( i=0; i<uref->layer_cnt; ++i ) {
		SplinePointListsFree(uref->layers[i].splines);
		GradientFree(uref->layers[i].fill_brush.gradient);
		PatternFree(uref->layers[i].fill_brush.pattern);
		GradientFree(uref->layers[i].stroke_pen.brush.gradient);
		PatternFree(uref->layers[i].stroke_pen.brush.pattern);
	    }
	    free(uref->layers);
	    uref->layers = NULL;
	    uref->layer_cnt = 0;
	}
    }
    undo->undotype = ut_state;
}

void CVRemoveTopUndo(CharViewBase *cv) {
    Undoes * undo = cv->layerheads[cv->drawmode]->undoes;

    cv->layerheads[cv->drawmode]->undoes = undo->next;
    undo->next = NULL;
    UndoesFree(undo);
}

static void BCUndoAct(BDFChar *bc,Undoes *undo) {
    uint8 *b;
    int temp;
    BDFFloat *sel;
    BDFRefChar *ref, *head, *prev = NULL, *uhead = NULL;

    switch ( undo->undotype ) {
      case ut_bitmap: {
	temp = bc->width; bc->width = undo->u.bmpstate.width; undo->u.bmpstate.width = temp;
	temp = bc->xmin; bc->xmin = undo->u.bmpstate.xmin; undo->u.bmpstate.xmin = temp;
	temp = bc->xmax; bc->xmax = undo->u.bmpstate.xmax; undo->u.bmpstate.xmax = temp;
	temp = bc->ymin; bc->ymin = undo->u.bmpstate.ymin; undo->u.bmpstate.ymin = temp;
	temp = bc->ymax; bc->ymax = undo->u.bmpstate.ymax; undo->u.bmpstate.ymax = temp;
	temp = bc->bytes_per_line; bc->bytes_per_line = undo->u.bmpstate.bytes_per_line; undo->u.bmpstate.bytes_per_line = temp;
	b = bc->bitmap; bc->bitmap = undo->u.bmpstate.bitmap; undo->u.bmpstate.bitmap = b;
	sel = bc->selection; bc->selection = undo->u.bmpstate.selection; undo->u.bmpstate.selection = sel;

	if ( !BDFRefCharsMatch( undo->u.bmpstate.refs,bc->refs )) {
	    for ( head=bc->refs; head!=NULL; head=head->next ) {
		ref = calloc( 1,sizeof( BDFRefChar ));
		memcpy( ref,head,sizeof( BDFRefChar ));
		if ( prev != NULL )
		    prev->next = ref;
		else
		    uhead = ref;
		prev = ref;
	    }
	    FixupBDFRefChars( bc,undo->u.bmpstate.refs );
	    undo->u.bmpstate.refs = uhead;
	}
      } break;
      default:
	IError( "Unknown undo type in BCUndoAct: %d", undo->undotype );
      break;
    }
}

void BCDoUndo(BDFChar *bc) {
    Undoes *undo = bc->undoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    bc->undoes = undo->next;
    undo->next = NULL;
    BCUndoAct(bc,undo);
    undo->next = bc->redoes;
    bc->redoes = undo;
    BCCharChangedUpdate(bc);
return;
}

void BCDoRedo(BDFChar *bc) {
    Undoes *undo = bc->redoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    bc->redoes = undo->next;
    undo->next = NULL;
    BCUndoAct(bc,undo);
    undo->next = bc->undoes;
    bc->undoes = undo;
    BCCharChangedUpdate(bc);
return;
}

/* **************************** Cut, Copy & Paste *************************** */

static Undoes copybuffer;

void CopyBufferFree(void) {
    BDFRefChar *brhead, *brnext;

    switch( copybuffer.undotype ) {
      case ut_hints:
	UHintListFree(copybuffer.u.state.hints);
	free(copybuffer.u.state.instrs);
      break;
      case ut_state: case ut_statehint: case ut_anchors: case ut_statelookup:
	SplinePointListsFree(copybuffer.u.state.splines);
	RefCharsFree(copybuffer.u.state.refs);
	AnchorPointsFree(copybuffer.u.state.anchor);
	UHintListFree(copybuffer.u.state.hints);
	free(copybuffer.u.state.instrs);
	ImageListsFree(copybuffer.u.state.images);
	GradientFree(copybuffer.u.state.fill_brush.gradient);
	PatternFree(copybuffer.u.state.fill_brush.pattern);
	GradientFree(copybuffer.u.state.stroke_pen.brush.gradient);
	PatternFree(copybuffer.u.state.stroke_pen.brush.pattern);
      break;
      case ut_bitmapsel:
	BDFFloatFree(copybuffer.u.bmpstate.selection);
      break;
      case ut_bitmap:
	for ( brhead=copybuffer.u.bmpstate.refs; brhead!=NULL; brhead = brnext ) {
	    brnext = brhead->next;
	    free( brhead );
	}
        free( copybuffer.u.bmpstate.bitmap );
      break;
      case ut_multiple: case ut_layers:
	UndoesFree( copybuffer.u.multiple.mult );
      break;
      case ut_composit:
	UndoesFree( copybuffer.u.composit.state );
	UndoesFree( copybuffer.u.composit.bitmaps );
      break;
      default:
      break;
    }
    memset(&copybuffer,'\0',sizeof(copybuffer));
    copybuffer.undotype = ut_none;
}

static void CopyBufferFreeGrab(void) {
    CopyBufferFree();
    if ( FontViewFirst()!=NULL && !no_windowing_ui && export_clipboard )
	ClipboardGrab();
}

static void noop(void *UNUSED(_copybuffer)) {
}

static void *copybufferPt2str(void *UNUSED(_copybuffer),int32 *len) {
    Undoes *cur = &copybuffer;
    SplinePoint *sp;
    char buffer[100];

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    cur = cur->u.multiple.mult;
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_state: case ut_statehint: case ut_statename: case ut_statelookup:
    goto out;
	  default:
	    cur = NULL;
	  break;
	}
    }
    out:
    if ( cur==NULL || FontViewFirst()==NULL ||
	    cur->u.state.splines==NULL || cur->u.state.refs!=NULL ||
	    cur->u.state.splines->next!=NULL ||
	    cur->u.state.splines->first->next!=NULL ) {
	*len=0;
return( copy(""));
    }

    sp = cur->u.state.splines->first;
    sprintf(buffer,"(%g%s%g)", (double) sp->me.x, coord_sep, (double) sp->me.y );
    *len = strlen(buffer);
return( copy(buffer));
}

static void *copybufferName2str(void *UNUSED(_copybuffer),int32 *len) {
    Undoes *cur = &copybuffer;

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    cur = cur->u.multiple.mult;
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_statename:
    goto out;
	  default:
	    cur = NULL;
	  break;
	}
    }
    out:
    if ( cur==NULL || FontViewFirst()==NULL || cur->u.state.charname==NULL ) {
	*len=0;
return( copy(""));
    }
    *len = strlen(cur->u.state.charname);
return( copy( cur->u.state.charname ));
}

static RefChar *XCopyInstanciateRefs(RefChar *refs,SplineChar *container,int layer) {
    /* References in the copybuffer don't include the translated splines */
    RefChar *head=NULL, *last, *cur;

    while ( refs!=NULL ) {
	cur = RefCharCreate();
	free(cur->layers);
	*cur = *refs;
	cur->layers = NULL;
	cur->layer_cnt = 0;
	cur->next = NULL;
	SCReinstanciateRefChar(container,cur,layer);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	refs = refs->next;
    }
return( head );
}

static int FFClipToSC(SplineChar *dummy,Undoes *cur) {
    int lcnt;

    if ( cur==NULL )
return( false );

    dummy->name = "dummy";
    if ( cur->undotype!=ut_layers )
	dummy->parent = cur->copied_from;
    else if ( cur->u.multiple.mult!=NULL && cur->u.multiple.mult->undotype == ut_state )
	dummy->parent = cur->u.multiple.mult->copied_from;
    if ( dummy->parent==NULL )
	dummy->parent = FontViewFirst()->sf;		/* Might not be right, but we need something */
    dummy->width = cur->u.state.width;
    if ( cur->undotype==ut_layers ) {
	Undoes *ulayer;
	for ( ulayer = cur->u.multiple.mult, lcnt=0; ulayer!=NULL; ulayer=ulayer->next, ++lcnt);
	dummy->layer_cnt = lcnt+1;
	if ( lcnt!=1 )
	    dummy->layers = calloc((lcnt+1),sizeof(Layer));
	for ( ulayer = cur->u.multiple.mult, lcnt=1; ulayer!=NULL; ulayer=ulayer->next, ++lcnt) {
	    if ( ulayer->undotype==ut_state || ulayer->undotype==ut_statehint ) {
		dummy->layers[lcnt].fill_brush = ulayer->u.state.fill_brush;
		dummy->layers[lcnt].stroke_pen = ulayer->u.state.stroke_pen;
		dummy->layers[lcnt].dofill = ulayer->u.state.dofill;
		dummy->layers[lcnt].dostroke = ulayer->u.state.dostroke;
		dummy->layers[lcnt].splines = ulayer->u.state.splines;
		dummy->layers[lcnt].refs = XCopyInstanciateRefs(ulayer->u.state.refs,dummy,ly_fore);
	    }
	}
    } else
    {
	dummy->layers[ly_fore].fill_brush = cur->u.state.fill_brush;
	dummy->layers[ly_fore].stroke_pen = cur->u.state.stroke_pen;
	dummy->layers[ly_fore].dofill = cur->u.state.dofill;
	dummy->layers[ly_fore].dostroke = cur->u.state.dostroke;
	dummy->layers[ly_fore].splines = cur->u.state.splines;
	dummy->layers[ly_fore].refs = XCopyInstanciateRefs(cur->u.state.refs,dummy,ly_fore);
    }
return( true );
}

static void *copybuffer2svg(void *UNUSED(_copybuffer),int32 *len) {
    Undoes *cur = &copybuffer;
    SplineChar dummy;
    static Layer layers[2];
    FILE *svg;
    char *ret;
    int old_order2;
    int lcnt;

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    cur = cur->u.multiple.mult;		/* will only handle one char in an "svg" file */
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_state: case ut_statehint: case ut_layers: case ut_statelookup:
    goto out;
	  default:
	    cur = NULL;
	  break;
	}
    }
    out:
    if ( FontViewFirst()==NULL || cur==NULL ) {
	*len=0;
return( copy(""));
    }

    svg = tmpfile();
    if ( svg==NULL ) {
	*len=0;
return( copy(""));
    }

    memset(&dummy,0,sizeof(dummy));
    dummy.layers = layers;
    dummy.layer_cnt = 2;

    if ( !FFClipToSC(&dummy,cur) ) {
	fclose(svg);
	*len=0;
return( copy(""));
    }

    old_order2 = dummy.parent->layers[ly_fore].order2;
    dummy.parent->layers[ly_fore].order2 = cur->was_order2;
    dummy.layers[ly_fore].order2 = cur->was_order2;
    _ExportSVG(svg,&dummy,ly_fore);
    dummy.parent->layers[ly_fore].order2 = old_order2;

    for ( lcnt = ly_fore; lcnt<dummy.layer_cnt; ++lcnt )
	RefCharsFree(dummy.layers[lcnt].refs);
    if ( dummy.layer_cnt!=2 && dummy.layers != layers )
	free( dummy.layers );

    fseek(svg,0,SEEK_END);
    *len = ftell(svg);
    ret = malloc(*len);
    rewind(svg);
    fread(ret,1,*len,svg);
    fclose(svg);
return( ret );
}

/* When a selection contains multiple glyphs, save them into an svg font */
static void *copybuffer2svgmult(void *UNUSED(_copybuffer),int32 *len) {
    Undoes *cur = &copybuffer, *c, *c2;
    SplineFont *sf;
    int cnt,i;
    char *ret;
    Layer *ly;
    SplineChar *sc=NULL;
    int old_order2, o2=false;
    FILE *svg;

    if ( cur->undotype!=ut_multiple || !CopyContainsVectors() || FontViewFirst()==NULL ) {
	*len=0;
return( copy(""));
    }

    svg = tmpfile();
    if ( svg==NULL ) {
	*len=0;
return( copy(""));
    }

    cur = cur->u.multiple.mult;
    for ( cnt=0, c=cur; c!=NULL; c=c->next, ++cnt );

    sf = SplineFontBlank(cnt);
    sf->glyphcnt = cnt;
    for ( i=0, c=cur; c!=NULL; c=c->next, ++i ) {
	sf->glyphs[i] = sc = SFSplineCharCreate(sf);
	sc->orig_pos = i;
	ly = sc->layers;
	if ( (c2 = c)->undotype==ut_composit )
	    c2 = c2->u.composit.state;
	FFClipToSC(sc,c2);
	if ( ly!=sc->layers )
	    free(ly);
	o2 = c2->was_order2;
    }

    if ( sc!=NULL ) {
	old_order2 = sc->parent->layers[ly_fore].order2;
	sc->parent->layers[ly_fore].order2 = o2;
	sc->layers[ly_fore].order2 = o2;
	sf->ascent = sc->parent->ascent;
	sf->descent = sc->parent->descent;
    }
    _WriteSVGFont(svg,sf,0,NULL,ly_fore);
    if ( sc!=NULL )
	sc->parent->layers[ly_fore].order2 = old_order2;

    for ( i=0, c=cur; c!=NULL; c=c->next, ++i ) {
	sc = sf->glyphs[i];
	sc->layers[ly_fore].splines=NULL;
	sc->layers[ly_fore].refs=NULL;
	sc->name = NULL;
    }
    SplineFontFree(sf);

    fseek(svg,0,SEEK_END);
    *len = ftell(svg);
    ret = malloc(*len);
    rewind(svg);
    fread(ret,1,*len,svg);
    fclose(svg);
return( ret );
}

static void *copybuffer2eps(void *UNUSED(_copybuffer),int32 *len) {
    Undoes *cur = &copybuffer;
    SplineChar dummy;
    static Layer layers[2];
    FILE *eps;
    char *ret;
    int old_order2;
    int lcnt;

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    cur = cur->u.multiple.mult;		/* Can only handle one char in an "eps" file */
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_state: case ut_statehint: case ut_layers: case ut_statelookup:
    goto out;
	  default:
	    cur = NULL;
	  break;
	}
    }
    out:
    if ( cur==NULL || FontViewFirst()==NULL ) {
	*len=0;
return( copy(""));
    }

    memset(&dummy,0,sizeof(dummy));
    dummy.layers = layers;
    dummy.layer_cnt = 2;
    dummy.name = "dummy";
    if ( cur->undotype!=ut_layers )
	dummy.parent = cur->copied_from;
    else if ( cur->u.multiple.mult!=NULL && cur->u.multiple.mult->undotype == ut_state )
	dummy.parent = cur->u.multiple.mult->copied_from;
    if ( dummy.parent==NULL )
	dummy.parent = FontViewFirst()->sf;		/* Might not be right, but we need something */
    if ( cur->undotype==ut_layers ) {
	Undoes *ulayer;
	for ( ulayer = cur->u.multiple.mult, lcnt=0; ulayer!=NULL; ulayer=ulayer->next, ++lcnt);
	dummy.layer_cnt = lcnt+1;
	if ( lcnt!=1 )
	    dummy.layers = calloc((lcnt+1),sizeof(Layer));
	for ( ulayer = cur->u.multiple.mult, lcnt=1; ulayer!=NULL; ulayer=ulayer->next, ++lcnt) {
	    if ( ulayer->undotype==ut_state || ulayer->undotype==ut_statehint ) {
		dummy.layers[lcnt].fill_brush = ulayer->u.state.fill_brush;
		dummy.layers[lcnt].stroke_pen = ulayer->u.state.stroke_pen;
		dummy.layers[lcnt].dofill = ulayer->u.state.dofill;
		dummy.layers[lcnt].dostroke = ulayer->u.state.dostroke;
		dummy.layers[lcnt].splines = ulayer->u.state.splines;
		dummy.layers[lcnt].refs = XCopyInstanciateRefs(ulayer->u.state.refs,&dummy,ly_fore);
	    }
	}
    } else {
	dummy.layers[ly_fore].fill_brush = cur->u.state.fill_brush;
	dummy.layers[ly_fore].stroke_pen = cur->u.state.stroke_pen;
	dummy.layers[ly_fore].dofill = cur->u.state.dofill;
	dummy.layers[ly_fore].dostroke = cur->u.state.dostroke;
	dummy.layers[ly_fore].splines = cur->u.state.splines;
	dummy.layers[ly_fore].refs = XCopyInstanciateRefs(cur->u.state.refs,&dummy,ly_fore);
    }

    eps = tmpfile();
    if ( eps==NULL ) {
	*len=0;
return( copy(""));
    }

    old_order2 = dummy.parent->layers[ly_fore].order2;
    dummy.parent->layers[ly_fore].order2 = cur->was_order2;
    dummy.layers[ly_fore].order2 = cur->was_order2;
    /* Don't bother to generate a preview here, that can take too long and */
    /*  cause the paster to time out */
    _ExportEPS(eps,&dummy,false,ly_fore);
    dummy.parent->layers[ly_fore].order2 = old_order2;

    for ( lcnt = ly_fore; lcnt<dummy.layer_cnt; ++lcnt )
	RefCharsFree(dummy.layers[lcnt].refs);
    if ( dummy.layer_cnt!=2 )
	free( dummy.layers );

    fseek(eps,0,SEEK_END);
    *len = ftell(eps);
    ret = malloc(*len);
    rewind(eps);
    fread(ret,1,*len,eps);
    fclose(eps);
return( ret );
}

static void XClipCheckEps(void) {
    Undoes *cur = &copybuffer;

    if ( FontViewFirst()==NULL )
return;
    if ( no_windowing_ui )
return;

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    if ( CopyContainsVectors())
		ClipboardAddDataType("application/x-font-svg",&copybuffer,0,sizeof(char),
			copybuffer2svgmult,noop);
	    cur = cur->u.multiple.mult;
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_state: case ut_statehint: case ut_statename: case ut_layers:
	    ClipboardAddDataType("image/eps",&copybuffer,0,sizeof(char),
		    copybuffer2eps,noop);
	    ClipboardAddDataType("image/svg+xml",&copybuffer,0,sizeof(char),
		    copybuffer2svg,noop);
	    ClipboardAddDataType("image/svg",&copybuffer,0,sizeof(char),
		    copybuffer2svg,noop);
	    /* If the selection is one point, then export the coordinates as a string */
	    if ( cur->u.state.splines!=NULL && cur->u.state.refs==NULL &&
		    cur->u.state.splines->next==NULL &&
		    cur->u.state.splines->first->next==NULL )
		ClipboardAddDataType("STRING",&copybuffer,0,sizeof(char),
			copybufferPt2str,noop);
	    else if ( cur->undotype==ut_statename )
		ClipboardAddDataType("STRING",&copybuffer,0,sizeof(char),
			copybufferName2str,noop);
	    cur = NULL;
	  break;
	  default:
	    cur = NULL;
	  break;
	}
    }
}

void ClipboardClear(void) {
    CopyBufferFree();
}

enum undotype CopyUndoType(void) {
    Undoes *paster;

    paster = &copybuffer;
    while ( paster->undotype==ut_composit || paster->undotype==ut_multiple ) {
	if ( paster->undotype==ut_multiple )
	    paster = paster->u.multiple.mult;
	else if ( paster->u.composit.state==NULL )
return( ut_none );
	else
	    paster = paster->u.composit.state;
    }
return( paster->undotype );
}

int CopyContainsSomething(void) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    if ( cur->undotype==ut_composit )
return( cur->u.composit.state!=NULL );

    if ( cur->undotype==ut_statelookup && cur->copied_from==NULL )
return( false );		/* That is, if the source font has been closed, we can't use this any more */

return( cur->undotype==ut_state || cur->undotype==ut_tstate ||
	cur->undotype==ut_statehint || cur->undotype==ut_statename ||
	cur->undotype==ut_statelookup ||
	cur->undotype==ut_width || cur->undotype==ut_vwidth ||
	cur->undotype==ut_lbearing || cur->undotype==ut_rbearing ||
	cur->undotype==ut_hints ||
	cur->undotype==ut_bitmap || cur->undotype==ut_bitmapsel ||
	cur->undotype==ut_anchors || cur->undotype==ut_noop );
}

int CopyContainsBitmap(void) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    if ( cur->undotype==ut_composit )
return( cur->u.composit.bitmaps!=NULL );

return( cur->undotype==ut_bitmap || cur->undotype==ut_bitmapsel || cur->undotype==ut_noop );
}

int CopyContainsVectors(void) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    if ( cur->undotype==ut_composit )
return( cur->u.composit.state!=NULL );

return( cur->undotype==ut_state || cur->undotype==ut_statehint ||
	cur->undotype==ut_statename || cur->undotype==ut_layers );
}

RefChar *CopyContainsRef(SplineFont *sf) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple ) {
	cur = cur->u.multiple.mult;
	if ( cur->next!=NULL )
return( NULL );
    }
    if ( cur->undotype==ut_composit )
	cur = cur->u.composit.state;
    if ( cur==NULL || (cur->undotype!=ut_state && cur->undotype!=ut_tstate &&
	    cur->undotype!=ut_statehint && cur->undotype!=ut_statename ))
return( NULL );
    if ( cur->u.state.splines!=NULL || cur->u.state.refs==NULL ||
	    cur->u.state.refs->next != NULL )
return( NULL );
    if ( sf!=cur->copied_from )
return( NULL );

return( cur->u.state.refs );
}

const Undoes *CopyBufferGet(void) {
return( &copybuffer );
}

static void _CopyBufferClearCopiedFrom(Undoes *cb, SplineFont *dying) {
    switch ( cb->undotype ) {
      case ut_noop:
      break;
      case ut_state: case ut_statehint: case ut_statename: case ut_statelookup:
	if ( cb->copied_from == dying )
	    cb->copied_from = NULL;
      break;
      case ut_width:
      case ut_vwidth:
      case ut_rbearing:
      case ut_lbearing:
      break;
      case ut_composit:
	if ( cb->copied_from == dying )
	    cb->copied_from = NULL;
	_CopyBufferClearCopiedFrom(cb->u.composit.state,dying);
      break;
      case ut_multiple: case ut_layers:
	if ( cb->copied_from == dying )
	    cb->copied_from = NULL;
	for ( cb=cb->u.multiple.mult; cb!=NULL; cb=cb->next )
	    _CopyBufferClearCopiedFrom(cb,dying);
      break;
      default:
      break;
    }
}

void CopyBufferClearCopiedFrom(SplineFont *dying) {
    _CopyBufferClearCopiedFrom(&copybuffer,dying);
}

int getAdobeEnc(const char *name) {
    int i;

    for ( i=0; i<256; ++i )
	if ( strcmp(name,AdobeStandardEncoding[i])==0 )
    break;
    if ( i==256 ) i = -1;
return( i );
}

void CopyReference(SplineChar *sc) {
    RefChar *ref;

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = sc->layers[ly_fore].order2;	/* Largely irrelevant */
    copybuffer.u.state.width = sc->width;
    copybuffer.u.state.vwidth = sc->vwidth;
    copybuffer.u.state.refs = ref = RefCharCreate();
    copybuffer.copied_from = sc->parent;
    if ( ly_fore<sc->layer_cnt ) {
	BrushCopy(&copybuffer.u.state.fill_brush, &sc->layers[ly_fore].fill_brush,NULL);
	PenCopy(&copybuffer.u.state.stroke_pen, &sc->layers[ly_fore].stroke_pen,NULL);
	copybuffer.u.state.dofill = sc->layers[ly_fore].dofill;
	copybuffer.u.state.dostroke = sc->layers[ly_fore].dostroke;
	copybuffer.u.state.fillfirst = sc->layers[ly_fore].fillfirst;
    }
    ref->unicode_enc = sc->unicodeenc;
    ref->orig_pos = sc->orig_pos;
    ref->adobe_enc = getAdobeEnc(sc->name);
    ref->transform[0] = ref->transform[3] = 1.0;

    XClipCheckEps();
}

void SCCopyLookupData(SplineChar *sc) {
    CopyReference(sc);
    copybuffer.undotype = ut_statelookup;
}

void CopySelected(CharViewBase *cv,int doanchors) {

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = cv->layerheads[cv->drawmode]->order2;
    copybuffer.u.state.width = cv->sc->width;
    copybuffer.u.state.vwidth = cv->sc->vwidth;
    if ( cv->sc->inspiro && hasspiro())
	copybuffer.u.state.splines = SplinePointListCopySpiroSelected(cv->layerheads[cv->drawmode]->splines);
    else
	copybuffer.u.state.splines = SplinePointListCopySelected(cv->layerheads[cv->drawmode]->splines);
    if ( cv->drawmode!=dm_grid ) {
	RefChar *refs, *new;
	for ( refs = cv->layerheads[cv->drawmode]->refs; refs!=NULL; refs = refs->next ) if ( refs->selected ) {
	    new = RefCharCreate();
	    free(new->layers);
	    *new = *refs;
	    new->layers = NULL;
	    new->layer_cnt = 0;
	    new->orig_pos = new->sc->orig_pos;
	    new->sc = NULL;
	    new->next = copybuffer.u.state.refs;
	    copybuffer.u.state.refs = new;
	}
	if ( doanchors ) {
	    AnchorPoint *ap, *new;
	    for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected ) {
		new = chunkalloc(sizeof(AnchorPoint));
		*new = *ap;
		new->next = copybuffer.u.state.anchor;
		copybuffer.u.state.anchor = new;
	    }
	}
    }
    if ( cv->drawmode!=dm_grid && CVLayer(cv)!=ly_fore ) {
	ImageList *imgs, *new;
	for ( imgs = cv->layerheads[cv->drawmode]->images; imgs!=NULL; imgs = imgs->next ) if ( imgs->selected ) {
	    new = chunkalloc(sizeof(ImageList));
	    *new = *imgs;
	    new->next = copybuffer.u.state.images;
	    copybuffer.u.state.images = new;
	}
    }
    if ( cv->drawmode==dm_fore || cv->drawmode==dm_back ) {
	BrushCopy(&copybuffer.u.state.fill_brush, &cv->layerheads[cv->drawmode]->fill_brush,NULL);
	PenCopy(&copybuffer.u.state.stroke_pen, &cv->layerheads[cv->drawmode]->stroke_pen,NULL);
	copybuffer.u.state.dofill = cv->layerheads[cv->drawmode]->dofill;
	copybuffer.u.state.dostroke = cv->layerheads[cv->drawmode]->dostroke;
	copybuffer.u.state.fillfirst = cv->layerheads[cv->drawmode]->fillfirst;
    }
    copybuffer.copied_from = cv->sc->parent;

    XClipCheckEps();
}

void CVCopyGridFit(CharViewBase *cv) {
    SplineChar *sc = cv->sc;

    if ( cv->gridfit==NULL )
return;

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = cv->layerheads[cv->drawmode]->order2;
    copybuffer.u.state.width = cv->ft_gridfitwidth;
    copybuffer.u.state.vwidth = sc->vwidth;
    copybuffer.u.state.splines = SplinePointListCopy(cv->gridfit);
    copybuffer.copied_from = cv->sc->parent;

    XClipCheckEps();
}

static Undoes *SCCopyAllLayer(SplineChar *sc,enum fvcopy_type full,int layer) {
    Undoes *cur;
    RefChar *ref;
    /* If full==ct_fullcopy copy the glyph as is. */
    /* If full==ct_reference put a reference to the glyph in the clipboard */
    /* If full==ct_unlinkrefs copy the glyph, but unlink any references it contains */
    /*	so we end up with no references and a bunch of splines */

    cur = chunkalloc(sizeof(Undoes));
    if ( sc==NULL ) {
	cur->undotype = ut_noop;
    } else {
	cur->was_order2 = sc->layers[ly_fore].order2;
	cur->u.state.width = sc->width;
	cur->u.state.vwidth = sc->vwidth;
	if ( full==ct_fullcopy || full == ct_unlinkrefs ) {
	    cur->undotype = copymetadata ? ut_statename : ut_statehint;
	    cur->u.state.splines = SplinePointListCopy(sc->layers[layer].splines);
	    if ( full==ct_unlinkrefs )
		cur->u.state.splines = RefCharsCopyUnlinked(cur->u.state.splines,sc,layer);
	    else
		cur->u.state.refs = RefCharsCopyState(sc,layer);
	    cur->u.state.anchor = AnchorPointsCopy(sc->anchor);
	    cur->u.state.hints = UHintCopy(sc,true);
	    if ( copyttfinstr ) {
		cur->u.state.instrs = (uint8 *) copyn((char *) sc->ttf_instrs, sc->ttf_instrs_len);
		cur->u.state.instrs_len = sc->ttf_instrs_len;
	    }
	    cur->u.state.unicodeenc = sc->unicodeenc;
	    if ( copymetadata && layer==ly_fore ) {
		cur->u.state.charname = copy(sc->name);
		cur->u.state.comment = copy(sc->comment);
		cur->u.state.possub = PSTCopy(sc->possub,sc,NULL);
	    } else {
		cur->u.state.charname = NULL;
		cur->u.state.comment = NULL;
		cur->u.state.possub = NULL;
	    }
	} else {		/* Or just make a reference */
	    cur->undotype = full==ct_reference ? ut_state : ut_statelookup;
	    cur->u.state.refs = ref = RefCharCreate();
	    ref->unicode_enc = sc->unicodeenc;
	    ref->orig_pos = sc->orig_pos;
	    ref->adobe_enc = getAdobeEnc(sc->name);
	    ref->transform[0] = ref->transform[3] = 1.0;
	}
	if ( layer<sc->layer_cnt ) {
	    cur->u.state.images = ImageListCopy(sc->layers[layer].images);
	    BrushCopy(&cur->u.state.fill_brush, &sc->layers[layer].fill_brush,NULL);
	    PenCopy(&cur->u.state.stroke_pen, &sc->layers[layer].stroke_pen,NULL);
	    cur->u.state.dofill = sc->layers[layer].dofill;
	    cur->u.state.dostroke = sc->layers[layer].dostroke;
	    cur->u.state.fillfirst = sc->layers[layer].fillfirst;
	}
	cur->copied_from = sc->parent;
    }
return( cur );
}

static Undoes *SCCopyAll(SplineChar *sc,int layer, enum fvcopy_type full) {
    Undoes *ret, *cur, *last=NULL;

    if ( sc!=NULL && sc->parent!=NULL && sc->parent->multilayer ) {
	ret = chunkalloc(sizeof(Undoes));
	if ( sc==NULL ) {
	    ret->undotype = ut_noop;
	} else if ( full==ct_reference || full==ct_lookups || !sc->parent->multilayer ) {	/* Make a reference */
	    chunkfree(ret,sizeof(Undoes));
	    ret = SCCopyAllLayer(sc,full,ly_fore );
	} else {
	    ret->undotype = ut_layers;
	    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		cur = SCCopyAllLayer(sc,full,layer);
		if ( ret->u.multiple.mult==NULL )
		    ret->u.multiple.mult = cur;
		else
		    last->next = cur;
		last = cur;
		/* full = ct_reference; 	Hunh? What was I thinking? */
	    }
	}
return( ret );
    } else
return( SCCopyAllLayer(sc,full,layer));
}

void SCCopyWidth(SplineChar *sc,enum undotype ut) {
    DBounds bb;

    CopyBufferFreeGrab();

    copybuffer.undotype = ut;
    copybuffer.copied_from = sc->parent;
    switch ( ut ) {
      case ut_width:
	copybuffer.u.width = sc->width;
      break;
      case ut_vwidth:
	copybuffer.u.width = sc->width;
      break;
      case ut_lbearing:
	SplineCharFindBounds(sc,&bb);
	copybuffer.u.lbearing = bb.minx;
      break;
      case ut_rbearing:
	SplineCharFindBounds(sc,&bb);
	copybuffer.u.rbearing = sc->width-bb.maxx;
      break;
      default:
      break;
    }
}

void CopyWidth(CharViewBase *cv,enum undotype ut) {
    SCCopyWidth(cv->sc,ut);
}

static SplineChar *FindCharacter(SplineFont *into, SplineFont *from,RefChar *rf,
	SplineChar **fromsc) {
    const char *fromname = NULL;

    if ( !SFIsActive(from))
	from = NULL;
    /* A subtler error: If we've closed the "from" font and subsequently */
    /*  opened the "into" font, there is a good chance they have the same */
    /*  address, and we just found ourselves. */
    /* More complicated cases are possible too, where from and into might */
    /*  be different -- but from has still been closed and reopened as something */
    /*  else */
    /* Should be fixed now. We clear copied_from when we close a font */
    if ( from!=NULL && (
	    rf->orig_pos>=from->glyphcnt || from->glyphs[rf->orig_pos]==NULL ||
	    from->glyphs[rf->orig_pos]->unicodeenc!=rf->unicode_enc ))
	from = NULL;

    if ( fromsc!=NULL ) *fromsc = NULL;

    if ( from!=NULL && rf->orig_pos<from->glyphcnt && from->glyphs[rf->orig_pos]!=NULL ) {
	fromname = from->glyphs[rf->orig_pos]->name;
	if ( fromsc!=NULL )
	    *fromsc = from->glyphs[rf->orig_pos];
    }

    if ( rf->orig_pos<into->glyphcnt && into->glyphs[rf->orig_pos]!=NULL &&
	    ((into->glyphs[rf->orig_pos]->unicodeenc == rf->unicode_enc && rf->unicode_enc!=-1 ) ||
	     (rf->unicode_enc==-1 && fromname!=NULL &&
		     strcmp(into->glyphs[rf->orig_pos]->name,fromname)==0 )) )
return( into->glyphs[rf->orig_pos] );

return( SFGetChar( into, rf->unicode_enc, fromname ));
}

int SCDependsOnSC(SplineChar *parent, SplineChar *child) {
    RefChar *ref;

    if ( parent==child )
return( true );
    for ( ref=parent->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( SCDependsOnSC(ref->sc,child))
return( true );
    }
return( false );
}

static int BCRefersToBC( BDFChar *parent, BDFChar *child ) {
    BDFRefChar *head;

    if ( parent==child )
return( true );
    for ( head=child->refs; head!=NULL; head=head->next ) {
	if ( BCRefersToBC( parent,head->bdfc ))
return( true );
    }
return( false );
}

static void PasteNonExistantRefCheck(SplineChar *sc,Undoes *paster,RefChar *ref,
	int *refstate) {
    SplineChar *rsc=NULL, *fromsc;
    SplineSet *new, *spl;
    int yes = 3;

    rsc = FindCharacter(sc->parent,paster->copied_from,ref,&fromsc);
    if ( rsc!=NULL )
	IError("We should never have called PasteNonExistantRefCheck if we had a glyph");
    if ( fromsc==NULL ) {
	if ( !(*refstate&0x4) ) {
	    char *buts[3];
	    char buf[80]; const char *name;
	    if ( ref->unicode_enc==-1 )
		name = "<Unknown>";
	    else
		name = StdGlyphName(buf,ref->unicode_enc,ui_none,(NameList *) -1);
	    buts[0] = _("Don't Warn Again"); buts[1] = _("_OK"); buts[2] = NULL;
	    yes = ff_ask(_("Bad Reference"),(const char **) buts,1,1,_("You are attempting to paste a reference to %1$s into %2$s.\nBut %1$s does not exist in this font, nor can I find the original character referred to.\nIt will not be copied."),name,sc->name);
	    if ( yes==0 )
		*refstate |= 0x4;
	}
    } else {
	if ( !(*refstate&0x3) ) {
	    char *buts[5];
	    buts[0] = _("_Yes");
	    buts[1] = _("Yes to _All");
	    buts[2] = _("No _to All");
	    buts[3] = _("_No");
	    buts[4] = NULL;
	    ff_progress_pause_timer();
	    yes = ff_ask(_("Bad Reference"),(const char **) buts,0,3,_("You are attempting to paste a reference to %1$s into %2$s.\nBut %1$s does not exist in this font.\nWould you like to copy the original splines (or delete the reference)?"),fromsc->name,sc->name);
	    ff_progress_resume_timer();
	    if ( yes==1 )
		*refstate |= 1;
	    else if ( yes==2 )
		*refstate |= 2;
	}
	if ( (*refstate&1) || yes<=1 ) {
	    new = SplinePointListTransform(SplinePointListCopy(fromsc->layers[ly_fore].splines),ref->transform,tpt_AllPoints);
	    SplinePointListSelect(new,true);
	    if ( new!=NULL ) {
		for ( spl = new; spl->next!=NULL; spl = spl->next );
		spl->next = sc->layers[ly_fore].splines;
		sc->layers[ly_fore].splines = new;
	    }
	}
    }
}

static void SCCheckXClipboard(SplineChar *sc,int layer,int doclear) {
    int type; int32 len;
    char *paste;
    FILE *temp;
    GImage *image;
    int sx=0, s_x=0;

    if ( no_windowing_ui )
return;
    type = 0;
    /* SVG is a better format (than eps) if we've got it because it doesn't */
    /*  force conversion of quadratic to cubic and back */
    if ( HasSVG() && ((sx = ClipboardHasType("image/svg+xml")) ||
			(s_x = ClipboardHasType("image/svg-xml")) ||
			ClipboardHasType("image/svg")) )
	type = sx ? 1 : s_x ? 2 : 3;
    else
    if ( ClipboardHasType("image/eps") )
	type = 4;
    else if ( ClipboardHasType("image/ps") )
	type = 5;
#ifndef _NO_LIBPNG
    else if ( ClipboardHasType("image/png") )
	type = 6;
#endif
    else if ( ClipboardHasType("image/bmp") )
	type = 7;

    if ( type==0 )
return;

    paste = ClipboardRequest(type==1?"image/svg+xml":
		type==2?"image/svg-xml":
		type==3?"image/svg":
		type==4?"image/eps":
		type==5?"image/ps":
		type==6?"image/png":"image/bmp",&len);
    if ( paste==NULL )
return;

    temp = tmpfile();
    if ( temp!=NULL ) {
	fwrite(paste,1,len,temp);
	rewind(temp);
	if ( type==4 || type==5 ) {	/* eps/ps */
	    SCImportPSFile(sc,layer,temp,doclear,-1);
	} else if ( type<=3 ) {
	    SCImportSVG(sc,layer,NULL,paste,len,doclear);
	} else {
#ifndef _NO_LIBPNG
	    if ( type==6 )
		image = GImageRead_Png(temp);
	    else
#endif
		image = GImageRead_Bmp(temp);
	    SCAddScaleImage(sc,image,doclear,layer);
	}
	fclose(temp);
    }
    free(paste);
}

static void XClipFontToFFClip(void) {
    int32 len;
    int i;
    char *paste;
    SplineFont *sf;
    SplineChar *sc;
    Undoes *head=NULL, *last=NULL, *cur;

    paste = ClipboardRequest("application/x-font-svg",&len);
    if ( paste==NULL )
return;
    sf = SFReadSVGMem(paste,0);
    if ( sf==NULL )
return;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	if ( strcmp(sc->name,".notdef")==0 )
    continue;
	cur = SCCopyAll(sc,ly_fore,ct_fullcopy);
	if ( cur!=NULL ) {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }

    SplineFontFree(sf);
    free(paste);

    if ( head==NULL )
return;

    CopyBufferFree();
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = head;
    copybuffer.copied_from = NULL;
}

static double PasteFigureScale(SplineFont *newsf,SplineFont *oldsf) {

    if ( newsf==oldsf )
return( 1.0 );
    if ( !SFIsActive(oldsf))		/* Font we copied from has been closed */
return( 1.0 );

return( (newsf->ascent+newsf->descent) / (double) (oldsf->ascent+oldsf->descent) );
}

static int anchor_lost_warning = false;

static void APMerge(SplineChar *sc,AnchorPoint *anchor) {
    AnchorPoint *ap, *prev, *next, *test;
    AnchorClass *ac;

    if ( anchor==NULL )
return;
    anchor = AnchorPointsCopy(anchor);
    /* If we pasted from one font to another, the anchor class list will be */
    /*  different. */
    for ( ac = sc->parent->anchor; ac!=NULL && ac!=anchor->anchor; ac=ac->next );
    if ( ac==NULL ) {		/* Into a different font. See if we can find a class with same name in new font */
	prev = NULL;
	for ( ap = anchor; ap!=NULL; ap=next ) {
	    next = ap->next;
	    for ( ac = sc->parent->anchor; ac!=NULL && strcmp(ac->name,ap->anchor->name)!=0; ac = ac->next );
	    if ( ac!=NULL ) {
		ap->anchor = ac;
		prev = ap;
	    } else {
		if ( prev==NULL )
		    anchor = next;
		else
		    prev->next = next;
		ap->next = NULL;
		AnchorPointsFree(ap);
		anchor_lost_warning = true;
	    }
	}
	if ( anchor_lost_warning )
	    ff_post_error(_("Anchor Lost"),_("At least one anchor point was lost when pasting from one font to another because no matching anchor class could be found in the new font."));
	if ( anchor==NULL )
return;
    }
    if ( sc->anchor==NULL ) {
	sc->anchor = anchor;
return;
    }

    prev = NULL;
    for ( ap=anchor; ap!=NULL; ap=next ) {
	next = ap->next;
	for ( test=sc->anchor; test!=NULL; test=test->next )
	    if ( test->anchor==ap->anchor ) {
		if (( test->type==at_centry && ap->type==at_cexit) ||
			(test->type==at_cexit && ap->type==at_centry))
		    /* It's ok */;
		else if ( test->type!=at_baselig || ap->type!=at_baselig ||
			test->lig_index==ap->lig_index )
	break;
	    }
	if ( test!=NULL ) {
	    ff_post_error(_("Duplicate Anchor"),_("There is already an anchor point named %1$.40s in %2$.40s."),test->anchor->name,sc->name);
	    if ( prev==NULL )
		anchor = next;
	    else
		prev->next = next;
	    ap->next = NULL;
	    AnchorPointsFree(ap);
	} else
	    prev = ap;
    }
    if ( prev!=NULL ) {
	prev->next = sc->anchor;
	sc->anchor = anchor;
    }
}

static int InstrsSameParent( SplineChar *sc, SplineFont *copied_from) {
    static SplineFont *dontask_parent = NULL, *dontask_copied_from;
    static int dontask_ret=0;
    char *buts[5];
    int ret;

    if ( sc->parent==copied_from )
return( true );
    if ( sc->parent == dontask_parent && copied_from==dontask_copied_from )
return( dontask_ret );
    buts[0] = _("_Yes"); buts[3] = _("_No");
    buts[1] = _("Yes to _All");
    buts[2] = _("No _to All");
    buts[4] = NULL;
    ret = ff_ask(_("Different Fonts"),(const char **) buts,0,3,_("You are attempting to paste glyph instructions from one font to another. Generally this will not work unless the 'prep', 'fpgm' and 'cvt ' tables are the same.\nDo you want to continue anyway?"));
    if ( ret==0 )
return( true );
    if ( ret==3 )
return( false );
    dontask_parent = sc->parent;
    dontask_copied_from = copied_from;
    dontask_ret = ret==1;
return( dontask_ret );
}

int SCWasEmpty(SplineChar *sc, int skip_this_layer) {
    int i;

    for ( i=ly_fore; i<sc->layer_cnt; ++i ) if ( i!=skip_this_layer && !sc->layers[i].background ) {
	if ( sc->layers[i].refs!=NULL )
return( false );
	else if ( sc->layers[i].splines!=NULL ) {
	    SplineSet *ss;
	    for ( ss = sc->layers[i].splines; ss!=NULL; ss=ss->next ) {
		if ( ss->first->prev!=NULL )
return( false );			/* Closed contour */
	    }
	}
    }
return( true );
}

/* when pasting from the fontview we do a clear first */
static void _PasteToSC(SplineChar *sc,Undoes *paster,FontViewBase *fv,int pasteinto,
	int layer, real trans[6], struct sfmergecontext *mc,int *refstate,
	int *already_complained) {
    DBounds bb;
    real transform[6];
    int width, vwidth;
    int xoff=0, yoff=0;
    int was_empty;
    RefChar *ref;

    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_anchors:
	if ( !sc->searcherdummy )
	    APMerge(sc,paster->u.state.anchor);
      break;
      case ut_state: case ut_statehint: case ut_statename:
	if ( paster->u.state.splines!=NULL || paster->u.state.refs!=NULL )
	    sc->parent->onlybitmaps = false;
	SCPreserveLayer(sc,layer,paster->undotype==ut_statehint);
	width = paster->u.state.width;
	vwidth = paster->u.state.vwidth;
	was_empty = sc->hstem==NULL && sc->vstem==NULL && sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs == NULL;
	if ( !pasteinto ) {
	    if ( !sc->layers[layer].background &&
		    SCWasEmpty(sc,pasteinto==0?layer:-1) ) {
		if ( !sc->parent->onlybitmaps )
		    SCSynchronizeWidth(sc,width,sc->width,fv);
		sc->vwidth = vwidth;
	    }
	    SplinePointListsFree(sc->layers[layer].splines);
	    sc->layers[layer].splines = NULL;
	    ImageListsFree(sc->layers[layer].images);
	    sc->layers[layer].images = NULL;
	    SCRemoveLayerDependents(sc,layer);
	    AnchorPointsFree(sc->anchor);
	    sc->anchor = NULL;
	    if ( paster->undotype==ut_statehint ) {
		StemInfosFree(sc->hstem);
		StemInfosFree(sc->vstem);
		sc->hstem = sc->vstem = NULL;
		sc->hconflicts = sc->vconflicts = false;
	    }
	    GradientFree(sc->layers[layer].fill_brush.gradient); sc->layers[layer].fill_brush.gradient = NULL;
	    PatternFree(sc->layers[layer].fill_brush.pattern); sc->layers[layer].fill_brush.pattern = NULL;
	    GradientFree(sc->layers[layer].stroke_pen.brush.gradient); sc->layers[layer].stroke_pen.brush.gradient = NULL;
	    PatternFree(sc->layers[layer].stroke_pen.brush.pattern); sc->layers[layer].stroke_pen.brush.pattern = NULL;
	    was_empty = true;
	} else if ( pasteinto==2 ) {
	    if ( sc->parent->hasvmetrics ) {
		yoff = -sc->vwidth;
		sc->vwidth += vwidth;
	    } else if ( SCRightToLeft(sc) ) {
		xoff = 0;
		SCSynchronizeWidth(sc,sc->width+width,sc->width,fv);
		transform[0] = transform[3] = 1; transform[1] = transform[2] = transform[5] = 0;
		transform[4] = width;
		sc->layers[layer].splines = SplinePointListTransform(
			sc->layers[layer].splines,transform,tpt_AllPoints);
		for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
		    ref->transform[4] += width;
		    SCReinstanciateRefChar(sc,ref,layer);
		}
	    } else {
		xoff = sc->width;
		SCSynchronizeWidth(sc,sc->width+width,sc->width,fv);
	    }
	} else if ( pasteinto==3 && trans!=NULL ) {
	    xoff = trans[4]; yoff = trans[5];
	}
	if ( layer>=ly_fore && sc->layers[layer].splines==NULL &&
		sc->layers[layer].refs==NULL && sc->layers[layer].images==NULL &&
		sc->parent->multilayer ) {
	    /* pasting into an empty layer sets the fill/stroke */
	    BrushCopy(&sc->layers[layer].fill_brush, &paster->u.state.fill_brush,NULL);
	    PenCopy(&sc->layers[layer].stroke_pen, &paster->u.state.stroke_pen,NULL);
	    sc->layers[layer].dofill = paster->u.state.dofill;
	    sc->layers[layer].dostroke = paster->u.state.dostroke;
	    sc->layers[layer].fillfirst = paster->u.state.fillfirst;
	}
	if ( sc->layers[layer].order2 &&
		!sc->layers[layer].background ) {
	    if ( paster->undotype==ut_statehint ) {
		/* if they are pasting instructions, I hope they know what */
		/*  they are doing... */
	    } else if (( paster->u.state.splines!=NULL || paster->u.state.refs!=NULL || pasteinto==0 ) &&
		    !sc->instructions_out_of_date &&
		    sc->ttf_instrs!=NULL ) {
		/* The normal change check doesn't respond properly to pasting a reference */
		SCClearInstrsOrMark(sc,layer,!*already_complained);
		*already_complained = true;
	    }
	}
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *temp = SplinePointListCopy(paster->u.state.splines);
	    if ( (pasteinto==2 || pasteinto==3 ) && (xoff!=0 || yoff!=0)) {
		transform[0] = transform[3] = 1; transform[1] = transform[2] = 0;
		transform[4] = xoff; transform[5] = yoff;
		temp = SplinePointListTransform(temp,transform,tpt_AllPoints);
	    }
	    if ( paster->was_order2 != sc->layers[layer].order2 )
		temp = SplineSetsConvertOrder(temp,sc->layers[layer].order2);
	    if ( sc->layers[layer].splines!=NULL ) {
		SplinePointList *e = sc->layers[layer].splines;
		while ( e->next!=NULL ) e = e->next;
		e->next = temp;
	    } else
		sc->layers[layer].splines = temp;
	}
	if ( !sc->searcherdummy )
	    APMerge(sc,paster->u.state.anchor);
	if ( paster->u.state.images!=NULL && sc->parent->multilayer ) {
	    ImageList *new, *cimg;
	    for ( cimg = paster->u.state.images; cimg!=NULL; cimg=cimg->next ) {
		new = malloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = sc->layers[layer].images;
		sc->layers[layer].images = new;
	    }
	    SCOutOfDateBackground(sc);
	}
	if ( (paster->undotype==ut_statehint || paster->undotype==ut_statename ) &&
		!sc->layers[layer].background ) {
	    if ( !pasteinto ) {	/* Hints aren't meaningful unless we've cleared first */
		ExtractHints(sc,paster->u.state.hints,true);
		if ( sc->layers[layer].order2 ) {
		    free(sc->ttf_instrs);
		    if ( paster->u.state.instrs_len!=0 && sc->layers[layer].order2 &&
			    InstrsSameParent(sc,paster->copied_from)) {
			sc->ttf_instrs = (uint8 *) copyn((char *) paster->u.state.instrs,paster->u.state.instrs_len);
			sc->ttf_instrs_len = paster->u.state.instrs_len;
		    } else {
			sc->ttf_instrs = NULL;
			sc->ttf_instrs_len = 0;
		    }
		    sc->instructions_out_of_date = false;
		}
	    }
	}
	if ( paster->undotype==ut_statename ) {
	    SCSetMetaData(sc,paster->u.state.charname,
		    paster->u.state.unicodeenc==0xffff?-1:paster->u.state.unicodeenc,
		    paster->u.state.comment);
	    if ( SFIsActive(paster->copied_from) ) {
		/* Only copy PSTs if we can find and translate their lookups */
		PSTFree(sc->possub);
		mc->sf_from = paster->copied_from; mc->sf_to = sc->parent;
		sc->possub = PSTCopy(paster->u.state.possub,sc,mc);
	    }
	}
	if ( paster->u.state.refs!=NULL ) {
	    RefChar *last = sc->layers[layer].refs;
	    while ( last != NULL && last->next != NULL) {
	       last = last->next;
	    }
	    RefChar *new, *refs;
	    SplineChar *rsc;
	    double scale = PasteFigureScale(sc->parent,paster->copied_from);
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( sc->views!=NULL && sc->views->container!=NULL ) {
		    if ( sc->views->container->funcs->type == cvc_searcher ||
			    sc->views->container->funcs->type == cvc_multiplepattern )
			rsc = FindCharacter((sc->views->container->funcs->sf_of_container)(sc->views->container),
				paster->copied_from,refs,NULL);
		    else {
			ff_post_error(_("Please don't do that"),_("You may not paste a reference into this window"));
			rsc = (SplineChar *) -1;
		    }
		} else
		    rsc = FindCharacter(sc->parent,paster->copied_from,refs,NULL);
		if ( rsc==(SplineChar *) -1 )
		    /* Error above */;
		else if ( rsc!=NULL && SCDependsOnSC(rsc,sc))
		    ff_post_error(_("Self-referential glyph"),_("Attempt to make a glyph that refers to itself"));
		else if ( rsc!=NULL ) {
		    new = RefCharCreate();
		    free(new->layers);
		    *new = *refs;
		    new->transform[4] *= scale; new->transform[5] *= scale;
		    new->transform[4] += xoff;  new->transform[5] += yoff;
		    new->layers = NULL;
		    new->layer_cnt = 0;
		    new->sc = rsc;
		    FFLIST_SINGLE_LINKED_APPEND( sc->layers[layer].refs, last, new );
		    SCReinstanciateRefChar(sc,new,layer);
		    SCMakeDependent(sc,rsc);
		} else {
		    PasteNonExistantRefCheck(sc,paster,refs,refstate);
		}
	    }
	}
	SCCharChangedUpdate(sc,layer);
	/* Bug here. we are assuming that the pasted hints are up to date */
	if ( was_empty && (sc->hstem!=NULL || sc->vstem!=NULL))
	    sc->changedsincelasthinted = false;
      break;
      case ut_width:
	SCPreserveWidth(sc);
	SCSynchronizeWidth(sc,paster->u.width,sc->width,fv);
	SCCharChangedUpdate(sc,layer);
      break;
      case ut_vwidth:
	if ( !sc->parent->hasvmetrics )
	    ff_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
	else {
	    SCPreserveVWidth(sc);
	    sc->vwidth = paster->u.width;
	    SCCharChangedUpdate(sc,layer);
	}
      break;
      case ut_rbearing:
	SCPreserveWidth(sc);
	SplineCharFindBounds(sc,&bb);
	SCSynchronizeWidth(sc,bb.maxx + paster->u.rbearing,sc->width,fv);
	SCCharChangedUpdate(sc,layer);
      break;
      case ut_lbearing:
	SplineCharFindBounds(sc,&bb);
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	transform[4] = paster->u.lbearing-bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(fv,sc,transform,NULL,false);
	/* FVTrans will preserve the state and update the chars */
      break;
      default:
      break;
    }
}

static void PasteToSC(SplineChar *sc,int layer,Undoes *paster,FontViewBase *fv,
	int pasteinto, real trans[6], struct sfmergecontext *mc,int *refstate,
	int *already_complained) {
    if ( paster->undotype==ut_layers && sc->parent->multilayer ) {
	int lc, start, layer;
	Undoes *pl;
	Layer *old = sc->layers;
	for ( lc=0, pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next, ++lc );
	if ( !pasteinto ) {
	    start = ly_fore;
	    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		SplinePointListsFree(sc->layers[layer].splines);
		sc->layers[layer].splines = NULL;
		ImageListsFree(sc->layers[layer].images);
		sc->layers[layer].images = NULL;
		SCRemoveLayerDependents(sc,layer);
		GradientFree(sc->layers[layer].fill_brush.gradient); sc->layers[layer].fill_brush.gradient = NULL;
		PatternFree(sc->layers[layer].fill_brush.pattern); sc->layers[layer].fill_brush.pattern = NULL;
		GradientFree(sc->layers[layer].stroke_pen.brush.gradient); sc->layers[layer].stroke_pen.brush.gradient = NULL;
		PatternFree(sc->layers[layer].stroke_pen.brush.pattern); sc->layers[layer].stroke_pen.brush.pattern = NULL;
	    }
	} else
	    start = sc->layer_cnt;
	if ( start+lc > sc->layer_cnt ) {
	    sc->layers = realloc(sc->layers,(start+lc)*sizeof(Layer));
	    for ( layer = sc->layer_cnt; layer<start+lc; ++layer )
		LayerDefault(&sc->layers[layer]);
	    sc->layer_cnt = start+lc;
	}
	for ( lc=0, pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next, ++lc )
	    _PasteToSC(sc,pl,fv,pasteinto,start+lc,trans,mc,refstate,already_complained);
	SCMoreLayers(sc,old);
    } else if ( paster->undotype==ut_layers ) {
	Undoes *pl;
	for ( pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next ) {
	    _PasteToSC(sc,pl,fv,pasteinto,ly_fore,trans,mc,refstate,already_complained);
	    pasteinto = true;		/* Merge other layers in */
	}
    } else
	_PasteToSC(sc,paster,fv,pasteinto,layer,trans,mc,refstate,already_complained);
}

static void DevTabInto(struct vr *vr) {
    ValDevTab *adjust;

    if ( vr->adjust==NULL )
return;		/* Nothing to do */
    adjust = chunkalloc(sizeof(ValDevTab));
    *adjust = *vr->adjust;
    if ( adjust->xadjust.corrections!=NULL ) {
	adjust->xadjust.corrections = malloc(adjust->xadjust.last_pixel_size-adjust->xadjust.first_pixel_size+1);
	memcpy(adjust->xadjust.corrections,vr->adjust->xadjust.corrections,adjust->xadjust.last_pixel_size-adjust->xadjust.first_pixel_size+1);
    }
    if ( adjust->yadjust.corrections!=NULL ) {
	adjust->yadjust.corrections = malloc(adjust->yadjust.last_pixel_size-adjust->yadjust.first_pixel_size+1);
	memcpy(adjust->yadjust.corrections,vr->adjust->yadjust.corrections,adjust->yadjust.last_pixel_size-adjust->yadjust.first_pixel_size+1);
    }
    if ( adjust->xadv.corrections!=NULL ) {
	adjust->xadv.corrections = malloc(adjust->xadv.last_pixel_size-adjust->xadv.first_pixel_size+1);
	memcpy(adjust->xadv.corrections,vr->adjust->xadv.corrections,adjust->xadv.last_pixel_size-adjust->xadv.first_pixel_size+1);
    }
    if ( adjust->yadv.corrections!=NULL ) {
	adjust->yadv.corrections = malloc(adjust->yadv.last_pixel_size-adjust->yadv.first_pixel_size+1);
	memcpy(adjust->yadv.corrections,vr->adjust->yadv.corrections,adjust->yadv.last_pixel_size-adjust->yadv.first_pixel_size+1);
    }
}

static void PSTInto(SplineChar *sc,PST *pst,PST *frompst, struct lookup_subtable *sub) {
    if ( pst==NULL ) {
	pst = chunkalloc(sizeof(PST));
	*pst = *frompst;
	pst->subtable = sub;
	pst->next = sc->possub;
	sc->possub = pst;
    } else {
	if ( pst->type == pst_pair ) {
	    free(pst->u.pair.paired);
	    chunkfree(pst->u.pair.vr,sizeof(struct vr[2]));	/* We fail to free device tables */
	} else if ( pst->type == pst_substitution || pst->type == pst_alternate ||
		pst->type == pst_multiple || pst->type == pst_ligature )
	    free(pst->u.subs.variant);
    }
    if ( pst->type == pst_substitution || pst->type == pst_alternate ||
		pst->type == pst_multiple )
	pst->u.subs.variant = copy( frompst->u.subs.variant );
    else if ( pst->type == pst_ligature ) {
	pst->u.lig.components = copy( frompst->u.lig.components );
	pst->u.lig.lig = sc;
    } else if ( pst->type==pst_pair ) {
	pst->u.pair.paired = copy( frompst->u.pair.paired );
	pst->u.pair.vr = chunkalloc(sizeof(struct vr[2]));
	memcpy(pst->u.pair.vr,frompst->u.pair.vr,sizeof(struct vr[2]));
	DevTabInto(&pst->u.pair.vr[0]);
	DevTabInto(&pst->u.pair.vr[1]);
    } else if ( pst->type==pst_position ) {
	memcpy(&pst->u.pos,&frompst->u.pos,sizeof(struct vr));
	DevTabInto(&pst->u.pos);
    }
}

static void APInto(SplineChar *sc,AnchorPoint *ap,AnchorPoint *fromap,
	AnchorClass *ac) {
    if ( ap==NULL ) {
	ap = chunkalloc(sizeof(AnchorPoint));
	*ap = *fromap;
	ap->anchor = ac;
	ap->next = sc->anchor;
	sc->anchor = ap;
    } else {
	free(ap->xadjust.corrections); free(ap->yadjust.corrections);
	ap->xadjust = fromap->xadjust;
	ap->yadjust = fromap->yadjust;
	ap->me = fromap->me;
    }
    if ( fromap->xadjust.corrections!=NULL ) {
	ap->xadjust.corrections = malloc(ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1);
	memcpy(ap->xadjust.corrections,fromap->xadjust.corrections,ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1);
    }
    if ( fromap->yadjust.corrections!=NULL ) {
	ap->yadjust.corrections = malloc(ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1);
	memcpy(ap->yadjust.corrections,fromap->yadjust.corrections,ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1);
    }
}

static void KPInto(SplineChar *owner,KernPair *kp,KernPair *fromkp,int isv,
	SplineChar *other, struct lookup_subtable *sub) {
    if ( kp==NULL ) {
	kp = chunkalloc(sizeof(KernPair));
	*kp = *fromkp;
	kp->subtable = sub;
	if ( isv ) {
	    kp->next = owner->vkerns;
	    owner->vkerns = kp;
	} else {
	    kp->next = owner->kerns;
	    owner->kerns = kp;
	}
    }
    kp->sc = other;
    kp->off = fromkp->off;
    if ( kp->adjust!=NULL )
	DeviceTableFree(kp->adjust);
    if ( fromkp->adjust!=NULL )
	kp->adjust = DeviceTableCopy(fromkp->adjust);
    else
	kp->adjust = NULL;
}

static void SCPasteLookups(SplineChar *sc,SplineChar *fromsc,
	OTLookup **list, OTLookup **backpairlist, struct sfmergecontext *mc) {
    PST *frompst, *pst;
    int isv, gid;
    KernPair *fromkp, *kp;
    AnchorPoint *fromap, *ap;
    AnchorClass *ac;
    int i;
    OTLookup *otl;
    struct lookup_subtable *sub;
    SplineFont *fromsf;
    SplineChar *othersc;
    SplineChar *test, *test2;
    int changed = false;

    for ( frompst = fromsc->possub; frompst!=NULL; frompst=frompst->next ) {
	if ( frompst->subtable==NULL )
    continue;
	if ( frompst->type == pst_ligature && fromsc->parent==sc->parent )
    continue;
	otl = frompst->subtable->lookup;
	for ( i=0; list[i]!=NULL && list[i]!=otl; ++i );
	if ( list[i]==NULL )
    continue;
	sub = MCConvertSubtable(mc,frompst->subtable);
	if ( otl->lookup_type!=gpos_pair ) {
	    for ( pst=sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
	} else {
	    for ( pst=sc->possub; pst!=NULL ; pst=pst->next ) {
		if ( pst->subtable==sub &&
			strcmp(frompst->u.pair.paired,pst->u.pair.paired)==0 )
	    break;
	    }
	}
	PSTInto(sc,pst,frompst,sub);
	changed = true;
    }

    for ( fromap = fromsc->anchor; fromap!=NULL; fromap=fromap->next ) {
	otl = fromap->anchor->subtable->lookup;
	for ( i=0; list[i]!=NULL && list[i]!=otl; ++i );
	if ( list[i]==NULL )
    continue;
	ac = MCConvertAnchorClass(mc,fromap->anchor);
	for ( ap=sc->anchor; ap!=NULL &&
		!(ap->anchor==ac && ap->type==fromap->type && ap->lig_index==fromap->lig_index);
		ap=ap->next );
	APInto(sc,ap,fromap,ac);
	changed = true;
    }

    for ( isv=0; isv<2; ++isv ) {
	for ( fromkp = isv ? fromsc->vkerns : fromsc->kerns; fromkp!=NULL; fromkp=fromkp->next ) {
	    otl = fromkp->subtable->lookup;
	    for ( i=0; list[i]!=NULL && list[i]!=otl; ++i );
	    if ( list[i]==NULL )
    continue;
	    sub = MCConvertSubtable(mc,fromkp->subtable);
	    if ( sub==NULL )
    continue;
	    if ( fromsc->parent==sc->parent )
		othersc = fromkp->sc;
	    else
		othersc = SFGetChar(sc->parent,fromkp->sc->unicodeenc,fromkp->sc->name);
	    if ( othersc!=NULL ) {
		for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->subtable == sub && kp->sc == othersc )
		break;
		}
		KPInto(sc,kp,fromkp,isv,othersc,sub);
		changed = true;
	    }
	}
	if ( backpairlist==NULL )
    continue;
	fromsf = fromsc->parent;
	for ( gid=fromsf->glyphcnt-1; gid>=0; --gid ) if ( (test=fromsf->glyphs[gid])!=NULL ) {
	    for ( fromkp = isv ? test->vkerns : test->kerns; fromkp!=NULL; fromkp=fromkp->next ) {
		if ( fromkp->sc!=fromsc )
	    continue;
		otl = fromkp->subtable->lookup;
		for ( i=0; backpairlist[i]!=NULL && backpairlist[i]!=otl; ++i );
		if ( backpairlist[i]==NULL )
	    continue;
		sub = MCConvertSubtable(mc,fromkp->subtable);
		if ( fromsf==sc->parent )
		    test2 = test;
		else
		    test2 = SFGetChar(sc->parent,test->unicodeenc,test->name);
		if ( test2==NULL || sub==NULL )
	    continue;
		for ( kp = isv ? test2->vkerns : test2->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->subtable==sub && kp->sc == sc )
		break;
		}
		KPInto(test2,kp,fromkp,isv,sc,sub);
		_SCCharChangedUpdate(test2,ly_none,2);
	    }
	}
    }
    if ( changed )
	_SCCharChangedUpdate(sc,ly_none,2);
}

static void SCPasteLookupsMid(SplineChar *sc,Undoes *paster,
	OTLookup **list, OTLookup **backpairlist, struct sfmergecontext *mc) {
    SplineChar *fromsc;

    (void) FindCharacter(sc->parent,paster->copied_from,
	    paster->u.state.refs,&fromsc);
    if ( fromsc==NULL ) {
	ff_post_error(_("Missing glyph"),_("Could not find original glyph"));
return;
    }
    SCPasteLookups(sc,fromsc,list,backpairlist,mc);
}

static int HasNonClass(OTLookup *otl) {
    struct lookup_subtable *sub;
    for ( sub = otl->subtables; sub!=NULL; sub=sub->next )
	if ( sub->kc==NULL )
return( true );

return( false );
}

static OTLookup **GetLookupsToCopy(SplineFont *sf,OTLookup ***backpairlist) {
    int cnt, bcnt, ftot=0, doit, isgpos, i, ret;
    char **choices = NULL, *sel = NULL;
    OTLookup *otl, **list1=NULL, **list2=NULL, **list, **blist;
    char *buttons[3];
    buttons[0] = _("_OK");
    buttons[1] = _("_Cancel");
    buttons[2] = NULL;

    *backpairlist = NULL;

    for ( doit=0; doit<2; ++doit ) {
	bcnt = cnt = 0;
	for ( isgpos=0; isgpos<2; ++isgpos ) {
	    for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
		if ( otl->lookup_type == gsub_single ||
			otl->lookup_type == gsub_multiple ||
			otl->lookup_type == gsub_alternate ||
			otl->lookup_type == gsub_ligature ||
			otl->lookup_type == gpos_single ||
			otl->lookup_type == gpos_cursive ||
			otl->lookup_type == gpos_mark2base ||
			otl->lookup_type == gpos_mark2ligature ||
			otl->lookup_type == gpos_mark2mark ||
			(otl->lookup_type == gpos_pair && HasNonClass(otl))) {
		    if ( doit ) {
			list1[cnt] = otl;
			choices[cnt++] = copy(otl->lookup_name);
			if ( otl->lookup_type==gpos_pair ) {
/* GT: I'm not happy with this phrase. Suggestions for improvements are welcome */
/* GT:  Here I am generating a list of lookup names representing data that can */
/* GT:  be copied from one glyph to another. For a kerning (pairwise) lookup */
/* GT:  the first entry in the list (marked by the lookup name by itself) will */
/* GT:  mean all data where the current glyph is the first glyph in a kerning */
/* GT:  pair. But we can also (separately) copy data where the current glyph */
/* GT:  is the second glyph in the kerning pair, and that's what this line */
/* GT:  refers to. The "%s" will be filled in with the lookup name */
			    char *format = _("Second glyph of %s");
			    char *space = malloc(strlen(format)+strlen(otl->lookup_name)+1);
			    sprintf(space, format, otl->lookup_name );
			    list2[bcnt] = otl;
			    choices[ftot+1+bcnt++] = space;
			}
		    } else {
			++cnt;
			if ( otl->lookup_type==gpos_pair )
			    ++bcnt;
		    }
		}
	    }
	}
	if ( cnt==0 ) {	/* If cnt==0 then bcnt must be too */
	    ff_post_error(_("No Lookups"),_("No lookups to copy"));
return( NULL );
	}
	if ( !doit ) {
	    ftot = cnt;
	    choices = malloc((cnt+bcnt+2)*sizeof(char *));
	    sel = calloc(cnt+bcnt+1,1);
	    list1 = malloc(cnt*sizeof(OTLookup *));
	    if ( bcnt==0 ) {
		choices[cnt] = NULL;
		list2 = NULL;
	    } else {
		choices[cnt] = copy("-");
		choices[bcnt+cnt+1] = NULL;
		list2 = malloc(bcnt*sizeof(OTLookup *));
	    }
	}
    }
    ret = ff_choose_multiple(_("Lookups"),(const char **) choices,sel,bcnt==0?cnt: cnt+bcnt+1,
	    buttons, _("Choose which lookups to copy"));
    list = NULL;
    if ( ret>=0 ) {
	for ( i=cnt=0; i<ftot; ++i ) {
	    if ( sel[i] )
		++cnt;
	}
	list = malloc((cnt+1)*sizeof(OTLookup *));
	for ( i=cnt=0; i<ftot; ++i ) {
	    if ( sel[i] )
		list[cnt++] = list1[i];
	}
	list[cnt] = NULL;
	blist = NULL;
	if ( bcnt!=0 ) {
	    for ( i=cnt=0; i<bcnt; ++i ) {
		if ( sel[i+ftot+1] )
		    ++cnt;
	    }
	    if ( cnt!=0 ) {
		blist = malloc((cnt+1)*sizeof(OTLookup *));
		for ( i=cnt=0; i<bcnt; ++i ) {
		    if ( sel[i+ftot+1] )
			blist[cnt++] = list2[i];
		}
		blist[cnt] = NULL;
	    }
	    *backpairlist = blist;
	}
	if ( blist==NULL && list[0]==NULL ) {
	    free(list);
	    list = NULL;
	}
    }
    free( sel );
    for ( i=0; choices[i]!=NULL; ++i )
	free( choices[i]);
    free(choices);
    free(list1);
    free(list2);
return( list );
}

static void SCPasteLookupsTop(SplineChar *sc,Undoes *paster) {
    OTLookup **list, **backpairlist;
    SplineChar *fromsc;
    struct sfmergecontext mc;

    if ( paster->copied_from==NULL )
return;
    (void) FindCharacter(sc->parent,paster->copied_from,
	    paster->u.state.refs,&fromsc);
    if ( fromsc==NULL ) {
	ff_post_error(_("Missing glyph"),_("Could not find original glyph"));
return;
    }
    list = GetLookupsToCopy(fromsc->parent,&backpairlist);
    if ( list==NULL )
return;
    memset(&mc,0,sizeof(mc));
    mc.sf_from = paster->copied_from; mc.sf_to = sc->parent;
    SCPasteLookups(sc,fromsc,list,backpairlist,&mc);
    free(list);
    free(backpairlist);
    SFFinishMergeContext(&mc);
}

static void _PasteToCV(CharViewBase *cv,SplineChar *cvsc,Undoes *paster) {
    int refstate = 0;
    DBounds bb;
    real transform[6];
    int wasempty = false;
    int wasemptylayer = false;
    int layer = CVLayer(cv);

    if ( copybuffer.undotype == ut_none ) {
	if ( cv->drawmode==dm_grid )
return;
	SCCheckXClipboard(cvsc,cv->layerheads[cv->drawmode]-cvsc->layers,false);
return;
    }

    anchor_lost_warning = false;

    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_anchors:
	APMerge(cvsc,paster->u.state.anchor);
      break;
      case ut_statelookup:
	SCPasteLookupsTop(cvsc,paster);
      break;
      case ut_state: case ut_statehint: case ut_statename:
	wasempty = SCWasEmpty(cvsc,-1);
	wasemptylayer = layer!=ly_grid && cvsc->layers[layer].splines==NULL &&
		cvsc->layers[layer].refs==NULL;
	if ( wasemptylayer && cv->layerheads[dm_fore]->images==NULL &&
		cvsc->parent->multilayer ) {
	    /* pasting into an empty layer sets the fill/stroke */
	    BrushCopy(&cv->layerheads[dm_fore]->fill_brush, &paster->u.state.fill_brush,NULL);
	    PenCopy(&cv->layerheads[dm_fore]->stroke_pen, &paster->u.state.stroke_pen,NULL);
	    cv->layerheads[dm_fore]->dofill = paster->u.state.dofill;
	    cv->layerheads[dm_fore]->dostroke = paster->u.state.dostroke;
	    cv->layerheads[dm_fore]->fillfirst = paster->u.state.fillfirst;
	}
	if ( cv->layerheads[cv->drawmode]->order2 &&
		!cv->layerheads[cv->drawmode]->background ) {
	    if ( paster->undotype==ut_statehint ) {
		/* if they are pasting instructions, I hope they know what */
		/*  they are doing... */
	    } else if (( paster->u.state.splines!=NULL || paster->u.state.refs!=NULL ) &&
		    !cvsc->instructions_out_of_date &&
		    cvsc->ttf_instrs!=NULL ) {
		/* The normal change check doesn't respond properly to pasting a reference */
		SCClearInstrsOrMark(cvsc,CVLayer(cv),true);
	    }
	}
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *spl, *new = SplinePointListCopy(paster->u.state.splines);
	    if ( paster->was_order2 != cv->layerheads[cv->drawmode]->order2 )
		new = SplineSetsConvertOrder(new,cv->layerheads[cv->drawmode]->order2 );
	    SplinePointListSelect(new,true);
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = cv->layerheads[cv->drawmode]->splines;
	    cv->layerheads[cv->drawmode]->splines = new;
	}
	if ( paster->undotype==ut_state && paster->u.state.images!=NULL ) {
	    /* Can't paste images to foreground layer (unless type3 font) */
	    ImageList *new, *cimg;
	    int ly = cvsc->parent->multilayer || cv->layerheads[cv->drawmode]->background ?
		    CVLayer(cv) : ly_back;
	    if ( ly==ly_grid ) ly = ly_back;
	    for ( cimg = paster->u.state.images; cimg!=NULL; cimg=cimg->next ) {
		new = malloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = cvsc->layers[ly].images;
		cvsc->layers[ly].images = new;
	    }
	    SCOutOfDateBackground(cvsc);
	} else if ( paster->undotype==ut_statehint && cv->container==NULL &&
		!cv->layerheads[cv->drawmode]->background ) {
	    ExtractHints(cvsc,paster->u.state.hints,true);
	    if ( cv->layerheads[cv->drawmode]->order2 ) {
		free(cvsc->ttf_instrs);
		if ( paster->u.state.instrs_len!=0 && cv->layerheads[cv->drawmode]->order2 &&
			InstrsSameParent(cvsc,paster->copied_from)) {
		    cvsc->ttf_instrs = (uint8 *) copyn((char *) paster->u.state.instrs,paster->u.state.instrs_len);
		    cvsc->ttf_instrs_len = paster->u.state.instrs_len;
		} else {
		    cvsc->ttf_instrs = NULL;
		    cvsc->ttf_instrs_len = 0;
		}
		cvsc->instructions_out_of_date = false;
	    }
	}
	if ( paster->u.state.anchor!=NULL && !cvsc->searcherdummy )
	    APMerge(cvsc,paster->u.state.anchor);
	if ( paster->u.state.refs!=NULL && cv->drawmode!=dm_grid ) {
	    RefChar *last=NULL;
	    RefChar *new, *refs;
	    SplineChar *sc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( cv->container==NULL ) {
		    sc = FindCharacter(cvsc->parent,paster->copied_from,refs,NULL);
		    if ( sc!=NULL && SCDependsOnSC(sc,cvsc)) {
			ff_post_error(_("Self-referential character"),_("Attempt to make a character that refers to itself"));
			sc = (SplineChar *) -1;
		    }
		} else if ( cv->container->funcs->type == cvc_searcher ||
			cv->container->funcs->type == cvc_multiplepattern )
		    sc = FindCharacter((cv->container->funcs->sf_of_container)(cv->container),paster->copied_from,refs,NULL);
		else
		    sc = (SplineChar *) -1;
		if ( sc==(SplineChar *) -1 )
		    /* Already complained */;
		else if ( sc!=NULL ) {
		    new = RefCharCreate();
		    free(new->layers);
		    *new = *refs;
		    new->layers = NULL;
		    new->layer_cnt = 0;
		    new->sc = sc;
		    new->selected = true;
		    new->next = cvsc->layers[layer].refs;
		    cvsc->layers[layer].refs = new;
		    SCReinstanciateRefChar(cvsc,new,layer);
		    SCMakeDependent(cvsc,sc);
		} else {
		    PasteNonExistantRefCheck(cvsc,paster,refs,&refstate);
		}
	    }
	}
	if ( paster->undotype==ut_statename ) {
	    SCSetMetaData(cvsc,paster->u.state.charname,
		    paster->u.state.unicodeenc==0xffff?-1:paster->u.state.unicodeenc,
		    paster->u.state.comment);
	    PSTFree(cvsc->possub);
	    cvsc->possub = paster->u.state.possub;
	}
	if ( wasempty && layer>=ly_fore && !cvsc->layers[layer].background ) {
	    /* Don't set the width in background or grid layers */
	    /* Don't set the width if any potential foreground layer contained something */
	    int width = paster->u.state.width;
	    int vwidth = paster->u.state.vwidth;
	    if ( cvsc->layers[ly_fore].splines==NULL && cvsc->layers[ly_fore].refs!=NULL ) {
		RefChar *r;
		/* If we copied a glyph containing refs from a different font */
		/*  then the glyphs referred to will be in this font, and the */
		/*  width should not be the original width, but the width in */
		/*  the current font */
		for ( r=cvsc->layers[ly_fore].refs; r!=NULL; r=r->next )
		    if ( r->use_my_metrics ) {
			width = r->sc->width;
			vwidth = r->sc->vwidth;
		break;
		    }
	    }
	    SCSynchronizeWidth(cvsc,width,cvsc->width,NULL);
	    cvsc->vwidth = vwidth;
	}
      break;
      case ut_width:
	SCSynchronizeWidth(cvsc,paster->u.width,cvsc->width,NULL);
      break;
      case ut_vwidth:
	if ( !cvsc->parent->hasvmetrics )
	    ff_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
	else
	    cvsc->vwidth = paster->u.state.vwidth;
      break;
      case ut_rbearing:
	SplineCharFindBounds(cvsc,&bb);
	SCSynchronizeWidth(cvsc,bb.maxx + paster->u.rbearing,cvsc->width,cv->fv);
      break;
      case ut_lbearing:
	SplineCharFindBounds(cvsc,&bb);
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	transform[4] = paster->u.lbearing-bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(cv->fv,cvsc,transform,NULL,false);
	/* FVTrans will preserve the state and update the chars */
	/* CVPaste depends on this behavior */
      break;
      case ut_composit:
	if ( paster->u.composit.state!=NULL )
	    _PasteToCV(cv,cvsc,paster->u.composit.state);
      break;
      case ut_multiple: case ut_layers:
	_PasteToCV(cv,cvsc,paster->u.multiple.mult);
      break;
      default:
      break;
    }
}

void PasteToCV(CharViewBase *cv) {
    _PasteToCV(cv,cv->sc,&copybuffer);
    if ( cv->sc->blended && cv->drawmode==dm_fore ) {
	int j, gid = cv->sc->orig_pos;
	MMSet *mm = cv->sc->parent->mm;
	for ( j=0; j<mm->instance_count; ++j )
	    _PasteToCV(cv,mm->instances[j]->glyphs[gid],&copybuffer);
    }
}


SplineSet *ClipBoardToSplineSet(void) {
    Undoes *paster = &copybuffer;

    while ( paster!=NULL ) {
	switch ( paster->undotype ) {
	  default:
	  case ut_noop: case ut_none:
return( NULL );
	  break;
	  case ut_state: case ut_statehint: case ut_statename:
	    if ( paster->u.state.refs!=NULL )
return( NULL );

return( paster->u.state.splines );
	  break;
	  case ut_width:
return( NULL );
	  case ut_vwidth:
return( NULL );
	  case ut_rbearing:
return( NULL );
	  case ut_lbearing:
return( NULL );
	  case ut_composit:
	    paster = paster->u.composit.state;
	  break;
	  case ut_multiple:
	    paster = paster->u.multiple.mult;
	  break;
	}
    }
return( NULL );
}

static Undoes *BCCopyAll(BDFChar *bc,int pixelsize, int depth, enum fvcopy_type full) {
    Undoes *cur;
    BDFRefChar *ref, *head;

    cur = chunkalloc(sizeof(Undoes));
    memset(&cur->u.bmpstate,'\0',sizeof( BDFChar ));
    if ( bc==NULL )
	cur->undotype = ut_noop;
    else {
	BCCompressBitmap(bc);
	cur->undotype = ut_bitmap;
	cur->u.bmpstate.width = bc->width;
	if ( full==ct_fullcopy || full == ct_unlinkrefs ) {
	    cur->u.bmpstate.xmin = bc->xmin;
	    cur->u.bmpstate.xmax = bc->xmax;
	    cur->u.bmpstate.ymin = bc->ymin;
	    cur->u.bmpstate.ymax = bc->ymax;
	    cur->u.bmpstate.bytes_per_line = bc->bytes_per_line;
	    cur->u.bmpstate.bitmap = bmpcopy(bc->bitmap,bc->bytes_per_line,
		bc->ymax-bc->ymin+1);
	    cur->u.bmpstate.selection = BDFFloatCopy(bc->selection);

	    for ( head = bc->refs; head != NULL; head = head->next ) {
		ref = calloc( 1,sizeof( BDFRefChar ));
		memcpy( ref,head,sizeof( BDFRefChar ));
		ref->next = cur->u.bmpstate.refs;
		cur->u.bmpstate.refs = ref;
	    }
	} else {		/* Or just make a reference */
	    cur->u.bmpstate.bytes_per_line = 1;
	    cur->u.bmpstate.bitmap = calloc(1,sizeof(uint8));

	    ref = calloc(1,sizeof(BDFRefChar));
	    ref->bdfc = bc;
	    ref->xoff = 0; ref->yoff = 0;
	    cur->u.bmpstate.refs = ref;
	}
    }
    cur->u.bmpstate.pixelsize = pixelsize;
    cur->u.bmpstate.depth = depth;
return( cur );
}

void BCCopySelected(BDFChar *bc,int pixelsize,int depth) {
    int has_selected_refs = false;
    BDFRefChar *head, *ref;

    CopyBufferFreeGrab();

    memset(&copybuffer,'\0',sizeof(copybuffer));
    if ( bc->selection!=NULL ) {
	copybuffer.undotype = ut_bitmapsel;
	copybuffer.u.bmpstate.selection = BDFFloatCopy(bc->selection);
    } else {
	for ( head=bc->refs; head!=NULL; head=head->next ) if ( head->selected ) {
	    has_selected_refs = true;
	    ref = calloc( 1,sizeof( BDFRefChar ));
	    memcpy( ref,head,sizeof( BDFRefChar ));
	    ref->next = copybuffer.u.bmpstate.refs;
	    copybuffer.u.bmpstate.refs = ref;
	}
	if ( has_selected_refs ) {
	    copybuffer.undotype = ut_bitmap;
	    copybuffer.u.bmpstate.width = bc->width;
	    copybuffer.u.bmpstate.bytes_per_line = 1;
	    copybuffer.u.bmpstate.bitmap = calloc(1,sizeof(uint8));
	    copybuffer.u.bmpstate.selection = NULL;
	} else {
	    copybuffer.undotype = ut_bitmapsel;
	    copybuffer.u.bmpstate.selection = BDFFloatCreate(bc,bc->xmin,bc->xmax,
		bc->ymin,bc->ymax, false);
	}
    }
    copybuffer.u.bmpstate.pixelsize = pixelsize;
    copybuffer.u.bmpstate.depth = depth;
}

void BCCopyReference(BDFChar *bc,int pixelsize,int depth) {
    Undoes *tmp;

    CopyBufferFreeGrab();
    tmp = BCCopyAll( bc,pixelsize,depth,ct_reference );

    memcpy( &copybuffer,tmp,sizeof( Undoes ));
    chunkfree( tmp,sizeof( Undoes ));
    XClipCheckEps();
}

static void _PasteToBC(BDFChar *bc,int pixelsize, int depth, Undoes *paster, int clearfirst) {
    BDFRefChar *head, *cur;

    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_bitmapsel:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	if ( clearfirst )
	    memset(bc->bitmap,'\0',bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	bc->selection = BDFFloatConvert(paster->u.bmpstate.selection,depth,paster->u.bmpstate.depth);
	BCCharChangedUpdate(bc);
      break;
      case ut_bitmap:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	if ( clearfirst ) {
	    for ( head = bc->refs; head != NULL; ) {
		cur = head; head = head->next; free( cur );
	    }
	    bc->refs = NULL;
	    memset(bc->bitmap,0,bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	    bc->width = paster->u.bmpstate.width;
	}
	BCPasteInto(bc,&paster->u.bmpstate,0,0, false, false);
	for ( head = paster->u.bmpstate.refs; head != NULL; head = head->next ) {
	    if ( BCRefersToBC( bc,head->bdfc )) {
		ff_post_error(_("Self-referential glyph"),_("Attempt to make a glyph that refers to itself"));
	    } else {
		cur = calloc( 1,sizeof( BDFRefChar ));
		memcpy( cur,head,sizeof( BDFRefChar ));
		cur->next = bc->refs; bc->refs = cur;
		BCMakeDependent( bc,head->bdfc );
	    }
	}

	BCCompressBitmap(bc);
	bc->selection = BDFFloatConvert(paster->u.bmpstate.selection,depth,paster->u.bmpstate.depth);
	BCCharChangedUpdate(bc);
      break;
      case ut_composit:
	/* if there's only one bitmap and no outline state (so we only copied a bitmap) */
	/*  then paste that thing with no questions. Otherwise search for a */
	/*  bitmap with the right pixel size. If we find it, paste it, else */
	/*  noop */
	if ( paster->u.composit.bitmaps==NULL )
	    /* Nothing to be done */;
	else if ( paster->u.composit.state==NULL && paster->u.composit.bitmaps->next==NULL )
	    _PasteToBC(bc,pixelsize,depth,paster->u.composit.bitmaps,clearfirst);
	else {
	    Undoes *b;
	    for ( b = paster->u.composit.bitmaps;
		    b!=NULL && b->u.bmpstate.pixelsize!=pixelsize;
		    b = b->next ) ;
	    if ( b!=NULL )
		_PasteToBC(bc,pixelsize,depth,b,clearfirst);
	}
      break;
      case ut_multiple:
	_PasteToBC(bc,pixelsize,depth,paster->u.multiple.mult,clearfirst);
      break;
      default:
      break;
    }
}

void PasteToBC(BDFChar *bc,int pixelsize,int depth) {
    _PasteToBC(bc,pixelsize,depth,&copybuffer,false);
}

void FVCopyWidth(FontViewBase *fv,enum undotype ut) {
    Undoes *head=NULL, *last=NULL, *cur;
    int i, any=false, gid;
    SplineChar *sc;
    DBounds bb;

    CopyBufferFreeGrab();

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	any = true;
	cur = chunkalloc(sizeof(Undoes));
	cur->undotype = ut;
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	    switch ( ut ) {
	      case ut_width:
		cur->u.width = sc->width;
	      break;
	      case ut_vwidth:
		cur->u.width = sc->vwidth;
	      break;
	      case ut_lbearing:
		SplineCharFindBounds(sc,&bb);
		cur->u.lbearing = bb.minx;
	      break;
	      case ut_rbearing:
		SplineCharFindBounds(sc,&bb);
		cur->u.rbearing = sc->width-bb.maxx;
	      break;
	      default:	      
	      break;
	    }
	} else
	    cur->undotype = ut_noop;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = head;
    copybuffer.copied_from = fv->sf;
    if ( !any )
	LogError( _("No selection\n") );
}

void FVCopyAnchors(FontViewBase *fv) {
    Undoes *head=NULL, *last=NULL, *cur;
    int i, any=false, gid;
    SplineChar *sc;

    CopyBufferFreeGrab();

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	any = true;
	cur = chunkalloc(sizeof(Undoes));
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	    cur->undotype = ut_anchors;
	    cur->u.state.anchor = AnchorPointsCopy(sc->anchor);
	} else
	    cur->undotype = ut_noop;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = head;
    copybuffer.copied_from = fv->sf;
    if ( !any )
	LogError( _("No selection\n") );
}

void FVCopy(FontViewBase *fv, enum fvcopy_type fullcopy) {
    int i, any = false;
    BDFFont *bdf;
    Undoes *head=NULL, *last=NULL, *cur;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;
    int gid;
    SplineChar *sc;
    /* If fullcopy==ct_fullcopy copy the glyph as is. */
    /* If fullcopy==ct_reference put a reference to the glyph in the clipboard */
    /* If fullcopy==ct_lookups put a reference to the glyph in the clip but mark that we are only interested in its lookups */
    /* If fullcopy==ct_unlinkrefs copy the glyph, but unlink any references it contains */
    /*	so we end up with no references and a bunch of splines */

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	any = true;
	sc = (gid = fv->map->map[i])==-1 ? NULL : fv->sf->glyphs[gid];
	if (( onlycopydisplayed && fv->active_bitmap==NULL ) || fullcopy==ct_lookups ) {
	    cur = SCCopyAll(sc,fv->active_layer,fullcopy);
	} else if ( onlycopydisplayed ) {
	    cur = BCCopyAll(gid==-1?NULL:fv->active_bitmap->glyphs[gid],fv->active_bitmap->pixelsize,BDFDepth(fv->active_bitmap),fullcopy);
	} else {
	    state = SCCopyAll(sc,fv->active_layer,fullcopy);
	    bhead = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		BDFChar *bdfc = gid==-1 || gid>=bdf->glyphcnt? NULL : bdf->glyphs[gid];
		bcur = BCCopyAll(bdfc,bdf->pixelsize,BDFDepth(bdf),fullcopy);
		if ( bhead==NULL )
		    bhead = bcur;
		else
		    blast->next = bcur;
		blast = bcur;
	    }
	    if ( bhead!=NULL || state!=NULL ) {
		cur = chunkalloc(sizeof(Undoes));
		cur->undotype = ut_composit;
		cur->u.composit.state = state;
		cur->u.composit.bitmaps = bhead;
	    } else
		cur = NULL;
	}
	if ( cur!=NULL ) {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }

    if ( !any )
	fprintf( stderr, "No selection\n" );

    if ( head==NULL )
return;
    CopyBufferFreeGrab();
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = head;
    copybuffer.copied_from = fv->sf;

    XClipCheckEps();
}

void MVCopyChar(FontViewBase *fv, BDFFont *mvbdf, SplineChar *sc, enum fvcopy_type fullcopy) {
    BDFFont *bdf;
    Undoes *cur=NULL;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;

    if (( onlycopydisplayed && mvbdf==NULL ) || fullcopy == ct_lookups ) {
	cur = SCCopyAll(sc,fv->active_layer,fullcopy);
    } else if ( onlycopydisplayed ) {
	cur = BCCopyAll(BDFMakeGID(mvbdf,sc->orig_pos),mvbdf->pixelsize,BDFDepth(mvbdf),fullcopy);
    } else {
	state = SCCopyAll(sc,fv->active_layer,fullcopy);
	bhead = NULL;
	for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    bcur = BCCopyAll(BDFMakeGID(bdf,sc->orig_pos),bdf->pixelsize,BDFDepth(bdf),fullcopy);
	    if ( bhead==NULL )
		bhead = bcur;
	    else
		blast->next = bcur;
	    blast = bcur;
	}
	if ( bhead!=NULL || state!=NULL ) {
	    cur = chunkalloc(sizeof(Undoes));
	    cur->undotype = ut_composit;
	    cur->u.composit.state = state;
	    cur->u.composit.bitmaps = bhead;
	} else
	    cur = NULL;
    }

    if ( cur==NULL )
return;
    CopyBufferFreeGrab();
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = cur;

    XClipCheckEps();
}

static BDFFont *BitmapCreateCheck(FontViewBase *fv,int *yestoall, int first, int pixelsize, int depth) {
    int yes = 0;
    BDFFont *bdf = NULL;

    if ( *yestoall>0 && first ) {
	char *buts[5];
	char buf[20];
	if ( depth!=1 )
	    sprintf( buf, "%d@%d", pixelsize, depth );
	else
	    sprintf( buf, "%d", pixelsize );
	buts[0] = _("_Yes");
	buts[1] = _("Yes to _All");
	buts[2] = _("No _to All");
	buts[3] = _("_No");
	buts[4] = NULL;
	yes = ff_ask(_("Bitmap Paste"),(const char **) buts,0,3,"The clipboard contains a bitmap character of size %s,\na size which is not in your database.\nWould you like to create a bitmap font of that size,\nor ignore this character?",buf);
	if ( yes==1 )
	    *yestoall = true;
	else if ( yes==2 )
	    *yestoall = -1;
	else
	    yes= yes!=3;
    }
    if ( yes==1 || *yestoall ) {
	void *freetypecontext = FreeTypeFontContext(fv->sf,NULL,NULL,fv->active_layer);
	if ( freetypecontext )
	    bdf = SplineFontFreeTypeRasterize(freetypecontext,pixelsize,depth);
	else
	    bdf = SplineFontAntiAlias(fv->sf,fv->active_layer,pixelsize,1<<(depth/2));
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	SFOrderBitmapList(fv->sf);
    }
return( bdf );
}

static int copybufferHasLookups(Undoes *cb) {
    if ( cb->undotype==ut_multiple )
	cb = cb->u.multiple.mult;
return( cb->undotype==ut_statelookup );
}

void PasteIntoFV(FontViewBase *fv,int pasteinto,real trans[6]) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int i, j, cnt=0, gid;
    int yestoall=0, first=true;
    uint8 *oldsel = fv->selected;
    SplineFont *sf = fv->sf, *origsf = sf;
    MMSet *mm = sf->mm;
    struct sfmergecontext mc;
    OTLookup **list = NULL, **backpairlist=NULL;
    int refstate = 0, already_complained = 0;

    memset(&mc,0,sizeof(mc));
    mc.sf_to = fv->sf; mc.sf_from = copybuffer.copied_from;

    cur = &copybuffer;
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
	++cnt;
    if ( cnt==0 ) {
	fprintf( stderr, "No Selection\n" );
return;
    }

    if ( copybufferHasLookups(&copybuffer)) {
	list = GetLookupsToCopy(copybuffer.copied_from,&backpairlist);
	if ( list==NULL )
return;
    }

    if ( copybuffer.undotype == ut_none && ClipboardHasType("application/x-font-svg"))
	XClipFontToFFClip();

    if ( copybuffer.undotype == ut_none ) {
	j = -1;
	while ( 1 ) {
	    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
		SCCheckXClipboard(SFMakeChar(sf,fv->map,i),ly_fore,!pasteinto);
	    ++j;
	    if ( mm==NULL || mm->normal!=origsf || j>=mm->instance_count )
	break;
	    sf = mm->instances[j];
	}
return;
    }

    /* If they select exactly one character but there are more things in the */
    /*  copy buffer, then temporarily change the selection so that everything*/
    /*  in the copy buffer gets pasted (into chars immediately following sele*/
    /*  cted one (unless we run out of chars...)) */
    if ( cnt==1 && cur->undotype==ut_multiple && cur->u.multiple.mult->next!=NULL ) {
	Undoes *tot; int j;
	for ( cnt=0, tot=cur->u.multiple.mult; tot!=NULL; ++cnt, tot=tot->next );
	fv->selected = malloc(fv->map->enccount);
	memcpy(fv->selected,oldsel,fv->map->enccount);
	for ( i=0; i<fv->map->enccount && !fv->selected[i]; ++i );
	for ( j=0; j<cnt && i+j<fv->map->enccount; ++j )
	    fv->selected[i+j] = 1;
	cnt = j;
    }

    anchor_lost_warning = false;
    ff_progress_start_indicator(10,_("Pasting..."),_("Pasting..."),0,cnt,1);

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    /* This little gem of code is to keep us from throwing out forward */
    /*  references. Say we are pasting into both "i" and dotlessi (and */
    /*  dotlessi isn't defined yet) without this the paste to "i" will */
    /*  search for dotlessi, not find it and ignore the reference */
    if ( cur->undotype==ut_state || cur->undotype==ut_statehint || cur->undotype==ut_statename ||
	    (cur->undotype==ut_composit && cur->u.composit.state!=NULL)) {
	for ( i=0; i<fv->map->enccount; ++i )
	    if ( fv->selected[i] && ((gid = fv->map->map[i])==-1 || sf->glyphs[gid]==NULL ))
		SFMakeChar(sf,fv->map,i);
    }
    cur = NULL;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	j=-1;
	if ( cur==NULL ) {
	    cur = &copybuffer;
	    if ( cur->undotype==ut_multiple )
		cur = cur->u.multiple.mult;
	}
	while ( 1 ) {
	    switch ( cur->undotype ) {
	      case ut_noop:
	      break;
	      case ut_statelookup:
		SCPasteLookupsMid(SFMakeChar(sf,fv->map,i),cur,list,backpairlist,&mc);
	      break;
	      case ut_state: case ut_width: case ut_vwidth:
	      case ut_lbearing: case ut_rbearing:
	      case ut_statehint: case ut_statename:
	      case ut_layers: case ut_anchors:
		if ( !sf->hasvmetrics && cur->undotype==ut_vwidth) {
		    ff_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
 goto err;
		}
		PasteToSC(SFMakeChar(sf,fv->map,i),fv->active_layer,cur,fv,pasteinto,trans,&mc,&refstate,&already_complained);
	      break;
	      case ut_bitmapsel: case ut_bitmap:
		if ( onlycopydisplayed && fv->active_bitmap!=NULL )
		    _PasteToBC(BDFMakeChar(fv->active_bitmap,fv->map,i),fv->active_bitmap->pixelsize,BDFDepth(fv->active_bitmap),cur,!pasteinto);
		else {
		    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth); bdf=bdf->next );
		    if ( bdf==NULL ) {
			bdf = BitmapCreateCheck(fv,&yestoall,first,cur->u.bmpstate.pixelsize,cur->u.bmpstate.depth);
			first = false;
		    }
		    if ( bdf!=NULL )
			_PasteToBC(BDFMakeChar(bdf,fv->map,i),bdf->pixelsize,BDFDepth(bdf),cur,!pasteinto);
		}
	      break;
	      case ut_composit:
		if ( cur->u.composit.state!=NULL )
		    PasteToSC(SFMakeChar(sf,fv->map,i),fv->active_layer,cur->u.composit.state,fv,pasteinto,trans,&mc,&refstate,&already_complained);
		for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
		    for ( bdf=sf->bitmaps; bdf!=NULL &&
			    (bdf->pixelsize!=bmp->u.bmpstate.pixelsize || BDFDepth(bdf)!=bmp->u.bmpstate.depth);
			    bdf=bdf->next );
		    if ( bdf==NULL )
			bdf = BitmapCreateCheck(fv,&yestoall,first,bmp->u.bmpstate.pixelsize,bmp->u.bmpstate.depth);
		    if ( bdf!=NULL )
			_PasteToBC(BDFMakeChar(bdf,fv->map,i),bdf->pixelsize,BDFDepth(bdf),bmp,!pasteinto);
		}
		first = false;
	      break;
	      default:
	      break;
	    }
	    ++j;
	    if ( mm==NULL || mm->normal!=origsf || j>=mm->instance_count )
	break;
	    sf = mm->instances[j];
	}
	cur = cur->next;
	if ( !ff_progress_next())
    break;
    }
    /* If we copy glyphs from one font to another, and if some of those glyphs*/
    /*  contain references, and the width of the original glyph is the same as*/
    /*  the width of the original referred character, then we should make sure */
    /*  that the new width of the glyph is the same as the current width of   */
    /*  the referred char. We can't do this earlier because of foreward refs.  */
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc = SFMakeChar(sf,fv->map,i);
	if ( sc->layers[ly_fore].refs!=NULL && sc->layers[ly_fore].splines==NULL ) {
	    RefChar *r;
	    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
		if ( r->use_my_metrics ) {
		    sc->vwidth = r->sc->vwidth;
		    if ( sc->width!=r->sc->width )
			SCSynchronizeWidth(sc,r->sc->width,sc->width,NULL);
		}
	    }
	}
    }
 err:
    ff_progress_end_indicator();
    if ( oldsel!=fv->selected )
	free(oldsel);
    SFFinishMergeContext(&mc);
    free(list); free(backpairlist);
}

void PasteIntoMV(FontViewBase *fv, BDFFont *mvbdf,SplineChar *sc, int doclear) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int yestoall=0, first=true;
    struct sfmergecontext mc;
    int refstate = 0, already_complained = 0;

    memset(&mc,0,sizeof(mc));
    mc.sf_to = fv->sf;

    cur = &copybuffer;

    if ( copybuffer.undotype == ut_none ) {
	SCCheckXClipboard(sc,ly_fore,doclear);
return;
    }

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    switch ( cur->undotype ) {
      case ut_noop:
      break;
      case ut_state: case ut_width: case ut_vwidth:
      case ut_lbearing: case ut_rbearing:
      case ut_statehint: case ut_statename:
	if ( !fv->sf->hasvmetrics && cur->undotype==ut_vwidth) {
	    ff_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
return;
	}
	PasteToSC(sc,fv->active_layer,cur,fv,!doclear,NULL,&mc,&refstate,&already_complained);
      break;
      case ut_bitmapsel: case ut_bitmap:
	if ( onlycopydisplayed && mvbdf!=NULL )
	    _PasteToBC(BDFMakeChar(mvbdf,fv->map,fv->map->backmap[sc->orig_pos]),mvbdf->pixelsize,BDFDepth(mvbdf),cur,doclear);
	else {
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL &&
		    (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth);
		    bdf=bdf->next );
	    if ( bdf==NULL ) {
		bdf = BitmapCreateCheck(fv,&yestoall,first,cur->u.bmpstate.pixelsize,cur->u.bmpstate.depth);
		first = false;
	    }
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,fv->map,fv->map->backmap[sc->orig_pos]),bdf->pixelsize,BDFDepth(bdf),cur,doclear);
	}
      break;
      case ut_composit:
	if ( cur->u.composit.state!=NULL )
	    PasteToSC(sc,fv->active_layer,cur->u.composit.state,fv,!doclear,NULL,&mc,&refstate,&already_complained);
	for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL &&
		    (bdf->pixelsize!=bmp->u.bmpstate.pixelsize || BDFDepth(bdf)!=bmp->u.bmpstate.depth);
		    bdf=bdf->next );
	    if ( bdf==NULL )
		bdf = BitmapCreateCheck(fv,&yestoall,first,bmp->u.bmpstate.pixelsize,bmp->u.bmpstate.depth);
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,fv->map,fv->map->backmap[sc->orig_pos]),bdf->pixelsize,BDFDepth(bdf),bmp,doclear);
	}
	first = false;
      break;
      default:
      break;
    }
    SFFinishMergeContext(&mc);
}

/* Look through the copy buffer. If it wasn't copied from the given font, then */
/*  we can stop. Otherwise: */
/*	If "from" is NULL, then remove all anchorpoints from the buffer */
/*	If "into" is NULL, then remove all anchorpoints with class "from" */
/*	Else replace the anchor class of all anchorpoints with class "from" with "info" */
static void _PasteAnchorClassManip(SplineFont *sf,AnchorClass *into,AnchorClass *from) {
    Undoes *cur = &copybuffer, *temp;

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    while ( cur!=NULL ) {
	temp = cur;
	switch ( temp->undotype ) {
	  case ut_composit:
	    if ( temp->u.composit.state==NULL )
	break;
	    temp = temp->u.composit.state;
	    /* Fall through */;
	  case ut_state: case ut_statehint: case ut_statename:
	    if ( temp->copied_from!=sf )
return;
	    if ( from==NULL ) {
		AnchorPointsFree(temp->u.state.anchor);
		temp->u.state.anchor = NULL;
	    } else
		temp->u.state.anchor = APAnchorClassMerge(temp->u.state.anchor,into,from);
	  break;
	  default:
	  break;
	}
	cur=cur->next;
    }
}

void PasteRemoveSFAnchors(SplineFont *sf) {
    _PasteAnchorClassManip(sf,NULL,NULL);
}

void PasteAnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from) {
    _PasteAnchorClassManip(sf,into,from);
}

void PasteRemoveAnchorClass(SplineFont *sf,AnchorClass *dying) {
    _PasteAnchorClassManip(sf,NULL,dying);
}

char* UndoToString( SplineChar* sc, Undoes *undo )
{
    int idx = 0;
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "/tmp/fontforge-undo-to-string.sfd");
    FILE* f = fopen( filename, "w" );
    SFDDumpUndo( f, sc, undo, "Undo", idx );
    fclose(f);
    char* sfd = GFileReadAll( filename );
    return sfd;
}

void dumpUndoChain( char* msg, SplineChar* sc, Undoes *undo )
{
    int idx = 0;

    printf("dumpUndoChain(start) %s\n", msg );
    for( ; undo; undo = undo->next, idx++ )
    {
	char* str = UndoToString( sc, undo );
	printf("\n\n*** undo: %d\n%s\n", idx, str );
    }
    printf("dumpUndoChain(end) %s\n", msg );
}


