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
#include "fontforgeui.h"
#include "cvundoes.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include "search.h"

static SearchView *searcher=NULL;

#define CID_Allow	1000
#define CID_Flipping	1001
#define CID_Scaling	1002
#define CID_Rotating	1003
#define CID_Selected	1004
#define CID_Find	1005
#define CID_FindAll	1006
#define CID_Replace	1007
#define CID_ReplaceAll	1008
#define CID_Cancel	1009
#define CID_TopBox	1010
#define CID_Fuzzy	1011
#define CID_Endpoints	1012

static double old_fudge = .001;

static void SVSelectSC(SearchView *sv) {
    SplineChar *sc = sv->sd.curchar;
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *rf;
    int i;
    int layer = sv->sd.fv->active_layer;

    /* Deselect all */;
    for ( spl = sc->layers[layer].splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first ;; ) {
	    sp->selected = false;
	    if ( sp->next == NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	if ( rf->selected ) rf->selected = false;

    if ( sv->sd.subpatternsearch ) {
	spl = sv->sd.matched_spl;
	for ( sp = sv->sd.matched_sp; ; ) {
	    sp->selected = true;
	    if ( sp->next == NULL || sv->sd.last_sp==NULL || sp==sv->sd.last_sp )
	break;
	    sp = sp->next->to;
	    /* Ok to wrap back to first */
	}
    } else {
	for ( rf=sc->layers[layer].refs, i=0; rf!=NULL; rf=rf->next, ++i )
	    if ( sv->sd.matched_refs&(1<<i) )
		rf->selected = true;
	for ( spl = sc->layers[layer].splines,i=0; spl!=NULL; spl = spl->next, ++i ) {
	    if ( sv->sd.matched_ss&(1<<i) ) {
		for ( sp=spl->first ;; ) {
		    sp->selected = true;
		    if ( sp->next == NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
    }
    SCUpdateAll(sc);
    sc->changed_since_search = false;
}

static int DoFindOne(SearchView *sv,int startafter) {
    int i, gid;
    SplineChar *startcur = sv->sd.curchar;

    /* It is possible that some idiot deleted the current character since */
    /*  the last search... do some mild checks */
    if ( sv->sd.curchar!=NULL &&
	    sv->sd.curchar->parent == sv->sd.fv->sf &&
	    sv->sd.curchar->orig_pos>=0 && sv->sd.curchar->orig_pos<sv->sd.fv->sf->glyphcnt &&
	    sv->sd.curchar==sv->sd.fv->sf->glyphs[sv->sd.curchar->orig_pos] )
	/* Looks ok */;
    else
	sv->sd.curchar=startcur=NULL;

    if ( !sv->sd.subpatternsearch ) startafter = false;

    if ( sv->showsfindnext && sv->sd.curchar!=NULL )
	i = sv->sd.fv->map->backmap[sv->sd.curchar->orig_pos]+1-startafter;
    else {
	startafter = false;
	if ( !sv->sd.onlyselected )
	    i = 0;
	else {
	    for ( i=0; i<sv->sd.fv->map->enccount; ++i )
		if ( sv->sd.fv->selected[i] && (gid=sv->sd.fv->map->map[i])!=-1 &&
			sv->sd.fv->sf->glyphs[gid]!=NULL )
	    break;
	}
    }
 
    for ( ; i<sv->sd.fv->map->enccount; ++i ) {
	if (( !sv->sd.onlyselected || sv->sd.fv->selected[i]) && (gid=sv->sd.fv->map->map[i])!=-1 &&
		sv->sd.fv->sf->glyphs[gid]!=NULL ) {
	    SCSplinePointsUntick(sv->sd.fv->sf->glyphs[gid],sv->sd.fv->active_layer);
	    if ( SearchChar(&sv->sd,gid,startafter) )
    break;
	}
	startafter = false;
    }
    if ( i>=sv->sd.fv->map->enccount ) {
	ff_post_notice(_("Not Found"),sv->showsfindnext?_("The search pattern was not found again in the font %.100s"):_("The search pattern was not found in the font %.100s"),sv->sd.fv->sf->fontname);
	sv->sd.curchar = startcur;
	GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find"));
	sv->showsfindnext = false;
return( false );
    }
    SVSelectSC(sv);
    if ( sv->lastcv!=NULL && sv->lastcv->b.sc==startcur && sv->lastcv->b.fv== sv->sd.fv ) {
	CVChangeSC(sv->lastcv,sv->sd.curchar);
	GDrawSetVisible(sv->lastcv->gw,true);
	GDrawRaise(sv->lastcv->gw);
    } else
	sv->lastcv = CharViewCreate(sv->sd.curchar,(FontView *) sv->sd.fv,-1);
    GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find Next"));
    sv->showsfindnext = true;
return( true );
}

static void DoFindAll(SearchView *sv) {
    int any;

    any = _DoFindAll(&sv->sd);
    GDrawRequestExpose(((FontView *) (sv->sd.fv))->v,NULL,false);
    if ( !any )
	ff_post_notice(_("Not Found"),_("The search pattern was not found in the font %.100s"),sv->sd.fv->sf->fontname);
}

static int pathpointcnt(SplineSet *ss) {
    SplinePoint *sp;
    int cnt;

    if ( ss==NULL )		/* No paths */
return( 0 );
    if ( ss->next!=NULL )	/* more than one path */
return( -1 );
    if ( ss->first->prev!=NULL )/* single path, but it is closed */
return( -2 );
    for ( sp=ss->first, cnt=1; sp->next!=NULL; sp=sp->next->to, ++cnt );
return( cnt );
}

static int SVParseDlg(SearchView *sv, int check_replace ) {
    int err=false;
    double fudge;

    fudge = GetReal8(sv->gw,CID_Fuzzy,_("Match Fuzziness:"),&err);
    if ( err )
return( false );
    old_fudge = fudge;

    sv->sd.tryreverse = true;
    sv->sd.tryflips = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Flipping));
    sv->sd.tryscale = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Scaling));
    sv->sd.tryrotate = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Rotating));
    sv->sd.endpoints = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Endpoints));
    sv->sd.onlyselected = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Selected));

    SVResetPaths(&sv->sd);
    if ( pathpointcnt(sv->sd.path)==0 )
	ff_post_error(_("Bad search pattern"),_("Nothing to match."));
    else if ( sv->sd.endpoints ) {
	if ( pathpointcnt(sv->sd.path)<3 ||
		(check_replace && pathpointcnt(sv->sd.replacepath)<3 && pathpointcnt(sv->sd.replacepath)!=0 )) {
	    if ( pathpointcnt(sv->sd.path)<0 )
		ff_post_error(_("Bad search pattern"),_("When \"Endpoints specify minimum length and direction only\" is checked, the search pattern must be a single open contour."));
	    else if ( pathpointcnt(sv->sd.path)<3 )
		ff_post_error(_("Bad search pattern"),_("When \"Endpoints specify minimum length and direction only\" is checked, the search pattern must be a single open contour with at least 3 points on it (otherwise there is nothing to match)."));
	    else
		ff_post_error(_("Bad replace pattern"),_("When \"Endpoints specify minimum length and direction only\" is checked, the replace pattern must be a single open contour with at least 3 points on it."));
return( false );
	}
    } else if ( pathpointcnt(sv->sd.path)>0 ) {
	/* It might make sense not to do a sub-pattern search if the */
	/*  replace pattern is closed... but that's kind of weird */
	if ( check_replace && pathpointcnt(sv->sd.replacepath)<0 ) {
	    ff_post_error(_("Bad replace pattern"),_("When the search path is a single open contour, the replace pattern must also be."));
return( false );
	}
    }

    sv->sd.fudge = fudge;
    sv->sd.fudge_percent = sv->sd.tryrotate ? .01 : .001;
