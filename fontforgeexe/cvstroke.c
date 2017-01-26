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
#include "fontforgeui.h"
#include "fvfonts.h"
#include "splinestroke.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include <math.h>
#define PI      3.1415926535897932

extern GDevEventMask input_em[];
extern const int input_em_cnt;

#define CID_ButtCap	1001
#define CID_RoundCap	1002
#define CID_SquareCap	1003
#define CID_BevelJoin	1004
#define CID_RoundJoin	1005
#define CID_MiterJoin	1006
#define CID_Width	1007
#define CID_MinorAxis	1008
#define CID_PenAngle	1009
#define CID_Circle	1010
#define CID_Caligraphic	1011
#define CID_Polygon	1012
#define CID_LineCapTxt	1014
#define CID_LineJoinTxt	1015
#define CID_MinorAxisTxt 1016
#define CID_PenAngleTxt	1017
	/* For Kanou (& me) */
#define CID_RmInternal	1019
#define CID_RmExternal	1020
/* #define CID_CleanupSelfIntersect	1021 */
#define CID_Apply	1022
#define CID_Nib		1023
#define CID_WidthTxt	1024
#define CID_TopBox	1025

	/* For freehand */
#define CID_CenterLine	1031

static void CVStrokeIt(void *_cv, StrokeInfo *si, int justapply) {
    CharView *cv = _cv;
    int anypoints;
    SplineSet *spl, *prev, *head=NULL, *cur, *snext;

    if ( cv->b.layerheads[cv->b.drawmode]->undoes!=NULL &&
	    cv->b.layerheads[cv->b.drawmode]->undoes->undotype==ut_tstate )
	CVDoUndo(&cv->b);

    if ( justapply )
	CVPreserveTState(cv);
    else
	CVPreserveState(&cv->b);
    if ( CVAnySel(cv,&anypoints,NULL,NULL,NULL) && anypoints ) {
	prev = NULL;
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = snext ) {
	    snext = spl->next;
	    if ( PointListIsSelected(spl)) {
		spl->next = NULL;
		cur = SplineSetStroke(spl,si,cv->b.layerheads[cv->b.drawmode]->order2);
		if ( cur!=NULL ) {
		    if ( prev==NULL )
			cv->b.layerheads[cv->b.drawmode]->splines=cur;
		    else
			prev->next = cur;
		    while ( cur->next ) cur=cur->next;
		    cur->next = snext;
		    prev = cur;
		} else {
		    if ( prev==NULL )
			cv->b.layerheads[cv->b.drawmode]->splines=snext;
		    else
			prev->next = snext;
		}
		spl->next = NULL;
		SplinePointListMDFree(cv->b.sc,spl);
	    } else
		prev = spl;
	}
    } else {
	head = SplineSetStroke(cv->b.layerheads[cv->b.drawmode]->splines,si,
		cv->b.layerheads[cv->b.drawmode]->order2);
	SplinePointListsFree( cv->b.layerheads[cv->b.drawmode]->splines );
	cv->b.layerheads[cv->b.drawmode]->splines = head;
    }
    CVCharChangedUpdate(&cv->b);
}

static int _Stroke_OK(StrokeDlg *sd,int isapply) {
    StrokeInfo *si, strokeinfo;
    int err;
    GWindow sw = sd->gw;
    char *msg;

    err = false;
    if ( (si = sd->si)==NULL ) {
	memset(&strokeinfo,'\0',sizeof(strokeinfo));
	si = &strokeinfo;
    }
    si->stroke_type = si_std;
    if ( GGadgetIsChecked( GWidgetGetControl(sw,CID_Caligraphic)) )
	si->stroke_type = si_caligraphic;
    else if ( GGadgetIsChecked( GWidgetGetControl(sw,CID_Polygon)) )
	si->stroke_type = si_poly;
    else if ( si!= &strokeinfo &&
	    GGadgetIsChecked( GWidgetGetControl(sw,CID_CenterLine)) )
	si->stroke_type = si_centerline;
    if ( si->stroke_type == si_poly ) {
	si->poly = sd->sc_stroke.layers[ly_fore].splines;
	if ( sd->sc_stroke.layers[ly_fore].refs != NULL ) {
	    ff_post_error(_("No References"),_("No references allowed in a pen."));
	    err = true;
	} else if ( si->poly == NULL ) {
	    ff_post_error(_("Nothing specified"),_("Please draw a convex polygon in the drawing area."));
	    err = true;
	} else {
	    SplineSet *ss;
	    SplinePoint *sp;
	    BasePoint pts[256];
	    int cnt, selectall;
	    for ( ss=si->poly ; ss!=NULL && !err; ss=ss->next ) {
		for ( sp=ss->first;;) {
		    sp->selected = false;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==ss->first )
		break;
		}
	    }
	    msg = NULL;
	    selectall = false;
	    for ( ss=si->poly ; ss!=NULL && !err; ss=ss->next ) {
		if ( ss->first->prev==NULL ) {
		    msg = _("The selected contour is open, but it must be a convex polygon.");
		    err = selectall = true;
		} else {
		    for ( sp=ss->first, cnt=0; cnt<255; ++cnt ) {
			pts[cnt] = sp->me;
			if ( !sp->next->knownlinear ) {
			    sp->selected = true;
			    sp->next->to->selected = true;
			    err = true;
			    msg = _("The selected spline has curved edges, but it must be a convex polygon (with straight edges).");
		    break;
			}
			sp = sp->next->to;
			if ( sp==ss->first ) {
			    ++cnt;
		    break;
			}
		    }
		    if ( err )
			/* Already handled */;
		    else if ( cnt==255 ) {
			msg = _("There are too many vertices on this polygon.");
			err = selectall = true;
		    } else {
			enum PolyType pt;
			int badindex;
			pt = PolygonIsConvex(pts,cnt,&badindex);
			if ( pt==Poly_Line ) {
			    msg = _("This is a line; it must enclose some area.");
			    err = selectall = true;
			} else if ( pt==Poly_TooFewPoints ) {
			    msg = _("There aren't enough vertices to be a polygon.");
			    err = selectall = true;
			} else if ( pt!=Poly_Convex ) {
			    err = true;
			    if ( pt==Poly_PointOnEdge )
				msg = _("There are at least 3 colinear vertices. Please remove (Edit->Merge) the selected point.");
			    else
				msg = _("The selected vertex makes this a concave polygon. Please remove (Edit->Merge) it.");
			    for ( sp=ss->first, cnt=0; cnt<255; ++cnt ) {
				if ( cnt==badindex ) {
				    sp->selected = true;
			    break;
				}
				sp = sp->next->to;
				if ( sp==ss->first )
			    break;
			    }
			}
		    }
		}
		if ( selectall ) {
		    for ( sp=ss->first;;) {
			sp->selected = true;
			if ( sp->next==NULL )
		    break;
			sp = sp->next->to;
			if ( sp==ss->first )
		    break;
		    }
		}
		if ( err ) {
		    GDrawRequestExpose(sd->cv_stroke.v,NULL,false);
		    ff_post_error(_("Not a convex polygon"),msg);
	    break;
		}
	    }
	}
	GDrawRequestExpose(sd->cv_stroke.v,NULL,false);
    } else if ( si->stroke_type==si_std || si->stroke_type==si_caligraphic ) {
	si->cap = GGadgetIsChecked( GWidgetGetControl(sw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundCap))?lc_round:
		lc_square;
	si->join = GGadgetIsChecked( GWidgetGetControl(sw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundJoin))?lj_round:
		lj_miter;
	si->radius = GetReal8(sw,CID_Width,_("Stroke _Width:"),&err)/2;
    if (si->radius == 0) {
        ff_post_error(_("Bad Value"), _("Stroke width cannot be zero"));
        err = true;
    }
	if ( si->radius<0 ) si->radius = -si->radius;	/* Behavior is said to be very slow (but correct) for negative strokes */
	si->penangle = GetReal8(sw,CID_PenAngle,_("Pen _Angle:"),&err);
	if ( si->penangle>180 || si->penangle < -180 ) {
	    si->penangle = fmod(si->penangle,360);
	    if ( si->penangle>180 )
		si->penangle -= 360;
	    else if ( si->penangle<-180 )
		si->penangle += 360;
	}
	si->penangle *= 3.1415926535897932/180;
	si->minorradius = GetReal8(sw,CID_MinorAxis,_("Minor A_xis:"),&err)/2;
    }
    si->removeinternal = GGadgetIsChecked( GWidgetGetControl(sw,CID_RmInternal));
    si->removeexternal = GGadgetIsChecked( GWidgetGetControl(sw,CID_RmExternal));
/*	si->removeoverlapifneeded = GGadgetIsChecked( GWidgetGetControl(sw,CID_CleanupSelfIntersect)); */
    if ( si->removeinternal && si->removeexternal ) {
	ff_post_error(_("Bad Value"),_("Removing both the internal and the external contours makes no sense"));
	err = true;
    }
    if ( err )
return( true );
    if ( sd->strokeit!=NULL )
	(sd->strokeit)(sd->cv,si,isapply);
    sd->done = !isapply;
return( true );
}

static int Stroke_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow sw = GGadgetGetWindow(g);
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(sw))->container;
return( _Stroke_OK(sd,GGadgetGetCid(g) == CID_Apply));
    }
return( true );
}

static void _Stroke_Cancel(StrokeDlg *sd ) {
    if ( sd->strokeit==CVStrokeIt && sd->cv->b.layerheads[sd->cv->b.drawmode]->undoes!=NULL &&
	    sd->cv->b.layerheads[sd->cv->b.drawmode]->undoes->undotype==ut_tstate )
	CVDoUndo(&sd->cv->b);
    sd->done = true;
}

static int Stroke_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	_Stroke_Cancel(sd);
    }
return( true );
}

static void Stroke_ShowNib(StrokeDlg *sd) {
    SplineSet *ss;
    real transform[6];
    double angle,c,s, width, height;
    int err=0;

    SplinePointListsFree(sd->dummy_sf.grid.splines);
    sd->dummy_sf.grid.splines = NULL;
    if ( GGadgetIsChecked(GWidgetGetControl(sd->gw,CID_Polygon)) ) {
	if ( sd->sc_stroke.layers[ly_fore].splines==NULL ) {
	    sd->sc_stroke.layers[ly_fore].splines = sd->old_poly;
	    sd->old_poly = NULL;
	}
    } else {
	if ( sd->old_poly==NULL ) {
	    sd->old_poly = sd->sc_stroke.layers[ly_fore].splines;
	    sd->sc_stroke.layers[ly_fore].splines = NULL;
	}
    }
    if ( GGadgetIsChecked(GWidgetGetControl(sd->gw,CID_Circle)) ||
	    GGadgetIsChecked(GWidgetGetControl(sd->gw,CID_Caligraphic)) ) {
	ss = UnitShape( GGadgetIsChecked(GWidgetGetControl(sd->gw,CID_Caligraphic)) );
	memset(transform,0,sizeof(transform));
	width = GetCalmReal8(sd->gw,CID_Width,"",&err)/2;
	height = GetCalmReal8(sd->gw,CID_MinorAxis,"",&err)/2;
	angle = GetCalmReal8(sd->gw,CID_PenAngle,"",&err)*PI/180;
	c = cos(angle); s=sin(angle);
	transform[0] = transform[3] = c;
	transform[1] = s; transform[2] = -s;
	transform[0] *= width;
	transform[1] *= width;
	transform[2] *= height;
	transform[3] *= height;
	SplinePointListTransform(ss,transform,tpt_AllPoints);
	sd->dummy_sf.grid.splines = ss;
    }
    GDrawRequestExpose(sd->cv_stroke.v,NULL,false);
}

