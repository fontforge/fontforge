/* Copyright (C) 2000-2002 by George Williams */
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
#include <math.h>
#include <ustring.h>

/* ********************************* Undoes ********************************* */

static int maxundoes = 12;		/* -1 is infinite */

static uint8 *bmpcopy(uint8 *bitmap,int bytes_per_line, int lines) {
    uint8 *ret = galloc(bytes_per_line*lines);
    memcpy(ret,bitmap,bytes_per_line*lines);
return( ret );
}

static RefChar *RefCharsCopyState(SplineChar *sc) {
    RefChar *head=NULL, *last=NULL, *new, *crefs;

    if ( sc->refs==NULL )
return( NULL );
    for ( crefs = sc->refs; crefs!=NULL; crefs=crefs->next ) {
	new = chunkalloc(sizeof(RefChar));
	*new = *crefs;
	new->splines = NULL;
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

static ImageList *SCImagesCopyState(SplineChar *sc) {
    ImageList *head=NULL, *last=NULL, *new, *cimg;

    if ( sc->backimages==NULL )
return( NULL );
    for ( cimg = sc->backimages; cimg!=NULL; cimg=cimg->next ) {
	new = chunkalloc(sizeof(ImageList));
	*new = *cimg;
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

static ImageList *ImagesCopyState(CharView *cv) {

    if ( cv->drawmode!=dm_back || cv->sc->backimages==NULL )
return( NULL );
return( SCImagesCopyState(cv->sc));
}

static MinimumDistance *MDsCopyState(SplineChar *sc,SplineSet *rpl) {
    MinimumDistance *head=NULL, *last, *md, *cur;

    if ( sc->md==NULL )
return( NULL );

    for ( md = sc->md; md!=NULL; md=md->next ) {
	cur = chunkalloc(sizeof(MinimumDistance));
	*cur = *md;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    cur->next = NULL;

    MDReplace(head,sc->splines,rpl);
return( head );
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

static void FixupRefChars(SplineChar *sc,RefChar *urefs) {
    RefChar *crefs = sc->refs, *cend, *cprev, *unext, *cnext;

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
		SCRemoveDependent(sc,crefs);
		crefs = cnext;
	    }
	} else { /* urefs isn't on the list. Add it here */
	    unext = urefs->next;
	    urefs->next = crefs;
	    if ( cprev==NULL )
		sc->refs = urefs;
	    else
		cprev->next = urefs;
	    cprev = urefs;
	    SCReinstanciateRefChar(sc,urefs);
	    SCMakeDependent(sc,urefs->sc);
	    urefs = unext;
	}
    }
    if ( crefs!=NULL ) {
	while ( crefs!=NULL ) {
	    cnext = crefs->next;
	    SCRemoveDependent(sc,crefs);
	    crefs = cnext;
	}
    } else if ( urefs!=NULL ) {
	if ( cprev==NULL )
	    sc->refs = urefs;
	else
	    cprev->next = urefs;
	while ( urefs!=NULL ) {
	    SCReinstanciateRefChar(sc,urefs);
	    SCMakeDependent(sc,urefs->sc);
	    urefs = urefs->next;
	}
    }
}

static void FixupImages(SplineChar *sc,ImageList *uimgs) {
    ImageList *cimgs = sc->backimages, *cend, *cprev, *unext, *cnext;

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
		sc->backimages = cend;
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
		sc->backimages = uimgs;
	    else
		cprev->next = uimgs;
	    cprev = uimgs;
	    uimgs = unext;
	}
    }
    if ( cimgs!=NULL ) {
	ImageListsFree(cimgs);
	if ( cprev==NULL )
	    sc->backimages = NULL;
	else
	    cprev->next = NULL;
    } else if ( uimgs!=NULL ) {
	if ( cprev==NULL )
	    sc->backimages = uimgs;
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

static void *UHintCopy(SplineChar *sc,int docopy) {
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

static void ExtractHints(SplineChar *sc,void *hints,int docopy) {
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

void UndoesFree(Undoes *undo) {
    Undoes *unext;

    while ( undo!=NULL ) {
	unext = undo->next;
	switch ( undo->undotype ) {
	  case ut_noop:
	  case ut_width: case ut_vwidth: case ut_lbearing: case ut_rbearing:
	    /* Nothing else to free */;
	  break;
	  case ut_state: case ut_tstate: case ut_statehint: case ut_statename:
	    SplinePointListsFree(undo->u.state.splines);
	    RefCharsFree(undo->u.state.refs);
	    MinimumDistancesFree(undo->u.state.md);
	    if ( undo->undotype==ut_statehint || undo->undotype==ut_statename )
		UHintListFree(undo->u.state.u.hints);
	    else
		ImageListsFree(undo->u.state.u.images);
	    if ( undo->undotype==ut_statename ) {
		free( undo->u.state.charname );
		free( undo->u.state.lig );
	    }
	  break;
	  case ut_bitmap:
	    free(undo->u.bmpstate.bitmap);
	    BDFFloatFree(undo->u.bmpstate.selection);
	  break;
	  case ut_composit:
	    UndoesFree(undo->u.composit.state);
	    UndoesFree(undo->u.composit.bitmaps);
	  break;
	  default:
	    GDrawIError( "Unknown undo type in UndoesFree: %d", undo->undotype );
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

static Undoes *CVAddUndo(CharView *cv,Undoes *undo) {
return( AddUndo(undo,cv->uheads[cv->drawmode],cv->rheads[cv->drawmode]));
}

Undoes *CVPreserveState(CharView *cv) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->u.state.width = cv->sc->width;
    undo->u.state.vwidth = cv->sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(*cv->heads[cv->drawmode]);
    if ( cv->drawmode==dm_fore ) {
	undo->u.state.refs = RefCharsCopyState(cv->sc);
	undo->u.state.md = MDsCopyState(cv->sc,undo->u.state.splines);
    }
    undo->u.state.u.images = ImagesCopyState(cv);
return( CVAddUndo(cv,undo));
}

Undoes *SCPreserveState(SplineChar *sc,int dohints) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->u.state.width = sc->width;
    undo->u.state.vwidth = sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(sc->splines);
    undo->u.state.refs = RefCharsCopyState(sc);
    undo->u.state.md = MDsCopyState(sc,undo->u.state.splines);
    undo->u.state.u.images = NULL;
    if ( dohints ) {
	undo->undotype = ut_statehint;
	undo->u.state.u.hints = UHintCopy(sc,true);
    }
    undo->u.state.copied_from = sc->parent;
return( AddUndo(undo,&sc->undoes[0],&sc->redoes[0]));
}

Undoes *SCPreserveBackground(SplineChar *sc) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->u.state.width = sc->width;
    undo->u.state.vwidth = sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(sc->backgroundsplines);
    undo->u.state.u.images = SCImagesCopyState(sc);
    undo->u.state.copied_from = sc->parent;
return( AddUndo(undo,&sc->undoes[1],&sc->redoes[1]));
}

void SCUndoSetLBearingChange(SplineChar *sc,int lbc) {
    Undoes *undo = sc->undoes[0];

    if ( undo==NULL || undo->undotype != ut_state )
return;
    undo->u.state.lbearingchange = lbc;
}

Undoes *CVPreserveTState(CharView *cv) {
    Undoes *undo;
    int anyrefs;
    RefChar *refs, *urefs;

    cv->p.transany = CVAnySel(cv,NULL,&anyrefs,NULL);
    cv->p.transanyrefs = anyrefs;

    undo = CVPreserveState(cv);
    if ( !cv->p.transany || cv->p.transanyrefs ) {
	for ( refs = cv->sc->refs, urefs=undo->u.state.refs; urefs!=NULL; refs=refs->next, urefs=urefs->next )
	    if ( !cv->p.transany || refs->selected )
		urefs->splines = SplinePointListCopy(refs->splines);
    }
    undo->undotype = ut_tstate;

return( undo );
}

Undoes *CVPreserveWidth(CharView *cv,int width) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->u.width = width;
return( CVAddUndo(cv,undo));
}

Undoes *CVPreserveVWidth(CharView *cv,int vwidth) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->u.width = vwidth;
return( CVAddUndo(cv,undo));
}

Undoes *SCPreserveWidth(SplineChar *sc) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->u.state.width = sc->width;
return( AddUndo(undo,&sc->undoes[0],&sc->redoes[0]));
}

Undoes *SCPreserveVWidth(SplineChar *sc) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->u.state.width = sc->vwidth;
return( AddUndo(undo,&sc->undoes[0],&sc->redoes[0]));
}

Undoes *BCPreserveState(BDFChar *bc) {
    Undoes *undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_bitmap;
    /*undo->u.bmpstate.width = bc->width;*/
    undo->u.bmpstate.xmin = bc->xmin;
    undo->u.bmpstate.xmax = bc->xmax;
    undo->u.bmpstate.ymin = bc->ymin;
    undo->u.bmpstate.ymax = bc->ymax;
    undo->u.bmpstate.bytes_per_line = bc->bytes_per_line;
    undo->u.bmpstate.bitmap = bmpcopy(bc->bitmap,bc->bytes_per_line,
	    bc->ymax-bc->ymin+1);
    undo->u.bmpstate.selection = BDFFloatCopy(bc->selection);
return( AddUndo(undo,&bc->undoes,&bc->redoes));
}

static void SCUndoAct(SplineChar *sc,int drawmode, Undoes *undo) {

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
      case ut_state: case ut_tstate: case ut_statehint: {
	SplinePointList **head = drawmode==dm_fore ? &sc->splines :
				 drawmode==dm_back ? &sc->backgroundsplines :
				    &sc->parent->gridsplines;
	SplinePointList *spl = *head;

	if ( drawmode==dm_fore ) {
	    int width = sc->width;
	    int vwidth = sc->vwidth;
	    if ( sc->width!=undo->u.state.width )
		SCSynchronizeWidth(sc,undo->u.state.width,width,NULL);
	    sc->vwidth = undo->u.state.vwidth;
	    undo->u.state.width = width;
	    undo->u.state.vwidth = vwidth;
	}
	*head = undo->u.state.splines;
	if ( drawmode==dm_fore ) {
	    MinimumDistance *md = sc->md;
	    sc->md = undo->u.state.md;
	    undo->u.state.md = md;
	}
	if ( drawmode==dm_fore && !RefCharsMatch(undo->u.state.refs,sc->refs)) {
	    RefChar *refs = RefCharsCopyState(sc);
	    FixupRefChars(sc,undo->u.state.refs);
	    undo->u.state.refs = refs;
	} else if ( drawmode==dm_fore && undo->undotype==ut_statehint ) {
	    void *hints = UHintCopy(sc,false);
	    ExtractHints(sc,undo->u.state.u.hints,false);
	    undo->u.state.u.hints = hints;
	}
	if ( drawmode==dm_back && undo->undotype!=ut_statehint &&
		!ImagesMatch(undo->u.state.u.images,sc->backimages)) {
	    ImageList *images = SCImagesCopyState(sc);
	    FixupImages(sc,undo->u.state.u.images);
	    undo->u.state.u.images = images;
	    SCOutOfDateBackground(sc);
	}
	undo->u.state.splines = spl;
	if ( undo->u.state.lbearingchange ) {
	    undo->u.state.lbearingchange = -undo->u.state.lbearingchange;
	    SCSynchronizeLBearing(sc,NULL,undo->u.state.lbearingchange);
	}
      } break;
      default:
	GDrawIError( "Unknown undo type in SCUndoAct: %d", undo->undotype );
      break;
    }
}