return( true );
}

static int SV_Find(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	if ( !SVParseDlg(sv,false))
return( true );
	sv->sd.findall = sv->sd.replaceall = false;
	DoFindOne(sv,false);
    }
return( true );
}

static int SV_FindAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	if ( !SVParseDlg(sv,false))
return( true );
	sv->sd.findall = true;
	sv->sd.replaceall = false;
	DoFindAll(sv);
    }
return( true );
}

static int SV_RplFind(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	RefChar *rf;
	if ( !SVParseDlg(sv,true))
return( true );
	sv->sd.findall = sv->sd.replaceall = false;
	for ( rf=sv->sd.sc_rpl.layers[ly_fore].refs; rf!=NULL; rf = rf->next ) {
	    if ( SCDependsOnSC(rf->sc,sv->sd.curchar)) {
		ff_post_error(_("Self-referential glyph"),_("Attempt to make a glyph that refers to itself"));
return( true );
	    }
	}
	DoRpl(&sv->sd);
	DoFindOne(sv,sv->sd.subpatternsearch);
    }
return( true );
}

static int SV_RplAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	if ( !SVParseDlg(sv,true))
return( true );
	sv->sd.findall = false;
	sv->sd.replaceall = true;
	DoFindAll(sv);
    }
return( true );
}