static void StrokeSetup(StrokeDlg *sd, enum si_type stroke_type) {

    sd->dontexpand = ( stroke_type==si_centerline );

    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineCapTxt), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ButtCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_SquareCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineJoinTxt), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_BevelJoin), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundJoin), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MiterJoin), stroke_type==si_std);

    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_WidthTxt    ), stroke_type==si_std || stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Width       ), stroke_type==si_std || stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MinorAxisTxt), stroke_type==si_std || stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MinorAxis   ), stroke_type==si_std || stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngleTxt ), stroke_type==si_std || stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle    ), stroke_type==si_std || stroke_type==si_caligraphic);
}

static int Stroke_TextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	Stroke_ShowNib(sd);
    }
return( true );
}

static int Stroke_CenterLine(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	StrokeSetup(sd,si_centerline);
	Stroke_ShowNib(sd);
    }
return( true );
}

static int Stroke_Caligraphic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	StrokeSetup(sd,si_caligraphic);
	Stroke_ShowNib(sd);
    }
return( true );
}

static int Stroke_Stroke(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	StrokeSetup(sd,si_std);
	Stroke_ShowNib(sd);
    }
return( true );
}

static int Stroke_Polygon(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container;
	StrokeSetup(sd,si_poly);
	Stroke_ShowNib(sd);
    }
return( true );
}

static void Stroke_SubResize(StrokeDlg *sd, GEvent *event) {
    int width, height;

    if ( !event->u.resize.sized )
return;

    width = event->u.resize.size.width;
    height = event->u.resize.size.height;
    if ( width!=sd->cv_width || height!=sd->cv_height ) {
	sd->cv_width = width; sd->cv_height = height;
	GDrawResize(sd->cv_stroke.gw,width,height);
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

static void Stroke_Draw(StrokeDlg *sd, GWindow pixmap, GEvent *event) {
    GRect r,pos;

    GGadgetGetSize(GWidgetGetControl(sd->gw,CID_Nib),&pos);
    r.x = pos.x-1; r.y = pos.y-1;
    r.width = pos.width+1; r.height = pos.height+1;
    GDrawDrawRect(pixmap,&r,0);
}

static void Stroke_MakeActive(StrokeDlg *sd,CharView *cv) {

    if ( sd==NULL )
return;
    cv->inactive = false;
    GDrawSetUserData(sd->gw,cv);
    GDrawRequestExpose(cv->v,NULL,false);
    GDrawRequestExpose(sd->gw,NULL,false);
}

static void Stroke_Char(StrokeDlg *sd, GEvent *event) {

    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	help("elementmenu.html#Expand");
return;
    } else if ( event->u.chr.keysym == GK_Return ) {
	_Stroke_OK(sd,false);
return;
    } else if ( event->u.chr.keysym == GK_Escape ) {
	_Stroke_Cancel(sd);
return;
    }

    CVChar((&sd->cv_stroke),event);
}

static void Stroke_DoClose(struct cvcontainer *cvc) {
    StrokeDlg *sd = (StrokeDlg *) cvc;

     {
	SplineChar *msc = &sd->sc_stroke;
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
	free( msc->layers );
    }

    sd->done = true;
}

static int stroke_sub_e_h(GWindow gw, GEvent *event) {
    StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_resize:
	if ( event->u.resize.sized )
	    Stroke_SubResize(sd,event);
      break;
      case et_char:
	Stroke_Char(sd,event);
      break;
      case et_mouseup: case et_mousedown: case et_mousemove:
return( false );
      break;
    }
return( true );
}

static int Stroke_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( false );
}

static int Stroke_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static SplineFont *SF_Of_Stroke(struct cvcontainer *foo) {
return( NULL );
}

struct cvcontainer_funcs stroke_funcs = {
    cvc_stroke,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) Stroke_MakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) Stroke_Char,
    Stroke_Can_Navigate,
    NULL,
    Stroke_Can_Open,
    Stroke_DoClose,
    SF_Of_Stroke
};


static void StrokeInit(StrokeDlg *sd) {
    real transform[6];

    /*memset(sd,0,sizeof(*sd));*/
    sd->base.funcs = &stroke_funcs;

    {
	SplineChar *msc = (&sd->sc_stroke);
	CharView *mcv = (&sd->cv_stroke);
	msc->orig_pos = 0;
	msc->unicodeenc = -1;
	msc->name = "Nib";
	msc->parent = &sd->dummy_sf;
	msc->layer_cnt = 2;
	msc->layers = calloc(2,sizeof(Layer));
	msc->width = 200;
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
	sd->chars[0] = msc;

	mcv->b.sc = msc;
	mcv->b.layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->b.layerheads[dm_back] = &msc->layers[ly_back];
	mcv->b.layerheads[dm_grid] = &sd->dummy_sf.grid;
	mcv->b.drawmode = dm_fore;
	mcv->b.container = (struct cvcontainer *) sd;
	mcv->inactive = false;
    }
    sd->dummy_sf.glyphs = sd->chars;
    sd->dummy_sf.glyphcnt = sd->dummy_sf.glyphmax = 1;
    sd->dummy_sf.pfminfo.fstype = -1;
    sd->dummy_sf.pfminfo.stylemap = -1;
    sd->dummy_sf.fontname = sd->dummy_sf.fullname = sd->dummy_sf.familyname = "dummy";
    sd->dummy_sf.weight = "Medium";
    sd->dummy_sf.origname = "dummy";
    sd->dummy_sf.ascent = 200;
    sd->dummy_sf.descent = 200;
    sd->dummy_sf.layers = sd->layerinfo;
    sd->dummy_sf.layer_cnt = 2;
    sd->layerinfo[ly_back].order2 = false;
    sd->layerinfo[ly_back].name = _("Back");
    sd->layerinfo[ly_fore].order2 = false;
    sd->layerinfo[ly_fore].name = _("Fore");
    sd->dummy_sf.grid.order2 = false;
    sd->dummy_sf.anchor = NULL;

    sd->dummy_sf.fv = (FontViewBase *) &sd->dummy_fv;
    sd->dummy_fv.b.active_layer = ly_fore;
    sd->dummy_fv.b.sf = &sd->dummy_sf;
    sd->dummy_fv.b.selected = sd->sel;
    sd->dummy_fv.cbw = sd->dummy_fv.cbh = default_fv_font_size+1;
    sd->dummy_fv.magnify = 1;

    sd->dummy_fv.b.map = &sd->dummy_map;
    sd->dummy_map.map = sd->map;
    sd->dummy_map.backmap = sd->backmap;
    sd->dummy_map.enccount = sd->dummy_map.encmax = sd->dummy_map.backmax = 1;
    sd->dummy_map.enc = &custom;

    /* Default poly to a 50x50 square */
    if ( sd->old_poly==NULL ) {
	sd->old_poly = UnitShape( true );
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 25;
	SplinePointListTransform(sd->old_poly,transform,tpt_AllPoints);
    }
}