void CVDoUndo(CharView *cv) {
    Undoes *undo = *cv->uheads[cv->drawmode];

    if ( undo==NULL )		/* Shouldn't happen */
return;
    *cv->uheads[cv->drawmode] = undo->next;
    undo->next = NULL;
    SCUndoAct(cv->sc,cv->drawmode,undo);
    undo->next = *cv->rheads[cv->drawmode];
    *cv->rheads[cv->drawmode] = undo;
    CVCharChangedUpdate(cv);
    cv->lastselpt = NULL;
return;
}

void CVDoRedo(CharView *cv) {
    Undoes *undo = *cv->rheads[cv->drawmode];

    if ( undo==NULL )		/* Shouldn't happen */
return;
    *cv->rheads[cv->drawmode] = undo->next;
    undo->next = NULL;
    SCUndoAct(cv->sc,cv->drawmode,undo);
    undo->next = *cv->uheads[cv->drawmode];
    *cv->uheads[cv->drawmode] = undo;
    CVCharChangedUpdate(cv);
    cv->lastselpt = NULL;
return;
}

void SCDoUndo(SplineChar *sc,int drawmode) {
    Undoes *undo = sc->undoes[drawmode];

    if ( drawmode!=dm_fore && drawmode!=dm_back ) {
	GDrawIError( "Unsupported drawmode in SCDoUndo");
return;
    }

    if ( undo==NULL )		/* Shouldn't happen */
return;
    sc->undoes[drawmode] = undo->next;
    undo->next = NULL;
    SCUndoAct(sc,drawmode,undo);
    undo->next = sc->redoes[drawmode];
    sc->redoes[drawmode] = undo;
    SCCharChangedUpdate(sc);
return;
}