/* ************************************************************************** */

void SV_DoClose(struct cvcontainer *cvc) {
    GDrawSetVisible(((SearchView *) cvc)->gw,false);
}

static int SV_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	SV_DoClose(((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
return( true );
}

static void SVResize(SearchView *sv, GEvent *event) {
    int width, height;

    if ( !event->u.resize.sized )
return;

    GGadgetMove(GWidgetGetControl(sv->gw,CID_TopBox),4,4);
    GGadgetResize(GWidgetGetControl(sv->gw,CID_TopBox),
	    event->u.resize.size.width-8,
	    event->u.resize.size.height-12);
    
    width = (event->u.resize.size.width-40)/2;
    height = (event->u.resize.size.height-sv->cv_y-sv->button_height-8);
    if ( width<70 || height<80 ) {
	if ( width<70 ) width = 70;
	width = 2*width+40;
	if ( height<80 ) height = 80;
	height += sv->cv_y+sv->button_height+8;
	GDrawResize(sv->gw,width,height);
return;
    }
    if ( width!=sv->cv_width || height!=sv->cv_height ) {
	GDrawResize(sv->cv_srch.gw,width,height);
	GDrawResize(sv->cv_rpl.gw,width,height);
	sv->cv_width = width; sv->cv_height = height;
	sv->rpl_x = 30+width;
	GDrawMove(sv->cv_rpl.gw,sv->rpl_x,sv->cv_y);
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawRequestExpose(sv->gw,NULL,false);
}

void SVMakeActive(SearchView *sv,CharView *cv) {
    GRect r;
    if ( sv==NULL )
return;
    sv->cv_srch.inactive = sv->cv_rpl.inactive = true;
    cv->inactive = false;
    GDrawSetUserData(sv->gw,cv);
    GDrawRequestExpose(sv->cv_srch.v,NULL,false);
    GDrawRequestExpose(sv->cv_rpl.v,NULL,false);
    GDrawGetSize(sv->gw,&r);
    r.x = 0;
    r.y = sv->mbh;
    r.height = sv->fh+10;
    GDrawRequestExpose(sv->gw,&r,false);
}

static void SVChar(SearchView *sv, GEvent *event) {
    if ( event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab )
	SVMakeActive(sv,sv->cv_srch.inactive ? &sv->cv_srch : &sv->cv_rpl);
    else
	CVChar(sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch,event);
}

static void SVDraw(SearchView *sv, GWindow pixmap, GEvent *event) {
    GRect r;

    GDrawSetLineWidth(pixmap,0);
    if ( sv->cv_srch.inactive )
	GDrawSetFont(pixmap,sv->plain);
    else
	GDrawSetFont(pixmap,sv->bold);
    GDrawDrawText8(pixmap,10,sv->mbh+5+sv->as,
	    _("Search Pattern:"),-1,0);
    if ( sv->cv_rpl.inactive )
	GDrawSetFont(pixmap,sv->plain);
    else
	GDrawSetFont(pixmap,sv->bold);
    GDrawDrawText8(pixmap,sv->rpl_x,sv->mbh+5+sv->as,
	    _("Replace Pattern:"),-1,0);
    r.x = 10-1; r.y=sv->cv_y-1;
    r.width = sv->cv_width+1; r.height = sv->cv_height+1;
    GDrawDrawRect(pixmap,&r,0);
    r.x = sv->rpl_x-1;
    GDrawDrawRect(pixmap,&r,0);
}

static void SVCheck(SearchView *sv) {
    int show = ( sv->sd.sc_srch.layers[ly_fore].splines!=NULL || sv->sd.sc_srch.layers[ly_fore].refs!=NULL );
    int showrplall=show, showrpl;

    if ( sv->sd.sc_srch.changed_since_autosave && sv->showsfindnext ) {
	GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find"));
	sv->showsfindnext = false;
    }
    if ( showrplall ) {
	if ( sv->sd.sc_srch.layers[ly_fore].splines!=NULL && sv->sd.sc_srch.layers[ly_fore].splines->next==NULL &&
		sv->sd.sc_srch.layers[ly_fore].splines->first->prev==NULL &&
		sv->sd.sc_rpl.layers[ly_fore].splines==NULL && sv->sd.sc_rpl.layers[ly_fore].refs==NULL )
	    showrplall = false;
    }
    showrpl = showrplall;
    if ( !sv->showsfindnext || sv->sd.curchar==NULL || sv->sd.curchar->parent!=sv->sd.fv->sf ||
	    sv->sd.curchar->orig_pos<0 || sv->sd.curchar->orig_pos>=sv->sd.fv->sf->glyphcnt ||
	    sv->sd.curchar!=sv->sd.fv->sf->glyphs[sv->sd.curchar->orig_pos] ||
	    sv->sd.curchar->changed_since_search )
	showrpl = false;

    if ( sv->findenabled != show ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_Find),show);
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_FindAll),show);
	sv->findenabled = show;
    }
    if ( sv->rplallenabled != showrplall ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_ReplaceAll),showrplall);
	sv->rplallenabled = showrplall;
    }
    if ( sv->rplenabled != showrpl ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_Replace),showrpl);
	sv->rplenabled = showrpl;
    }
}