static int stroke_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_expose ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;
	Stroke_Draw(sd,gw,event);
    } else if ( event->type==et_close ) {
	StrokeDlg *sd = (StrokeDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;
	sd->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("elementmenu.html#Expand");
return( true );
	}
return( false );
    } else if ( event->type == et_mousemove ) {
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

#define SD_Width	230
#define SD_Height	335
#define FH_Height	(SD_Height+75)

static void MakeStrokeDlg(void *cv,void (*strokeit)(void *,StrokeInfo *,int),StrokeInfo *si) {
    static StrokeDlg strokedlg;
    StrokeDlg *sd, freehand_dlg;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[39], boxes[9], *buttons[13], *mainarray[11][2],
	    *caparray[7], *joinarray[7], *swarray[7][5], *pens[10];
    GTextInfo label[39];
    int yoff=0;
    int gcdoff, mi, swpos;
    static StrokeInfo defaults = {
        25,
        lj_round,
        lc_butt,
        si_std,
        false, /* removeinternal */
        false, /* removeexternal */
        false, /* leave users center */
        3.1415926535897932/4,
        25,
        NULL,
        50,
        0.0,
        0,
        0,
        NULL,
        NULL
    };
    StrokeInfo *def = si?si:&defaults;
    char anglebuf[20], widthbuf[20], axisbuf[20];

    if ( strokeit!=NULL )
	sd = &strokedlg;
    else {
	sd = &freehand_dlg;
	memset(&freehand_dlg,0,sizeof(freehand_dlg));
	sd->si = si;
	yoff = 18;
    }
    if ( sd->old_poly==NULL && si!=NULL && si->poly!=NULL ) {
	sd->old_poly = si->poly;
	si->poly = NULL;
    }

    if ( sd->gw==NULL ) {
	StrokeInit(sd);
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = strokeit!=NULL ? _("Expand Stroke") :
/* GT: This does not mean the program, but freehand drawing */
		    _("Freehand");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,SD_Width));
	pos.height = GDrawPointsToPixels(NULL,strokeit!=NULL ? SD_Height : FH_Height);
	sd->gw = gw = GDrawCreateTopWindow(NULL,&pos,stroke_e_h,&sd->cv_stroke,&wattrs);
	if ( si!=NULL )
	    GDrawRequestDeviceEvents(gw,input_em_cnt,input_em);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	gcdoff = mi = swpos = 0;

	gcd[gcdoff].gd.flags = gg_visible|gg_enabled ;		/* This space is for the menubar */
	gcd[gcdoff].gd.pos.height = 18; gcd[gcdoff].gd.pos.width = 20;
	gcd[gcdoff++].creator = GSpacerCreate;
	mainarray[mi][0] = &gcd[gcdoff-1]; mainarray[mi++][1] = NULL;

	gcd[gcdoff].gd.pos.width = gcd[gcdoff].gd.pos.height = 200;
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled;
	gcd[gcdoff].gd.cid = CID_Nib;
	gcd[gcdoff].gd.u.drawable_e_h = stroke_sub_e_h;
	gcd[gcdoff++].creator = GDrawableCreate;
	mainarray[mi][0] = &gcd[gcdoff-1]; mainarray[mi++][1] = NULL;

	label[gcdoff].text = (unichar_t *) _("Pen Type:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GLabelCreate;
	pens[0] = &gcd[gcdoff-1];

	label[gcdoff].text = (unichar_t *) _("_Circular\n(Elliptical)");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type==si_std? gg_cb_on : 0);
	gcd[gcdoff].gd.u.radiogroup = 1;
	gcd[gcdoff].gd.cid = CID_Circle;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Stroke;
	gcd[gcdoff++].creator = GRadioCreate;
	pens[1] = &gcd[gcdoff-1];

	label[gcdoff].text = (unichar_t *) _("Ca_lligraphic\n(Rectangular)");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type == si_caligraphic ? gg_cb_on : 0);
	gcd[gcdoff].gd.u.radiogroup = 1;
	gcd[gcdoff].gd.cid = CID_Caligraphic;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Caligraphic;
	gcd[gcdoff++].creator = GRadioCreate;
	pens[2] = &gcd[gcdoff-1];

	label[gcdoff].text = (unichar_t *) _("_Polygon");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type == si_poly ? gg_cb_on : 0);
	gcd[gcdoff].gd.u.radiogroup = 1;
	gcd[gcdoff].gd.cid = CID_Polygon;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Polygon;
	gcd[gcdoff++].creator = GRadioCreate;
	pens[3] = &gcd[gcdoff-1]; pens[4] = NULL; pens[5] = NULL;

	if ( strokeit==NULL ) {
	    label[gcdoff].text = (unichar_t *) _("_Don't Expand");
	    label[gcdoff].text_is_1byte = true;
	    label[gcdoff].text_in_resource = true;
	    gcd[gcdoff].gd.label = &label[gcdoff];
	    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type==si_centerline ? gg_cb_on : 0 );
	    gcd[gcdoff].gd.u.radiogroup = 1;
	    gcd[gcdoff].gd.cid = CID_CenterLine;
	    gcd[gcdoff].gd.handle_controlevent = Stroke_CenterLine;
	    gcd[gcdoff++].creator = GRadioCreate;
	    pens[3] = NULL; pens[4] = GCD_Glue;
	    pens[5] = &gcd[gcdoff-2]; pens[6] = &gcd[gcdoff-1]; pens[7] = NULL; pens[8] = NULL;
	}

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = pens;
	boxes[2].creator = GHVBoxCreate;
	mainarray[mi][0] = &boxes[2]; mainarray[mi++][1] = NULL;

	label[gcdoff].text = (unichar_t *) _("Main Stroke _Width:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_WidthTxt;
	gcd[gcdoff++].creator = GLabelCreate;
	swarray[swpos][0] = &gcd[gcdoff-1];

	sprintf( widthbuf, "%g", (double) (2*def->radius) );
	label[gcdoff].text = (unichar_t *) widthbuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.pos.width = 50;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_Width;
	gcd[gcdoff].gd.handle_controlevent = Stroke_TextChanged;
	gcd[gcdoff++].creator = GTextFieldCreate;
	swarray[swpos][1] = &gcd[gcdoff-1]; swarray[swpos][2] = swarray[swpos][3] = GCD_Glue; swarray[swpos++][4] = NULL;

	label[gcdoff].text = (unichar_t *) _("Minor Stroke _Height:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled;
	gcd[gcdoff].gd.cid = CID_MinorAxisTxt;
	gcd[gcdoff++].creator = GLabelCreate;
	swarray[swpos][0] = &gcd[gcdoff-1];

	sprintf( axisbuf, "%g", (double) (2*def->minorradius) );
	label[gcdoff].text = (unichar_t *) axisbuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.pos.width = 50;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[gcdoff].gd.cid = CID_MinorAxis;
	gcd[gcdoff].gd.handle_controlevent = Stroke_TextChanged;
	gcd[gcdoff].gd.popup_msg = (unichar_t *) _(
	    "A calligraphic pen or an eliptical pen has two widths\n"
	    "(which may be the same, giving a circular or square pen,\n"
	    "or different giving an eliptical or rectangular pen).");
	gcd[gcdoff++].creator = GTextFieldCreate;
	swarray[swpos][1] = &gcd[gcdoff-1]; swarray[swpos][2] = swarray[swpos][3] = GCD_Glue; swarray[swpos++][4] = NULL;

	label[gcdoff].text = (unichar_t *) _("Pen _Angle:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled;
	gcd[gcdoff].gd.cid = CID_PenAngleTxt;
	gcd[gcdoff++].creator = GLabelCreate;
	swarray[2][0] = &gcd[gcdoff-1];

	sprintf( anglebuf, "%g", (double) (def->penangle*180/3.1415926535897932) );
	label[gcdoff].text = (unichar_t *) anglebuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.pos.width = 50;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled;
	gcd[gcdoff].gd.cid = CID_PenAngle;
	gcd[gcdoff].gd.handle_controlevent = Stroke_TextChanged;
	gcd[gcdoff++].creator = GTextFieldCreate;
	swarray[swpos][1] = &gcd[gcdoff-1]; swarray[swpos][2] = swarray[swpos][3] = GCD_Glue; swarray[swpos++][4] = NULL;
	swarray[swpos][0] = NULL;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = swarray[0];
	boxes[3].creator = GHVBoxCreate;
	mainarray[mi][0] = &boxes[3]; mainarray[mi++][1] = NULL;

/* GT: Butt is a PostScript concept which refers to a way of ending strokes */
/* GT: In the following image the line drawn with "=" is the original, and */
/* GT: the others are the results. The "Round" style is hard to draw with */
/* GT: ASCII glyphs. If this is unclear I suggest you look at the Expand Stroke */
/* GT: dialog which has little pictures */
/* GT: */
/* GT: -----------------+    -----------------+    ----------------+--+ */
/* GT:                  |                      \                      | */
/* GT: =================+    ================== )  =================  | */
/* GT:                  |                      /                      | */
/* GT: -----------------+    -----------------+    ----------------+--+ */
/* GT:       Butt                 Round                Square */
	label[gcdoff].text = (unichar_t *) _("_Butt");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_buttcap;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_butt?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_ButtCap;
	gcd[gcdoff++].creator = GRadioCreate;
	caparray[0] = &gcd[gcdoff-1]; caparray[1] = GCD_Glue;

	label[gcdoff].text = (unichar_t *) _("_Round");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundcap;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundCap;
	gcd[gcdoff++].creator = GRadioCreate;
	caparray[2] = &gcd[gcdoff-1]; caparray[3] = GCD_Glue;

	label[gcdoff].text = (unichar_t *) _("S_quare");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_squarecap;
	gcd[gcdoff].gd.mnemonic = 'q';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_square?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_SquareCap;
	gcd[gcdoff++].creator = GRadioCreate;
	caparray[4] = &gcd[gcdoff-1]; caparray[5] = NULL; caparray[6] = NULL;

	label[gcdoff].text = (unichar_t *) _("Line Cap");
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_LineCapTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	boxes[4].gd.label = (GTextInfo *) &gcd[gcdoff-1];
	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = caparray;
	boxes[4].creator = GHVGroupCreate;
	mainarray[mi][0] = &boxes[4]; mainarray[mi++][1] = NULL;

	label[gcdoff].text = (unichar_t *) _("_Miter");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_miterjoin;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_miter?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_MiterJoin;
	gcd[gcdoff++].creator = GRadioCreate;
	joinarray[0] = &gcd[gcdoff-1]; joinarray[1] = GCD_Glue;

	label[gcdoff].text = (unichar_t *) _("Ro_und");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundjoin;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundJoin;
	gcd[gcdoff++].creator = GRadioCreate;
	joinarray[2] = &gcd[gcdoff-1]; joinarray[3] = GCD_Glue;

	label[gcdoff].text = (unichar_t *) _("Be_vel");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_beveljoin;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_bevel?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_BevelJoin;
	gcd[gcdoff++].creator = GRadioCreate;
	joinarray[4] = &gcd[gcdoff-1]; joinarray[5] = NULL; joinarray[6] = NULL;

	label[gcdoff].text = (unichar_t *) _("Line Join");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_LineJoinTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	boxes[5].gd.label = (GTextInfo *) &gcd[gcdoff-1];
	boxes[5].gd.flags = gg_enabled|gg_visible;
	boxes[5].gd.u.boxelements = joinarray;
	boxes[5].creator = GHVGroupCreate;
	mainarray[mi][0] = &boxes[5]; mainarray[mi++][1] = NULL;

	label[gcdoff].text = (unichar_t *) _("Remove Internal Contour");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->removeinternal?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RmInternal;
	gcd[gcdoff++].creator = GCheckBoxCreate;
	mainarray[mi][0] = &gcd[gcdoff-1]; mainarray[mi++][1] = NULL;

	label[gcdoff].text = (unichar_t *) _("Remove External Contour");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->removeexternal?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RmExternal;
	gcd[gcdoff++].creator = GCheckBoxCreate;
	mainarray[mi][0] = &gcd[gcdoff-1]; mainarray[mi++][1] = NULL;

	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[gcdoff].text = (unichar_t *) _("_OK");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.handle_controlevent = Stroke_OK;
	gcd[gcdoff++].creator = GButtonCreate;
	buttons[0] = GCD_Glue; buttons[1] = &gcd[gcdoff-1]; buttons[2] = GCD_Glue; buttons[3] = GCD_Glue;

	gcd[gcdoff].gd.flags = gg_visible | gg_enabled;
	label[gcdoff].text = (unichar_t *) _("_Apply");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.handle_controlevent = Stroke_OK;
	gcd[gcdoff].gd.cid = CID_Apply;
	gcd[gcdoff++].creator = GButtonCreate;
	buttons[4] = GCD_Glue; buttons[5] = &gcd[gcdoff-1]; buttons[6] = GCD_Glue; buttons[7] = GCD_Glue;

	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[gcdoff].text = (unichar_t *) _("_Cancel");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.handle_controlevent = Stroke_Cancel;
	gcd[gcdoff].creator = GButtonCreate;
	buttons[8] = GCD_Glue; buttons[9] = &gcd[gcdoff]; buttons[10] = GCD_Glue;
	buttons[11] = NULL;

	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = buttons;
	boxes[6].creator = GHBoxCreate;
	mainarray[mi][0] = &boxes[6]; mainarray[mi++][1] = NULL;
	mainarray[mi][0] = GCD_Glue; mainarray[mi++][1] = NULL;
	mainarray[mi][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = mainarray[0];
	boxes[0].gd.cid = CID_TopBox;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,1);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);

	StrokeCharViewInits(sd,CID_Nib);
	sd->cv_stroke.showfore = true;
	sd->cv_stroke.showgrids = true;
    } else
	GDrawSetTransientFor(sd->gw,(GWindow) -1);

    Stroke_MakeActive(sd,&sd->cv_stroke);
    Stroke_ShowNib(sd);

    sd->cv = cv;
    sd->strokeit = strokeit;
    sd->done = false;
    sd->up[0] = sd->up[1] = true;
    GWidgetHidePalettes();
    GWidgetIndicateFocusGadget(GWidgetGetControl(sd->gw,CID_Width));
    if ( si==NULL ) {
	StrokeSetup(sd,GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Caligraphic))?si_caligraphic:
		GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Circle))?si_std:
		GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Polygon))?si_poly:
		si_centerline);
	GGadgetSetVisible( GWidgetGetControl(sd->gw,CID_Apply),strokeit==CVStrokeIt );
    } else {
	StrokeSetup(sd,def->stroke_type);
	GGadgetSetVisible( GWidgetGetControl(sd->gw,CID_Apply),false );
    }
    GWidgetToDesiredSize(sd->gw);

    GDrawSetVisible(sd->gw,true);
    while ( !sd->done )
	GDrawProcessOneEvent(NULL);

    CVPalettesHideIfMine(&sd->cv_stroke);
    if ( strokeit!=NULL )
	GDrawSetVisible(sd->gw,false);
    else
	GDrawDestroyWindow(sd->gw);
}

void CVStroke(CharView *cv) {

    if ( cv->b.layerheads[cv->b.drawmode]->splines==NULL )
return;

    MakeStrokeDlg(cv,CVStrokeIt,NULL);
}

void FVStroke(FontView *fv) {
    MakeStrokeDlg(fv,FVStrokeItScript,NULL);
}

void FreeHandStrokeDlg(StrokeInfo *si) {
    MakeStrokeDlg(NULL,NULL,si);
}

/* ************************************************************************** */
/* ****************************** Layer Dialog ****************************** */
/* ************************************************************************** */
#define LY_Width	300
#define LY_Height	336