void SCDoRedo(SplineChar *sc, int drawmode) {
    Undoes *undo = sc->redoes[drawmode];

    if ( drawmode!=dm_fore && drawmode!=dm_back ) {
	GDrawIError( "Unsupported drawmode in SCDoUndo");
return;
    }

    if ( undo==NULL )		/* Shouldn't happen */
return;
    sc->redoes[drawmode] = undo->next;
    undo->next = NULL;
    SCUndoAct(sc,drawmode,undo);
    undo->next = sc->undoes[drawmode];
    sc->undoes[drawmode] = undo;
    SCCharChangedUpdate(sc);
return;
}

/* Used when doing incremental transformations. If I just keep doing increments*/
/*  then rounding errors will mount. Instead I go back to the original state */
/*  each time */
void CVRestoreTOriginalState(CharView *cv) {
    Undoes *undo = *cv->uheads[cv->drawmode];
    RefChar *ref, *uref;
    ImageList *img, *uimg;

    SplinePointListSet(*cv->heads[cv->drawmode],undo->u.state.splines);
    if ( cv->drawmode==dm_fore && (!cv->p.anysel || cv->p.transanyrefs)) {
	for ( ref=cv->sc->refs, uref=undo->u.state.refs; uref!=NULL; ref=ref->next, uref=uref->next )
	    if ( uref->splines!=NULL ) {
		SplinePointListSet(ref->splines,uref->splines);
		memcpy(&ref->transform,&uref->transform,sizeof(ref->transform));
	    }
    }
    if ( cv->drawmode==dm_back ) {
	for ( img=cv->sc->backimages, uimg=undo->u.state.u.images; uimg!=NULL;
		img = img->next, uimg = uimg->next ) {
	    img->xoff = uimg->xoff;
	    img->yoff = uimg->yoff;
	    img->xscale = uimg->xscale;
	    img->yscale = uimg->yscale;
	}
    }
}

void CVUndoCleanup(CharView *cv) {
    Undoes * undo = *cv->uheads[cv->drawmode];
    RefChar *uref;

    if ( cv->drawmode==dm_fore && (!cv->p.anysel || cv->p.transanyrefs)) {
	for ( uref=undo->u.state.refs; uref!=NULL; uref=uref->next ) {
	    SplinePointListFree(uref->splines);
	    uref->splines = NULL;
	}
    }
    undo->undotype = ut_state;
}

void CVRemoveTopUndo(CharView *cv) {
    Undoes * undo = *cv->uheads[cv->drawmode];

    *cv->uheads[cv->drawmode] = undo->next;
    undo->next = NULL;
    UndoesFree(undo);
}

static void BCUndoAct(BDFChar *bc,Undoes *undo) {

    switch ( undo->undotype ) {
      case ut_bitmap: {
	uint8 *b;
	int temp;
	BDFFloat *sel;
	/*temp = bc->width; bc->width = undo->u.bmpstate.width; undo->u.bmpstate.width = temp;*/
	temp = bc->xmin; bc->xmin = undo->u.bmpstate.xmin; undo->u.bmpstate.xmin = temp;
	temp = bc->xmax; bc->xmax = undo->u.bmpstate.xmax; undo->u.bmpstate.xmax = temp;
	temp = bc->ymin; bc->ymin = undo->u.bmpstate.ymin; undo->u.bmpstate.ymin = temp;
	temp = bc->ymax; bc->ymax = undo->u.bmpstate.ymax; undo->u.bmpstate.ymax = temp;
	temp = bc->bytes_per_line; bc->bytes_per_line = undo->u.bmpstate.bytes_per_line; undo->u.bmpstate.bytes_per_line = temp;
	b = bc->bitmap; bc->bitmap = undo->u.bmpstate.bitmap; undo->u.bmpstate.bitmap = b;
	sel = bc->selection; bc->selection = undo->u.bmpstate.selection; undo->u.bmpstate.selection = sel;
      } break;
      default:
	GDrawIError( "Unknown undo type in BCUndoAct: %d", undo->undotype );
      break;
    }
}