static void SearchViewFree(SearchView *sv) {
    SplinePointListsFree(sv->sd.sc_srch.layers[ly_fore].splines);
    SplinePointListsFree(sv->sd.sc_rpl.layers[ly_fore].splines);
    RefCharsFree(sv->sd.sc_srch.layers[ly_fore].refs);
    RefCharsFree(sv->sd.sc_rpl.layers[ly_fore].refs);
    free(sv);
}

static int sv_e_h(GWindow gw, GEvent *event) {
    SearchView *sv = (SearchView *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_expose:
	SVDraw(sv,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    SVResize(sv,event);
      break;
      case et_char:
	SVChar(sv,event);
      break;
      case et_timer:
	SVCheck(sv);
      break;
      case et_close:
	SV_DoClose((struct cvcontainer *) sv);
      break;
      case et_create:
      break;
      case et_destroy:
	SearchViewFree(sv);
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(sv->cv_srch.inactive?&sv->cv_rpl:&sv->cv_srch);
	else
	    CVPalettesHideIfMine(sv->cv_srch.inactive?&sv->cv_rpl:&sv->cv_srch);
	sv->isvisible = event->u.map.is_visible;
      break;
    }
return( true );
}

static void SVSetTitle(SearchView *sv) {
    char ubuf[150];
    sprintf(ubuf,_("Find in %.100s"),sv->sd.fv->sf->fontname);
    GDrawSetWindowTitles8(sv->gw,ubuf,_("Find"));
}

int SVAttachFV(FontView *fv,int ask_if_difficult) {
    int i, doit, pos, any=0, gid;
    RefChar *r, *rnext, *rprev;

    if ( searcher==NULL )
return( false );

    if ( searcher->sd.fv==(FontViewBase *) fv )
return( true );
    if ( searcher->sd.fv!=NULL && searcher->sd.fv->sf==fv->b.sf ) {
	((FontView *) searcher->sd.fv)->sv = NULL;
	fv->sv = searcher;
	searcher->sd.fv = (FontViewBase *) fv;
	SVSetTitle(searcher);
	searcher->sd.curchar = NULL;
return( true );
    }

    if ( searcher->dummy_sf.layers[ly_fore].order2 != fv->b.sf->layers[ly_fore].order2 ) {
	SCClearContents(&searcher->sd.sc_srch,ly_fore);
	SCClearContents(&searcher->sd.sc_rpl,ly_fore);
	for ( i=0; i<searcher->sd.sc_srch.layer_cnt; ++i )
	    UndoesFree(searcher->sd.sc_srch.layers[i].undoes);
	for ( i=0; i<searcher->sd.sc_rpl.layer_cnt; ++i )
	    UndoesFree(searcher->sd.sc_rpl.layers[i].undoes);
    }

    for ( doit=!ask_if_difficult; doit<=1; ++doit ) {
	for ( i=0; i<2; ++i ) {
	    rprev = NULL;
	    for ( r = searcher->chars[i]->layers[ly_fore].refs; r!=NULL; r=rnext ) {
		rnext = r->next;
		pos = SFFindSlot(fv->b.sf,fv->b.map,r->sc->unicodeenc,r->sc->name);
		gid = -1;
		if ( pos!=-1 )
		    gid = fv->b.map->map[pos];
		if ( (gid==-1 || fv->b.sf->glyphs[gid]!=NULL) && !doit ) {
		    char *buttons[3];
		    buttons[0] = _("Yes");
		    buttons[1] = _("Cancel");
		    buttons[2] = NULL;
		    if ( ask_if_difficult==2 && !searcher->isvisible )
return( false );
		    if ( gwwv_ask(_("Bad Reference"),(const char **) buttons,1,1,
			    _("The %1$s in the search dialog contains a reference to %2$.20hs which does not exist in the new font.\nShould I remove the reference?"),
				i==0?_("Search Pattern"):_("Replace Pattern"),
			        r->sc->name)==1 )
return( false );
		} else if ( !doit )
		    /* Do Nothing */;
		else if ( gid==-1 || fv->b.sf->glyphs[gid]!=NULL ) {
		    if ( rprev==NULL )
			searcher->chars[i]->layers[ly_fore].refs = rnext;
		    else
			rprev->next = rnext;
		    RefCharFree(r);
		    any = true;
		} else {
		    /*SplinePointListsFree(r->layers[0].splines); r->layers[0].splines = NULL;*/
		    r->sc = fv->b.sf->glyphs[gid];
		    r->orig_pos = gid;
		    SCReinstanciateRefChar(searcher->chars[i],r,fv->b.active_layer);
		    any = true;
		    rprev = r;
		}
	    }
	}
    }
    fv->sv = searcher;
    searcher->sd.fv = (FontViewBase *) fv;
    searcher->sd.curchar = NULL;
    if ( any ) {
	GDrawRequestExpose(searcher->cv_srch.v,NULL,false);
	GDrawRequestExpose(searcher->cv_rpl.v,NULL,false);
    }
    SVSetTitle(searcher);
return( true );
}

void SVDetachFV(FontView *fv) {
    FontView *other;

    fv->sv = NULL;
    if ( searcher==NULL || searcher->sd.fv!=(FontViewBase *) fv )
return;
    SV_DoClose((struct cvcontainer *) searcher);
    for ( other=fv_list; other!=NULL; other=(FontView *) other->b.next ) {
	if ( other!=fv ) {
	    SVAttachFV(fv,false);
return;
	}
    }
}

static int SV_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( false );
}