#define CID_FillColor		2001
#define CID_FillCInherit	2002
#define CID_FillOpacity		2003
#define CID_FillOInherit	2004
#define CID_StrokeColor		2005
#define CID_StrokeCInherit	2006
#define CID_StrokeOpacity	2007
#define CID_StrokeOInherit	2008
#define CID_StrokeWInherit	2009
#define CID_Trans		2010
#define CID_InheritCap		2011
#define CID_InheritJoin		2012
#define CID_Fill		2013
#define CID_Dashes		2014
#define CID_DashesTxt		2015
#define CID_DashesInherit	2016
#define CID_Stroke		2017

#define CID_FillGradAdd		2100
#define CID_FillGradEdit	2101
#define CID_FillGradDelete	2102
#define CID_StrokeGradAdd	2110
#define CID_StrokeGradEdit	2111
#define CID_StrokeGradDelete	2112

#define CID_FillPatAdd		2200
#define CID_FillPatEdit		2201
#define CID_FillPatDelete	2202
#define CID_StrokePatAdd	2210
#define CID_StrokePatEdit	2211
#define CID_StrokePatDelete	2212

struct layer_dlg {
    int done;
    int ok;
    Layer *layer;
    SplineFont *sf;
    GWindow gw;
    struct gradient *fillgrad, *strokegrad;
    struct pattern  *fillpat,  *strokepat;

    int pat_done;
    struct pattern *curpat;
};

#define CID_Gradient	1001
#define CID_Stops	1002
#define CID_Pad		1003
#define	CID_Repeat	1004
#define CID_Reflect	1005
#define CID_Linear	1006
#define CID_Radial	1007
#define CID_GradStops	1008

static void GDDSubResize(GradientDlg *gdd, GEvent *event) {
    int width, height;

    if ( !event->u.resize.sized )
return;

    width = event->u.resize.size.width;
    height = event->u.resize.size.height;
    if ( width!=gdd->cv_width || height!=gdd->cv_height ) {
	gdd->cv_width = width; gdd->cv_height = height;
	GDrawResize(gdd->cv_grad.gw,width,height);
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

static void GDDDraw(GradientDlg *gdd, GWindow pixmap, GEvent *event) {
    GRect r,pos;

    GGadgetGetSize(GWidgetGetControl(gdd->gw,CID_Gradient),&pos);
    r.x = pos.x-1; r.y = pos.y-1;
    r.width = pos.width+1; r.height = pos.height+1;
    GDrawDrawRect(pixmap,&r,0);
}

static void GDDMakeActive(GradientDlg *gdd,CharView *cv) {

    if ( gdd==NULL )
return;
    cv->inactive = false;
    GDrawSetUserData(gdd->gw,cv);
    GDrawRequestExpose(cv->v,NULL,false);
    GDrawRequestExpose(gdd->gw,NULL,false);
}

static void GDDChar(GradientDlg *gdd, GEvent *event) {
    CVChar((&gdd->cv_grad),event);
}

static void GDD_DoClose(struct cvcontainer *cvc) {
    GradientDlg *gdd = (GradientDlg *) cvc;

     {
	SplineChar *msc = &gdd->sc_grad;
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
	free( msc->layers );
    }

    gdd->done = true;
}

static int gdd_sub_e_h(GWindow gw, GEvent *event) {
    GradientDlg *gdd = (GradientDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_resize:
	if ( event->u.resize.sized )
	    GDDSubResize(gdd,event);
      break;
      case et_char:
	GDDChar(gdd,event);
      break;
      case et_mouseup: case et_mousedown: case et_mousemove:
return( false );
      break;
    }
return( true );
}

static int gdd_e_h(GWindow gw, GEvent *event) {
    GradientDlg *gdd = (GradientDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_expose:
	GDDDraw(gdd, gw, event);
      break;
      case et_char:
	GDDChar(gdd,event);
      break;
      case et_close:
	GDD_DoClose((struct cvcontainer *) gdd);
      break;
      case et_create:
      break;
      case et_map:
	 {
	    CharView *cv = (&gdd->cv_grad);
	    if ( !cv->inactive ) {
		if ( event->u.map.is_visible )
		    CVPaletteActivate(cv);
		else
		    CVPalettesHideIfMine(cv);
	    }
	}
	/* gdd->isvisible = event->u.map.is_visible; */
      break;
    }
return( true );
}

static int Gradient_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GradientDlg *gdd = (GradientDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	GDD_DoClose(&gdd->base);
    }
return( true );
}

static int orderstops(const void *_md1, const void *_md2) {
    const struct matrix_data *md1 = _md1, *md2 = _md2;

    if ( md1->u.md_real>md2->u.md_real )
return( 1 );
    else if ( md1->u.md_real==md2->u.md_real )
return( 0 );
    else
return( -1 );
}

static int Gradient_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GradientDlg *gdd = (GradientDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	struct gradient *gradient = gdd->active;
	BasePoint start, end, offset;
	real radius;
	int pad, linear;
	SplineSet *ss, *ss2;
	int i, rows, cols = GMatrixEditGetColCnt(GWidgetGetControl(gdd->gw,CID_GradStops));
	struct matrix_data *md = GMatrixEditGet(GWidgetGetControl(gdd->gw,CID_GradStops), &rows);

	if ( rows<2 ) {
	    ff_post_error(_("Bad Gradient"),_("There must be at least 2 gradient stops"));
return( true );
	}
	for ( i=0; i<rows; ++i ) {
	    if ( md[cols*i+0].u.md_real<0 || md[cols*i+0].u.md_real>100.0 ) {
		ff_post_error(_("Bad Gradient"),_("Bad offset on line %d, must be between 0% and 100%."), i );
return( true );
	    }
	    if ( md[cols*i+1].u.md_ival<0 || md[cols*i+1].u.md_ival>0xffffff ) {
		ff_post_error(_("Bad Gradient"),_("Bad color on line %d, must be between 000000 and ffffff."), i );
return( true );
	    }
	    if ( md[cols*i+2].u.md_real<0 || md[cols*i+2].u.md_real>1.0 ) {
		ff_post_error(_("Bad Gradient"),_("Bad opacity on line %d, must be between 0.0 and 1.0."), i );
return( true );
	    }
	}

	linear = GGadgetIsChecked(GWidgetGetControl(gdd->gw,CID_Linear));
	if ( GGadgetIsChecked(GWidgetGetControl(gdd->gw,CID_Pad)) )
	    pad = sm_pad;
	else if ( GGadgetIsChecked(GWidgetGetControl(gdd->gw,CID_Repeat)) )
	    pad = sm_repeat;
	else
	    pad = sm_reflect;
	ss = gdd->sc_grad.layers[ly_fore].splines;
	if ( ss==NULL || gdd->sc_grad.layers[ly_fore].refs!=NULL ||
		(linear && ss->next!=NULL) ||
		(!linear && ss->next!=NULL && ss->next->next!=NULL)) {
	    ff_post_error(_("Bad Gradient"),_("You must draw a line"));
return( true );
	}
	ss2 = NULL;
	if ( !linear && ss->next!=NULL ) {
	    if ( ss->first->next==NULL ) {
		ss2 = ss;
		ss = ss2->next;
	    } else {
		ss2 = ss->next;
		if ( ss2->first->next!=NULL ) {
		    ff_post_error(_("Bad Gradient"),_("You must draw a line, with at most one additional point"));
return( true );
		}
	    }
	}
	if ( (ss->first->next==NULL || ss->first==ss->last ||
		ss->first->next->to->next!=NULL ) ||
		!ss->first->next->islinear ) {
	    ff_post_error(_("Bad Gradient"),_("You must draw a line"));
return( true );
	}

	if ( linear ) {
	    start = ss->first->me;
	    end = ss->last->me;
	    radius = 0;
	} else {
	    end = ss->first->me;
	    offset.x = ss->last->me.x-end.x;
	    offset.y = ss->last->me.y-end.y;
	    radius = sqrt(offset.x*offset.x + offset.y*offset.y);
	    if ( ss2!=NULL )
		start = ss2->first->me;
	    else
		start = end;
	}
	if ( gradient==NULL )
	    gdd->active = gradient = chunkalloc(sizeof(struct gradient));
	gradient->start = start;
	gradient->stop = end;
	gradient->radius = radius;
	gradient->sm = pad;

	/* Stops must be stored in ascending order */
	qsort(md,rows,cols*sizeof(struct matrix_data),orderstops);
	gradient->grad_stops = realloc(gradient->grad_stops,rows*sizeof(struct grad_stops));
	gradient->stop_cnt = rows;
	for ( i=0; i<rows; ++i ) {
	    gradient->grad_stops[i].offset  = md[cols*i+0].u.md_real/100.0;
	    gradient->grad_stops[i].col     = md[cols*i+1].u.md_ival;
	    gradient->grad_stops[i].opacity = md[cols*i+2].u.md_real;
	}

	GDD_DoClose(&gdd->base);
	gdd->oked = true;
    }
return( true );
}

static int GDD_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( false );
}

static int GDD_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static SplineFont *SF_Of_GDD(struct cvcontainer *foo) {
return( NULL );
}

struct cvcontainer_funcs gradient_funcs = {
    cvc_gradient,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) GDDMakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) GDDChar,
    GDD_Can_Navigate,
    NULL,
    GDD_Can_Open,
    GDD_DoClose,
    SF_Of_GDD
};


static void GDDInit(GradientDlg *gdd,SplineFont *sf,Layer *ly,struct gradient *grad) {

    memset(gdd,0,sizeof(*gdd));
    gdd->base.funcs = &gradient_funcs;

    {
	SplineChar *msc = (&gdd->sc_grad);
	CharView *mcv = (&gdd->cv_grad);
	msc->orig_pos = 0;
	msc->unicodeenc = -1;
	msc->name = "Gradient";
	msc->parent = &gdd->dummy_sf;
	msc->layer_cnt = 2;
	msc->layers = calloc(2,sizeof(Layer));
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
	gdd->chars[0] = msc;

	mcv->b.sc = msc;
	mcv->b.layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->b.layerheads[dm_back] = &msc->layers[ly_back];
	mcv->b.layerheads[dm_grid] = &gdd->dummy_sf.grid;
	mcv->b.drawmode = dm_fore;
	mcv->b.container = (struct cvcontainer *) gdd;
	mcv->inactive = false;
    }
    gdd->dummy_sf.glyphs = gdd->chars;
    gdd->dummy_sf.glyphcnt = gdd->dummy_sf.glyphmax = 1;
    gdd->dummy_sf.pfminfo.fstype = -1;
    gdd->dummy_sf.pfminfo.stylemap = -1;
    gdd->dummy_sf.fontname = gdd->dummy_sf.fullname = gdd->dummy_sf.familyname = "dummy";
    gdd->dummy_sf.weight = "Medium";
    gdd->dummy_sf.origname = "dummy";
    gdd->dummy_sf.ascent = sf->ascent;
    gdd->dummy_sf.descent = sf->descent;
    gdd->dummy_sf.layers = gdd->layerinfo;
    gdd->dummy_sf.layer_cnt = 2;
    gdd->layerinfo[ly_back].order2 = sf->layers[ly_back].order2;
    gdd->layerinfo[ly_back].name = _("Back");
    gdd->layerinfo[ly_fore].order2 = sf->layers[ly_fore].order2;
    gdd->layerinfo[ly_fore].name = _("Fore");
    gdd->dummy_sf.grid.order2 = sf->grid.order2;
    gdd->dummy_sf.anchor = NULL;

    gdd->dummy_sf.fv = (FontViewBase *) &gdd->dummy_fv;
    gdd->dummy_fv.b.active_layer = ly_fore;
    gdd->dummy_fv.b.sf = &gdd->dummy_sf;
    gdd->dummy_fv.b.selected = gdd->sel;
    gdd->dummy_fv.cbw = gdd->dummy_fv.cbh = default_fv_font_size+1;
    gdd->dummy_fv.magnify = 1;

    gdd->dummy_fv.b.map = &gdd->dummy_map;
    gdd->dummy_map.map = gdd->map;
    gdd->dummy_map.backmap = gdd->backmap;
    gdd->dummy_map.enccount = gdd->dummy_map.encmax = gdd->dummy_map.backmax = 1;
    gdd->dummy_map.enc = &custom;

    if ( grad!=NULL ) {
	SplineSet *ss1, *ss2;
	SplinePoint *sp1, *sp2, *sp3;
	ss1 = chunkalloc(sizeof(SplineSet));
	sp2 = SplinePointCreate(grad->stop.x,grad->stop.y);
	if ( grad->radius==0 ) {
	    sp1 = SplinePointCreate(grad->start.x,grad->start.y);
	    SplineMake(sp1,sp2,sf->layers[ly_fore].order2);
	    ss1->first = sp1; ss1->last = sp2;
	} else {
	    sp3 = SplinePointCreate(grad->start.x+grad->radius,grad->start.y);
	    SplineMake(sp2,sp3,sf->layers[ly_fore].order2);
	    ss1->first = sp2; ss1->last = sp3;
	    if ( grad->start.x!=grad->stop.x || grad->start.y!=grad->stop.y ) {
		ss2 = chunkalloc(sizeof(SplineSet));
		sp1 = SplinePointCreate(grad->start.x,grad->start.y);
		ss2->first = ss2->last = sp1;
		ss1->next = ss2;
	    }
	}
	gdd->sc_grad.layers[ly_fore].splines = ss1;
    }

    gdd->sc_grad.layers[ly_back]. splines = SplinePointListCopy( LayerAllSplines(ly));
    LayerUnAllSplines(ly);
}

