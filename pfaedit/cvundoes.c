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
	new = malloc(sizeof(RefChar));
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

static ImageList *ImagesCopyState(CharView *cv) {
    ImageList *head=NULL, *last=NULL, *new, *cimg;

    if ( cv->drawmode!=dm_back || cv->sc->backimages==NULL )
return( NULL );
    for ( cimg = cv->sc->backimages; cimg!=NULL; cimg=cimg->next ) {
	new = malloc(sizeof(RefChar));
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

void UndoesFree(Undoes *undo) {
    Undoes *unext;

    while ( undo!=NULL ) {
	unext = undo->next;
	switch ( undo->undotype ) {
	  case ut_noop:
	  case ut_width:
	    /* Nothing else to free */;
	  break;
	  case ut_state: case ut_tstate:
	    SplinePointListsFree(undo->u.state.splines);
	    RefCharsFree(undo->u.state.refs);
	    ImageListsFree(undo->u.state.images);
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
	free(undo);
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
    Undoes *undo = calloc(1,sizeof(Undoes));

    undo->undotype = ut_state;
    undo->u.state.width = cv->sc->width;
    undo->u.state.splines = SplinePointListCopy(*cv->heads[cv->drawmode]);
    if ( cv->drawmode==dm_fore )
	undo->u.state.refs = RefCharsCopyState(cv->sc);
    undo->u.state.images = ImagesCopyState(cv);
return( CVAddUndo(cv,undo));
}

Undoes *SCPreserveState(SplineChar *sc) {
    Undoes *undo = calloc(1,sizeof(Undoes));

    undo->undotype = ut_state;
    undo->u.state.width = sc->width;
    undo->u.state.splines = SplinePointListCopy(sc->splines);
    undo->u.state.refs = RefCharsCopyState(sc);
    undo->u.state.images = NULL;
return( AddUndo(undo,&sc->undoes[0],&sc->redoes[0]));
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
    Undoes *undo = calloc(1,sizeof(Undoes));

    undo->undotype = ut_width;
    undo->u.width = width;
return( CVAddUndo(cv,undo));
}

Undoes *SCPreserveWidth(SplineChar *sc) {
    Undoes *undo = calloc(1,sizeof(Undoes));

    undo->undotype = ut_width;
    undo->u.state.width = sc->width;
return( AddUndo(undo,&sc->undoes[0],&sc->redoes[0]));
}

Undoes *BCPreserveState(BDFChar *bc) {
    Undoes *undo = calloc(1,sizeof(Undoes));

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

static void CVUndoAct(CharView *cv,Undoes *undo) {
    SplineChar *sc;

    sc = cv->sc;
    switch ( undo->undotype ) {
      case ut_noop:
      break;
      case ut_width: {
	int width = sc->width;
	if ( sc->width!=undo->u.width )
	    SCSynchronizeWidth(sc,undo->u.width,width,cv->fv);
	undo->u.width = width;
      } break;
      case ut_state: case ut_tstate: {
	SplinePointList *spl = *cv->heads[cv->drawmode];
	if ( cv->drawmode==dm_fore ) {
	    int width = sc->width;
	    if ( sc->width!=undo->u.state.width )
		SCSynchronizeWidth(sc,undo->u.state.width,width,cv->fv);
	    undo->u.state.width = width;
	}
	*cv->heads[cv->drawmode] = undo->u.state.splines;
	if ( cv->drawmode==dm_fore && !RefCharsMatch(undo->u.state.refs,sc->refs)) {
	    RefChar *refs = RefCharsCopyState(cv->sc);
	    FixupRefChars(sc,undo->u.state.refs);
	    undo->u.state.refs = refs;
	}
	if ( cv->drawmode==dm_back && !ImagesMatch(undo->u.state.images,sc->backimages)) {
	    ImageList *images = ImagesCopyState(cv);
	    FixupImages(sc,undo->u.state.images);
	    undo->u.state.images = images;
	    SCOutOfDateBackground(cv->sc);
	}
	undo->u.state.splines = spl;
	if ( undo->u.state.lbearingchange ) {
	    undo->u.state.lbearingchange = -undo->u.state.lbearingchange;
	    SCSynchronizeLBearing(sc,NULL,undo->u.state.lbearingchange);
	}
      } break;
      default:
	GDrawIError( "Unknown undo type in CVUndoAct: %d", undo->undotype );
      break;
    }
}

void CVDoUndo(CharView *cv) {
    Undoes *undo = *cv->uheads[cv->drawmode];

    if ( undo==NULL )		/* Shouldn't happen */
return;
    *cv->uheads[cv->drawmode] = undo->next;
    undo->next = NULL;
    CVUndoAct(cv,undo);
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
    CVUndoAct(cv,undo);
    undo->next = *cv->uheads[cv->drawmode];
    *cv->uheads[cv->drawmode] = undo;
    CVCharChangedUpdate(cv);
    cv->lastselpt = NULL;
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
	for ( img=cv->sc->backimages, uimg=undo->u.state.images; uimg!=NULL;
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
	GDrawIError( "Unknown undo type in CVUndoAct: %d", undo->undotype );
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
    BCCharChangedUpdate(bc,fv);
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
    BCCharChangedUpdate(bc,fv);
return;
}

/* **************************** Cut, Copy & Paste *************************** */

static Undoes copybuffer;

static void CopyBufferFree(void) {

    switch( copybuffer.undotype ) {
      case ut_state:
	SplinePointListsFree(copybuffer.u.state.splines);
	RefCharsFree(copybuffer.u.state.refs);
	ImageListsFree(copybuffer.u.state.images);
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

int CopyContainsSomething(void) {
return( copybuffer.undotype==ut_state || copybuffer.undotype==ut_tstate ||
	copybuffer.undotype==ut_width || copybuffer.undotype==ut_composit ||
	copybuffer.undotype==ut_noop );
}

int CopyContainsBitmap(void) {
return( copybuffer.undotype==ut_bitmapsel || copybuffer.undotype==ut_composit || copybuffer.undotype==ut_noop );
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
    copybuffer.u.state.refs = ref = gcalloc(1,sizeof(RefChar));
    ref->unicode_enc = sc->unicodeenc;
    ref->local_enc = sc->enc;
    ref->adobe_enc = getAdobeEnc(sc->name);
    ref->transform[0] = ref->transform[3] = 1.0;
}

void CopySelected(CharView *cv) {

    CopyBufferFree();

    copybuffer.undotype = ut_state;
    copybuffer.u.state.width = cv->sc->width;
    copybuffer.u.state.splines = SplinePointListCopySelected(*cv->heads[cv->drawmode]);
    if ( cv->drawmode==dm_fore ) {
	RefChar *refs, *new;
	for ( refs = cv->sc->refs; refs!=NULL; refs = refs->next ) if ( refs->selected ) {
	    new = galloc(sizeof(RefChar));
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
	    new = galloc(sizeof(RefChar));
	    *new = *imgs;
	    new->next = copybuffer.u.state.images;
	    copybuffer.u.state.images = new;
	}
    }
}

static Undoes *SCCopyAll(SplineChar *sc,int full) {
    Undoes *cur;
    RefChar *ref;

    cur = gcalloc(1,sizeof(Undoes));
    if ( sc==NULL ) {
	cur->undotype = ut_noop;
    } else {
	cur->undotype = ut_state;
	cur->u.state.width = sc->width;
	if ( full ) {
	    cur->u.state.splines = SplinePointListCopy(sc->splines);
	    cur->u.state.refs = RefCharsCopyState(sc);
	    cur->u.state.images = NULL;
	} else {		/* Or just make a reference */
	    cur->u.state.refs = ref = gcalloc(1,sizeof(RefChar));
	    ref->unicode_enc = sc->unicodeenc;
	    ref->local_enc = sc->enc;
	    ref->adobe_enc = getAdobeEnc(sc->name);
	    ref->transform[0] = ref->transform[3] = 1.0;
	}
    }
return( cur );
}

void CopyWidth(CharView *cv) {

    CopyBufferFree();

    copybuffer.undotype = ut_width;
    copybuffer.u.width = cv->sc->width;
}

static SplineChar *FindCharacter(SplineFont *sf,RefChar *rf) {
    extern char *AdobeStandardEncoding[256];
    int i;

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

/* when pasting from the fontview we do a clear first */
static void PasteToSC(SplineChar *sc,Undoes *paster,FontView *fv) {

    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_state:
	sc->parent->onlybitmaps = false;
	SCPreserveState(sc);
	SCSynchronizeWidth(sc,paster->u.state.width,sc->width,fv);
	SplinePointListsFree(sc->splines);
	sc->splines = NULL;
	if ( paster->u.state.splines!=NULL )
	    sc->splines = SplinePointListCopy(paster->u.state.splines);
	/* Ignore any images, can't be in foreground level */
	SCRemoveDependents(sc);
	if ( paster->u.state.refs!=NULL ) {
	    RefChar *new, *refs;
	    SplineChar *rsc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		rsc = FindCharacter(sc->parent,refs);
		if ( rsc!=NULL && SCDependsOnSC(rsc,sc))
		    GDrawError("Attempt to make a character that refers to itself");
		else if ( rsc!=NULL ) {
		    new = galloc(sizeof(RefChar));
		    *new = *refs;
		    new->splines = NULL;
		    new->sc = rsc;
		    new->next = sc->refs;
		    sc->refs = new;
		    SCReinstanciateRefChar(sc,new);
		    SCMakeDependent(sc,rsc);
		}
	    }
	}
	SCCharChangedUpdate(sc,fv);
      break;
      case ut_width:
	SCPreserveState(sc);
	SCSynchronizeWidth(sc,paster->u.width,sc->width,fv);
	SCCharChangedUpdate(sc,fv);
      break;
    }
}


static void _PasteToCV(CharView *cv,Undoes *paster) {

    cv->lastselpt = NULL;
    switch ( paster->undotype ) {
      case ut_noop:
      break;
      case ut_state:
	if ( cv->drawmode==dm_fore && cv->sc->splines==NULL && cv->sc->refs==NULL )
	    SCSynchronizeWidth(cv->sc,paster->u.state.width,cv->sc->width,cv->fv);
	if ( paster->u.state.splines!=NULL ) {
	    SplinePointList *spl, *new = SplinePointListCopy(paster->u.state.splines);
	    SplinePointListSelect(new,true);
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = *cv->heads[cv->drawmode];
	    *cv->heads[cv->drawmode] = new;
	}
	if ( paster->u.state.images!=NULL ) {
	    /* Images can only be pasted into background, so do that */
	    /*  even if we aren't in background mode */
	    ImageList *new, *cimg;
	    for ( cimg = paster->u.state.images; cimg!=NULL; cimg=cimg->next ) {
		new = galloc(sizeof(ImageList));
		*new = *cimg;
		new->selected = true;
		new->next = cv->sc->backimages;
		cv->sc->backimages = new;
	    }
	    SCOutOfDateBackground(cv->sc);
	}
	if ( paster->u.state.refs!=NULL && cv->drawmode==dm_fore ) {
	    RefChar *new, *refs;
	    SplineChar *sc;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
		sc = FindCharacter(cv->sc->parent,refs);
		if ( sc!=NULL && SCDependsOnSC(sc,cv->sc))
		    GDrawError("Attempt to make a character that refers to itself");
		else if ( sc!=NULL ) {
		    new = galloc(sizeof(RefChar));
		    *new = *refs;
		    new->splines = NULL;
		    new->sc = sc;
		    new->selected = true;
		    new->next = cv->sc->refs;
		    cv->sc->refs = new;
		    SCReinstanciateRefChar(cv->sc,new);
		    SCMakeDependent(cv->sc,sc);
		}
	    }
	} else if ( paster->u.state.refs!=NULL && cv->drawmode==dm_back ) {
	    /* Paste the CONTENTS of the referred character into this one */
	    /*  (background contents I think) */
	    RefChar *refs;
	    SplineChar *sc;
	    SplinePointList *new, *spl;
	    for ( refs = paster->u.state.refs; refs!=NULL; refs=refs->next ) {
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
	SCSynchronizeWidth(cv->sc,paster->u.width,cv->sc->width,cv->fv);
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

    cur = calloc(1,sizeof(Undoes));
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
	BCCharChangedUpdate(bc,fv);
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
	BCCharChangedUpdate(bc,fv);
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

void FVCopyWidth(FontView *fv) {
    Undoes *head=NULL, *last=NULL, *cur;
    int i;

    CopyBufferFree();

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	cur = gcalloc(1,sizeof(Undoes));
	cur->undotype = ut_width;
	if ( fv->sf->chars[i]!=NULL )
	    cur->undotype = ut_noop;
	else
	    cur->u.width = fv->sf->ascent+fv->sf->descent;
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

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	if ( fv->onlycopydisplayed && fv->filled==fv->show ) {
	    cur = SCCopyAll(fv->sf->chars[i],fullcopy);
	} else if ( fv->onlycopydisplayed ) {
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
		cur = gcalloc(1,sizeof(Undoes));
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

static BDFFont *BitmapCreateCheck(FontView *fv,int *yestoall, int first, int pixelsize) {
    int yes = 0;
    BDFFont *bdf = NULL;

    if ( !*yestoall && first ) {
	static unichar_t title[] = { 'B','i','t','m','a','p',' ','P','a','s','t','e',  '\0' };
	static unichar_t yesb[] = { 'Y','e','s',  '\0' };
	static unichar_t yestoallb[] = { 'Y','e','s',' ','t','o',' ','A','l','l',  '\0' };
	static unichar_t no[] = { 'N','o',  '\0' };
	static const unichar_t *buts[] = { yesb, yestoallb, no, NULL };
	static unichar_t mn[] = { 'Y', 'A', 'N', '\0' };
	char buf[300]; unichar_t ubuf[300];
	sprintf( buf, "The clipboard contains a bitmap character of size %d,\na size which is not in your database.\nWould you like to create a bitmap font of that size,\nor ignore this character?",
		pixelsize );
	uc_strcpy(ubuf,buf);
	yes = GWidgetAskCentered(title,ubuf,buts,mn,0,2);
	if ( yes==1 )
	    *yestoall = true;
	else
	    yes= yes!=2;
    }
    if ( yes || *yestoall ) {
	bdf = SplineFontRasterize(fv->sf,pixelsize,false);
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	SFOrderBitmapList(fv->sf);
    }
return( bdf );
}

void PasteIntoFV(FontView *fv) {
    Undoes *cur=NULL, *bmp;
    BDFFont *bdf;
    int i, cnt=0;
    static unichar_t pasting[] = { 'P','a','s','t','i','n','g',' ','.','.','.',  '\0' };
    int yestoall=0, first=true;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,pasting,pasting,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	if ( cur==NULL ) {
	    cur = &copybuffer;
	    if ( cur->undotype==ut_multiple )
		cur = cur->u.multiple.mult;
	}
	switch ( cur->undotype ) {
	  case ut_noop:
	  break;
	  case ut_state: case ut_width:
	    PasteToSC(SFMakeChar(fv->sf,i),cur,fv);
	  break;
	  case ut_bitmapsel: case ut_bitmap:
	    if ( fv->onlycopydisplayed && fv->show!=fv->filled )
		_PasteToBC(BDFMakeChar(fv->show,i),fv->show->pixelsize,cur,true,fv);
	    else {
		for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize!=cur->u.bmpstate.pixelsize; bdf=bdf->next );
		if ( bdf==NULL ) {
		    bdf = BitmapCreateCheck(fv,&yestoall,first,cur->u.bmpstate.pixelsize);
		    first = false;
		}
		if ( bdf!=NULL )
		    _PasteToBC(BDFMakeChar(bdf,i),bdf->pixelsize,cur,true,fv);
	    }
	  break;
	  case ut_composit:
	    if ( cur->u.composit.state!=NULL )
		PasteToSC(SFMakeChar(fv->sf,i),cur->u.composit.state,fv);
	    for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
		for ( bdf=fv->sf->bitmaps; bdf!=NULL &&
			bdf->pixelsize!=bmp->u.bmpstate.pixelsize; bdf=bdf->next );
		if ( bdf==NULL )
		    bdf = BitmapCreateCheck(fv,&yestoall,first,bmp->u.bmpstate.pixelsize);
		if ( bdf!=NULL )
		    _PasteToBC(BDFMakeChar(bdf,i),bdf->pixelsize,bmp,true,fv);
	    }
	     first = false;
	  break;
	}
	cur = cur->next;
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}