static int SV_Can_Open(struct cvcontainer *cvc) {
return( true );
}

static void SV_Do_Navigate(struct cvcontainer *cvc, enum nav_type type) {
}

static SplineFont *SF_Of_SV(struct cvcontainer *cvc) {
return( ((SearchView *) cvc)->sd.fv->sf );
}

struct cvcontainer_funcs searcher_funcs = {
    cvc_searcher,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) SVMakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) SVChar,
    SV_Can_Navigate,
    SV_Do_Navigate,
    SV_Can_Open,
    SV_DoClose,
    SF_Of_SV
};

static SearchView *SVFillup(SearchView *sv, FontView *fv) {

    SDFillup(&sv->sd,(FontViewBase *) fv);
    sv->base.funcs = &searcher_funcs;

    sv->chars[0] = &sv->sd.sc_srch;
    sv->chars[1] = &sv->sd.sc_rpl;
    sv->dummy_sf.glyphs = sv->chars;
    sv->dummy_sf.glyphcnt = sv->dummy_sf.glyphmax = 2;
    sv->dummy_sf.pfminfo.fstype = -1;
    sv->dummy_sf.pfminfo.stylemap = -1;
    sv->dummy_sf.fontname = sv->dummy_sf.fullname = sv->dummy_sf.familyname = "dummy";
    sv->dummy_sf.weight = "Medium";
    sv->dummy_sf.origname = "dummy";
    sv->dummy_sf.ascent = fv->b.sf->ascent;
    sv->dummy_sf.descent = fv->b.sf->descent;
    sv->dummy_sf.layers = sv->layerinfo;
    sv->dummy_sf.layer_cnt = 2;
    sv->layerinfo[ly_back].order2 = fv->b.sf->layers[ly_back].order2;
    sv->layerinfo[ly_back].name = _("Back");
    sv->layerinfo[ly_fore].order2 = fv->b.sf->layers[ly_fore].order2;
    sv->layerinfo[ly_fore].name = _("Fore");
    sv->dummy_sf.grid.order2 = fv->b.sf->grid.order2;
    sv->dummy_sf.anchor = fv->b.sf->anchor;
    sv->sd.sc_srch.width = sv->sd.sc_srch.vwidth = sv->sd.sc_rpl.vwidth = sv->sd.sc_rpl.width =
	    sv->dummy_sf.ascent + sv->dummy_sf.descent;
    sv->sd.sc_srch.parent = sv->sd.sc_rpl.parent = &sv->dummy_sf;

    sv->dummy_sf.fv = (FontViewBase *) &sv->dummy_fv;
    sv->dummy_fv.b.active_layer = ly_fore;
    sv->dummy_fv.b.sf = &sv->dummy_sf;
    sv->dummy_fv.b.selected = sv->sel;
    sv->dummy_fv.cbw = sv->dummy_fv.cbh = default_fv_font_size+1;
    sv->dummy_fv.magnify = 1;

    sv->cv_srch.b.container = (struct cvcontainer *) sv;
    sv->cv_rpl.inactive = true;
    sv->cv_rpl.b.container = (struct cvcontainer *) sv;

    sv->dummy_fv.b.map = &sv->dummy_map;
    sv->dummy_map.map = sv->map;
    sv->dummy_map.backmap = sv->backmap;
    sv->dummy_map.enccount = sv->dummy_map.encmax = sv->dummy_map.backmax = 2;
    sv->dummy_map.enc = &custom;

    sv->sd.fv = (FontViewBase *) fv;
    if ( fv!=NULL )
	fv->sv = sv;
return( sv );
}