void BCDoUndo(BDFChar *bc,FontView *fv) {
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

void BCDoRedo(BDFChar *bc,FontView *fv) {
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

static void CopyBufferFree(void) {

    switch( copybuffer.undotype ) {
      case ut_state: case ut_statehint:
	SplinePointListsFree(copybuffer.u.state.splines);
	RefCharsFree(copybuffer.u.state.refs);
	if ( copybuffer.undotype==ut_statehint )
	    UHintListFree(copybuffer.u.state.u.hints);
	else
	    ImageListsFree(copybuffer.u.state.u.images);
      break;
      case ut_bitmapsel:
	BDFFloatFree(copybuffer.u.bmpstate.selection);
      break;
      case ut_multiple:
	UndoesFree( copybuffer.u.multiple.mult );
      break;
    }
    memset(&copybuffer,'\0',sizeof(copybuffer));
    copybuffer.undotype = ut_none;
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

return( cur->undotype==ut_state || cur->undotype==ut_tstate ||
	cur->undotype==ut_width || cur->undotype==ut_vwidth ||
	cur->undotype==ut_lbearing || cur->undotype==ut_rbearing ||
	cur->undotype==ut_noop );
}

int CopyContainsBitmap(void) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    if ( cur->undotype==ut_composit )
return( cur->u.composit.bitmaps!=NULL );

return( cur->undotype==ut_bitmapsel || cur->undotype==ut_noop );
}

int getAdobeEnc(char *name) {
    extern char *AdobeStandardEncoding[256];
    int i;

    for ( i=0; i<256; ++i )
	if ( strcmp(name,AdobeStandardEncoding[i])==0 )
    break;
    if ( i==256 ) i = -1;
return( i );
}

void CopyReference(SplineChar *sc) {
    RefChar *ref;

    CopyBufferFree();

    copybuffer.undotype = ut_state;
    copybuffer.u.state.width = sc->width;
    copybuffer.u.state.vwidth = sc->vwidth;
    copybuffer.u.state.refs = ref = chunkalloc(sizeof(RefChar));
    ref->unicode_enc = sc->unicodeenc;
    ref->local_enc = sc->enc;
    ref->adobe_enc = getAdobeEnc(sc->name);
    ref->transform[0] = ref->transform[3] = 1.0;
}

void CopySelected(CharView *cv) {

    CopyBufferFree();

    copybuffer.undotype = ut_state;
    copybuffer.u.state.width = cv->sc->width;
    copybuffer.u.state.vwidth = cv->sc->vwidth;
    copybuffer.u.state.splines = SplinePointListCopySelected(*cv->heads[cv->drawmode]);
    if ( cv->drawmode==dm_fore ) {
	RefChar *refs, *new;
	for ( refs = cv->sc->refs; refs!=NULL; refs = refs->next ) if ( refs->selected ) {
	    new = chunkalloc(sizeof(RefChar));
	    *new = *refs;
	    new->splines = NULL;
	    new->local_enc = new->sc->enc;
	    new->sc = NULL;
	    new->next = copybuffer.u.state.refs;
	    copybuffer.u.state.refs = new;
	}
    }
    if ( cv->drawmode==dm_back ) {
	ImageList *imgs, *new;
	for ( imgs = cv->sc->backimages; imgs!=NULL; imgs = imgs->next ) if ( imgs->selected ) {
	    new = chunkalloc(sizeof(ImageList));
	    *new = *imgs;
	    new->next = copybuffer.u.state.u.images;
	    copybuffer.u.state.u.images = new;
	}
    }
    copybuffer.u.state.copied_from = cv->sc->parent;
}

static Undoes *SCCopyAll(SplineChar *sc,int full) {
    Undoes *cur;
    RefChar *ref;
    extern int copymetadata;

    cur = chunkalloc(sizeof(Undoes));
    if ( sc==NULL ) {
	cur->undotype = ut_noop;
    } else {
	cur->u.state.width = sc->width;
	cur->u.state.vwidth = sc->vwidth;
	if ( full ) {
	    cur->undotype = copymetadata ? ut_statename : ut_statehint;
	    cur->u.state.splines = SplinePointListCopy(sc->splines);
	    cur->u.state.refs = RefCharsCopyState(sc);
	    cur->u.state.u.hints = UHintCopy(sc,true);
	    cur->u.state.unicodeenc = sc->unicodeenc;
	    cur->u.state.charname = copymetadata ? copy(sc->name) : NULL;
	    cur->u.state.lig = copymetadata && sc->lig? copy(sc->lig->components) : NULL;
	} else {		/* Or just make a reference */
	    cur->undotype = ut_state;
	    cur->u.state.refs = ref = chunkalloc(sizeof(RefChar));
	    ref->unicode_enc = sc->unicodeenc;
	    ref->local_enc = sc->enc;
	    ref->adobe_enc = getAdobeEnc(sc->name);
	    ref->transform[0] = ref->transform[3] = 1.0;
	}
	cur->u.state.copied_from = sc->parent;
    }
return( cur );
}

void SCCopyWidth(SplineChar *sc,enum undotype ut) {
    DBounds bb;

    CopyBufferFree();

    copybuffer.undotype = ut;
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
    }
}

void CopyWidth(CharView *cv,enum undotype ut) {
    SCCopyWidth(cv->sc,ut);
}

static SplineChar *FindCharacter(SplineFont *sf,RefChar *rf) {
    extern char *AdobeStandardEncoding[256];
    int i;

    if ( rf->local_enc<sf->charcnt && sf->chars[rf->local_enc]!=NULL &&
	    sf->chars[rf->local_enc]->unicodeenc == rf->unicode_enc )
return( sf->chars[rf->local_enc] );

    if ( rf->unicode_enc!=-1 ) {
	for ( i=0; i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc == rf->unicode_enc )
return( sf->chars[i] );
    }
    if ( rf->adobe_enc!=-1 ) {
	for ( i=0; i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL &&
		    strcmp(sf->chars[i]->name,AdobeStandardEncoding[rf->adobe_enc])==0 )
return( sf->chars[i] );
    }
    if ( rf->local_enc<sf->charcnt )
return( sf->chars[rf->local_enc] );

return( NULL );
}

