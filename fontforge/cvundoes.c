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
#define _DEFINE_SEARCHVIEW_
#include "pfaeditui.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>

extern char *coord_sep;

/* ********************************* Undoes ********************************* */

int maxundoes = 12;		/* -1 is infinite */

static uint8 *bmpcopy(uint8 *bitmap,int bytes_per_line, int lines) {
    uint8 *ret = galloc(bytes_per_line*lines);
    memcpy(ret,bitmap,bytes_per_line*lines);
return( ret );
}

static RefChar *RefCharsCopyState(SplineChar *sc,int layer) {
    RefChar *head=NULL, *last=NULL, *new, *crefs;

    if ( layer<0 || sc->layers[layer].refs==NULL )
return( NULL );
    for ( crefs = sc->layers[layer].refs; crefs!=NULL; crefs=crefs->next ) {
	new = RefCharCreate();
	*new = *crefs;
#ifdef FONTFORGE_CONFIG_TYPE3
	new->layers = gcalloc(new->layer_cnt,sizeof(struct reflayer));
#else
	new->layers[0].splines = NULL;
#endif
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

    MDReplace(head,sc->layers[ly_fore].splines,rpl);
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

static void FixupRefChars(SplineChar *sc,RefChar *urefs,int layer) {
    RefChar *crefs = sc->layers[layer].refs, *cend, *cprev, *unext, *cnext;

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
		sc->layers[layer].refs = urefs;
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
	    sc->layers[layer].refs = urefs;
	else
	    cprev->next = urefs;
	while ( urefs!=NULL ) {
	    SCReinstanciateRefChar(sc,urefs);
	    SCMakeDependent(sc,urefs->sc);
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
	  case ut_hints: case ut_anchors: case ut_statelookup:
	    SplinePointListsFree(undo->u.state.splines);
	    RefCharsFree(undo->u.state.refs);
	    MinimumDistancesFree(undo->u.state.md);
	    UHintListFree(undo->u.state.hints);
	    free(undo->u.state.instrs);
	    ImageListsFree(undo->u.state.images);
	    if ( undo->undotype==ut_statename ) {
		free( undo->u.state.charname );
		free( undo->u.state.comment );
		PSTFree( undo->u.state.possub );
	    }
	    AnchorPointsFree(undo->u.state.anchor);
	  break;
	  case ut_bitmap:
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static Undoes *CVAddUndo(CharView *cv,Undoes *undo) {
return( AddUndo(undo,&cv->layerheads[cv->drawmode]->undoes,
	&cv->layerheads[cv->drawmode]->redoes));
}

int CVLayer(CharView *cv) {
    if ( cv->drawmode==dm_grid )
return( ly_grid );

return( cv->layerheads[cv->drawmode]-cv->sc->layers );
}

Undoes *CVPreserveState(CharView *cv) {
    Undoes *undo;
    int layer = CVLayer(cv);

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->sc->parent->order2;
    undo->u.state.width = cv->sc->width;
    undo->u.state.vwidth = cv->sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(cv->layerheads[cv->drawmode]->splines);
    undo->u.state.refs = RefCharsCopyState(cv->sc,layer);
    if ( layer==ly_fore ) {
	undo->u.state.md = MDsCopyState(cv->sc,undo->u.state.splines);
	undo->u.state.anchor = AnchorPointsCopy(cv->sc->anchor);
    }
    undo->u.state.images = ImageListCopy(cv->layerheads[cv->drawmode]->images);
#ifdef FONTFORGE_CONFIG_TYPE3
    undo->u.state.fill_brush = cv->layerheads[cv->drawmode]->fill_brush;
    undo->u.state.stroke_pen = cv->layerheads[cv->drawmode]->stroke_pen;
    undo->u.state.dofill = cv->layerheads[cv->drawmode]->dofill;
    undo->u.state.dostroke = cv->layerheads[cv->drawmode]->dostroke;
    undo->u.state.fillfirst = cv->layerheads[cv->drawmode]->fillfirst;
#endif
return( CVAddUndo(cv,undo));
}

Undoes *CVPreserveStateHints(CharView *cv) {
    Undoes *undo = CVPreserveState(cv);
    if ( CVLayer(cv)==ly_fore ) {
	undo->undotype = ut_statehint;
	undo->u.state.hints = UHintCopy(cv->sc,true);
	undo->u.state.instrs = (uint8*) copyn((char*) cv->sc->ttf_instrs, cv->sc->ttf_instrs_len);
	undo->u.state.instrs_len = cv->sc->ttf_instrs_len;
    }
return( undo );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

Undoes *SCPreserveHints(SplineChar *sc) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->was_modified = sc->changed;
    undo->undotype = ut_hints;
    undo->u.state.hints = UHintCopy(sc,true);
    undo->u.state.instrs = (uint8*) copyn((char *) sc->ttf_instrs, sc->ttf_instrs_len);
    undo->u.state.instrs_len = sc->ttf_instrs_len;
    undo->copied_from = sc->parent;
return( AddUndo(undo,&sc->layers[ly_fore].undoes,&sc->layers[ly_fore].redoes));
}

Undoes *SCPreserveLayer(SplineChar *sc,int layer, int dohints) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->parent->order2;
    undo->u.state.width = sc->width;
    undo->u.state.vwidth = sc->vwidth;
    undo->u.state.splines = SplinePointListCopy(sc->layers[layer].splines);
    undo->u.state.refs = RefCharsCopyState(sc,layer);
    if ( layer==ly_fore ) {
	undo->u.state.md = MDsCopyState(sc,undo->u.state.splines);
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
#ifdef FONTFORGE_CONFIG_TYPE3
    undo->u.state.fill_brush = sc->layers[layer].fill_brush;
    undo->u.state.stroke_pen = sc->layers[layer].stroke_pen;
    undo->u.state.dofill = sc->layers[layer].dofill;
    undo->u.state.dostroke = sc->layers[layer].dostroke;
    undo->u.state.fillfirst = sc->layers[layer].fillfirst;
#endif
    undo->copied_from = sc->parent;
return( AddUndo(undo,&sc->layers[layer].undoes,&sc->layers[layer].redoes));
}

Undoes *SCPreserveState(SplineChar *sc,int dohints) {
#ifdef FONTFORGE_CONFIG_TYPE3
    int i;

    for ( i=ly_fore+1; i<sc->layer_cnt; ++i )
	SCPreserveLayer(sc,i,false);
#endif
return( SCPreserveLayer(sc,ly_fore,dohints));
}

Undoes *SCPreserveBackground(SplineChar *sc) {
return( SCPreserveLayer(sc,ly_back,false));
}

Undoes *SFPreserveGuide(SplineFont *sf) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_state;
    undo->was_modified = sf->changed;
    undo->was_order2 = sf->order2;
    undo->u.state.splines = SplinePointListCopy(sf->grid.splines);
    undo->u.state.images = ImageListCopy(sf->grid.images);
#ifdef FONTFORGE_CONFIG_TYPE3
    undo->u.state.fill_brush = sf->grid.fill_brush;
    undo->u.state.stroke_pen = sf->grid.stroke_pen;
    undo->u.state.dofill = sf->grid.dofill;
    undo->u.state.dostroke = sf->grid.dostroke;
    undo->u.state.fillfirst = sf->grid.fillfirst;
#endif
    undo->copied_from = sf;
return( AddUndo(undo,&sf->grid.undoes,&sf->grid.redoes));
}

void SCUndoSetLBearingChange(SplineChar *sc,int lbc) {
    Undoes *undo = sc->layers[ly_fore].undoes;

    if ( undo==NULL || undo->undotype != ut_state )
return;
    undo->u.state.lbearingchange = lbc;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
Undoes *CVPreserveTState(CharView *cv) {
    Undoes *undo;
    int anyrefs;
    RefChar *refs, *urefs;
    int was0 = false, j;

    cv->p.transany = CVAnySel(cv,NULL,&anyrefs,NULL,NULL);
    cv->p.transanyrefs = anyrefs;

    if ( maxundoes==0 ) {
	was0 = true;
	maxundoes = 1;
    }

    undo = CVPreserveState(cv);
    if ( !cv->p.transany || cv->p.transanyrefs ) {
	for ( refs = cv->layerheads[cv->drawmode]->refs, urefs=undo->u.state.refs; urefs!=NULL; refs=refs->next, urefs=urefs->next )
	    if ( !cv->p.transany || refs->selected )
		for ( j=0; j<urefs->layer_cnt; ++j )
		    urefs->layers[j].splines = SplinePointListCopy(refs->layers[j].splines);
    }
    undo->undotype = ut_tstate;

    if ( was0 )
	maxundoes = 0;

return( undo );
}

Undoes *CVPreserveWidth(CharView *cv,int width) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->sc->parent->order2;
    undo->u.width = width;
return( CVAddUndo(cv,undo));
}

Undoes *CVPreserveVWidth(CharView *cv,int vwidth) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->was_modified = cv->sc->changed;
    undo->was_order2 = cv->sc->parent->order2;
    undo->u.width = vwidth;
return( CVAddUndo(cv,undo));
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

Undoes *SCPreserveWidth(SplineChar *sc) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_width;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->parent->order2;
    undo->u.state.width = sc->width;
return( AddUndo(undo,&sc->layers[ly_fore].undoes,&sc->layers[ly_fore].redoes));
}

Undoes *SCPreserveVWidth(SplineChar *sc) {
    Undoes *undo;

    if ( no_windowing_ui || maxundoes==0 )		/* No use for undoes in scripting */
return(NULL);

    undo = chunkalloc(sizeof(Undoes));

    undo->undotype = ut_vwidth;
    undo->was_modified = sc->changed;
    undo->was_order2 = sc->parent->order2;
    undo->u.state.width = sc->vwidth;
return( AddUndo(undo,&sc->layers[ly_fore].undoes,&sc->layers[ly_fore].redoes));
}

Undoes *BCPreserveState(BDFChar *bc) {
    Undoes *undo;

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

	if ( layer==ly_fore ) {
	    int width = sc->width;
	    int vwidth = sc->vwidth;
	    if ( sc->width!=undo->u.state.width )
		SCSynchronizeWidth(sc,undo->u.state.width,width,NULL);
	    sc->vwidth = undo->u.state.vwidth;
	    undo->u.state.width = width;
	    undo->u.state.vwidth = vwidth;
	}
	head->splines = undo->u.state.splines;
	if ( layer==ly_fore ) {
	    MinimumDistance *md = sc->md;
	    AnchorPoint *ap = sc->anchor;
	    sc->md = undo->u.state.md;
	    undo->u.state.md = md;
	    sc->anchor = undo->u.state.anchor;
	    undo->u.state.anchor = ap;
	}
	if ( layer>=ly_fore && !RefCharsMatch(undo->u.state.refs,sc->layers[layer].refs)) {
	    RefChar *refs = RefCharsCopyState(sc,layer);
	    FixupRefChars(sc,undo->u.state.refs,layer);
	    undo->u.state.refs = refs;
	} else if ( layer==ly_fore &&
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
	    SCSynchronizeLBearing(sc,undo->u.state.lbearingchange);
	}
	if ( layer==ly_fore && undo->undotype==ut_statename ) {
	    char *temp = sc->name;
	    int uni = sc->unicodeenc;
	    PST *possub = sc->possub;
	    char *comment = sc->comment;
	    sc->name = undo->u.state.charname;
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void CVDoUndo(CharView *cv) {
    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    cv->layerheads[cv->drawmode]->undoes = undo->next;
    undo->next = NULL;
    SCUndoAct(cv->sc,CVLayer(cv),undo);
    undo->next = cv->layerheads[cv->drawmode]->redoes;
    cv->layerheads[cv->drawmode]->redoes = undo;
    _CVCharChangedUpdate(cv,undo->was_modified);
    cv->lastselpt = NULL;
return;
}

void CVDoRedo(CharView *cv) {
    Undoes *undo = cv->layerheads[cv->drawmode]->redoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    cv->layerheads[cv->drawmode]->redoes = undo->next;
    undo->next = NULL;
    SCUndoAct(cv->sc,CVLayer(cv),undo);
    undo->next = cv->layerheads[cv->drawmode]->undoes;
    cv->layerheads[cv->drawmode]->undoes = undo;
    CVCharChangedUpdate(cv);
    cv->lastselpt = NULL;
return;
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SCDoUndo(SplineChar *sc,int layer) {
    Undoes *undo = sc->layers[layer].undoes;

    if ( undo==NULL )		/* Shouldn't happen */
return;
    sc->layers[layer].undoes = undo->next;
    undo->next = NULL;
    SCUndoAct(sc,layer,undo);
    undo->next = sc->layers[layer].redoes;
    sc->layers[layer].redoes = undo;
    _SCCharChangedUpdate(sc,undo->was_modified);
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
    SCCharChangedUpdate(sc);
return;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* Used when doing incremental transformations. If I just keep doing increments*/
/*  then rounding errors will mount. Instead I go back to the original state */
/*  each time */
void CVRestoreTOriginalState(CharView *cv) {
    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;
    RefChar *ref, *uref;
    ImageList *img, *uimg;
    int j;

    SplinePointListSet(cv->layerheads[cv->drawmode]->splines,undo->u.state.splines);
    if ( cv->drawmode==dm_fore && (!cv->p.anysel || cv->p.transanyrefs)) {
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

void CVUndoCleanup(CharView *cv) {
    Undoes * undo = cv->layerheads[cv->drawmode]->undoes;
    RefChar *uref;

    if ( cv->drawmode==dm_fore && (!cv->p.anysel || cv->p.transanyrefs)) {
	for ( uref=undo->u.state.refs; uref!=NULL; uref=uref->next ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	    int i;
	    for ( i=0; i<uref->layer_cnt; ++i )
		SplinePointListsFree(uref->layers[i].splines);
	    free(uref->layers);
	    uref->layers = NULL;
	    uref->layer_cnt = 0;
#else
	    SplinePointListsFree(uref->layers[0].splines);
	    uref->layers[0].splines = NULL;
#endif
	}
    }
    undo->undotype = ut_state;
}

void CVRemoveTopUndo(CharView *cv) {
    Undoes * undo = cv->layerheads[cv->drawmode]->undoes;

    cv->layerheads[cv->drawmode]->undoes = undo->next;
    undo->next = NULL;
    UndoesFree(undo);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void BCUndoAct(BDFChar *bc,Undoes *undo) {

    switch ( undo->undotype ) {
      case ut_bitmap: {
	uint8 *b;
	int temp;
	BDFFloat *sel;
	temp = bc->width; bc->width = undo->u.bmpstate.width; undo->u.bmpstate.width = temp;
	temp = bc->xmin; bc->xmin = undo->u.bmpstate.xmin; undo->u.bmpstate.xmin = temp;
	temp = bc->xmax; bc->xmax = undo->u.bmpstate.xmax; undo->u.bmpstate.xmax = temp;
	temp = bc->ymin; bc->ymin = undo->u.bmpstate.ymin; undo->u.bmpstate.ymin = temp;
	temp = bc->ymax; bc->ymax = undo->u.bmpstate.ymax; undo->u.bmpstate.ymax = temp;
	temp = bc->bytes_per_line; bc->bytes_per_line = undo->u.bmpstate.bytes_per_line; undo->u.bmpstate.bytes_per_line = temp;
	b = bc->bitmap; bc->bitmap = undo->u.bmpstate.bitmap; undo->u.bmpstate.bitmap = b;
	sel = bc->selection; bc->selection = undo->u.bmpstate.selection; undo->u.bmpstate.selection = sel;
      } break;
      default:
	IError( "Unknown undo type in BCUndoAct: %d", undo->undotype );
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

void CopyBufferFree(void) {

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
      break;
      case ut_bitmapsel:
	BDFFloatFree(copybuffer.u.bmpstate.selection);
      break;
      case ut_multiple: case ut_layers:
	UndoesFree( copybuffer.u.multiple.mult );
      break;
      case ut_composit:
	UndoesFree( copybuffer.u.composit.state );
	UndoesFree( copybuffer.u.composit.bitmaps );
      break;
    }
    memset(&copybuffer,'\0',sizeof(copybuffer));
    copybuffer.undotype = ut_none;
}

static void CopyBufferFreeGrab(void) {
    CopyBufferFree();
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv_list!=NULL && !no_windowing_ui )
	GDrawGrabSelection(fv_list->gw,sn_clipboard);	/* Grab the selection to one of my windows, doesn't matter which, aren't going to export it, but just want to clear things out so no one else thinks they have the selection */
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void noop(void *_copybuffer) {
}

static void *copybufferPt2str(void *_copybuffer,int32 *len) {
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
    if ( cur==NULL || fv_list==NULL ||
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

static void *copybufferName2str(void *_copybuffer,int32 *len) {
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
    if ( cur==NULL || fv_list==NULL || cur->u.state.charname==NULL ) {
	*len=0;
return( copy(""));
    }
    *len = strlen(cur->u.state.charname);
return( copy( cur->u.state.charname ));
}

static RefChar *XCopyInstanciateRefs(RefChar *refs,SplineChar *container) {
    /* References in the copybuffer don't include the translated splines */
    RefChar *head=NULL, *last, *cur;

    while ( refs!=NULL ) {
	cur = RefCharCreate();
	*cur = *refs;
#ifdef FONTFORGE_CONFIG_TYPE3
	cur->layers = NULL;
	cur->layer_cnt = 0;
#else
	cur->layers[0].splines = NULL;
#endif
	cur->next = NULL;
	SCReinstanciateRefChar(container,cur);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	refs = refs->next;
    }
return( head );
}

#ifndef _NO_LIBXML
static void *copybuffer2svg(void *_copybuffer,int32 *len) {
    Undoes *cur = &copybuffer;
    SplineChar dummy;
#ifdef FONTFORGE_CONFIG_TYPE3
    static Layer layers[2];
#endif
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
    if ( cur==NULL || fv_list==NULL ) {
	*len=0;
return( copy(""));
    }

    memset(&dummy,0,sizeof(dummy));
#ifdef FONTFORGE_CONFIG_TYPE3
    dummy.layers = layers;
#endif
    dummy.layer_cnt = 2;
    dummy.name = "dummy";
    if ( cur->undotype!=ut_layers )
	dummy.parent = cur->copied_from;
    else if ( cur->u.multiple.mult!=NULL && cur->u.multiple.mult->undotype == ut_state )
	dummy.parent = cur->u.multiple.mult->copied_from;
    if ( dummy.parent==NULL )
	dummy.parent = fv_list->sf;		/* Might not be right, but we need something */
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cur->undotype==ut_layers ) {
	Undoes *ulayer;
	for ( ulayer = cur->u.multiple.mult, lcnt=0; ulayer!=NULL; ulayer=ulayer->next, ++lcnt);
	dummy.layer_cnt = lcnt+1;
	if ( lcnt!=1 )
	    dummy.layers = gcalloc((lcnt+1),sizeof(Layer));
	for ( ulayer = cur->u.multiple.mult, lcnt=1; ulayer!=NULL; ulayer=ulayer->next, ++lcnt) {
	    if ( ulayer->undotype==ut_state || ulayer->undotype==ut_statehint ) {
		dummy.layers[lcnt].fill_brush = ulayer->u.state.fill_brush;
		dummy.layers[lcnt].stroke_pen = ulayer->u.state.stroke_pen;
		dummy.layers[lcnt].dofill = ulayer->u.state.dofill;
		dummy.layers[lcnt].dostroke = ulayer->u.state.dostroke;
		dummy.layers[lcnt].splines = ulayer->u.state.splines;
		dummy.layers[lcnt].refs = XCopyInstanciateRefs(ulayer->u.state.refs,&dummy);
	    }
	}
    } else
#endif
    {
#ifdef FONTFORGE_CONFIG_TYPE3
	dummy.layers[ly_fore].fill_brush = cur->u.state.fill_brush;
	dummy.layers[ly_fore].stroke_pen = cur->u.state.stroke_pen;
	dummy.layers[ly_fore].dofill = cur->u.state.dofill;
	dummy.layers[ly_fore].dostroke = cur->u.state.dostroke;
#endif
	dummy.layers[ly_fore].splines = cur->u.state.splines;
	dummy.layers[ly_fore].refs = XCopyInstanciateRefs(cur->u.state.refs,&dummy);
    }

    svg = tmpfile();
    if ( svg==NULL ) {
	*len=0;
return( copy(""));
    }

    old_order2 = dummy.parent->order2;
    dummy.parent->order2 = cur->was_order2;
    /* Don't bother to generate a preview here, that can take too long and */
    /*  cause the paster to time out */
    _ExportSVG(svg,&dummy);
    dummy.parent->order2 = old_order2;

    for ( lcnt = ly_fore; lcnt<dummy.layer_cnt; ++lcnt )
	RefCharsFree(dummy.layers[lcnt].refs);
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( dummy.layer_cnt!=2 )
	free( dummy.layers );
#endif

    fseek(svg,0,SEEK_END);
    *len = ftell(svg);
    ret = galloc(*len);
    rewind(svg);
    fread(ret,1,*len,svg);
    fclose(svg);
return( ret );
}
#endif

static void *copybuffer2eps(void *_copybuffer,int32 *len) {
    Undoes *cur = &copybuffer;
    SplineChar dummy;
#ifdef FONTFORGE_CONFIG_TYPE3
    static Layer layers[2];
#endif
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
    if ( cur==NULL || fv_list==NULL ) {
	*len=0;
return( copy(""));
    }

    memset(&dummy,0,sizeof(dummy));
#ifdef FONTFORGE_CONFIG_TYPE3
    dummy.layers = layers;
#endif
    dummy.layer_cnt = 2;
    dummy.name = "dummy";
    if ( cur->undotype!=ut_layers )
	dummy.parent = cur->copied_from;
    else if ( cur->u.multiple.mult!=NULL && cur->u.multiple.mult->undotype == ut_state )
	dummy.parent = cur->u.multiple.mult->copied_from;
    if ( dummy.parent==NULL )
	dummy.parent = fv_list->sf;		/* Might not be right, but we need something */
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cur->undotype==ut_layers ) {
	Undoes *ulayer;
	for ( ulayer = cur->u.multiple.mult, lcnt=0; ulayer!=NULL; ulayer=ulayer->next, ++lcnt);
	dummy.layer_cnt = lcnt+1;
	if ( lcnt!=1 )
	    dummy.layers = gcalloc((lcnt+1),sizeof(Layer));
	for ( ulayer = cur->u.multiple.mult, lcnt=1; ulayer!=NULL; ulayer=ulayer->next, ++lcnt) {
	    if ( ulayer->undotype==ut_state || ulayer->undotype==ut_statehint ) {
		dummy.layers[lcnt].fill_brush = ulayer->u.state.fill_brush;
		dummy.layers[lcnt].stroke_pen = ulayer->u.state.stroke_pen;
		dummy.layers[lcnt].dofill = ulayer->u.state.dofill;
		dummy.layers[lcnt].dostroke = ulayer->u.state.dostroke;
		dummy.layers[lcnt].splines = ulayer->u.state.splines;
		dummy.layers[lcnt].refs = XCopyInstanciateRefs(ulayer->u.state.refs,&dummy);
	    }
	}
    } else
#endif
    {
#ifdef FONTFORGE_CONFIG_TYPE3
	dummy.layers[ly_fore].fill_brush = cur->u.state.fill_brush;
	dummy.layers[ly_fore].stroke_pen = cur->u.state.stroke_pen;
	dummy.layers[ly_fore].dofill = cur->u.state.dofill;
	dummy.layers[ly_fore].dostroke = cur->u.state.dostroke;
#endif
	dummy.layers[ly_fore].splines = cur->u.state.splines;
	dummy.layers[ly_fore].refs = XCopyInstanciateRefs(cur->u.state.refs,&dummy);
    }

    eps = tmpfile();
    if ( eps==NULL ) {
	*len=0;
return( copy(""));
    }

    old_order2 = dummy.parent->order2;
    dummy.parent->order2 = cur->was_order2;
    /* Don't bother to generate a preview here, that can take too long and */
    /*  cause the paster to time out */
    _ExportEPS(eps,&dummy,false);
    dummy.parent->order2 = old_order2;

    for ( lcnt = ly_fore; lcnt<dummy.layer_cnt; ++lcnt )
	RefCharsFree(dummy.layers[lcnt].refs);
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( dummy.layer_cnt!=2 )
	free( dummy.layers );
#endif

    fseek(eps,0,SEEK_END);
    *len = ftell(eps);
    ret = galloc(*len);
    rewind(eps);
    fread(ret,1,*len,eps);
    fclose(eps);
return( ret );
}

static void XClipCheckEps(void) {
    Undoes *cur = &copybuffer;

    if ( fv_list==NULL )
return;
    if ( no_windowing_ui )
return;

    while ( cur ) {
	switch ( cur->undotype ) {
	  case ut_multiple:
	    cur = cur->u.multiple.mult;
	  break;
	  case ut_composit:
	    cur = cur->u.composit.state;
	  break;
	  case ut_state: case ut_statehint: case ut_statename: case ut_layers:
	    GDrawAddSelectionType(fv_list->gw,sn_clipboard,"image/eps",&copybuffer,0,sizeof(char),
		    copybuffer2eps,noop);
#ifndef _NO_LIBXML
	    GDrawAddSelectionType(fv_list->gw,sn_clipboard,"image/svg",&copybuffer,0,sizeof(char),
		    copybuffer2svg,noop);
#endif
	    /* If the selection is one point, then export the coordinates as a string */
	    if ( cur->u.state.splines!=NULL && cur->u.state.refs==NULL &&
		    cur->u.state.splines->next==NULL &&
		    cur->u.state.splines->first->next==NULL )
		GDrawAddSelectionType(fv_list->gw,sn_clipboard,"STRING",&copybuffer,0,sizeof(char),
			copybufferPt2str,noop);
	    else if ( cur->undotype==ut_statename )
		GDrawAddSelectionType(fv_list->gw,sn_clipboard,"STRING",&copybuffer,0,sizeof(char),
			copybufferName2str,noop);
	    cur = NULL;
	  break;
	  default:
	    cur = NULL;
	  break;
	}
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
	cur->undotype==ut_anchors || cur->undotype==ut_noop );
}

int CopyContainsBitmap(void) {
    Undoes *cur = &copybuffer;
    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;
    if ( cur->undotype==ut_composit )
return( cur->u.composit.bitmaps!=NULL );

return( cur->undotype==ut_bitmapsel || cur->undotype==ut_noop );
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
    }
}

void CopyBufferClearCopiedFrom(SplineFont *dying) {
    _CopyBufferClearCopiedFrom(&copybuffer,dying);
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

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = sc->parent->order2;
    copybuffer.u.state.width = sc->width;
    copybuffer.u.state.vwidth = sc->vwidth;
    copybuffer.u.state.refs = ref = RefCharCreate();
    copybuffer.copied_from = sc->parent;
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( ly_fore<sc->layer_cnt ) {
	copybuffer.u.state.fill_brush = sc->layers[ly_fore].fill_brush;
	copybuffer.u.state.stroke_pen = sc->layers[ly_fore].stroke_pen;
	copybuffer.u.state.dofill = sc->layers[ly_fore].dofill;
	copybuffer.u.state.dostroke = sc->layers[ly_fore].dostroke;
	copybuffer.u.state.fillfirst = sc->layers[ly_fore].fillfirst;
    }
#endif
    ref->unicode_enc = sc->unicodeenc;
    ref->orig_pos = sc->orig_pos;
    ref->adobe_enc = getAdobeEnc(sc->name);
    ref->transform[0] = ref->transform[3] = 1.0;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    XClipCheckEps();
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

void SCCopyLookupData(SplineChar *sc) {
    CopyReference(sc);
    copybuffer.undotype = ut_statelookup;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void CopySelected(CharView *cv) {

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = cv->sc->parent->order2;
    copybuffer.u.state.width = cv->sc->width;
    copybuffer.u.state.vwidth = cv->sc->vwidth;
    copybuffer.u.state.splines = SplinePointListCopySelected(cv->layerheads[cv->drawmode]->splines);
    if ( cv->drawmode==dm_fore ) {
	RefChar *refs, *new;
	for ( refs = cv->layerheads[cv->drawmode]->refs; refs!=NULL; refs = refs->next ) if ( refs->selected ) {
	    new = RefCharCreate();
	    *new = *refs;
#ifdef FONTFORGE_CONFIG_TYPE3
	    new->layers = NULL;
	    new->layer_cnt = 0;
#else
	    new->layers[0].splines = NULL;
#endif
	    new->orig_pos = new->sc->orig_pos;
	    new->sc = NULL;
	    new->next = copybuffer.u.state.refs;
	    copybuffer.u.state.refs = new;
	}
	if ( cv->showanchor ) {
	    AnchorPoint *ap, *new;
	    for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected ) {
		new = chunkalloc(sizeof(AnchorPoint));
		*new = *ap;
		new->next = copybuffer.u.state.anchor;
		copybuffer.u.state.anchor = new;
	    }
	}
    }
#ifndef FONTFORGE_CONFIG_TYPE3
    if ( cv->drawmode==dm_back )
#endif
    {
	ImageList *imgs, *new;
	for ( imgs = cv->layerheads[cv->drawmode]->images; imgs!=NULL; imgs = imgs->next ) if ( imgs->selected ) {
	    new = chunkalloc(sizeof(ImageList));
	    *new = *imgs;
	    new->next = copybuffer.u.state.images;
	    copybuffer.u.state.images = new;
	}
    }
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cv->drawmode==dm_fore || cv->drawmode==dm_back ) {
	copybuffer.u.state.fill_brush = cv->layerheads[cv->drawmode]->fill_brush;
	copybuffer.u.state.stroke_pen = cv->layerheads[cv->drawmode]->stroke_pen;
	copybuffer.u.state.dofill = cv->layerheads[cv->drawmode]->dofill;
	copybuffer.u.state.dostroke = cv->layerheads[cv->drawmode]->dostroke;
	copybuffer.u.state.fillfirst = cv->layerheads[cv->drawmode]->fillfirst;
    }
#endif
    copybuffer.copied_from = cv->sc->parent;

    XClipCheckEps();
}

void CVCopyGridFit(CharView *cv) {
    SplineChar *sc = cv->sc;

    if ( cv->gridfit==NULL )
return;

    CopyBufferFreeGrab();

    copybuffer.undotype = ut_state;
    copybuffer.was_order2 = sc->parent->order2;
    copybuffer.u.state.width = cv->ft_gridfitwidth;
    copybuffer.u.state.vwidth = sc->vwidth;
    copybuffer.u.state.splines = SplinePointListCopy(cv->gridfit);
    copybuffer.copied_from = cv->sc->parent;

    XClipCheckEps();
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#ifdef FONTFORGE_CONFIG_TYPE3
static Undoes *SCCopyAllLayer(SplineChar *sc,enum fvcopy_type full,int layer) {
#else
static Undoes *SCCopyAll(SplineChar *sc,enum fvcopy_type full) {
    const int layer = ly_fore;
#endif
    Undoes *cur;
    RefChar *ref;
    extern int copymetadata, copyttfinstr;
    /* If full==ct_fullcopy copy the glyph as is. */
    /* If full==ct_reference put a reference to the glyph in the clipboard */
    /* If full==ct_unlinkrefs copy the glyph, but unlink any references it contains */
    /*	so we end up with no references and a bunch of splines */

    cur = chunkalloc(sizeof(Undoes));
    if ( sc==NULL ) {
	cur->undotype = ut_noop;
    } else {
	cur->was_order2 = sc->parent->order2;
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
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( layer<sc->layer_cnt ) {
	    cur->u.state.images = ImageListCopy(sc->layers[layer].images);
	    cur->u.state.fill_brush = sc->layers[layer].fill_brush;
	    cur->u.state.stroke_pen = sc->layers[layer].stroke_pen;
	    cur->u.state.dofill = sc->layers[layer].dofill;
	    cur->u.state.dostroke = sc->layers[layer].dostroke;
	    cur->u.state.fillfirst = sc->layers[layer].fillfirst;
	}
#endif
	cur->copied_from = sc->parent;
    }
return( cur );
}

#ifdef FONTFORGE_CONFIG_TYPE3
static Undoes *SCCopyAll(SplineChar *sc,enum fvcopy_type full) {
    int layer;
    Undoes *ret, *cur, *last=NULL;

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
}
#endif

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
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void CopyWidth(CharView *cv,enum undotype ut) {
    SCCopyWidth(cv->sc,ut);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static SplineChar *FindCharacter(SplineFont *into, SplineFont *from,RefChar *rf,
	SplineChar **fromsc) {
    FontView *fv;
    char *fromname = NULL;

    for ( fv = fv_list; fv!=NULL && fv->sf!=from; fv=fv->next );
    if ( fv==NULL ) from=NULL;
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[0] = _("Don't Warn Again"); buts[1] = _("_OK"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[0] = _("Don't Warn Again"); buts[1] = GTK_STOCK_OK; buts[2] = NULL;
#endif
	    yes = gwwv_ask(_("Bad Reference"),(const char **) buts,1,1,_("You are attempting to paste a reference to %1$s into %2$s.\nBut %1$s does not exist in this font, nor can I find the original character referred to.\nIt will not be copied."),name,sc->name);
	    if ( yes==0 )
		*refstate |= 0x4;
	}
    } else {
	if ( !(*refstate&0x3) ) {
	    char *buts[5];
	    buts[1] = _("Yes to All");
	    buts[2] = _("No to All");
	    buts[4] = NULL;
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[0] = _("_Yes");
	    buts[3] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[0] = GTK_STOCK_YES;
	    buts[3] = GTK_STOCK_NO;
#endif
	    gwwv_progress_pause_timer();
	    yes = gwwv_ask(_("Bad Reference"),(const char **) buts,0,3,_("You are attempting to paste a reference to %1$s into %2$s.\nBut %1$s does not exist in this font.\nWould you like to copy the original splines (or delete the reference)?"),fromsc->name,sc->name);
	    gwwv_progress_resume_timer();
	    if ( yes==1 )
		*refstate |= 1;
	    else if ( yes==2 )
		*refstate |= 2;
	}
	if ( (*refstate&1) || yes<=1 ) {
	    new = SplinePointListTransform(SplinePointListCopy(fromsc->layers[ly_fore].splines),ref->transform,true);
	    SplinePointListSelect(new,true);
	    if ( new!=NULL ) {
		for ( spl = new; spl->next!=NULL; spl = spl->next );
		spl->next = sc->layers[ly_fore].splines;
		sc->layers[ly_fore].splines = new;
	    }
	}
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void SCCheckXClipboard(GWindow awindow,SplineChar *sc,int layer,int doclear) {
    int type; int32 len;
    char *paste;
    FILE *temp;
    GImage *image;

    if ( no_windowing_ui )
return;
    type = 0;
#ifndef _NO_LIBPNG
    if ( GDrawSelectionHasType(awindow,sn_clipboard,"image/png") )
	type = 1;
    else
#endif
#ifndef _NO_LIBXML
    /* SVG is a better format (than eps) if we've got it because it doesn't */
    /*  force conversion of quadratic to cubic and back */
    if ( HasSVG() && GDrawSelectionHasType(awindow,sn_clipboard,"image/svg") )
	type = 2;
    else
#endif
    if ( GDrawSelectionHasType(awindow,sn_clipboard,"image/bmp") )
	type = 3;
    else if ( GDrawSelectionHasType(awindow,sn_clipboard,"image/eps") )
	type = 4;
    else if ( GDrawSelectionHasType(awindow,sn_clipboard,"image/ps") )
	type = 5;

    if ( type==0 )
return;

    paste = GDrawRequestSelection(awindow,sn_clipboard,type==1?"image/png":
		type==2?"image/svg":
		type==3?"image/bmp":
		type==4?"image/eps":"image/ps",&len);
    if ( paste==NULL )
return;

    temp = tmpfile();
    if ( temp!=NULL ) {
	fwrite(paste,1,len,temp);
	rewind(temp);
	if ( type>=4 ) {	/* eps/ps */
	    SCImportPSFile(sc,layer,temp,doclear,-1);
#ifndef _NO_LIBXML
	} else if ( type==2 ) {
	    SCImportSVG(sc,layer,NULL,paste,len,doclear);
#endif
	} else {
#ifndef _NO_LIBPNG
	    if ( type==1 )
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static double PasteFigureScale(SplineFont *newsf,SplineFont *oldsf) {
    FontView *fv;

    if ( newsf==oldsf )
return( 1.0 );
    for ( fv = fv_list; fv!=NULL && fv->sf!=oldsf; fv=fv->next );
    if ( fv==NULL )		/* Font we copied from has been closed */
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
	    gwwv_post_error(_("Anchor Lost"),_("At least one anchor point was lost when pasting from one font to another because no matching anchor class could be found in the new font."));
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
	    gwwv_post_error(_("Duplicate Anchor"),_("There is already an anchor point named %1$.40s in %2$.40s."),test->anchor->name,sc->name);
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
    buts[4] = NULL;
#if defined(FONTFORGE_CONFIG_GDRAW)
    buts[0] = _("_Yes"); buts[3] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
    buts[0] = GTK_STOCK_YES; buts[3] = GTK_STOCK_NO;
#endif
    buts[1] = _("Yes to all");
    buts[2] = _("No to all");
    ret = gwwv_ask(_("Different Fonts"),(const char **) buts,0,3,_("You are attempting to paste glyph instructions from one font to another. Generally this will not work unless the 'prep', 'fpgm' and 'cvt ' tables are the same.\nDo you want to continue anyway?"));
    if ( ret==0 )
return( true );
    if ( ret==3 )
return( false );
    dontask_parent = sc->parent;
    dontask_copied_from = copied_from;
    dontask_ret = ret==1;
return( dontask_ret );
}

/* when pasting from the fontview we do a clear first */
#ifdef FONTFORGE_CONFIG_TYPE3
static void _PasteToSC(SplineChar *sc,Undoes *paster,FontView *fv,int pasteinto,
	int layer, real trans[6], struct sfmergecontext *mc) {
#else
static void PasteToSC(SplineChar *sc,Undoes *paster,FontView *fv,int pasteinto,
	real trans[6], struct sfmergecontext *mc) {
    const int layer = ly_fore;
#endif
    DBounds bb;
    real transform[6];
    int width, vwidth;
    FontView *fvs;
    int xoff=0, yoff=0;
    int was_empty;
#ifdef FONTFORGE_CONFIG_PASTEAFTER
    RefChar *ref;
#endif

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
	SCPreserveState(sc,paster->undotype==ut_statehint);
	width = paster->u.state.width;
	vwidth = paster->u.state.vwidth;
	if (( pasteinto!=1 || paster->u.state.splines!=NULL ) && sc->ttf_instrs!=NULL ) {
	    free(sc->ttf_instrs); sc->ttf_instrs = NULL;
	    sc->ttf_instrs_len = 0;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    SCMarkInstrDlgAsChanged(sc);
#endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	}
	was_empty = sc->hstem==NULL && sc->vstem==NULL && sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs == NULL;
	if ( !pasteinto ) {
	    if ( !sc->parent->onlybitmaps )
		SCSynchronizeWidth(sc,width,sc->width,fv);
	    sc->vwidth = vwidth;
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
	    was_empty = true;
#ifdef FONTFORGE_CONFIG_PASTEAFTER
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
			sc->layers[layer].splines,transform,true);
		for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
		    ref->transform[4] += width;
		    SCReinstanciateRefChar(sc,ref);
		}
	    } else {
		xoff = sc->width;
		SCSynchronizeWidth(sc,sc->width+width,sc->width,fv);
	    }
#endif
	} else if ( pasteinto==3 && trans!=NULL ) {
	    xoff = trans[4]; yoff = trans[5];
	}
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( layer>=ly_fore && sc->layers[layer].splines==NULL &&
		sc->layers[layer].refs==NULL && sc->layers[layer].images==NULL &&
		sc->parent->multilayer ) {
	    /* pasting into an empty layer sets the fill/stroke */
	    sc->layers[layer].fill_brush = paster->u.state.fill_brush;
	    sc->layers[layer].stroke_pen = paster->u.state.stroke_pen;
	    sc->layers[layer].dofill = paster->u.state.dofill;
	    sc->layers[layer].dostroke = paster->u.state.dostroke;
	    sc->layers[layer].fillfirst = paster->u.state.fillfirst;
	}
#endif
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *temp = SplinePointListCopy(paster->u.state.splines);
	    if ( (pasteinto==2 || pasteinto==3 ) && (xoff!=0 || yoff!=0)) {
		transform[0] = transform[3] = 1; transform[1] = transform[2] = 0;
		transform[4] = xoff; transform[5] = yoff;
		temp = SplinePointListTransform(temp,transform,true);
	    }
	    if ( paster->was_order2 != sc->parent->order2 )
		temp = SplineSetsConvertOrder(temp,sc->parent->order2);
	    if ( sc->layers[layer].splines!=NULL ) {
		SplinePointList *e = sc->layers[layer].splines;
		while ( e->next!=NULL ) e = e->next;
		e->next = temp;
	    } else
		sc->layers[layer].splines = temp;
	}
	if ( !sc->searcherdummy )
	    APMerge(sc,paster->u.state.anchor);
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( paster->u.state.images!=NULL && sc->parent->multilayer ) {
	    ImageList *new, *cimg;
	    for ( cimg = paster->u.state.images; cimg!=NULL; cimg=cimg->next ) {
		new = galloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = sc->layers[layer].images;
		sc->layers[layer].images = new;
	    }
	    SCOutOfDateBackground(sc);
	}
#else
	/* Ignore any images, can't be in foreground level */
	/* but might be hints */
#endif
	if ( paster->undotype==ut_statehint || paster->undotype==ut_statename )
	    if ( !pasteinto ) {	/* Hints aren't meaningful unless we've cleared first */
		ExtractHints(sc,paster->u.state.hints,true);
		free(sc->ttf_instrs);
		if ( paster->u.state.instrs_len!=0 && sc->parent->order2 &&
			InstrsSameParent(sc,paster->copied_from)) {
		    sc->ttf_instrs = (uint8 *) copyn((char *) paster->u.state.instrs,paster->u.state.instrs_len);
		    sc->ttf_instrs_len = paster->u.state.instrs_len;
		} else {
		    sc->ttf_instrs = NULL;
		    sc->ttf_instrs_len = 0;
		}
	    }
	if ( paster->undotype==ut_statename ) {
	    SCSetMetaData(sc,paster->u.state.charname,
		    paster->u.state.unicodeenc==0xffff?-1:paster->u.state.unicodeenc,
		    paster->u.state.comment);
	    for ( fvs = fv_list; fvs!=NULL && fvs->sf!=paster->copied_from; fvs=fvs->next );
	    if ( fvs!=NULL ) {	/* If we can't translate the lookups, the PSTs are meaningless */
		PSTFree(sc->possub);
		mc->sf_from = fvs->sf; mc->sf_to = sc->parent;
		sc->possub = PSTCopy(paster->u.state.possub,sc,mc);
	    }
	}
	if ( paster->u.state.refs!=NULL ) {
	    RefChar *new, *refs;
	    SplineChar *rsc;
	    double scale = PasteFigureScale(sc->parent,paster->copied_from);
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		if ( sc->searcherdummy )
		    rsc = FindCharacter(sc->views->searcher->fv->sf,paster->copied_from,refs,NULL);
		else
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
		    rsc = FindCharacter(sc->parent,paster->copied_from,refs,NULL);
		if ( rsc!=NULL && SCDependsOnSC(rsc,sc))
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Self-referential character"),_("Attempt to make a character that refers to itself"));
#else
		    gwwv_post_error(_("Self-referential glyph"),_("Attempt to make a glyph that refers to itself"));
#endif
		else if ( rsc!=NULL ) {
		    new = RefCharCreate();
		    *new = *refs;
		    new->transform[4] *= scale; new->transform[5] *= scale;
		    new->transform[4] += xoff;  new->transform[5] += yoff;
#ifdef FONTFORGE_CONFIG_TYPE3
		    new->layers = NULL;
		    new->layer_cnt = 0;
#else
		    new->layers[0].splines = NULL;
#endif
		    new->sc = rsc;
		    new->next = sc->layers[layer].refs;
		    sc->layers[layer].refs = new;
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
	/* Bug here. we are assuming that the pasted hints are up to date */
	if ( was_empty && (sc->hstem!=NULL || sc->vstem!=NULL))
	    sc->changedsincelasthinted = false;
      break;
      case ut_width:
	SCPreserveWidth(sc);
	SCSynchronizeWidth(sc,paster->u.width,sc->width,fv);
	SCCharChangedUpdate(sc);
      break;
      case ut_vwidth:
	if ( !sc->parent->hasvmetrics )
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#else
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#endif
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
	/* FVTrans will preserve the state and update the chars */
      break;
    }
}

#ifdef FONTFORGE_CONFIG_TYPE3
static void PasteToSC(SplineChar *sc,Undoes *paster,FontView *fv,int pasteinto,
	real trans[6], struct sfmergecontext *mc) {
    if ( paster->undotype==ut_layers && sc->parent->multilayer ) {
	int lc, start, layer;
	Undoes *pl;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	Layer *old = sc->layers;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	for ( lc=0, pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next, ++lc );
	if ( !pasteinto ) {
	    start = ly_fore;
	    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		SplinePointListsFree(sc->layers[layer].splines);
		sc->layers[layer].splines = NULL;
		ImageListsFree(sc->layers[layer].images);
		sc->layers[layer].images = NULL;
		SCRemoveLayerDependents(sc,layer);
	    }
	} else
	    start = sc->layer_cnt;
	if ( start+lc > sc->layer_cnt ) {
	    sc->layers = grealloc(sc->layers,(start+lc)*sizeof(Layer));
	    for ( layer = sc->layer_cnt; layer<start+lc; ++layer )
		LayerDefault(&sc->layers[layer]);
	    sc->layer_cnt = start+lc;
	}
	for ( lc=0, pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next, ++lc )
	    _PasteToSC(sc,pl,fv,pasteinto,start+lc,trans,mc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	SCMoreLayers(sc,old);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    } else if ( paster->undotype==ut_layers ) {
	Undoes *pl;
	for ( pl = paster->u.multiple.mult; pl!=NULL; pl=pl->next ) {
	    _PasteToSC(sc,pl,fv,pasteinto,ly_fore,trans,mc);
	    pasteinto = true;		/* Merge other layers in */
	}
    } else
	_PasteToSC(sc,paster,fv,pasteinto,ly_fore,trans,mc);
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void DevTabInto(struct vr *vr) {
    ValDevTab *adjust;

    if ( vr->adjust==NULL )
return;		/* Nothing to do */
    adjust = chunkalloc(sizeof(ValDevTab));
    *adjust = *vr->adjust;
    if ( adjust->xadjust.corrections!=NULL ) {
	adjust->xadjust.corrections = galloc(adjust->xadjust.last_pixel_size-adjust->xadjust.first_pixel_size+1);
	memcpy(adjust->xadjust.corrections,vr->adjust->xadjust.corrections,adjust->xadjust.last_pixel_size-adjust->xadjust.first_pixel_size+1);
    }
    if ( adjust->yadjust.corrections!=NULL ) {
	adjust->yadjust.corrections = galloc(adjust->yadjust.last_pixel_size-adjust->yadjust.first_pixel_size+1);
	memcpy(adjust->yadjust.corrections,vr->adjust->yadjust.corrections,adjust->yadjust.last_pixel_size-adjust->yadjust.first_pixel_size+1);
    }
    if ( adjust->xadv.corrections!=NULL ) {
	adjust->xadv.corrections = galloc(adjust->xadv.last_pixel_size-adjust->xadv.first_pixel_size+1);
	memcpy(adjust->xadv.corrections,vr->adjust->xadv.corrections,adjust->xadv.last_pixel_size-adjust->xadv.first_pixel_size+1);
    }
    if ( adjust->yadv.corrections!=NULL ) {
	adjust->yadv.corrections = galloc(adjust->yadv.last_pixel_size-adjust->yadv.first_pixel_size+1);
	memcpy(adjust->yadv.corrections,vr->adjust->yadv.corrections,adjust->yadv.last_pixel_size-adjust->yadv.first_pixel_size+1);
    }
}
#endif

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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	DevTabInto(&pst->u.pair.vr[0]);
	DevTabInto(&pst->u.pair.vr[1]);
#endif
    } else if ( pst->type==pst_position ) {
	memcpy(&pst->u.pos,&frompst->u.pos,sizeof(struct vr));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	DevTabInto(&pst->u.pos);
#endif
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(ap->xadjust.corrections); free(ap->yadjust.corrections);
	ap->xadjust = fromap->xadjust;
	ap->yadjust = fromap->yadjust;
#endif
	ap->me = fromap->me;
    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( fromap->xadjust.corrections!=NULL ) {
	ap->xadjust.corrections = galloc(ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1);
	memcpy(ap->xadjust.corrections,fromap->xadjust.corrections,ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1);
    }
    if ( fromap->yadjust.corrections!=NULL ) {
	ap->yadjust.corrections = galloc(ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1);
	memcpy(ap->yadjust.corrections,fromap->yadjust.corrections,ap->yadjust.last_pixel_size-ap->xadjust.first_pixel_size+1);
    }
#endif
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( kp->adjust!=NULL )
	DeviceTableFree(kp->adjust);
    if ( fromkp->adjust!=NULL )
	kp->adjust = DeviceTableCopy(fromkp->adjust);
    else
	kp->adjust = NULL;
#endif
}

static void SCPasteLookups(SplineChar *sc,SplineChar *fromsc,int pasteinto,
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

#if 0
/* I don't think I want to do this. anything in the same lookup will be replaced */
/*  anyway, and I don't think I should clear other entries */
    if ( !pasteinto && sc!=fromsc ) {
	PSTFree(sc->possub); sc->possub = NULL;
	KernPairFree(sc->kerns); sc->kerns = NULL;
	KernPairFree(sc->vkerns); sc->vkerns = NULL;
    }
#endif

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
		_SCCharChangedUpdate(test2,2);
	    }
	}
    }
    if ( changed )
	_SCCharChangedUpdate(sc,2);
}

static void SCPasteLookupsMid(SplineChar *sc,Undoes *paster,int pasteinto,
	OTLookup **list, OTLookup **backpairlist, struct sfmergecontext *mc) {
    SplineChar *fromsc;

    (void) FindCharacter(sc->parent,paster->copied_from,
	    paster->u.state.refs,&fromsc);
    if ( fromsc==NULL ) {
	gwwv_post_error(_("Missing glyph"),_("Could not find original glyph"));
return;
    }
    SCPasteLookups(sc,fromsc,pasteinto,list,backpairlist,mc);
}

static int HasNonClass(OTLookup *otl) {
    struct lookup_subtable *sub;
    for ( sub = otl->subtables; sub!=NULL; sub=sub->next )
	if ( sub->kc==NULL )
return( true );

return( false );
}

static OTLookup **GetLookupsToCopy(SplineFont *sf,OTLookup ***backpairlist, int is_same) {
    int cnt, bcnt, ftot=0, doit, isgpos, i, ret;
    char **choices = NULL, *sel;
    OTLookup *otl, **list1=NULL, **list2=NULL, **list, **blist;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buttons[3];
    buttons[0] = _("_OK");
    buttons[1] = _("_Cancel");
    buttons[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buttons[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

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
/* GT:  pair. But we can also (separatedly) copy data where the current glyph */
/* GT:  is the second glyph in the kerning pair, and that's what this line */
/* GT:  refers to. The "%s" will be filled in with the lookup name */
			    char *format = _("Second glyph of %s");
			    char *space = galloc(strlen(format)+strlen(otl->lookup_name)+1);
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
	    gwwv_post_error(_("No Lookups"),_("No lookups to copy"));
return( NULL );
	}
	if ( !doit ) {
	    ftot = cnt;
	    choices = galloc((cnt+bcnt+2)*sizeof(char *));
	    sel = gcalloc(cnt+bcnt+1,1);
	    list1 = galloc(cnt*sizeof(OTLookup *));
	    if ( bcnt==0 ) {
		choices[cnt] = NULL;
		list2 = NULL;
	    } else {
		choices[cnt] = copy("-");
		choices[bcnt+cnt+1] = NULL;
		list2 = galloc(bcnt*sizeof(OTLookup *));
	    }
	}
    }
    ret = gwwv_choose_multiple(_("Lookups"),(const char **) choices,sel,bcnt==0?cnt: cnt+bcnt+1,
	    buttons, _("Choose which lookups to copy"));
    list = NULL;
    if ( ret>=0 ) {
	for ( i=cnt=0; i<ftot; ++i ) {
	    if ( sel[i] )
		++cnt;
	}
	list = galloc((cnt+1)*sizeof(OTLookup *));
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
		blist = galloc((cnt+1)*sizeof(OTLookup *));
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
#endif

static void SCPasteLookupsTop(SplineChar *sc,Undoes *paster) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    OTLookup **list, **backpairlist;
    SplineChar *fromsc;
    struct sfmergecontext mc;

    if ( paster->copied_from==NULL )
return;
    (void) FindCharacter(sc->parent,paster->copied_from,
	    paster->u.state.refs,&fromsc);
    if ( fromsc==NULL ) {
	gwwv_post_error(_("Missing glyph"),_("Could not find original glyph"));
return;
    }
    list = GetLookupsToCopy(fromsc->parent,&backpairlist,fromsc->parent==sc->parent);
    if ( list==NULL )
return;
    memset(&mc,0,sizeof(mc));
    mc.sf_from = paster->copied_from; mc.sf_to = sc->parent;
    SCPasteLookups(sc,fromsc,true,list,backpairlist,&mc);
    free(list);
    free(backpairlist);
    SFFinishMergeContext(&mc);
#endif
}

/* I wish I could get rid of this if FONTFORGE_CONFIG_NO_WINDOWING_UI	*/
/*  but FVReplaceOutlineWithReference depends on it, and I want that	*/
/*  available even if no UI						*/
static void _PasteToCV(CharView *cv,SplineChar *cvsc,Undoes *paster) {
    int refstate = 0;
    DBounds bb;
    real transform[6];
    int wasempty = false;

    if ( copybuffer.undotype == ut_none ) {
	if ( cv->drawmode==dm_grid )
return;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	SCCheckXClipboard(cv->gw,cvsc,cv->layerheads[cv->drawmode]-cvsc->layers,false);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return;
    }

    anchor_lost_warning = false;

    cv->lastselpt = NULL;
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
	wasempty =cv->drawmode==dm_fore && cvsc->layers[ly_fore].splines==NULL &&
		cvsc->layers[ly_fore].refs==NULL;
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( wasempty && cv->layerheads[dm_fore]->images==NULL &&
		cvsc->parent->multilayer ) {
	    /* pasting into an empty layer sets the fill/stroke */
	    cv->layerheads[dm_fore]->fill_brush = paster->u.state.fill_brush;
	    cv->layerheads[dm_fore]->stroke_pen = paster->u.state.stroke_pen;
	    cv->layerheads[dm_fore]->dofill = paster->u.state.dofill;
	    cv->layerheads[dm_fore]->dostroke = paster->u.state.dostroke;
	    cv->layerheads[dm_fore]->fillfirst = paster->u.state.fillfirst;
	}
#endif
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *spl, *new = SplinePointListCopy(paster->u.state.splines);
	    if ( paster->was_order2 != cvsc->parent->order2 )
		new = SplineSetsConvertOrder(new,cvsc->parent->order2 );
	    SplinePointListSelect(new,true);
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = cv->layerheads[cv->drawmode]->splines;
	    cv->layerheads[cv->drawmode]->splines = new;
	}
	if ( paster->undotype==ut_state && paster->u.state.images!=NULL ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	    int dm = cvsc->parent->multilayer ? cv->drawmode : dm_back;
#else
	    const int dm = dm_back;
	    /* Images can only be pasted into background, so do that */
	    /*  even if we aren't in background mode */
#endif
	    ImageList *new, *cimg;
	    for ( cimg = paster->u.state.images; cimg!=NULL; cimg=cimg->next ) {
		new = galloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = cv->layerheads[dm]->images;
		cv->layerheads[dm]->images = new;
	    }
	    SCOutOfDateBackground(cvsc);
	} else if ( paster->undotype==ut_statehint && cv->searcher==NULL ) {
	    ExtractHints(cvsc,paster->u.state.hints,true);
	    free(cvsc->ttf_instrs);
	    if ( paster->u.state.instrs_len!=0 && cvsc->parent->order2 &&
		    InstrsSameParent(cvsc,paster->copied_from)) {
		cvsc->ttf_instrs = (uint8 *) copyn((char *) paster->u.state.instrs,paster->u.state.instrs_len);
		cvsc->ttf_instrs_len = paster->u.state.instrs_len;
	    } else {
		cvsc->ttf_instrs = NULL;
		cvsc->ttf_instrs_len = 0;
	    }
	}
	if ( paster->u.state.anchor!=NULL && cv->drawmode==dm_fore && !cvsc->searcherdummy )
	    APMerge(cvsc,paster->u.state.anchor);
	if ( paster->u.state.refs!=NULL && cv->drawmode==dm_fore ) {
	    RefChar *new, *refs;
	    SplineChar *sc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		if ( cv->searcher!=NULL )
		    sc = FindCharacter(cv->searcher->fv->sf,paster->copied_from,refs,NULL);
		else {
		    sc = FindCharacter(cvsc->parent,paster->copied_from,refs,NULL);
		    if ( sc!=NULL && SCDependsOnSC(sc,cvsc)) {
			gwwv_post_error(_("Self-referential character"),_("Attempt to make a character that refers to itself"));
			sc = NULL;
		    }
		}
		if ( sc!=NULL ) {
		    new = RefCharCreate();
		    *new = *refs;
#ifdef FONTFORGE_CONFIG_TYPE3
		    new->layers = NULL;
		    new->layer_cnt = 0;
#else
		    new->layers[0].splines = NULL;
#endif
		    new->sc = sc;
		    new->selected = true;
		    new->next = cvsc->layers[ly_fore].refs;
		    cvsc->layers[ly_fore].refs = new;
		    SCReinstanciateRefChar(cvsc,new);
		    SCMakeDependent(cvsc,sc);
		} else {
		    PasteNonExistantRefCheck(cvsc,paster,refs,&refstate);
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
		    sc = FindCharacter(cv->searcher->fv->sf,paster->copied_from,refs,NULL);
		else
		    sc = FindCharacter(cvsc->parent,paster->copied_from,refs,NULL);
		if ( sc!=NULL ) {
		    new = SplinePointListTransform(SplinePointListCopy(sc->layers[ly_back].splines),refs->transform,true);
		    SplinePointListSelect(new,true);
		    if ( new!=NULL ) {
			for ( spl = new; spl->next!=NULL; spl = spl->next );
			spl->next = cvsc->layers[ly_back].splines;
			cvsc->layers[ly_back].splines = new;
		    }
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
	if ( wasempty ) {
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
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#else
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#endif
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
    }
}

void PasteToCV(CharView *cv) {
    _PasteToCV(cv,cv->sc,&copybuffer);
    if ( cv->sc->blended && cv->drawmode==dm_fore ) {
	int j, gid = cv->sc->orig_pos;
	MMSet *mm = cv->sc->parent->mm;
	for ( j=0; j<mm->instance_count; ++j )
	    _PasteToCV(cv,mm->instances[j]->glyphs[gid],&copybuffer);
    }
}
/* See comment at _PasteToCV */
/* #endif */		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */


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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void BCCopySelected(BDFChar *bc,int pixelsize,int depth) {

    CopyBufferFreeGrab();

    memset(&copybuffer,'\0',sizeof(copybuffer));
    copybuffer.undotype = ut_bitmapsel;
    if ( bc->selection!=NULL )
	copybuffer.u.bmpstate.selection = BDFFloatCopy(bc->selection);
    else
	copybuffer.u.bmpstate.selection = BDFFloatCreate(bc,bc->xmin,bc->xmax,
		bc->ymin,bc->ymax, false);
    copybuffer.u.bmpstate.pixelsize = pixelsize;
    copybuffer.u.bmpstate.depth = depth;
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static Undoes *BCCopyAll(BDFChar *bc,int pixelsize, int depth) {
    Undoes *cur;

    cur = chunkalloc(sizeof(Undoes));
    if ( bc==NULL )
	cur->undotype = ut_noop;
    else {
	BCCompressBitmap(bc);
	cur->undotype = ut_bitmap;
	cur->u.bmpstate.width = bc->width;
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
    cur->u.bmpstate.depth = depth;
return( cur );
}

static void _PasteToBC(BDFChar *bc,int pixelsize, int depth, Undoes *paster, int clearfirst, FontView *fv) {
    BDFFloat temp;

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
	memset(bc->bitmap,0,bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	temp.xmin = paster->u.bmpstate.xmin; temp.xmax = paster->u.bmpstate.xmax;
	temp.ymin = paster->u.bmpstate.ymin; temp.ymax = paster->u.bmpstate.ymax;
	temp.bytes_per_line = paster->u.bmpstate.bytes_per_line; temp.byte_data = depth!=1;
	temp.bitmap = paster->u.bmpstate.bitmap;
	bc->selection = BDFFloatConvert(&temp,depth,paster->u.bmpstate.depth);
	BCFlattenFloat(bc);
	BCCompressBitmap(bc);
	bc->selection = BDFFloatConvert(paster->u.bmpstate.selection,depth,paster->u.bmpstate.depth);
	bc->width = paster->u.bmpstate.width;
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
	    _PasteToBC(bc,pixelsize,depth,paster->u.composit.bitmaps,clearfirst,fv);
	else {
	    Undoes *b;
	    for ( b = paster->u.composit.bitmaps;
		    b!=NULL && b->u.bmpstate.pixelsize!=pixelsize;
		    b = b->next );
	    if ( b!=NULL )
		_PasteToBC(bc,pixelsize,depth,paster->u.composit.bitmaps,clearfirst,fv);
	}
      break;
      case ut_multiple:
	_PasteToBC(bc,pixelsize,depth,paster->u.multiple.mult,clearfirst,fv);
      break;
    }
}

void PasteToBC(BDFChar *bc,int pixelsize,int depth,FontView *fv) {
    _PasteToBC(bc,pixelsize,depth,&copybuffer,false,fv);
}

void FVCopyWidth(FontView *fv,enum undotype ut) {
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

void FVCopyAnchors(FontView *fv) {
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

void FVCopy(FontView *fv, enum fvcopy_type fullcopy) {
    int i, any = false;
    BDFFont *bdf;
    Undoes *head=NULL, *last=NULL, *cur;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;
    extern int onlycopydisplayed;
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
	if (( onlycopydisplayed && fv->filled==fv->show ) || fullcopy==ct_lookups ) {
	    cur = SCCopyAll(sc,fullcopy);
	} else if ( onlycopydisplayed ) {
	    cur = BCCopyAll(gid==-1?NULL:fv->show->glyphs[gid],fv->show->pixelsize,BDFDepth(fv->show));
	} else {
	    state = SCCopyAll(sc,fullcopy);
	    bhead = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		BDFChar *bdfc = gid==-1 || gid>=bdf->glyphcnt? NULL : bdf->glyphs[gid];
		bcur = BCCopyAll(bdfc,bdf->pixelsize,BDFDepth(bdf));
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    XClipCheckEps();
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void MVCopyChar(MetricsView *mv, SplineChar *sc, enum fvcopy_type fullcopy) {
    BDFFont *bdf;
    Undoes *cur=NULL;
    Undoes *bhead=NULL, *blast=NULL, *bcur;
    Undoes *state;
    extern int onlycopydisplayed;

    if (( onlycopydisplayed && mv->bdf==NULL ) || fullcopy == ct_lookups ) {
	cur = SCCopyAll(sc,fullcopy);
    } else if ( onlycopydisplayed ) {
	cur = BCCopyAll(BDFMakeGID(mv->bdf,sc->orig_pos),mv->bdf->pixelsize,BDFDepth(mv->bdf));
    } else {
	state = SCCopyAll(sc,fullcopy);
	bhead = NULL;
	for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    bcur = BCCopyAll(BDFMakeGID(bdf,sc->orig_pos),bdf->pixelsize,BDFDepth(bdf));
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static BDFFont *BitmapCreateCheck(FontView *fv,int *yestoall, int first, int pixelsize, int depth) {
    int yes = 0;
    BDFFont *bdf = NULL;

    if ( *yestoall>0 && first ) {
	char *buts[5];
	char buf[20];
	if ( depth!=1 )
	    sprintf( buf, "%d@%d", pixelsize, depth );
	else
	    sprintf( buf, "%d", pixelsize );
#if defined(FONTFORGE_CONFIG_GDRAW)
	buts[0] = _("_Yes");
	buts[3] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
	buts[0] = GTK_STOCK_YES;
	buts[3] = GTK_STOCK_NO;
#endif
	buts[1] = _("Yes to All");
	buts[2] = _("No to All");
	buts[4] = NULL;
	yes = gwwv_ask(_("Bitmap Paste"),(const char **) buts,0,3,"The clipboard contains a bitmap character of size %s,\na size which is not in your database.\nWould you like to create a bitmap font of that size,\nor ignore this character?",buf);
	if ( yes==1 )
	    *yestoall = true;
	else if ( yes==2 )
	    *yestoall = -1;
	else
	    yes= yes!=3;
    }
    if ( yes==1 || *yestoall ) {
	void *freetypecontext = FreeTypeFontContext(fv->sf,NULL,NULL);
	if ( freetypecontext )
	    bdf = SplineFontFreeTypeRasterize(freetypecontext,pixelsize,depth);
	else
	    bdf = SplineFontAntiAlias(fv->sf,pixelsize,1<<(depth/2));
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

void PasteIntoFV(FontView *fv,int pasteinto,real trans[6]) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int i, j, cnt=0, gid;
    int yestoall=0, first=true;
    uint8 *oldsel = fv->selected;
    extern int onlycopydisplayed;
    SplineFont *sf = fv->sf, *origsf = sf;
    MMSet *mm = sf->mm;
    struct sfmergecontext mc;
    OTLookup **list = NULL, **backpairlist=NULL;

    fv->refstate = 0;
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
	list = GetLookupsToCopy(copybuffer.copied_from,&backpairlist,
		copybuffer.copied_from==fv->sf);
	if ( list==NULL )
return;
    }

    if ( copybuffer.undotype == ut_none ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	j = -1;
	forever {
	    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
		SCCheckXClipboard(fv->gw,SFMakeChar(sf,fv->map,i),ly_fore,!pasteinto);
	    ++j;
	    if ( mm==NULL || mm->normal!=origsf || j>=mm->instance_count )
	break;
	    sf = mm->instances[j];
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return;
    }

    /* If they select exactly one character but there are more things in the */
    /*  copy buffer, then temporarily change the selection so that everything*/
    /*  in the copy buffer gets pasted (into chars immediately following sele*/
    /*  cted one (unless we run out of chars...)) */
    if ( cnt==1 && cur->undotype==ut_multiple && cur->u.multiple.mult->next!=NULL ) {
	Undoes *tot; int j;
	for ( cnt=0, tot=cur->u.multiple.mult; tot!=NULL; ++cnt, tot=tot->next );
	fv->selected = galloc(fv->map->enccount);
	memcpy(fv->selected,oldsel,fv->map->enccount);
	for ( i=0; i<fv->map->enccount && !fv->selected[i]; ++i );
	for ( j=0; j<cnt && i+j<fv->map->enccount; ++j )
	    fv->selected[i+j] = 1;
	cnt = j;
    }

    anchor_lost_warning = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Pasting..."),_("Pasting..."),0,cnt,1);
#endif

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
	forever {
	    switch ( cur->undotype ) {
	      case ut_noop:
	      break;
	      case ut_statelookup:
		SCPasteLookupsMid(SFMakeChar(sf,fv->map,i),cur,pasteinto,list,backpairlist,&mc);
	      break;
	      case ut_state: case ut_width: case ut_vwidth:
	      case ut_lbearing: case ut_rbearing:
	      case ut_statehint: case ut_statename:
	      case ut_layers: case ut_anchors:
		if ( !sf->hasvmetrics && cur->undotype==ut_vwidth) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#endif
 goto err;
		}
		PasteToSC(SFMakeChar(sf,fv->map,i),cur,fv,pasteinto,trans,&mc);
	      break;
	      case ut_bitmapsel: case ut_bitmap:
		if ( onlycopydisplayed && fv->show!=fv->filled )
		    _PasteToBC(BDFMakeChar(fv->show,fv->map,i),fv->show->pixelsize,BDFDepth(fv->show),cur,!pasteinto,fv);
		else {
		    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth); bdf=bdf->next );
		    if ( bdf==NULL ) {
			bdf = BitmapCreateCheck(fv,&yestoall,first,cur->u.bmpstate.pixelsize,cur->u.bmpstate.depth);
			first = false;
		    }
		    if ( bdf!=NULL )
			_PasteToBC(BDFMakeChar(bdf,fv->map,i),bdf->pixelsize,BDFDepth(bdf),cur,!pasteinto,fv);
		}
	      break;
	      case ut_composit:
		if ( cur->u.composit.state!=NULL )
		    PasteToSC(SFMakeChar(sf,fv->map,i),cur->u.composit.state,fv,pasteinto,trans,&mc);
		for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
		    for ( bdf=sf->bitmaps; bdf!=NULL &&
			    (bdf->pixelsize!=bmp->u.bmpstate.pixelsize || BDFDepth(bdf)!=bmp->u.bmpstate.depth);
			    bdf=bdf->next );
		    if ( bdf==NULL )
			bdf = BitmapCreateCheck(fv,&yestoall,first,bmp->u.bmpstate.pixelsize,bmp->u.bmpstate.depth);
		    if ( bdf!=NULL )
			_PasteToBC(BDFMakeChar(bdf,fv->map,i),bdf->pixelsize,BDFDepth(bdf),bmp,!pasteinto,fv);
		}
		first = false;
	      break;
	    }
	    ++j;
	    if ( mm==NULL || mm->normal!=origsf || j>=mm->instance_count )
	break;
	    sf = mm->instances[j];
	}
	cur = cur->next;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
#endif
    if ( oldsel!=fv->selected )
	free(oldsel);
    SFFinishMergeContext(&mc);
    free(list); free(backpairlist);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void PasteIntoMV(MetricsView *mv,SplineChar *sc, int doclear) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int yestoall=0, first=true;
    extern int onlycopydisplayed;
    struct sfmergecontext mc;

    memset(&mc,0,sizeof(mc));
    mc.sf_to = mv->fv->sf;

    cur = &copybuffer;

    if ( copybuffer.undotype == ut_none ) {
	SCCheckXClipboard(mv->gw,sc,dm_fore,doclear);
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
	if ( !mv->fv->sf->hasvmetrics && cur->undotype==ut_vwidth) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("No Vertical Metrics"),_("This font does not have vertical metrics enabled.\nUse Element->Font Info to enable them."));
#endif
return;
	}
	PasteToSC(sc,cur,mv->fv,!doclear,NULL,&mc);
      break;
      case ut_bitmapsel: case ut_bitmap:
	if ( onlycopydisplayed && mv->bdf!=NULL )
	    _PasteToBC(BDFMakeChar(mv->bdf,mv->fv->map,mv->fv->map->backmap[sc->orig_pos]),mv->bdf->pixelsize,BDFDepth(mv->bdf),cur,doclear,mv->fv);
	else {
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL &&
		    (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth);
		    bdf=bdf->next );
	    if ( bdf==NULL ) {
		bdf = BitmapCreateCheck(mv->fv,&yestoall,first,cur->u.bmpstate.pixelsize,cur->u.bmpstate.depth);
		first = false;
	    }
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,mv->fv->map,mv->fv->map->backmap[sc->orig_pos]),bdf->pixelsize,BDFDepth(bdf),cur,doclear,mv->fv);
	}
      break;
      case ut_composit:
	if ( cur->u.composit.state!=NULL )
	    PasteToSC(sc,cur->u.composit.state,mv->fv,!doclear,NULL,&mc);
	for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
	    for ( bdf=mv->fv->sf->bitmaps; bdf!=NULL &&
		    (bdf->pixelsize!=bmp->u.bmpstate.pixelsize || BDFDepth(bdf)!=bmp->u.bmpstate.depth);
		    bdf=bdf->next );
	    if ( bdf==NULL )
		bdf = BitmapCreateCheck(mv->fv,&yestoall,first,bmp->u.bmpstate.pixelsize,bmp->u.bmpstate.depth);
	    if ( bdf!=NULL )
		_PasteToBC(BDFMakeChar(bdf,mv->fv->map,mv->fv->map->backmap[sc->orig_pos]),bdf->pixelsize,BDFDepth(bdf),bmp,doclear,mv->fv);
	}
	first = false;
      break;
    }
    SFFinishMergeContext(&mc);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