SearchView *SVCreate(FontView *fv) {
    SearchView *sv;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14], boxes[6], *butarray[14], *allowarray[6],
	    *fudgearray[4], *halfarray[3], *varray[14];
    GTextInfo label[14];
    FontRequest rq;
    int as, ds, ld;
    char fudgebuf[20];
    int k, sel_pos, efdo_pos;
    static GFont *plainfont = NULL, *boldfont=NULL;

    if ( searcher!=NULL ) {
	if ( SVAttachFV(fv,true)) {
	    GDrawSetVisible(fv->sv->gw,true);
	    GDrawRaise(fv->sv->gw);
return( searcher );
	} else
return( NULL );
    }

    searcher = sv = SVFillup( calloc(1,sizeof(SearchView)), fv );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg/*|wam_icon*/;
    wattrs.is_dlg = true;
    wattrs.event_masks = -1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    /*wattrs.icon = icon;*/
    pos.width = 600;
    pos.height = 400;
    pos.x = GGadgetScale(104)+6;
    DefaultY(&pos);
    sv->gw = gw = GDrawCreateTopWindow(NULL,&pos,sv_e_h,&sv->cv_srch,&wattrs);
    SVSetTitle(sv);

    if ( plainfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	plainfont = GDrawInstanciateFont(NULL,&rq);
	plainfont = GResourceFindFont("SearchView.Font",plainfont);
	GDrawDecomposeFont(plainfont, &rq);
	rq.weight = 700;
	boldfont = GDrawInstanciateFont(NULL,&rq);
	boldfont = GResourceFindFont("SearchView.BoldFont",boldfont);
    }
    sv->plain = plainfont; sv->bold = boldfont;
    GDrawWindowFontMetrics(sv->gw,sv->plain,&as,&ds,&ld);
    sv->fh = as+ds; sv->as = as;

    SVCharViewInits(sv);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=0;

    label[k].text = (unichar_t *) _("Allow:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,sv->cv_y+sv->cv_height+8);
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_Allow;
    gcd[k].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[k].creator = GLabelCreate;
    allowarray[k] = &gcd[k]; ++k;

    label[k].text = (unichar_t *) _("Flipping");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 35; gcd[k].gd.pos.y = gcd[0].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
    gcd[k].gd.cid = CID_Flipping;
    gcd[k].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[k].creator = GCheckBoxCreate;
    allowarray[k] = &gcd[k]; ++k;

    label[k].text = (unichar_t *) _("Scaling");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 100; gcd[k].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_Scaling;
    gcd[k].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[k].creator = GCheckBoxCreate;
    allowarray[k] = &gcd[k]; ++k;

    label[k].text = (unichar_t *) _("Rotating");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 170; gcd[k].gd.pos.y = gcd[1].gd.pos.y;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_Rotating;
    gcd[k].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[k].creator = GCheckBoxCreate;
    allowarray[k] = &gcd[k]; allowarray[++k] = GCD_Glue; allowarray[5] = NULL;

    label[k].text = (unichar_t *) _("_Match Fuzziness:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].creator = GLabelCreate;
    fudgearray[0] = &gcd[k++];

    sprintf(fudgebuf,"%g",old_fudge);
    label[k].text = (unichar_t *) fudgebuf;
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Fuzzy;
    gcd[k].creator = GTextFieldCreate;
    fudgearray[1] = &gcd[k++]; fudgearray[2] = GCD_Glue; fudgearray[3] = NULL;

    efdo_pos = k;
    label[k].text = (unichar_t *) _("Endpoints specify minimum length and direction only");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[1].gd.pos.y+18;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_Endpoints;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"If the search pattern is a single open contour\n"
	"then do not match the end points. They merely\n"
	"specify the direction from which the curve should\n"
	"move toward the next point (which will be matched),\n"
	"and the minimum distance between the first matched\n"
	"point and the one before it. The endpoints of the\n"
	"replace contour will also only be used for positioning.\n"
	"\n"
	"This allows you to match a right angle corner\n"
	"without needed to specify exactly how long the edges\n"
	"are which form the right angle.");
    gcd[k++].creator = GCheckBoxCreate;

    sel_pos = k;
    label[k].text = (unichar_t *) _("Search Selected Chars Only");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[1].gd.pos.y+18;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_Selected;
    gcd[k].gd.popup_msg = (unichar_t *) _("Only search characters selected in the fontview.\nNormally we search all characters in the font.");
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) _("Find Next");	/* Start with this to allow sufficient space */
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[sel_pos].gd.pos.y+24;
    gcd[k].gd.flags = gg_visible|gg_but_default;
    gcd[k].gd.cid = CID_Find;
    gcd[k].gd.handle_controlevent = SV_Find;
    gcd[k].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = GCD_Glue; butarray[2] = &gcd[k++];

    label[k].text = (unichar_t *) _("Find All");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 0; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_FindAll;
    gcd[k].gd.handle_controlevent = SV_FindAll;
    gcd[k].creator = GButtonCreate;
    butarray[3] = GCD_Glue; butarray[4] = &gcd[k++];

    label[k].text = (unichar_t *) _("Replace");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 0; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_Replace;
    gcd[k].gd.handle_controlevent = SV_RplFind;
    gcd[k].creator = GButtonCreate;
    butarray[5] = GCD_Glue; butarray[6] = &gcd[k++];

    label[k].text = (unichar_t *) _("Replace All");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 0; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_ReplaceAll;
    gcd[k].gd.handle_controlevent = SV_RplAll;
    gcd[k].creator = GButtonCreate;
    butarray[7] = GCD_Glue; butarray[8] = &gcd[k++];

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 0; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k].gd.handle_controlevent = SV_Cancel;
    gcd[k].creator = GButtonCreate;
    butarray[9] = GCD_Glue; butarray[10] = &gcd[k++];
    butarray[11] = GCD_Glue; butarray[12] = GCD_Glue; butarray[13] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = allowarray;
    boxes[2].creator = GHBoxCreate;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = fudgearray;
    boxes[3].creator = GHBoxCreate;
    halfarray[0] = &boxes[2]; halfarray[1] = &boxes[3]; halfarray[2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = halfarray;
    boxes[4].creator = GHBoxCreate;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = butarray;
    boxes[5].creator = GHBoxCreate;

    varray[0] = GCD_Glue;       varray[1] = NULL;
    varray[2] = &boxes[4];      varray[3] = NULL;
    varray[4] = &gcd[efdo_pos]; varray[5] = NULL;
    varray[6] = GCD_Glue;       varray[7] = NULL;
    varray[8] = &gcd[sel_pos];  varray[9] = NULL;
    varray[10] = &boxes[5];     varray[11]= NULL;
    varray[12] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].gd.cid = CID_TopBox;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetPadding(boxes[2].ret,6,3);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Find),_("Find"));
    sv->showsfindnext = false;
    GDrawRequestTimer(gw,1000,1000,NULL);
    sv->button_height = GDrawPointsToPixels(gw,100);
    GDrawResize(gw,650,400);		/* Force a resize event */

    GDrawSetVisible(sv->gw,true);
return( sv );
}

void SVDestroy(SearchView *sv) {

    if ( sv==NULL )
return;

    SDDestroy(&sv->sd);
    free(sv);
}

void FVReplaceOutlineWithReference( FontView *fv, double fudge ) {

    if ( fv->v!=NULL )
	GDrawSetCursor(fv->v,ct_watch);

    FVBReplaceOutlineWithReference((FontViewBase *) fv, fudge);

    if ( fv->v!=NULL ) {
	GDrawRequestExpose(fv->v,NULL,false);
	GDrawSetCursor(fv->v,ct_pointer);
    }
}