int SCDependsOnSC(SplineChar *parent, SplineChar *child) {
    RefChar *ref;

    if ( parent==child )
return( true );
    for ( ref=parent->refs; ref!=NULL; ref=ref->next ) {
	if ( SCDependsOnSC(ref->sc,child))
return( true );
    }
return( false );
}

static void PasteNonExistantRefCheck(SplineChar *sc,Undoes *paster,RefChar *ref,
	int *refstate) {
    FontView *fv;
    SplineChar *rsc=NULL;
    SplineSet *new, *spl;
    int yes = 3;

    for ( fv = fv_list; fv!=NULL && fv->sf!=paster->u.state.copied_from; fv=fv->next );
    if ( fv!=NULL ) {
	rsc = FindCharacter(fv->sf,ref);
	if ( rsc==NULL )
	    fv = NULL;
    }
    if ( fv==NULL ) {
	if ( !(*refstate&0x4) ) {
	    static int buts[] = { _STR_DontWarnAgain, _STR_OK, 0 };
	    char buf[10]; const char *name;
	    if ( ref->unicode_enc==-1 )
		name = "<Unknown>";
	    else if ( psunicodenames[ref->unicode_enc]!=NULL )
		name = psunicodenames[ref->unicode_enc];
	    else {
		sprintf( buf, "uni%04X", ref->unicode_enc );
		name = buf;
	    }
	    yes = GWidgetAskCenteredR(_STR_BadReference,buts,1,1,_STR_FontNoRefNoOrig,name,sc->name);
	    if ( yes==0 )
		*refstate |= 0x4;
	}
    } else {
	if ( !(*refstate&0x3) ) {
	    static int buts[] = { _STR_Yes, _STR_YesToAll, _STR_NoToAll, _STR_No, 0 };
	    yes = GWidgetAskCenteredR(_STR_BadReference,buts,0,3,_STR_FontNoRef,rsc->name,sc->name);
	    if ( yes==1 )
		*refstate |= 1;
	    else if ( yes==2 )
		*refstate |= 2;
	}
	if ( (*refstate&1) || yes<=1 ) {
	    new = SplinePointListTransform(SplinePointListCopy(rsc->splines),ref->transform,true);
	    SplinePointListSelect(new,true);
	    if ( new!=NULL ) {
		for ( spl = new; spl->next!=NULL; spl = spl->next );
		spl->next = sc->splines;
		sc->splines = new;
	    }
	}
    }
}

/* when pasting from the fontview we do a clear first */
static void PasteToSC(SplineChar *sc,Undoes *paster,FontView *fv,int doclear) {
    DBounds bb;
    real transform[6];

    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_state: case ut_statehint: case ut_statename:
	sc->parent->onlybitmaps = false;
	SCPreserveState(sc,paster->undotype==ut_statehint);
	SCSynchronizeWidth(sc,paster->u.state.width,sc->width,fv);
	sc->vwidth = paster->u.state.vwidth;
	if ( doclear ) {
	    SplinePointListsFree(sc->splines);
	    sc->splines = NULL;
	    SCRemoveDependents(sc);
	}
	if ( paster->u.state.splines!=NULL )
	    sc->splines = SplinePointListCopy(paster->u.state.splines);
	/* Ignore any images, can't be in foreground level */
	/* but might be hints */
	if ( paster->undotype==ut_statehint || paster->undotype==ut_statename )
	    ExtractHints(sc,paster->u.state.u.hints,true);
	if ( paster->undotype==ut_statename )
	    SCSetMetaData(sc,paster->u.state.charname,
		    paster->u.state.unicodeenc==0xffff?-1:paster->u.state.unicodeenc,
		    paster->u.state.lig);
	if ( paster->u.state.refs!=NULL ) {
	    RefChar *new, *refs;
	    SplineChar *rsc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( sc->searcherdummy )
		    rsc = FindCharacter(sc->views->searcher->fv->sf,refs);
		else
		    rsc = FindCharacter(sc->parent,refs);
		if ( rsc!=NULL && SCDependsOnSC(rsc,sc))
		    GWidgetErrorR(_STR_SelfRef,_STR_AttemptSelfRef);
		else if ( rsc!=NULL ) {
		    new = chunkalloc(sizeof(RefChar));
		    *new = *refs;
		    new->splines = NULL;
		    new->sc = rsc;
		    new->next = sc->refs;
		    sc->refs = new;
		    SCReinstanciateRefChar(sc,new);
		    SCMakeDependent(sc,rsc);
		} else {
		    int refstate = fv->refstate;
		    PasteNonExistantRefCheck(sc,paster,refs,&refstate);
		    fv->refstate = refstate;
		}
	    }
	}
	SCCharChangedUpdate(sc);
      break;
      case ut_width:
	SCPreserveWidth(sc);
	SCSynchronizeWidth(sc,paster->u.width,sc->width,fv);
	SCCharChangedUpdate(sc);
      break;
      case ut_vwidth:
	if ( !sc->parent->hasvmetrics )
	    GWidgetErrorR(_STR_NoVerticalMetrics,_STR_FontNoVerticalMetrics);
	else {
	    SCPreserveVWidth(sc);
	    sc->vwidth = paster->u.width;
	    SCCharChangedUpdate(sc);
	}
      break;
      case ut_rbearing:
	SCPreserveWidth(sc);
	SplineCharFindBounds(sc,&bb);
	SCSynchronizeWidth(sc,bb.maxx + paster->u.rbearing,sc->width,fv);
	SCCharChangedUpdate(sc);
      break;
      case ut_lbearing:
	SplineCharFindBounds(sc,&bb);
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	transform[4] = paster->u.lbearing-bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(fv,sc,transform,NULL,false);
	/* FVTrans will preserver the state and update the chars */
      break;
    }
}