static struct col_init stopci[] = {
    { me_real , NULL, NULL, NULL, N_("Offset %") },
    { me_hex, NULL, NULL, NULL, N_("Color") },
    { me_real, NULL, NULL, NULL, N_("Opacity") }
    };

static int Grad_CanDelete(GGadget *g,int row) {
    int rows;
    struct matrix_data *md = GMatrixEditGet(g, &rows);
    if ( md==NULL )
return( false );

    /* There must always be at least two entries in the table */
return( rows>2 );
}

static void Grad_NewRow(GGadget *g,int row) {
    int rows;
    struct matrix_data *md = GMatrixEditGet(g, &rows);
    if ( md==NULL )
return;

    md[3*row+2].u.md_real = 1.0;
}

static void StopMatrixInit(struct matrixinit *mi,struct gradient *grad) {
    int i;
    struct matrix_data *md;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 3;
    mi->col_init = stopci;

    if ( grad==NULL ) {
	md = calloc(2*mi->col_cnt,sizeof(struct matrix_data));
	md[3*0+0].u.md_real = 0;
	md[3*0+1].u.md_ival = 0x000000;
	md[3*0+2].u.md_real = 1;
	md[3*1+0].u.md_real = 100;
	md[3*1+1].u.md_ival = 0xffffff;
	md[3*1+2].u.md_real = 1;
	mi->initial_row_cnt = 2;
    } else {
	md = calloc(3*grad->stop_cnt,sizeof(struct matrix_data));
	for ( i=0; i<grad->stop_cnt; ++i ) {
	    md[3*i+0].u.md_real = grad->grad_stops[i].offset*100.0;
	    md[3*i+1].u.md_ival = grad->grad_stops[i].col;
	    md[3*i+2].u.md_real = grad->grad_stops[i].opacity;
	}
	mi->initial_row_cnt = grad->stop_cnt;
    }
    mi->matrix_data = md;

    mi->initrow = Grad_NewRow;
    mi->candelete = Grad_CanDelete;
}

static struct gradient *GradientEdit(struct layer_dlg *ld,struct gradient *active) {
    GradientDlg gdd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14], boxes[5], *harray[8], *varray[10],
	*rharray[5], *gtarray[5];
    GTextInfo label[14];
    int j,k;
    struct matrixinit stopmi;

    GDDInit( &gdd,ld->sf,ld->layer,active );
    gdd.active = active;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Gradient");
    pos.width = 600;
    pos.height = 300;
    gdd.gw = gw = GDrawCreateTopWindow(NULL,&pos,gdd_e_h,&gdd.cv_grad,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = j = 0;
    gcd[k].gd.flags = gg_visible|gg_enabled ;		/* This space is for the menubar */
    gcd[k].gd.pos.height = 18; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;
    varray[j++] = &gcd[k-1];

    label[k].text = (unichar_t *) _(
	"  A linear gradient is represented by a line drawn\n"
	"from its start point to its end point.\n"
	"  A radial gradient is represented by a line drawn\n"
	"from its center whose length is the ultimate radius.\n"
	"If there is a single additional point, that point\n"
	"represents the gradient's focus, if omitted the focus\n"
	"is the same as the radius." );
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[j++] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Linear");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _(
	    "The gradient will be a linear gradient,\n"
	    "With the color change happening along\n"
	    "the line drawn in the view" );
    gcd[k].gd.cid = CID_Linear;
    gcd[k++].creator = GRadioCreate;
    gtarray[0] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Radial");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _(
	    "The gradient will be a radial gradient,\n"
	    "With the color change happening in circles\n"
	    "starting at the focus (if specified) and\n"
	    "extending outward until it reaches the\n"
	    "specified radius." );
    gcd[k].gd.cid = CID_Radial;
    gcd[k++].creator = GRadioCreate;
    gtarray[1] = &gcd[k-1]; gtarray[2] = GCD_Glue; gtarray[3] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = gtarray;
    boxes[4].creator = GHBoxCreate;
    varray[j++] = &boxes[4];

    gcd[k].gd.pos.width = gcd[k].gd.pos.height = 200;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Gradient;
    gcd[k].gd.u.drawable_e_h = gdd_sub_e_h;
    gcd[k++].creator = GDrawableCreate;
    varray[j++] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("_Pad");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("Beyond the endpoints, the gradient takes on the color at the end-points\n"
		"This does not work for PostScript linear gradients");
    gcd[k].gd.cid = CID_Pad;
    gcd[k++].creator = GRadioCreate;
    rharray[0] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Repeat");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("Beyond the endpoints the gradient repeats itself\n"
	    "This does not work for PostScript gradients." );
    gcd[k].gd.cid = CID_Repeat;
    gcd[k++].creator = GRadioCreate;
    rharray[1] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Reflect");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("Beyond the endpoint the gradient repeats itself, but reflected.\n"
	    "This does not work for PostScript gradients");
    gcd[k].gd.cid = CID_Reflect;
    gcd[k++].creator = GRadioCreate;
    rharray[2] = &gcd[k-1];
    rharray[3] = GCD_Glue; rharray[4] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = rharray;
    boxes[2].creator = GHBoxCreate;
    varray[j++] = &boxes[2];

    label[k].text = (unichar_t *) _(
	    "Specify the color (& opacity) at stop points\n"
	    "along the line drawn above. The offset is a\n"
	    "percentage of the distance from the start to\n"
	    "the end of the line. The color is a 6 (hex)\n"
	    "digit number expressing an RGB color.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[j++] = &gcd[k-1];

    StopMatrixInit(&stopmi,active);

    gcd[k].gd.pos.width = 300; gcd[k].gd.pos.height = 200;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_GradStops;
    gcd[k].gd.u.matrix = &stopmi;
    gcd[k++].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = Gradient_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = Gradient_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[k-2]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray;
    boxes[3].creator = GHBoxCreate;
    varray[j++] = &boxes[3];
    varray[j] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    GDDCharViewInits(&gdd,CID_Gradient);

    GHVBoxSetExpandableRow(boxes[0].ret,3);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[3].ret,gb_expandgluesame);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    if ( active!=NULL ) {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Linear),active->radius==0);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Radial),active->radius!=0);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Pad),active->sm==sm_pad);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Reflect),active->sm==sm_reflect);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Repeat),active->sm==sm_repeat);
    } else {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Linear),true);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Pad),true);
    }

    GDDMakeActive(&gdd,&gdd.cv_grad);

    GDrawResize(gw,400,768);		/* Force a resize event */

    GDrawSetVisible(gdd.gw,true);

    while ( !gdd.done )
	GDrawProcessOneEvent(NULL);

    {
	CharView *cv = &gdd.cv_grad;
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	CVPalettesHideIfMine(cv);
    }
    GDrawDestroyWindow(gdd.gw);
    /* Now because the cv of the nested window is on the stack, we need to */
    /*  handle its destruction HERE while that stack location still makes */
    /*  sense. Can't do it in whoever calls us */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return( gdd.active );
}

static void Layer_GradSet(struct layer_dlg *ld) {
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillGradAdd),ld->fillgrad==NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillGradEdit),ld->fillgrad!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillGradDelete),ld->fillgrad!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokeGradAdd),ld->strokegrad==NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokeGradEdit),ld->strokegrad!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokeGradDelete),ld->strokegrad!=NULL);
}

static int Layer_FillGradDelete(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	GradientFree(ld->fillgrad);
	ld->fillgrad=NULL;
	Layer_GradSet(ld);
    }
return( true );
}

static int Layer_FillGradAddEdit(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->fillgrad = GradientEdit(ld,ld->fillgrad);
	Layer_GradSet(ld);
    }
return( true );
}

static int Layer_StrokeGradDelete(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	GradientFree(ld->strokegrad);
	ld->strokegrad=NULL;
	Layer_GradSet(ld);
    }
return( true );
}

static int Layer_StrokeGradAddEdit(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->strokegrad = GradientEdit(ld,ld->strokegrad);
	Layer_GradSet(ld);
    }
return( true );
}

#define CID_PatternName	1001
#define CID_Skew	1002
#define CID_Rotate	1003
#define	CID_TransX	1004
#define CID_TransY	1005
#define CID_Transform	1006
#define CID_Aspect	1007
#define CID_TWidth	1008
#define CID_THeight	1009

static int Pat_WidthChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent &&
	    (e->u.control.subtype == et_textchanged ||
	     e->u.control.subtype == et_radiochanged)) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(gw);
	char *name = GGadgetGetTitle8(GWidgetGetControl(gw,CID_PatternName));
	SplineChar *patternsc = SFGetChar(ld->sf,-1,name);
	DBounds b;
	int err = false;
	real height, width;
	char buffer[50];

	free(name);
	if ( patternsc==NULL )
return( true );
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_Aspect)))
return( true );
	width = GetCalmReal8(gw,CID_TWidth,_("Width"),&err);
	if ( err )
return( true );
	PatternSCBounds(patternsc,&b);
	height = width * (b.maxy - b.miny)/(b.maxx - b.minx);
	sprintf( buffer, "%g", (double) height );
	GGadgetSetTitle8(GWidgetGetControl(gw,CID_THeight), buffer);
    }
return( true );
}

static int Pat_HeightChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(gw);
	char *name = GGadgetGetTitle8(GWidgetGetControl(gw,CID_PatternName));
	SplineChar *patternsc = SFGetChar(ld->sf,-1,name);
	DBounds b;
	int err = false;
	real height, width;
	char buffer[50];

	free(name);
	if ( patternsc==NULL )
return( true );
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_Aspect)))
return( true );
	height = GetCalmReal8(gw,CID_THeight,_("Height"),&err);
	if ( err )