static void _PasteToCV(CharView *cv,Undoes *paster) {
    int refstate = 0;
    DBounds bb;
    real transform[6];

    cv->lastselpt = NULL;
    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_state: case ut_statehint:
	if ( cv->drawmode==dm_fore && cv->sc->splines==NULL && cv->sc->refs==NULL ) {
	    SCSynchronizeWidth(cv->sc,paster->u.state.width,cv->sc->width,NULL);
	    cv->sc->vwidth = paster->u.state.vwidth;
	}
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *spl, *new = SplinePointListCopy(paster->u.state.splines);
	    SplinePointListSelect(new,true);
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = *cv->heads[cv->drawmode];
	    *cv->heads[cv->drawmode] = new;
	}
	if ( paster->undotype==ut_state && paster->u.state.u.images!=NULL ) {
	    /* Images can only be pasted into background, so do that */
	    /*  even if we aren't in background mode */
	    ImageList *new, *cimg;
	    for ( cimg = paster->u.state.u.images; cimg!=NULL; cimg=cimg->next ) {
		new = galloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = cv->sc->backimages;
		cv->sc->backimages = new;
	    }
	    SCOutOfDateBackground(cv->sc);
	} else if ( paster->undotype==ut_statehint && cv->searcher==NULL )
	    ExtractHints(cv->sc,paster->u.state.u.hints,true);
	if ( paster->u.state.refs!=NULL && cv->drawmode==dm_fore ) {
	    RefChar *new, *refs;
	    SplineChar *sc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( cv->searcher!=NULL )
		    sc = FindCharacter(cv->searcher->fv->sf,refs);
		else
		    sc = FindCharacter(cv->sc->parent,refs);
		if ( sc!=NULL && SCDependsOnSC(sc,cv->sc))
		    GWidgetErrorR(_STR_SelfRef,_STR_AttemptSelfRef);
		else if ( sc!=NULL ) {
		    new = chunkalloc(sizeof(RefChar));
		    *new = *refs;
		    new->splines = NULL;
		    new->sc = sc;
		    new->selected = true;
		    new->next = cv->sc->refs;
		    cv->sc->refs = new;
		    SCReinstanciateRefChar(cv->sc,new);
		    SCMakeDependent(cv->sc,sc);
		} else {
		    PasteNonExistantRefCheck(cv->sc,paster,refs,&refstate);
		}
	    }
	} else if ( paster->u.state.refs!=NULL && cv->drawmode==dm_back ) {
	    /* Paste the CONTENTS of the referred character into this one */
	    /*  (background contents I think) */
	    RefChar *refs;
	    SplineChar *sc;
	    SplinePointList *new, *spl;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( cv->searcher!=NULL )
		    sc = FindCharacter(cv->searcher->fv->sf,refs);
		else
		    sc = FindCharacter(cv->sc->parent,refs);
		if ( sc!=NULL ) {
		    new = SplinePointListTransform(SplinePointListCopy(sc->backgroundsplines),refs->transform,true);
		    SplinePointListSelect(new,true);
		    if ( new!=NULL ) {
			for ( spl = new; spl->next!=NULL; spl = spl->next );
			spl->next = cv->sc->backgroundsplines;
			cv->sc->backgroundsplines = new;
		    }
		}
	    }
	}
      break;
      case ut_width:
	SCSynchronizeWidth(cv->sc,paster->u.width,cv->sc->width,NULL);
      break;
      case ut_vwidth:
	if ( !cv->sc->parent->hasvmetrics )
	    GWidgetErrorR(_STR_NoVerticalMetrics,_STR_FontNoVerticalMetrics);
	else
	    cv->sc->vwidth = paster->u.state.vwidth;
      break;
      case ut_rbearing:
	SplineCharFindBounds(cv->sc,&bb);
	SCSynchronizeWidth(cv->sc,bb.maxx + paster->u.rbearing,cv->sc->width,cv->fv);
      break;
      case ut_lbearing:
	SplineCharFindBounds(cv->sc,&bb);
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	transform[4] = paster->u.lbearing-bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(cv->fv,cv->sc,transform,NULL,false);
	/* FVTrans will preserver the state and update the chars */
	/* CVPaste depends on this behavior */
      break;
      case ut_composit:
	if ( paster->u.composit.state!=NULL )
	    _PasteToCV(cv,paster->u.composit.state);
      break;
      case ut_multiple:
	_PasteToCV(cv,paster->u.multiple.mult);
      break;
    }
}

void PasteToCV(CharView *cv) {
    _PasteToCV(cv,&copybuffer);
}

void BCCopySelected(BDFChar *bc,int pixelsize) {

    CopyBufferFree();

    memset(&copybuffer,'\0',sizeof(copybuffer));
    copybuffer.undotype = ut_bitmapsel;
    if ( bc->selection!=NULL )
	copybuffer.u.bmpstate.selection = BDFFloatCopy(bc->selection);
    else
	copybuffer.u.bmpstate.selection = BDFFloatCreate(bc,bc->xmin,bc->xmax,
		bc->ymin,bc->ymax, false);
    copybuffer.u.bmpstate.pixelsize = pixelsize;
}

static Undoes *BCCopyAll(BDFChar *bc,int pixelsize) {
    Undoes *cur;

    cur = chunkalloc(sizeof(Undoes));
    if ( bc==NULL )
	cur->undotype = ut_noop;
    else {
	cur->undotype = ut_bitmap;
	/*cur->u.bmpstate.width = bc->width;*/
	cur->u.bmpstate.xmin = bc->xmin;
	cur->u.bmpstate.xmax = bc->xmax;
	cur->u.bmpstate.ymin = bc->ymin;
	cur->u.bmpstate.ymax = bc->ymax;
	cur->u.bmpstate.bytes_per_line = bc->bytes_per_line;
	cur->u.bmpstate.bitmap = bmpcopy(bc->bitmap,bc->bytes_per_line,
		bc->ymax-bc->ymin+1);
	cur->u.bmpstate.selection = BDFFloatCopy(bc->selection);
    }
    cur->u.bmpstate.pixelsize = pixelsize;
return( cur );
}

static void _PasteToBC(BDFChar *bc,int pixelsize, Undoes *paster, int clearfirst,FontView *fv) {
    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_bitmapsel:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	if ( clearfirst )
	    memset(bc->bitmap,'\0',bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	bc->selection = BDFFloatCopy(paster->u.bmpstate.selection);
	BCCharChangedUpdate(bc);
      break;
      case ut_bitmap:
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	/* bc->width = paster->u.bmpstate.width; */
	bc->xmin = paster->u.bmpstate.xmin;
	bc->xmax = paster->u.bmpstate.xmax;
	bc->ymin = paster->u.bmpstate.ymin;
	bc->ymax = paster->u.bmpstate.ymax;
	bc->bytes_per_line = paster->u.bmpstate.bytes_per_line;
	free(bc->bitmap);
	bc->bitmap = galloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	memcpy(bc->bitmap,paster->u.bmpstate.bitmap,bc->bytes_per_line*(bc->ymax-bc->ymin+1));
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
	    _PasteToBC(bc,pixelsize,paster->u.composit.bitmaps,clearfirst,fv);
	else {
	    Undoes *b;
	    for ( b = paster->u.composit.bitmaps;
		    b!=NULL && b->u.bmpstate.pixelsize!=pixelsize;
		    b = b->next );
	    if ( b!=NULL )
		_PasteToBC(bc,pixelsize,paster->u.composit.bitmaps,clearfirst,fv);
	}
      break;
      case ut_multiple:
	_PasteToBC(bc,pixelsize,paster->u.multiple.mult,clearfirst,fv);
      break;
    }
}

void PasteToBC(BDFChar *bc,int pixelsize,FontView *fv) {
    _PasteToBC(bc,pixelsize,&copybuffer,false,fv);
}