return( true );
	PatternSCBounds(patternsc,&b);
	width = height * (b.maxx - b.minx)/(b.maxy - b.miny);
	sprintf( buffer, "%g", (double) width );
	GGadgetSetTitle8(GWidgetGetControl(gw,CID_TWidth), buffer);
    }
return( true );
}

static int Pat_TransformChanged(GGadget *g, GEvent *e) {

    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	GWindow gw = GGadgetGetWindow(g);
	double trans[6];
	char *name = GGadgetGetTitle8(g);
	double c, s, t;
	char buffer[50];

	if ( sscanf( name, "[%lg %lg %lg %lg %lg %lg]", &trans[0], &trans[1], &trans[2],
		&trans[3], &trans[4], &trans[5])!=6 ) {
	    free(name );
return( true );
	}
	free(name );

	c = trans[0]; s = trans[1];
	if ( c!=0 )
	    t = (trans[2]+s)/c;
	else if ( s!=0 )
	    t = (trans[3]-c)/s;
	else
	    t = 9999;
	if ( RealWithin(c*c+s*s,1,.005) && RealWithin(t*c-s,trans[2],.01) && RealWithin(t*s+c,trans[3],.01)) {
	    double skew = atan(t)*180/3.1415926535897932;
	    double rot  = atan2(s,c)*180/3.1415926535897932;
	    sprintf( buffer, "%g", skew );
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Skew), buffer);
	    sprintf( buffer, "%g", rot );
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Rotate), buffer);
	    sprintf( buffer, "%g", trans[4] );
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_TransX), buffer);
	    sprintf( buffer, "%g", trans[5] );
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_TransY), buffer);
	} else {
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Skew), "");
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Rotate), "");
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_TransX), "");
	    GGadgetSetTitle8(GWidgetGetControl(gw,CID_TransY), "");
	}
    }
return( true );
}

static int Pat_AnglesChanged(GGadget *g, GEvent *e) {

    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	GWindow gw = GGadgetGetWindow(g);
	double rotate, skew, x, y;
	double c, s, t;
	char buffer[340];
	int err=false;

	skew   = GetCalmReal8(gw,CID_Skew,_("Skew"),&err)*3.1415926535897932/180;
	rotate = GetCalmReal8(gw,CID_Rotate,_("Rotate"),&err)*3.1415926535897932/180;
	x      = GetCalmReal8(gw,CID_TransX,_("Translation in X"),&err);
	y      = GetCalmReal8(gw,CID_TransY,_("Translation in Y"),&err);
	if ( err )
return( true );
	t = tan(skew);
	c = cos(rotate); s = sin(rotate);
	sprintf( buffer, "[%g %g %g %g %g %g]", c, s, t*c-s, t*s+c, x, y );
	GGadgetSetTitle8(GWidgetGetControl(gw,CID_Transform),buffer );
    }
return( true );
}

static unichar_t **Pat_GlyphNameCompletion(GGadget *t,int from_tab) {
    struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(t));

return( SFGlyphNameCompletion(ld->sf,t,from_tab,false));
}

static int Pat_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(gw);
	char *transstring = GGadgetGetTitle8(GWidgetGetControl(gw,CID_Transform));
	char *name;
	double width, height;
	double trans[6];
	int i;
	SplineChar *patternsc;
	int err;

	if ( sscanf( transstring, "[%lg %lg %lg %lg %lg %lg]", &trans[0], &trans[1], &trans[2],
		&trans[3], &trans[4], &trans[5])!=6 ) {
	    free( transstring );
	    ff_post_error(_("Bad Transformation matrix"),_("Bad Transformation matrix"));
return( true );
	}
	free(transstring);

	name = GGadgetGetTitle8(GWidgetGetControl(gw,CID_PatternName));
	patternsc = SFGetChar(ld->sf,-1,name);
	if ( patternsc==NULL ) {
	    ff_post_error(_("No Glyph"),_("This font does not contain a glyph named \"%.40s\""), name);
	    free(name);
return( true );
	}

	err = false;
	width = GetReal8(gw,CID_TWidth,_("Width"),&err);
	height = GetReal8(gw,CID_THeight,_("Height"),&err);
	if ( err )
return( true );

	if ( ld->curpat == NULL )
	    ld->curpat = chunkalloc(sizeof(struct pattern));
	free( ld->curpat->pattern );
	ld->curpat->pattern = name;
	for ( i=0; i<6; ++i )
	    ld->curpat->transform[i] = trans[i];
	ld->curpat->width = width;
	ld->curpat->height = height;
	ld->pat_done = true;
    }
return( true );
}

static int Pat_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(gw);
	ld->pat_done = true;
    }
return( true );
}

static int pat_e_h(GWindow gw, GEvent *event) {
    struct layer_dlg *ld = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
return( false );
      break;
      case et_close:
	ld->pat_done = true;
      break;
    }
return( true );
}

static struct pattern *PatternEdit(struct layer_dlg *ld,struct pattern *active) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25], boxes[5], *harray[8], *varray[10],
	*hvarray[42];
    GTextInfo label[25];
    int j,k;
    char *name;
    char width[50], height[50], transform[340];
    int aspect_fixed = true;

    ld->pat_done = false;
    ld->curpat = active;

    name = "";
    width[0] = height[0] = '\0';
    strcpy(transform,"[1 0 0 1 0 0]");
    aspect_fixed = true;
    if ( active!=NULL ) {
	SplineChar *patternsc = SFGetChar(ld->sf,-1,active->pattern);
	name = active->pattern;
	sprintf( width, "%g", (double) active->width );
	sprintf( height, "%g", (double) active->height );
	sprintf( transform, "[%g %g %g %g %g %g]",
		(double) active->transform[0], (double) active->transform[1],
		(double) active->transform[2], (double) active->transform[3],
		(double) active->transform[4], (double) active->transform[5]);
	if ( patternsc!=NULL ) {
	    DBounds b;
	    PatternSCBounds(patternsc,&b);
	    aspect_fixed = RealNear(active->width*(b.maxy-b.miny),active->height*(b.maxx-b.minx));
	}
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Tile Pattern");
    pos.width = 600;
    pos.height = 300;
    gw = GDrawCreateTopWindow(NULL,&pos,pat_e_h,ld,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = j = 0;

    label[k].text = (unichar_t *) _(
	"The pattern itself should be drawn in another glyph\n"
	"of the current font. Specify a glyph name:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[j++] = &gcd[k-1];

    label[k].text = (unichar_t *) name;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_WidthChanged;		/* That will make sure the aspect ratio stays correct */
    gcd[k].gd.cid = CID_PatternName;
    gcd[k++].creator = GTextCompletionCreate;
    varray[j++] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    if ( aspect_fixed )
	gcd[k].gd.flags |= gg_cb_on;
    label[k].text = (unichar_t *) _("Aspect Ratio same as Tile Glyph");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_Aspect;
    gcd[k].gd.handle_controlevent = Pat_WidthChanged;		/* That will make sure the aspect ratio stays correct */
    gcd[k++].creator = GCheckBoxCreate;
    varray[j++] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Width:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) width;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_WidthChanged;
    gcd[k].gd.cid = CID_TWidth;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[1] = &gcd[k-1]; hvarray[2] = GCD_Glue; hvarray[3] = NULL;

    label[k].text = (unichar_t *) _("Height:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[4] = &gcd[k-1];

    label[k].text = (unichar_t *) height;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_HeightChanged;
    gcd[k].gd.cid = CID_THeight;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[5] = &gcd[k-1]; hvarray[6] = GCD_Glue; hvarray[7] = NULL;

    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLineCreate;
    hvarray[8] = &gcd[k-1];
    hvarray[9] = GCD_ColSpan; hvarray[10] = GCD_Glue; hvarray[11] = NULL;

    label[k].text = (unichar_t *) _("Rotate:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[12] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_AnglesChanged;
    gcd[k].gd.cid = CID_Rotate;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[13] = &gcd[k-1]; hvarray[14] = GCD_Glue; hvarray[15] = NULL;

    label[k].text = (unichar_t *) _("Skew:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[16] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_AnglesChanged;
    gcd[k].gd.cid = CID_Skew;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[17] = &gcd[k-1]; hvarray[18] = GCD_Glue; hvarray[19] = NULL;

    label[k].text = (unichar_t *) _("Translate By");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[20] = &gcd[k-1];
    hvarray[21] = GCD_ColSpan; hvarray[22] = GCD_Glue; hvarray[23] = NULL;

    label[k].text = (unichar_t *) _("X:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[24] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_AnglesChanged;
    gcd[k].gd.cid = CID_TransX;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[25] = &gcd[k-1]; hvarray[26] = GCD_Glue; hvarray[27] = NULL;

    label[k].text = (unichar_t *) _("Y:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[28] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_AnglesChanged;
    gcd[k].gd.cid = CID_TransY;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[29] = &gcd[k-1]; hvarray[30] = GCD_Glue; hvarray[31] = NULL;

    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLineCreate;
    hvarray[32] = &gcd[k-1];
    hvarray[33] = GCD_ColSpan; hvarray[34] = GCD_Glue; hvarray[35] = NULL;

    label[k].text = (unichar_t *) _("Transform:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[36] = &gcd[k-1];

    label[k].text = (unichar_t *) transform;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = Pat_TransformChanged;
    gcd[k].gd.cid = CID_Transform;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[37] = &gcd[k-1]; hvarray[38] = GCD_ColSpan; hvarray[39] = NULL;
    hvarray[40] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = hvarray;
    boxes[2].creator = GHVBoxCreate;
    varray[j++] = &boxes[2];

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = Pat_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = Pat_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[k-2]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray;
    boxes[3].creator = GHBoxCreate;
    varray[j++] = &boxes[3];
    varray[j++] = GCD_Glue;
    varray[j] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,2);
    GHVBoxSetExpandableRow(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);


    Pat_TransformChanged(GWidgetGetControl(gw,CID_Transform),NULL);
    GCompletionFieldSetCompletion(gcd[1].ret,Pat_GlyphNameCompletion);

    GDrawSetVisible(gw,true);

    while ( !ld->pat_done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
return( ld->curpat );
}

static void Layer_PatSet(struct layer_dlg *ld) {
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillPatAdd),ld->fillpat==NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillPatEdit),ld->fillpat!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_FillPatDelete),ld->fillpat!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokePatAdd),ld->strokepat==NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokePatEdit),ld->strokepat!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ld->gw,CID_StrokePatDelete),ld->strokepat!=NULL);
}

static int Layer_FillPatDelete(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	PatternFree(ld->fillpat);
	ld->fillpat=NULL;
	Layer_PatSet(ld);
    }
return( true );
}

static int Layer_FillPatAddEdit(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->fillpat = PatternEdit(ld,ld->fillpat);
	Layer_PatSet(ld);
    }
return( true );
}

static int Layer_StrokePatDelete(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	PatternFree(ld->strokepat);
	ld->strokepat=NULL;
	Layer_PatSet(ld);
    }
return( true );
}

static int Layer_StrokePatAddEdit(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->strokepat = PatternEdit(ld,ld->strokepat);
	Layer_PatSet(ld);
    }
return( true );
}

static uint32 getcol(GGadget *g,int *err) {
    const unichar_t *ret=_GGadgetGetTitle(g);
    unichar_t *end;
    uint32 col = COLOR_INHERITED;

    if ( *ret=='#' ) ++ret;
    col = u_strtol(ret,&end,16);
    if ( col<0 || col>0xffffff || *end!='\0' ) {
	*err = true;
	ff_post_error(_("Bad Color"),_("Bad Color"));
    }
return( col );
}