void FVCopyWidth(FontView *fv,enum undotype ut) {
    Undoes *head=NULL, *last=NULL, *cur;
    int i;
    SplineChar *sc;
    DBounds bb;

    CopyBufferFree();

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	cur = chunkalloc(sizeof(Undoes));
	cur->undotype = ut;
	if ( (sc=fv->sf->chars[i])!=NULL ) {
	    switch ( ut ) {
	      case ut_width:
		cur->u.width = sc->width;
	      break;
	      case ut_vwidth:
		cur->u.width = sc->width;
	      break;
	      case ut_lbearing:
		SplineCharFindBounds(sc,&bb);
		cur->u.lbearing = bb.minx;
	      break;
	      case ut_rbearing:
		SplineCharFindBounds(sc,&bb);
		cur->u.rbearing = sc->width-bb.maxx;
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
}

void FVCopy(FontView *fv, int fullcopy) {
    int i;
    BDFFont *bdf;
    Undoes *head=NULL, *last=NULL, *cur;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;
    extern int onlycopydisplayed;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	if ( onlycopydisplayed && fv->filled==fv->show ) {
	    cur = SCCopyAll(fv->sf->chars[i],fullcopy);
	} else if ( onlycopydisplayed ) {
	    cur = BCCopyAll(fv->show->chars[i],fv->show->pixelsize);
	} else {
	    state = SCCopyAll(fv->sf->chars[i],fullcopy);
	    bhead = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		bcur = BCCopyAll(bdf->chars[i],bdf->pixelsize);
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

    if ( head==NULL )
return;
    CopyBufferFree();
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = head;
}

void MVCopyChar(MetricsView *mv, SplineChar *sc, int fullcopy) {
    BDFFont *bdf;
    Undoes *cur=NULL;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;
    extern int onlycopydisplayed;

    if ( onlycopydisplayed && mv->bdf==NULL ) {
	cur = SCCopyAll(sc,fullcopy);
    } else if ( onlycopydisplayed ) {
	cur = BCCopyAll(mv->bdf->chars[sc->enc],mv->bdf->pixelsize);
    } else {
	state = SCCopyAll(sc,fullcopy);
	bhead = NULL;
	for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    bcur = BCCopyAll(bdf->chars[sc->enc],bdf->pixelsize);
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
    CopyBufferFree();
    copybuffer.undotype = ut_multiple;
    copybuffer.u.multiple.mult = cur;
}

static BDFFont *BitmapCreateCheck(FontView *fv,int *yestoall, int first, int pixelsize) {
    int yes = 0;
    BDFFont *bdf = NULL;

    if ( *yestoall>0 && first ) {
	static int buts[] = { _STR_Yes, _STR_YesToAll, _STR_NoToAll, _STR_No, 0 };
	char buf[20]; unichar_t ubuf[400];
	sprintf( buf, "%d", pixelsize );
	u_strcpy(ubuf,GStringGetResource(_STR_ClipContainsPre,NULL));
	uc_strcat(ubuf,buf);
	u_strcat(ubuf,GStringGetResource(_STR_ClipContainsPost,NULL));
	yes = GWidgetAskCenteredR_(_STR_BitmapPaste,buts,0,3,ubuf);
	if ( yes==1 )
	    *yestoall = true;
	else if ( yes==2 )
	    *yestoall = -1;
	else
	    yes= yes!=3;
    }
    if ( yes==1 || *yestoall ) {
	bdf = SplineFontRasterize(fv->sf,pixelsize,false);
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	SFOrderBitmapList(fv->sf);
    }
return( bdf );
}

void PasteIntoFV(FontView *fv,int doclear) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int i, cnt=0;
    int yestoall=0, first=true;
    char *oldsel = fv->selected;
    extern int onlycopydisplayed;

    fv->refstate = 0;

    cur = &copybuffer;
    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] )
	++cnt;
    /* If they select exactly one character but there are more things in the */
    /*  copy buffer, then temporarily change the selection so that everything*/
    /*  in the copy buffer gets pasted (into chars immediately following sele*/
    /*  cted one (unless we run out of chars...)) */
    if ( cnt==1 && cur->undotype==ut_multiple && cur->u.multiple.mult->next!=NULL ) {
	Undoes *tot; int j;
	for ( cnt=0, tot=cur->u.multiple.mult; tot!=NULL; ++cnt, tot=tot->next );
	fv->selected = galloc(fv->sf->charcnt);
	memcpy(fv->selected,oldsel,fv->sf->charcnt);
	for ( i=0; i<fv->sf->charcnt && !fv->selected[i]; ++i );
	for ( j=0; j<cnt && i+j<fv->sf->charcnt; ++j )
	    fv->selected[i+j] = 1;
	cnt = j;
    }

    GProgressStartIndicatorR(10,_STR_Pasting,_STR_Pasting,0,cnt,1);

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    /* This little gem of code is to keep us from throwing out forward */
    /*  references. Say we are pasting into both "i" and dotlessi (and */
    /*  dotlessi isn't defined yet) without this the paste to "i" will */
    /*  search for dotlessi, not find it and ignore the reference */
    if ( cur->undotype==ut_state || cur->undotype==ut_statehint || cur->undotype==ut_statename ||
	    (cur->undotype==ut_composit && cur->u.composit.state!=NULL)) {
	for ( i=0; i<fv->sf->charcnt; ++i )
	    if ( fv->selected[i] && fv->sf->chars[i]==NULL )
		SFMakeChar(fv->sf,i);
    }
    cur = NULL;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	if ( cur==NULL ) {
	    cur = &copybuffer;
	    if ( cur->undotype==ut_multiple )
		cur = cur->u.multiple.mult;
	}
	switch ( cur->undotype ) {
	  case ut_noop:
	  break;
	  case ut_state: case ut_width: case ut_vwidth:
	  case ut_lbearing: case ut_rbearing:
	  case ut_statehint: case ut_statename:
	    if ( !fv->sf->hasvmetrics && cur->undotype==ut_vwidth) {
		GWidgetErrorR(_STR_NoVerticalMetrics,_STR_FontNoVerticalMetrics);
 goto err;
	    }
	    PasteToSC(SFMakeChar(fv->sf,i),cur,fv,doclear);
	  break;
	  case ut_bitmapsel: case ut_bitmap:
	    if ( onlycopydisplayed && fv->show!=fv->filled )
		_PasteToBC(BDFMakeChar(fv->show,i),fv->show->pixelsize,cur,doclear,fv);
	    else {
		for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize!=cur->u.bmpstate.pixelsize; bdf=bdf->next );
		if ( bdf==NULL ) {
		    bdf = BitmapCreateCheck(fv,&yestoall,first,cur->u.bmpstate.pixelsize);
		    first = false;
		}
		if ( bdf!=NULL )
		    _PasteToBC(BDFMakeChar(bdf,i),bdf->pixelsize,cur,doclear,fv);
	    }
	  break;
	  case ut_composit:
	    if ( cur->u.composit.state!=NULL )
		PasteToSC(SFMakeChar(fv->sf,i),cur->u.composit.state,fv,doclear);
	    for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
		for ( bdf=fv->sf->bitmaps; bdf!=NULL &&
			bdf->pixelsize!=bmp->u.bmpstate.pixelsize; bdf=bdf->next );
		if ( bdf==NULL )
		    bdf = BitmapCreateCheck(fv,&yestoall,first,bmp->u.bmpstate.pixelsize);
		if ( bdf!=NULL )
		    _PasteToBC(BDFMakeChar(bdf,i),bdf->pixelsize,bmp,doclear,fv);
	    }
	    first = false;
	  break;
	}
	cur = cur->next;
	if ( !GProgressNext())
    break;
    }
 err:
    GProgressEndIndicator();
    if ( oldsel!=fv->selected )
	free(oldsel);
}

void PasteIntoMV(MetricsView *mv,SplineChar *sc, int doclear) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int yestoall=0, first=true;
    extern int onlycopydisplayed;

    cur = &copybuffer;

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    switch ( cur->undotype ) {
      case ut_noop:
      break;
      case ut_state: case ut_width: case ut_vwidth:
      case ut_lbearing: case ut_rbearing:
      case ut_statehint: case ut_statename:
	if ( !mv->fv->sf->hasvmetrics && cur->undotype==ut_vwidth) {
	    GWidgetErrorR(_STR_NoVerticalMetrics,_STR_FontNoVerticalMetrics);
return;
	}
	PasteToSC(sc,cur,mv->fv,doclear);
      break;
      case ut_bitmapsel: case ut_bitmap:
	if ( onlycopydisplayed && mv->bdf!=NULL )
	    _PasteToBC(BDFMakeChar(mv->bdf,sc->enc),mv->bdf->pixelsize,cur,doclear,mv->fv);
	else {
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize!=cur->u.bmpstate.pixelsize; bdf=bdf->next );
	    if ( bdf==NULL ) {
		bdf = BitmapCreateCheck(mv->fv,&yestoall,first,cur->u.bmpstate.pixelsize);
		first = false;
	    }
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,sc->enc),bdf->pixelsize,cur,doclear,mv->fv);
	}
      break;
      case ut_composit:
	if ( cur->u.composit.state!=NULL )
	    PasteToSC(sc,cur->u.composit.state,mv->fv,doclear);
	for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL &&
		    bdf->pixelsize!=bmp->u.bmpstate.pixelsize; bdf=bdf->next );
	    if ( bdf==NULL )
		bdf = BitmapCreateCheck(mv->fv,&yestoall,first,bmp->u.bmpstate.pixelsize);
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,sc->enc),bdf->pixelsize,bmp,doclear,mv->fv);
	}
	first = false;
      break;
    }
}