static int Layer_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	Layer temp;
	int err=false;
	const unichar_t *ret;
	unichar_t *end, *end2;
	int i;

	LayerDefault(&temp);
	temp.dofill = GGadgetIsChecked(GWidgetGetControl(gw,CID_Fill));
	temp.dostroke = GGadgetIsChecked(GWidgetGetControl(gw,CID_Stroke));
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FillCInherit)) )
	    temp.fill_brush.col = COLOR_INHERITED;
	else
	    temp.fill_brush.col = getcol(GWidgetGetControl(gw,CID_FillColor),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FillOInherit)) )
	    temp.fill_brush.opacity = -1;
	else
	    temp.fill_brush.opacity = GetReal8(gw,CID_FillOpacity,_("Opacity:"),&err);

	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeCInherit)) )
	    temp.stroke_pen.brush.col = COLOR_INHERITED;
	else
	    temp.stroke_pen.brush.col = getcol(GWidgetGetControl(gw,CID_StrokeColor),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeOInherit)) )
	    temp.stroke_pen.brush.opacity = -1;
	else
	    temp.stroke_pen.brush.opacity = GetReal8(gw,CID_StrokeOpacity,_("Opacity:"),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeWInherit)) )
	    temp.stroke_pen.width = WIDTH_INHERITED;
	else
	    temp.stroke_pen.width = GetReal8(gw,CID_Width,_("_Width"),&err);
	if ( err )
return( true );

	ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Trans));
	while ( *ret==' ' || *ret=='[' ) ++ret;
	temp.stroke_pen.trans[0] = u_strtod(ret,&end);
	temp.stroke_pen.trans[1] = u_strtod(end,&end);
	temp.stroke_pen.trans[2] = u_strtod(end,&end);
	temp.stroke_pen.trans[3] = u_strtod(end,&end2);
	for ( ret = end2 ; *ret==' ' || *ret==']' ; ++ret );
	if ( end2==end || *ret!='\0' || temp.stroke_pen.trans[0] ==0 || temp.stroke_pen.trans[3]==0 ) {
	    ff_post_error(_("Bad Transformation Matrix"),_("Bad Transformation Matrix"));
return( true );
	}
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_DashesInherit)) ) {
	    temp.stroke_pen.dashes[0] = 0; temp.stroke_pen.dashes[1] = DASH_INHERITED;
	} else {
	    ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Dashes));
	    while ( *ret==' ' || *ret=='[' ) ++ret;
	    for ( i=0; ; ++i ) {
		long val = u_strtol(ret,&end,10);
		if ( *end=='\0' )
	    break;
		if ( val<0 || val>255 ) {
		    ff_post_error(_("Bad dash list"),_("Value out of range"));
return( true );
		} else if ( *end!=' ' ) {
		    ff_post_error(_("Bad dash list"),_("Bad Number"));
return( true );
		} else if ( i>=DASH_MAX ) {
		    ff_post_error(_("Bad dash list"),_("Too many dashes (at most %d allowed)"), DASH_MAX);
return( true );
		}
		temp.stroke_pen.dashes[i] = val;
		ret = end;
		while ( *ret==' ' ) ++ret;
	    }
	    if ( i<DASH_MAX ) temp.stroke_pen.dashes[i] = 0;
	}
	temp.stroke_pen.linecap =
		GGadgetIsChecked(GWidgetGetControl(gw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_RoundCap))?lc_round:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_SquareCap))?lc_square:
			lc_inherited;
	temp.stroke_pen.linejoin =
		GGadgetIsChecked(GWidgetGetControl(gw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_RoundJoin))?lj_round:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_MiterJoin))?lj_miter:
			lj_inherited;

	GradientFree(ld->layer->fill_brush.gradient);
	PatternFree(ld->layer->fill_brush.pattern);
	GradientFree(ld->layer->stroke_pen.brush.gradient);
	PatternFree(ld->layer->stroke_pen.brush.pattern);

	ld->done = ld->ok = true;
	ld->layer->stroke_pen = temp.stroke_pen;
	ld->layer->fill_brush = temp.fill_brush;
	ld->layer->dofill = temp.dofill;
	ld->layer->dostroke = temp.dostroke;
	ld->layer->fillfirst = temp.fillfirst;

	ld->layer->fill_brush.gradient = ld->fillgrad;
	ld->layer->stroke_pen.brush.gradient = ld->strokegrad;
	ld->layer->fill_brush.pattern = ld->fillpat;
	ld->layer->stroke_pen.brush.pattern = ld->strokepat;
    }
return( true );
}

static int Layer_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->done = true;
    }
return( true );
}

static int Layer_Inherit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int cid = (intpt) GGadgetGetUserData(g);
	GGadgetSetEnabled(GWidgetGetControl(gw,cid),
		!GGadgetIsChecked(g));
    }
return( true );
}

static int Layer_DoColorWheel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	int cid = (intpt) GGadgetGetUserData(g);
	GGadget *tf = GWidgetGetControl(gw,cid);
	if ( GGadgetIsEnabled(tf)) {
	    char *pt, *text = GGadgetGetTitle8(tf);
	    char buf[12];
	    Color val;
	    struct hslrgb col;
	    pt = text;
	    while ( isspace(*pt)) ++pt;
	    if ( *pt=='0' && (pt[1]=='x' || pt[1]=='X')) pt += 2;
	    else if ( *pt=='#' ) ++pt;
	    val = strtoul(pt,NULL,16);
	    gColor2Hslrgb(&col,val);
	    col = GWidgetColor(_("Pick a color"),&col,NULL);
	    if ( col.rgb ) {
		val = gHslrgb2Color(&col);
		sprintf(buf,"#%06x", val );
		GGadgetSetTitle8(tf,buf);
	    }
	}
    }
return( true );
}

static int layer_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct layer_dlg *ld = GDrawGetUserData(gw);
	ld->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multilayer.html#Layer");
return( true );
	}
return( false );
    }
return( true );
}

int LayerDialog(Layer *layer,SplineFont *sf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[62], boxes[12];
    GGadgetCreateData *barray[10], *varray[4], *fhvarray[22], *shvarray[46],
	    *lcarray[6], *ljarray[6], *fgarray[5], *fparray[5], *sgarray[5],
	    *sparray[5];
    GTextInfo label[62];
    struct layer_dlg ld;
    int yoff=0;
    int gcdoff, fill_gcd, stroke_gcd, k, j;
    char widthbuf[20], fcol[12], scol[12], fopac[30], sopac[30], transbuf[150],
	    dashbuf[60];
    int i;
    char *pt;

    memset(&ld,0,sizeof(ld));
    ld.layer = layer;
    ld.sf    = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Layer");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,LY_Width));
    pos.height = GDrawPointsToPixels(NULL,LY_Height);
    ld.gw = gw = GDrawCreateTopWindow(NULL,&pos,layer_e_h,&ld,&wattrs);

    ld.fillgrad = GradientCopy(layer->fill_brush.gradient,NULL);
    ld.strokegrad = GradientCopy(layer->stroke_pen.brush.gradient,NULL);
    ld.fillpat = PatternCopy(layer->fill_brush.pattern,NULL);
    ld.strokepat = PatternCopy(layer->stroke_pen.brush.pattern,NULL);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    gcdoff = k = 0;

    label[gcdoff].text = (unichar_t *) _("Fi_ll");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = 5+yoff;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->dofill? gg_cb_on : 0);
    gcd[gcdoff].gd.cid = CID_Fill;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    fill_gcd = gcdoff;

    label[gcdoff].text = (unichar_t *) _("Color:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    sprintf( fcol, "#%06x", layer->fill_brush.col );
    label[gcdoff].text = (unichar_t *) fcol;
    label[gcdoff].text_is_1byte = true;
    if ( layer->fill_brush.col==COLOR_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_FillColor;
    gcd[gcdoff++].creator = GTextFieldCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->fill_brush.col==COLOR_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_FillColor;
    gcd[gcdoff].gd.cid = CID_FillCInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].image = GGadgetImageCache("colorwheel.png");
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.flags = gg_enabled|gg_visible;
    gcd[gcdoff].data = (void *) CID_FillColor;
    gcd[gcdoff].gd.handle_controlevent = Layer_DoColorWheel;
    gcd[gcdoff++].creator = GButtonCreate;
    fhvarray[k++] = &gcd[gcdoff-1];
    fhvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Opacity:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    sprintf( fopac, "%g", layer->fill_brush.opacity );
    label[gcdoff].text = (unichar_t *) fopac;
    label[gcdoff].text_is_1byte = true;
    if ( layer->fill_brush.opacity<0 ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_FillOpacity;
    gcd[gcdoff++].creator = GTextFieldCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->fill_brush.opacity<0? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_FillOpacity;
    gcd[gcdoff].gd.cid = CID_FillOInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;
    fhvarray[k++] = &gcd[gcdoff-1];
    fhvarray[k++] = GCD_Glue;
    fhvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Gradient:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.gradient==NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Add");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillGradAddEdit;
    gcd[gcdoff].gd.cid = CID_FillGradAdd;
    gcd[gcdoff++].creator = GButtonCreate;
    fgarray[0] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.gradient!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Edit");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillGradAddEdit;
    gcd[gcdoff].gd.cid = CID_FillGradEdit;
    gcd[gcdoff++].creator = GButtonCreate;
    fgarray[1] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.gradient!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Delete");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillGradDelete;
    gcd[gcdoff].gd.cid = CID_FillGradDelete;
    gcd[gcdoff++].creator = GButtonCreate;
    fgarray[2] = &gcd[gcdoff-1];
    fgarray[3] = GCD_Glue;
    fgarray[4] = NULL;

    boxes[7].gd.flags = gg_enabled|gg_visible;
    boxes[7].gd.u.boxelements = fgarray;
    boxes[7].creator = GHBoxCreate;
    fhvarray[k++] = &boxes[7];
    fhvarray[k++] = GCD_ColSpan;
    fhvarray[k++] = GCD_Glue;
    fhvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Pattern:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    fhvarray[k++] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.pattern==NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Add");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillPatAddEdit;
    gcd[gcdoff].gd.cid = CID_FillPatAdd;
    gcd[gcdoff++].creator = GButtonCreate;
    fparray[0] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.pattern!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Edit");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillPatAddEdit;
    gcd[gcdoff].gd.cid = CID_FillPatEdit;
    gcd[gcdoff++].creator = GButtonCreate;
    fparray[1] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->fill_brush.pattern!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Delete");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_FillPatDelete;
    gcd[gcdoff].gd.cid = CID_FillPatDelete;
    gcd[gcdoff++].creator = GButtonCreate;
    fparray[2] = &gcd[gcdoff-1];
    fparray[3] = GCD_Glue;
    fparray[4] = NULL;

    boxes[8].gd.flags = gg_enabled|gg_visible;
    boxes[8].gd.u.boxelements = fparray;
    boxes[8].creator = GHBoxCreate;
    fhvarray[k++] = &boxes[8];
    fhvarray[k++] = GCD_ColSpan;
    fhvarray[k++] = GCD_Glue;
    fhvarray[k++] = NULL;
    fhvarray[k++] = NULL;


    boxes[2].gd.pos.x = boxes[2].gd.pos.y = 2;
    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = fhvarray;
    boxes[2].gd.label = (GTextInfo *) &gcd[fill_gcd-1];
    boxes[2].creator = GHVGroupCreate;

    label[gcdoff].text = (unichar_t *) _("Stroke");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'S';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[fill_gcd].gd.pos.y+gcd[fill_gcd].gd.pos.height+4;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->dostroke? gg_cb_on : 0);
    gcd[gcdoff].gd.cid = CID_Stroke;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    stroke_gcd = gcdoff;
    k = 0;

    label[gcdoff].text = (unichar_t *) _("Color:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    sprintf( scol, "#%06x", layer->stroke_pen.brush.col );
    label[gcdoff].text = (unichar_t *) scol;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.brush.col==COLOR_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_StrokeColor;
    gcd[gcdoff++].creator = GTextFieldCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.brush.col==COLOR_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_StrokeColor;
    gcd[gcdoff].gd.cid = CID_StrokeCInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].image = GGadgetImageCache("colorwheel.png");
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.flags = gg_enabled|gg_visible;
    gcd[gcdoff].data = (void *) CID_StrokeColor;
    gcd[gcdoff].gd.handle_controlevent = Layer_DoColorWheel;
    gcd[gcdoff++].creator = GButtonCreate;
    shvarray[k++] = &gcd[gcdoff-1];
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Opacity:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'W';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    sprintf( sopac, "%g", layer->stroke_pen.brush.opacity );
    label[gcdoff].text = (unichar_t *) sopac;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.brush.opacity<0 ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_StrokeOpacity;
    gcd[gcdoff++].creator = GTextFieldCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.brush.opacity<0? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_StrokeOpacity;
    gcd[gcdoff].gd.cid = CID_StrokeOInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;
    shvarray[k++] = &gcd[gcdoff-1];
    shvarray[k++] = GCD_Glue;
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Gradient:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.gradient==NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Add");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokeGradAddEdit;
    gcd[gcdoff].gd.cid = CID_StrokeGradAdd;
    gcd[gcdoff++].creator = GButtonCreate;
    sgarray[0] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.gradient!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Edit");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokeGradAddEdit;
    gcd[gcdoff].gd.cid = CID_StrokeGradEdit;
    gcd[gcdoff++].creator = GButtonCreate;
    sgarray[1] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.gradient!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Delete");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokeGradDelete;
    gcd[gcdoff].gd.cid = CID_StrokeGradDelete;
    gcd[gcdoff++].creator = GButtonCreate;
    sgarray[2] = &gcd[gcdoff-1];
    sgarray[3] = GCD_Glue;
    sgarray[4] = NULL;

    boxes[9].gd.flags = gg_enabled|gg_visible;
    boxes[9].gd.u.boxelements = sgarray;
    boxes[9].creator = GHBoxCreate;
    shvarray[k++] = &boxes[9];
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_Glue;
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Pattern:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.pattern==NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Add");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokePatAddEdit;
    gcd[gcdoff].gd.cid = CID_StrokePatAdd;
    gcd[gcdoff++].creator = GButtonCreate;
    sparray[0] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.pattern!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Edit");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokePatAddEdit;
    gcd[gcdoff].gd.cid = CID_StrokePatEdit;
    gcd[gcdoff++].creator = GButtonCreate;
    sparray[1] = &gcd[gcdoff-1];

    gcd[gcdoff].gd.flags = layer->stroke_pen.brush.pattern!=NULL ? (gg_visible | gg_enabled) : gg_visible;
    label[gcdoff].text = (unichar_t *) _("Delete");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_StrokePatDelete;
    gcd[gcdoff].gd.cid = CID_StrokePatDelete;
    gcd[gcdoff++].creator = GButtonCreate;
    sparray[2] = &gcd[gcdoff-1];
    sparray[3] = GCD_Glue;
    sparray[4] = NULL;

    boxes[10].gd.flags = gg_enabled|gg_visible;
    boxes[10].gd.u.boxelements = sparray;
    boxes[10].creator = GHBoxCreate;
    shvarray[k++] = &boxes[10];
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_Glue;
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Stroke _Width:");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+26;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_WidthTxt;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    sprintf( widthbuf, "%g", layer->stroke_pen.width );
    label[gcdoff].text = (unichar_t *) widthbuf;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.width==WIDTH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_Width;
    gcd[gcdoff++].creator = GTextFieldCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.width==WIDTH_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_Width;
    gcd[gcdoff].gd.cid = CID_StrokeWInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;
    shvarray[k++] = &gcd[gcdoff-1];
    shvarray[k++] = GCD_Glue;
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("Dashes");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+26;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[gcdoff].gd.cid = CID_DashesTxt;
    gcd[gcdoff].gd.popup_msg = (unichar_t *) _("This specifies the dash pattern for a line.\nLeave this field blank for a solid line.\nOtherwise specify a list of up to 8 integers\n(between 0 and 255) which give the dash pattern\nin em-units. So \"10 10\" will draw the first\n10 units of a line, leave the next 10 blank,\ndraw the next 10, and so on.");
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    pt = dashbuf; dashbuf[0] = '\0';
    for ( i=0; i<DASH_MAX && layer->stroke_pen.dashes[i]!=0; ++i ) {
	sprintf( pt, "%d ", layer->stroke_pen.dashes[i]);
	pt += strlen(pt);
    }
    if ( pt>dashbuf ) pt[-1] = '\0';
    label[gcdoff].text = (unichar_t *) dashbuf;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.dashes[0]==0 && layer->stroke_pen.dashes[1]==DASH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_Dashes;
    gcd[gcdoff++].creator = GTextFieldCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    if ( layer->stroke_pen.dashes[0]==0 && layer->stroke_pen.dashes[1]==DASH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_cb_on | gg_utf8_popup;
    else
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[gcdoff].data = (void *) CID_Dashes;
    gcd[gcdoff].gd.cid = CID_DashesInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff].gd.popup_msg = (unichar_t *) _("This specifies the dash pattern for a line.\nLeave this field blank for a solid line.\nOtherwise specify a list of up to 8 integers\n(between 0 and 255) which give the dash pattern\nin em-units. So \"10 10\" will draw the first\n10 units of a line, leave the next 10 blank,\ndraw the next 10, and so on.");
    gcd[gcdoff++].creator = GCheckBoxCreate;
    shvarray[k++] = &gcd[gcdoff-1];
    shvarray[k++] = GCD_Glue;
    shvarray[k++] = NULL;

    label[gcdoff].text = (unichar_t *) _("_Transform Pen:");
    label[gcdoff].text_in_resource = true;
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;
    shvarray[k++] = &gcd[gcdoff-1];

    sprintf( transbuf, "[%.4g %.4g %.4g %.4g]", (double) layer->stroke_pen.trans[0],
	    (double) layer->stroke_pen.trans[1], (double) layer->stroke_pen.trans[2],
	    (double) layer->stroke_pen.trans[3]);
    label[gcdoff].text = (unichar_t *) transbuf;
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 210;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_Trans;
    gcd[gcdoff++].creator = GTextFieldCreate;
    shvarray[k++] = &gcd[gcdoff-1];
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = NULL;

    j = 0;
    label[gcdoff].text = (unichar_t *) _("Line Cap");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 10; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+20;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_LineCapTxt;
    gcd[gcdoff++].creator = GLabelCreate;

    label[gcdoff].text = (unichar_t *) _("_Butt");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_buttcap;
    gcd[gcdoff].gd.mnemonic = 'B';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 15; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_butt?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_ButtCap;
    gcd[gcdoff++].creator = GRadioCreate;
    lcarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("_Round");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_roundcap;
    gcd[gcdoff].gd.mnemonic = 'R';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_round?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_RoundCap;
    gcd[gcdoff++].creator = GRadioCreate;
    lcarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("S_quare");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_squarecap;
    gcd[gcdoff].gd.mnemonic = 'q';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 150; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_square?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_SquareCap;
    gcd[gcdoff++].creator = GRadioCreate;
    lcarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 220; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_inherited?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_InheritCap;
    gcd[gcdoff++].creator = GRadioCreate;
    lcarray[j++] = &gcd[gcdoff-1];
    lcarray[j++] = NULL;
    lcarray[j++] = NULL;

    boxes[3].gd.pos.x = boxes[3].gd.pos.y = 2;
    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = lcarray;
    boxes[3].gd.label = (GTextInfo *) &gcd[gcdoff-5];
    boxes[3].creator = GHVGroupCreate;
    shvarray[k++] = &boxes[3];
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = NULL;

    j=0;
    label[gcdoff].text = (unichar_t *) _("Line Join");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_LineJoinTxt;
    gcd[gcdoff++].creator = GLabelCreate;

    label[gcdoff].text = (unichar_t *) _("_Miter");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_miterjoin;
    gcd[gcdoff].gd.mnemonic = 'M';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_miter?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_MiterJoin;
    gcd[gcdoff++].creator = GRadioCreate;
    ljarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Ro_und");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_roundjoin;
    gcd[gcdoff].gd.mnemonic = 'u';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_round?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_RoundJoin;
    gcd[gcdoff++].creator = GRadioCreate;
    ljarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Be_vel");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_beveljoin;
    gcd[gcdoff].gd.mnemonic = 'v';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_bevel?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_BevelJoin;
    gcd[gcdoff++].creator = GRadioCreate;
    ljarray[j++] = &gcd[gcdoff-1];

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'q';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 220; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_inherited?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_InheritCap;
    gcd[gcdoff++].creator = GRadioCreate;
    ljarray[j++] = &gcd[gcdoff-1];
    ljarray[j++] = NULL;
    ljarray[j++] = NULL;

    boxes[4].gd.pos.x = boxes[4].gd.pos.y = 2;
    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = ljarray;
    boxes[4].gd.label = (GTextInfo *) &gcd[gcdoff-5];
    boxes[4].creator = GHVGroupCreate;
    shvarray[k++] = &boxes[4];
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = GCD_ColSpan;
    shvarray[k++] = NULL;
    shvarray[k++] = NULL;

    boxes[5].gd.pos.x = boxes[5].gd.pos.y = 2;
    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = shvarray;
    boxes[5].gd.label = (GTextInfo *) &gcd[stroke_gcd-1];
    boxes[5].creator = GHVGroupCreate;


    gcd[gcdoff].gd.pos.x = 30-3; gcd[gcdoff].gd.pos.y = LY_Height-30-3;
    gcd[gcdoff].gd.pos.width = -1;
    gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gcdoff].text = (unichar_t *) _("_OK");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_OK;
    gcd[gcdoff++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[gcdoff-1]; barray[2] = GCD_Glue; barray[3] = GCD_Glue;

    gcd[gcdoff].gd.pos.x = -30; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+3;
    gcd[gcdoff].gd.pos.width = -1;
    gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gcdoff].text = (unichar_t *) _("_Cancel");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_Cancel;
    gcd[gcdoff++].creator = GButtonCreate;
    barray[4] = GCD_Glue; barray[5] = &gcd[gcdoff-1]; barray[6] = GCD_Glue; barray[7] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = barray;
    boxes[6].creator = GHBoxCreate;

    varray[0] = &boxes[2]; varray[1] = &boxes[5]; varray[2] = &boxes[6]; varray[3] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,2);
    GHVBoxSetExpandableCol(boxes[5].ret,2);
    GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[7].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[8].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[9].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[10].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GWidgetHidePalettes();
    /*GWidgetIndicateFocusGadget(GWidgetGetControl(ld.gw,CID_Width));*/
    GDrawSetVisible(ld.gw,true);
    while ( !ld.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(ld.gw);
    if ( !ld.ok ) {
	GradientFree(ld.fillgrad);
	GradientFree(ld.strokegrad);
	PatternFree(ld.fillpat);
	PatternFree(ld.strokepat);
    }
return( ld.ok );
}
