/* -*- coding: utf-8 -*- */
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

#include <fontforge-config.h>

#include <vector>
#include <assert.h>
#include <math.h>

extern "C" {
#include "fontforgeui.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "gwidget.h"
}

#include "autohint.h"
#include "cvundoes.h"
#include "fvfonts.h"
#include "namelist.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottf.h"
#include "tottfgpos.h"
#include "ttf.h"
#include "ustring.h"
#include "gtk/find_problems_shim.hpp"

using ff::dlg::ProblemRecord;
using ff::dlg::ProblemTab;

/* ************************************************************************** */
/* ***************************** Problems Dialog **************************** */
/* ************************************************************************** */

GResFont validate_font = GRESFONT_INIT("400 11pt " SANS_UI_FAMILIES);

struct mgrpl {
    char *search;
    char *rpl;		/* a rpl of "" means delete (NULL means not found) */
};

struct mlrpl {
    uint32_t search;
    uint32_t rpl;
};

struct problems {
    FontView *fv;
    CharView *cv;
    SplineChar *sc;
    SplineChar *msc;
    int layer;
    unsigned int openpaths: 1;
    unsigned int intersectingpaths: 1;
    unsigned int nonintegral: 1;
    unsigned int pointstooclose: 1;
    unsigned int pointstoofar: 1;
    unsigned int xnearval: 1;
    unsigned int ynearval: 1;
    unsigned int ynearstd: 1;		/* baseline, xheight, cap, ascent, descent, etc. */
    unsigned int linenearstd: 1;	/* horizontal, vertical, italicangle */
    unsigned int cpnearstd: 1;		/* control points near: horizontal, vertical, italicangle */
    unsigned int cpodd: 1;		/* control points beyond points on spline */
    unsigned int hintwithnopt: 1;
    unsigned int ptnearhint: 1;
    unsigned int hintwidthnearval: 1;
    unsigned int missingextrema: 1;
    unsigned int direction: 1;
    unsigned int flippedrefs: 1;
    unsigned int cidmultiple: 1;
    unsigned int cidblank: 1;
    unsigned int bitmaps: 1;
    unsigned int bitmapwidths: 1;
    unsigned int advancewidth: 1;
    unsigned int vadvancewidth: 1;
    unsigned int stem3: 1;
    unsigned int showexactstem3: 1;
    unsigned int irrelevantcontrolpoints: 1;
    unsigned int multuni: 1;
    unsigned int multname: 1;
    unsigned int uninamemismatch: 1;
    unsigned int missinganchor: 1;
    unsigned int badsubs: 1;
    unsigned int missingglyph: 1;
    unsigned int missingscriptinfeature: 1;
    unsigned int toomanypoints: 1;
    unsigned int toomanyhints: 1;
    unsigned int toodeeprefs: 1;
    unsigned int ptmatchrefsoutofdate: 1;
    unsigned int multusemymetrics: 1;
    unsigned int refsbadtransformttf: 1;
    unsigned int refsbadtransformps: 1;
    unsigned int mixedcontoursrefs: 1;
    unsigned int bbymax: 1;
    unsigned int bbymin: 1;
    unsigned int bbxmax: 1;
    unsigned int bbxmin: 1;
    unsigned int overlappedhints: 1;
    unsigned int explain: 1;
    unsigned int doneexplain: 1;
    unsigned int finish: 1;
    unsigned int ignorethis: 1;
    double near, xval, yval, widthval;
    char *explaining;
    double found, expected;
    double xheight, caph, ascent, descent;
    double irrelevantfactor;
    int advancewidthval, vadvancewidthval;
    int bbymax_val, bbymin_val, bbxmax_val, bbxmin_val;
    int pointsmax, hintsmax, refdepthmax;
    GWindow explainw;
    GGadget *explaintext, *explainvals, *ignoregadg, *topbox;
    SplineChar *lastcharopened;
    CharView *cvopened;
    char *badsubsname;
    struct lookup_subtable *badsubs_lsubtable;
    AnchorClass *missinganchor_class;
    int rpl_cnt, rpl_max;
    struct mgrpl *mg;
    struct mlrpl *mlt;
    char *glyphname;
    int glyphenc;
    EncMap *map;
};

static int openpaths=0, pointstooclose=0/*, missing=0*/, doxnear=0, doynear=0;
static int nonintegral=0, pointstoofar=0;
static int intersectingpaths=0, missingextrema=0;
static int doynearstd=0, linestd=0, cpstd=0, cpodd=0, hintnopt=0, ptnearhint=0;
static int hintwidth=0, direction=0, flippedrefs=0, bitmaps=0, bitmapwidths=0;
static int cidblank=0, cidmultiple=0, advancewidth=0, vadvancewidth=0;
static int bbymax=0, bbymin=0, bbxmax=0, bbxmin=0;
static int irrelevantcp=0, missingglyph=0, missingscriptinfeature=0;
static int badsubs=0, missinganchor=0, toomanypoints=0, pointsmax = 1500;
static int multuni=0, multname=0, uninamemismatch=0, overlappedhints=0;
static int toomanyhints=0, hintsmax=96, toodeeprefs=0, refdepthmax=9;
static int ptmatchrefsoutofdate=0, refsbadtransformttf=0, refsbadtransformps=0;
static int mixedcontoursrefs=0, multusemymetrics=0;
static int stem3=0, showexactstem3=0;
static double near=3, xval=0, yval=0, widthval=50, advancewidthval=0, vadvancewidthval=0;
static double bbymax_val=0, bbymin_val=0, bbxmax_val=0, bbxmin_val=0;
static double irrelevantfactor = .005;

#define CID_Stop		2001
#define CID_Next		2002
#define CID_Fix			2003
#define CID_ClearAll		2004
#define CID_SetAll		2005

#define CID_OpenPaths		1001
#define CID_IntersectingPaths	1002
#define CID_PointsTooClose	1003
#define CID_XNear		1004
#define CID_YNear		1005
#define CID_YNearStd		1006
#define CID_HintNoPt		1007
#define CID_PtNearHint		1008
#define CID_HintWidthNear	1009
#define CID_HintWidth		1010
#define CID_Near		1011
#define CID_XNearVal		1012
#define CID_YNearVal		1013
#define CID_LineStd		1014
#define CID_Direction		1015
#define CID_CpStd		1016
#define CID_CpOdd		1017
#define CID_CIDMultiple		1018
#define CID_CIDBlank		1019
#define CID_FlippedRefs		1020
#define CID_Bitmaps		1021
#define CID_AdvanceWidth	1022
#define CID_AdvanceWidthVal	1023
#define CID_VAdvanceWidth	1024
#define CID_VAdvanceWidthVal	1025
#define CID_Stem3		1026
#define CID_ShowExactStem3	1027
#define CID_IrrelevantCP	1028
#define CID_IrrelevantFactor	1029
#define CID_BadSubs		1030
#define CID_MissingGlyph	1031
#define CID_MissingScriptInFeature 1032
#define CID_TooManyPoints	1033
#define CID_PointsMax		1034
#define CID_TooManyHints	1035
#define CID_HintsMax		1036
#define CID_TooDeepRefs		1037
#define CID_RefDepthMax		1038
#define CID_MultUni		1040
#define CID_MultName		1041
#define CID_PtMatchRefsOutOfDate 1042
#define CID_RefBadTransformTTF	1043
#define CID_RefBadTransformPS	1044
#define CID_MixedContoursRefs	1045
#define CID_MissingExtrema	1046
#define CID_UniNameMisMatch	1047
#define CID_BBYMax		1048
#define CID_BBYMin		1049
#define CID_BBXMax		1050
#define CID_BBXMin		1051
#define CID_BBYMaxVal		1052
#define CID_BBYMinVal		1053
#define CID_BBXMaxVal		1054
#define CID_BBXMinVal		1055
#define CID_NonIntegral		1056
#define CID_PointsTooFar	1057
#define CID_BitmapWidths	1058
#define CID_MissingAnchor	1059
#define CID_MultUseMyMetrics	1060
#define CID_OverlappedHints	1061


static void FixIt(struct problems *p) {
    SplinePointList *spl;
    SplinePoint *sp;
    /*StemInfo *h;*/
    RefChar *r;
    int ncp_changed, pcp_changed;

    if ( p->explaining==_("This reference has been flipped, so the paths in it are drawn backwards") ) {
	for ( r=p->sc->layers[p->layer].refs; r!=NULL && !r->selected; r = r->next );
	if ( r!=NULL ) {
	    SplineSet *ss, *spl;
	    SCPreserveLayer(p->sc,p->layer,false);
	    ss = p->sc->layers[p->layer].splines;
	    p->sc->layers[p->layer].splines = NULL;
	    SCRefToSplines(p->sc,r,p->layer);
	    for ( spl = p->sc->layers[p->layer].splines; spl!=NULL; spl=spl->next )
		SplineSetReverse(spl);
	    if ( p->sc->layers[p->layer].splines!=NULL ) {
		for ( spl = p->sc->layers[p->layer].splines; spl->next!=NULL; spl=spl->next );
		spl->next = ss;
	    } else
		p->sc->layers[p->layer].splines = ss;
	    SCCharChangedUpdate(p->sc,p->layer);
	} else
	    IError("Could not find reference");
return;
    } else if ( p->explaining==_("This glyph's advance width is different from the standard width") ) {
	SCSynchronizeWidth(p->sc,p->advancewidthval,p->sc->width,NULL);
return;
    } else if ( p->explaining==_("This glyph's vertical advance is different from the standard width") ) {
	p->sc->vwidth=p->vadvancewidthval;
return;
    }

    if ( p->explaining==_("This glyph is not mapped to any unicode code point, but its name should be.") ||
	    p->explaining==_("This glyph is mapped to a unicode code point which is different from its name.") ) {
	char buf[100]; const char *newname;
	SplineChar *foundsc;
	newname = StdGlyphName(buf,p->sc->unicodeenc,p->sc->parent->uni_interp,p->sc->parent->for_new_glyphs);
	foundsc = SFHashName(p->sc->parent,newname);
	if ( foundsc==NULL ) {
	    free(p->sc->name);
	    p->sc->name = copy(newname);
	} else {
	    ff_post_error(_("Can't fix"), _("The name FontForge would like to assign to this glyph, %.30s, is already used by a different glyph."),
		    newname );
	}
return;
    }

    sp = NULL;
    for ( spl=p->sc->layers[p->layer].splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( sp->selected )
	break;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( sp->selected )
    break;
    }
    if ( sp==NULL ) {
	IError("Nothing selected");
return;
    }

/* I do not handle:
    _("The two selected points are the endpoints of an open path")
    _("The paths that make up this glyph intersect one another")
    _("The selected points are too close to each other")
    _("The selected line segment is near the italic angle"), _("The control point above the selected point is near the italic angle"), _("The control point below the selected point is near the italic angle"), _("The control point right of the selected point is near the italic angle"), _("The control point left of the selected point is near the italic angle")
    _("The control point above the selected point is outside the spline segment"), _("The control point below the selected point is outside the spline segment"), _("The control point right of the selected point is outside the spline segment"), _("The control point left of the selected point is outside the spline segment")
    _("This hint does not control any points")
    _STR_ProbHint3*
    _STR_ProbMultUni, STR_ProbMultName
*/

    SCPreserveLayer(p->sc,p->layer,false);
    ncp_changed = pcp_changed = false;
    if ( p->explaining==_("The x coord of the selected point is near the specified value") || p->explaining==_("The selected point is near a vertical stem hint")) {
	sp->prevcp.x += p->expected-sp->me.x;
	sp->nextcp.x += p->expected-sp->me.x;
	sp->me.x = p->expected;
	ncp_changed = pcp_changed = true;
    } else if ( p->explaining==_("The selected point is not at integral coordinates") ||
	    p->explaining==_("The selected point does not have integral control points")) {
	sp->me.x = rint(sp->me.x); sp->me.y = rint(sp->me.y);
	sp->nextcp.x = rint(sp->nextcp.x); sp->nextcp.y = rint(sp->nextcp.y);
	sp->prevcp.x = rint(sp->prevcp.x); sp->prevcp.y = rint(sp->prevcp.y);
	ncp_changed = pcp_changed = true;
    } else if ( p->explaining==_("The y coord of the selected point is near the specified value") || p->explaining==_("The selected point is near a horizontal stem hint") ||
	    p->explaining==_("The y coord of the selected point is near the baseline") || p->explaining==_("The y coord of the selected point is near the xheight") ||
	    p->explaining==_("The y coord of the selected point is near the cap height") || p->explaining==_("The y coord of the selected point is near the ascender height") ||
	    p->explaining==_("The y coord of the selected point is near the descender height") ) {
	sp->prevcp.y += p->expected-sp->me.y;
	sp->nextcp.y += p->expected-sp->me.y;
	sp->me.y = p->expected;
	ncp_changed = pcp_changed = true;
    } else if ( p->explaining==_("The selected spline attains its extrema somewhere other than its endpoints") ) {
	SplineCharAddExtrema(p->sc,p->sc->layers[p->layer].splines,
		ae_between_selected,p->sc->parent->ascent+p->sc->parent->descent);
    } else if ( p->explaining==_("The selected line segment is nearly horizontal") ) {
	if ( sp->me.y!=p->found ) {
	    sp=sp->next->to;
	    if ( !sp->selected || sp->me.y!=p->found ) {
		IError("Couldn't find line");
return;
	    }
	}
	sp->prevcp.y += p->expected-sp->me.y;
	sp->nextcp.y += p->expected-sp->me.y;
	sp->me.y = p->expected;
	ncp_changed = pcp_changed = true;
    } else if ( p->explaining==_("The control point above the selected point is nearly horizontal") || p->explaining==_("The control point below the selected point is nearly horizontal") ||
	    p->explaining==_("The control point right of the selected point is nearly horizontal") || p->explaining==_("The control point left of the selected point is nearly horizontal") ) {
	BasePoint *tofix, *other;
	if ( sp->nextcp.y==p->found ) {
	    tofix = &sp->nextcp;
	    other = &sp->prevcp;
	} else {
	    tofix = &sp->prevcp;
	    other = &sp->nextcp;
	}
	if ( tofix->y!=p->found ) {
	    IError("Couldn't find control point");
return;
	}
	tofix->y = p->expected;
	ncp_changed = pcp_changed = true;
	if ( sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve )
	    other->y = p->expected;
	else {
	    sp->pointtype = pt_corner;
	    if ( other==&sp->nextcp )
		ncp_changed = false;
	    else
		pcp_changed = false;
	}
    } else if ( p->explaining==_("The selected line segment is nearly vertical") ) {
	if ( sp->me.x!=p->found ) {
	    sp=sp->next->to;
	    if ( !sp->selected || sp->me.x!=p->found ) {
		IError("Couldn't find line");
return;
	    }
	}
	sp->prevcp.x += p->expected-sp->me.x;
	sp->nextcp.x += p->expected-sp->me.x;
	sp->me.x = p->expected;
	ncp_changed = pcp_changed = true;
    } else if ( p->explaining==_("The control point above the selected point is nearly vertical") || p->explaining==_("The control point below the selected point is nearly vertical") ||
	    p->explaining==_("The control point right of the selected point is nearly vertical") || p->explaining==_("The control point left of the selected point is nearly vertical") ) {
	BasePoint *tofix, *other;
	if ( sp->nextcp.x==p->found ) {
	    tofix = &sp->nextcp;
	    other = &sp->prevcp;
	} else {
	    tofix = &sp->prevcp;
	    other = &sp->nextcp;
	}
	if ( tofix->x!=p->found ) {
	    IError("Couldn't find control point");
return;
	}
	tofix->x = p->expected;
	ncp_changed = pcp_changed = true;
	if ( sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve )
	    other->x = p->expected;
	else {
	    sp->pointtype = pt_corner;
	    if ( other==&sp->nextcp )
		ncp_changed = false;
	    else
		pcp_changed = false;
	}
    } else if ( p->explaining==_("This path should have been drawn in a counter-clockwise direction") || p->explaining==_("This path should have been drawn in a clockwise direction") ) {
	SplineSetReverse(spl);
    } else if ( p->explaining==_("This glyph contains control points which are probably too close to the main points to alter the look of the spline") ) {
	if ( sp->next!=NULL ) {
	    double len = sqrt((sp->me.x-sp->next->to->me.x)*(sp->me.x-sp->next->to->me.x) +
		    (sp->me.y-sp->next->to->me.y)*(sp->me.y-sp->next->to->me.y));
	    double cplen = sqrt((sp->me.x-sp->nextcp.x)*(sp->me.x-sp->nextcp.x) +
		    (sp->me.y-sp->nextcp.y)*(sp->me.y-sp->nextcp.y));
	    if ( cplen!=0 && cplen<p->irrelevantfactor*len ) {
		sp->nextcp = sp->me;
		ncp_changed = true;
	    }
	}
	if ( sp->prev!=NULL ) {
	    double len = sqrt((sp->me.x-sp->prev->from->me.x)*(sp->me.x-sp->prev->from->me.x) +
		    (sp->me.y-sp->prev->from->me.y)*(sp->me.y-sp->prev->from->me.y));
	    double cplen = sqrt((sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x) +
		    (sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y));
	    if ( cplen!=0 && cplen<p->irrelevantfactor*len ) {
		sp->prevcp = sp->me;
		pcp_changed = true;
	    }
	}
    } else
	IError("Did not fix: %d", p->explaining );
    if ( p->sc->layers[p->layer].order2 ) {
	if ( ncp_changed )
	    SplinePointNextCPChanged2(sp);
	if ( pcp_changed )
	    SplinePointPrevCPChanged2(sp);
    }
    if ( sp->next!=NULL )
	SplineRefigure(sp->next);
    if ( sp->prev!=NULL )
	SplineRefigure(sp->prev);
    SCCharChangedUpdate(p->sc,p->layer);
}

static int explain_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct problems *p = (struct problems *) GDrawGetUserData(gw);
	p->doneexplain = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_buttonactivate ) {
	struct problems *p = (struct problems *) GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==CID_Stop )
	    p->finish = true;
	else if ( GGadgetGetCid(event->u.control.g)==CID_Fix )
	    FixIt(p);
	p->doneexplain = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_radiochanged ) {
	struct problems *p = (struct problems *) GDrawGetUserData(gw);
	p->ignorethis = GGadgetIsChecked(event->u.control.g);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/problems.html", NULL);
return( true );
	}
return( false );
    }
return( true );
}

static void ExplainIt(struct problems *p, SplineChar *sc, char *explain,
	real found, real expected ) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[3], *varray[10], *barray[14];
    GTextInfo label[9];
    char buf[200];
    SplinePointList *spl; Spline *spline, *first;
    int fixable;

    if ( !p->explain || p->finish )
return;
    if ( p->explainw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = (enum window_attr_mask)(wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor);
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Problem explanation");
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,400));
	pos.height = GDrawPointsToPixels(NULL,86);
	p->explainw = GDrawCreateTopWindow(NULL,&pos,explain_e_h,p,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) explain;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 6; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 400-12;
	gcd[0].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
	gcd[0].creator = GLabelCreate;
	varray[0] = &gcd[0]; varray[1] = NULL;

	label[4].text = (unichar_t *) "";
	label[4].text_is_1byte = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 6; gcd[4].gd.pos.y = gcd[0].gd.pos.y+12; gcd[4].gd.pos.width = 400-12;
	gcd[4].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
	gcd[4].creator = GLabelCreate;
	varray[2] = &gcd[4]; varray[3] = NULL;

	label[5].text = (unichar_t *) _("Ignore this problem in the future");
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = 6; gcd[5].gd.pos.y = gcd[4].gd.pos.y+12;
	gcd[5].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
	gcd[5].creator = GCheckBoxCreate;
	varray[4] = &gcd[5]; varray[5] = NULL;

	gcd[1].gd.pos.x = 15-3; gcd[1].gd.pos.y = gcd[5].gd.pos.y+20;
	gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
	gcd[1].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_but_default);
	label[1].text = (unichar_t *) _("_Next");
	label[1].text_is_1byte = true;
	label[1].text_in_resource = true;
	gcd[1].gd.mnemonic = 'N';
	gcd[1].gd.label = &label[1];
	gcd[1].gd.cid = CID_Next;
	gcd[1].creator = GButtonCreate;
	barray[0] = GCD_Glue; barray[1] = &gcd[1]; barray[2] = GCD_Glue; barray[3] = GCD_Glue;

	gcd[6].gd.pos.x = 200-30; gcd[6].gd.pos.y = gcd[2].gd.pos.y;
	gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
	gcd[6].gd.flags = /*gg_visible |*/ gg_enabled;
	label[6].text = (unichar_t *) _("Fix");
	label[6].text_is_1byte = true;
	gcd[6].gd.mnemonic = 'F';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = CID_Fix;
	gcd[6].creator = GButtonCreate;
	barray[4] = GCD_Glue; barray[5] = GCD_Glue; barray[6] = &gcd[6]; barray[7] = GCD_Glue;

	gcd[2].gd.pos.x = -15; gcd[2].gd.pos.y = gcd[1].gd.pos.y+3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_but_cancel);
	label[2].text = (unichar_t *) _("_Stop");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = 'S';
	gcd[2].gd.cid = CID_Stop;
	gcd[2].creator = GButtonCreate;
	barray[8] = GCD_Glue; barray[9] = GCD_Glue; barray[10] = GCD_Glue;
	barray[11] = &gcd[2]; barray[12] = GCD_Glue;
	barray[13] = NULL;

	boxes[2].gd.flags = (enum gg_flags) (gg_enabled|gg_visible);
	boxes[2].gd.u.boxelements = barray;
	boxes[2].creator = GHBoxCreate;
	varray[6] = &boxes[2]; varray[7] = NULL; varray[8] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = (enum gg_flags) (gg_enabled|gg_visible);
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(p->explainw,boxes);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	p->explaintext = gcd[0].ret;
	p->explainvals = gcd[4].ret;
	p->ignoregadg = gcd[5].ret;
	p->topbox = boxes[0].ret;
    } else
	GGadgetSetTitle8(p->explaintext,explain);
    p->explaining = explain;
    fixable = /*explain==_("This glyph contains a horizontal hint near the specified width") || explain==_("This glyph contains a vertical hint near the specified width") ||*/
	    explain==_("This reference has been flipped, so the paths in it are drawn backwards") ||
	    explain==_("The x coord of the selected point is near the specified value") || explain==_("The selected point is near a vertical stem hint") ||
	    explain==_("The y coord of the selected point is near the specified value") || explain==_("The selected point is near a horizontal stem hint") ||
	    explain==_("This glyph contains control points which are probably too close to the main points to alter the look of the spline") ||
	    explain==_("The y coord of the selected point is near the baseline") || explain==_("The y coord of the selected point is near the xheight") ||
	    explain==_("The y coord of the selected point is near the cap height") || explain==_("The y coord of the selected point is near the ascender height") ||
	    explain==_("The y coord of the selected point is near the descender height") ||
	    explain==_("The selected line segment is nearly horizontal") || explain==_("The selected line segment is nearly vertical") ||
	    explain==_("The control point above the selected point is nearly horizontal") || explain==_("The control point below the selected point is nearly horizontal") ||
	    explain==_("The control point right of the selected point is nearly horizontal") || explain==_("The control point left of the selected point is nearly horizontal") ||
	    explain==_("The control point above the selected point is nearly vertical") || explain==_("The control point below the selected point is nearly vertical") ||
	    explain==_("The control point right of the selected point is nearly vertical") || explain==_("The control point left of the selected point is nearly vertical") ||
	    explain==_("This path should have been drawn in a counter-clockwise direction") || explain==_("This path should have been drawn in a clockwise direction") ||
	    explain==_("The selected spline attains its extrema somewhere other than its endpoints") ||
	    explain==_("This glyph's advance width is different from the standard width") ||
	    explain==_("This glyph's vertical advance is different from the standard width") ||
	    explain==_("This glyph is not mapped to any unicode code point, but its name should be.") ||
	    explain==_("The selected point is not at integral coordinates") ||
	    explain==_("The selected point does not have integral control points") ||
	    explain==_("This glyph is mapped to a unicode code point which is different from its name.");

    GGadgetSetVisible(GWidgetGetControl(p->explainw,CID_Fix),fixable);

    if ( explain==_("This glyph contains a substitution or ligature entry which refers to an empty char") ) {
	snprintf(buf,sizeof(buf),
		_("%2$.20s refers to an empty character \"%1$.20s\""), p->badsubsname,
		p->badsubs_lsubtable->subtable_name );
    } else if ( explain==_("This glyph contains anchor points from some, but not all anchor classes in a subtable") ) {
	snprintf(buf,sizeof(buf),
		_("There is no anchor for class %1$.30s in subtable %2$.30s"),
		p->missinganchor_class->name,
		p->missinganchor_class->subtable->subtable_name );
    } else if ( explain==_("Two glyphs share the same unicode code point.\nChange the encoding to \"Glyph Order\" and use\nEdit->Select->Wildcard with the following code point") ) {
	snprintf(buf,sizeof(buf), _("U+%04x"), sc->unicodeenc );
    } else if ( explain==_("Two glyphs have the same name.\nChange the encoding to \"Glyph Order\" and use\nEdit->Select->Wildcard with the following name") ) {
	snprintf(buf,sizeof(buf), _("%.40s"), sc->name );
    } else if ( found==expected )
	buf[0]='\0';
    else {
	sprintf(buf,_("Found %1$.4g, expected %2$.4g"), (double) found, (double) expected );
    }
    p->found = found; p->expected = expected;
    GGadgetSetTitle8(p->explainvals,buf);
    GGadgetSetChecked(p->ignoregadg,false);
    GHVBoxFitWindow(p->topbox);

    p->doneexplain = false;
    p->ignorethis = false;

    if ( sc!=p->lastcharopened || (CharView *) (sc->views)==NULL ) {
	if ( p->cvopened!=NULL && CVValid(p->fv->b.sf,p->lastcharopened,p->cvopened) )
	    GDrawDestroyWindow(p->cvopened->gw);
	p->cvopened = NULL;
	if ( (CharView *) (sc->views)!=NULL )
	    GDrawRaise(((CharView *) (sc->views))->gw);
	else
	    p->cvopened = CharViewCreate(sc,p->fv,-1);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	GDrawProcessPendingEvents(NULL);
	p->lastcharopened = sc;
    }
    if ( explain==_("This glyph contains a substitution or ligature entry which refers to an empty char") ) {
	SCCharInfo(sc,p->layer,p->fv->b.map,-1);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	GDrawProcessPendingEvents(NULL);
    }

    SCUpdateAll(sc);		/* We almost certainly just selected something */

    GDrawSetVisible(p->explainw,true);
    GDrawRaise(p->explainw);

    while ( !p->doneexplain )
	GDrawProcessOneEvent(NULL);
    /*GDrawSetVisible(p->explainw,false);*/		/* KDE gets unhappy about this and refuses to show the window later. I don't know why */

    if ( p->cv!=NULL ) {
	CVClearSel(p->cv);
    } else {
	for ( spl = p->sc->layers[p->layer].splines; spl!=NULL; spl = spl->next ) {
	    spl->first->selected = false;
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		spline->to->selected = false;
		if ( first==NULL ) first = spline;
	    }
	}
    }
}

static void _ExplainIt(struct problems *p, int enc, char *explain,
	real found, real expected ) {
    ExplainIt(p,p->sc=SFMakeChar(p->fv->b.sf,p->fv->b.map,enc),explain,found,expected);
}

/* if they deleted a point or a splineset while we were explaining then we */
/*  need to do some fix-ups. This routine detects a deletion and lets us know */
/*  that more processing is needed */
static int missing(struct problems *p,SplineSet *test, SplinePoint *sp) {
    SplinePointList *spl, *check;
    SplinePoint *tsp;

    if ( !p->explain )
return( false );

    if ( p->cv!=NULL )
	spl = p->cv->b.layerheads[p->cv->b.drawmode]->splines;
    else
	spl = p->sc->layers[p->layer].splines;
    for ( check = spl; check!=test && check!=NULL; check = check->next );
    if ( check==NULL )
return( true );		/* Deleted splineset */

    if ( sp!=NULL ) {
	for ( tsp=test->first; tsp!=sp ; ) {
	    if ( tsp->next==NULL )
return( true );
	    tsp = tsp->next->to;
	    if ( tsp==test->first )
return( true );
	}
    }
return( false );
}

static int missingspline(struct problems *p,SplineSet *test, Spline *spline) {
    SplinePointList *spl, *check;
    Spline *t, *first=NULL;

    if ( !p->explain )
return( false );

    if ( p->cv!=NULL )
	spl = p->cv->b.layerheads[p->cv->b.drawmode]->splines;
    else
	spl = p->sc->layers[p->layer].splines;
    for ( check = spl; check!=test && check!=NULL; check = check->next );
    if ( check==NULL )
return( true );		/* Deleted splineset */

    for ( t=test->first->next; t!=NULL && t!=first && t!=spline; t = t->to->next )
	if ( first==NULL ) first = t;
return( t!=spline );
}

static int missinghint(StemInfo *base, StemInfo *findme) {

    while ( base!=NULL && base!=findme )
	base = base->next;
return( base==NULL );
}

static int missingschint(StemInfo *findme, SplineChar *sc) {
    StemInfo *base;

    for ( base = sc->hstem; base!=NULL; base=base->next )
	if ( base==findme )
return( false );		/* Hasn't been deleted */
    for ( base = sc->vstem; base!=NULL; base=base->next )
	if ( base==findme )
return( false );

return( true );
}

static int HVITest(struct problems *p,BasePoint *to, BasePoint *from,
	Spline *spline, int hasia, real ia) {
    real yoff, xoff, angle;
    int ishor=false, isvert=false, isital=false;
    int isto;
    int type;
    BasePoint *base, *other;
    static const char *hmsgs[5] = {
	N_("The selected line segment is nearly horizontal"),
	N_("The control point above the selected point is nearly horizontal"),
	N_("The control point below the selected point is nearly horizontal"),
	N_("The control point right of the selected point is nearly horizontal"),
	N_("The control point left of the selected point is nearly horizontal")
    };
    static const char *vmsgs[5] = {
	N_("The selected line segment is nearly vertical"),
	N_("The control point above the selected point is nearly vertical"),
	N_("The control point below the selected point is nearly vertical"),
	N_("The control point right of the selected point is nearly vertical"),
	N_("The control point left of the selected point is nearly vertical")
    };
    static const char *imsgs[5] = {
	N_("The selected line segment is near the italic angle"),
	N_("The control point above the selected point is near the italic angle"),
	N_("The control point below the selected point is near the italic angle"),
	N_("The control point right of the selected point is near the italic angle"),
	N_("The control point left of the selected point is near the italic angle")
    };

    yoff = to->y-from->y;
    xoff = to->x-from->x;
    angle = atan2(yoff,xoff);
    if ( angle<0 )
	angle += FF_PI;
    if ( angle<.1 || angle>FF_PI-.1 ) {
	if ( yoff!=0 )
	    ishor = true;
    } else if ( angle>1.5707963-.1 && angle<1.5707963+.1 ) {
	if ( xoff!=0 )
	    isvert = true;
    } else if ( hasia && angle>ia-.1 && angle<ia+.1 ) {
	if ( angle<ia-.03 || angle>ia+.03 )
	    isital = true;
    }
    if ( ishor || isvert || isital ) {
	isto = false;
	if ( &spline->from->me==from || &spline->from->me==to )
	    spline->from->selected = true;
	if ( &spline->to->me==from || &spline->to->me==to )
	    spline->to->selected = isto = true;
	if ( from==&spline->from->me || from == &spline->to->me ) {
	    base = from; other = to;
	} else {
	    base = to; other = from;
	}
	if ( &spline->from->me==from && &spline->to->me==to ) {
	    type = 0;	/* Line */
	    if ( (ishor && xoff<0) || (isvert && yoff<0)) {
		base = from;
		other = to;
	    } else {
		base = to;
		other = from;
	    }
	} else if ( fabs(yoff)>fabs(xoff) )
	    type = ((yoff>0) ^ isto)?1:2;
	else
	    type = ((xoff>0) ^ isto)?3:4;
	if ( ishor )
	    ExplainIt(p,p->sc,_(hmsgs[type]), other->y,base->y);
	else if ( isvert )
	    ExplainIt(p,p->sc,_(vmsgs[type]), other->x,base->x);
	else
	    ExplainIt(p,p->sc,_(imsgs[type]),0,0);
return( true );
    }
return( false );
}

/* Is the control point outside of the spline segment when projected onto the */
/*  vector between the end points of the spline segment? */
static int OddCPCheck(BasePoint *cp,BasePoint *base,BasePoint *v,
	SplinePoint *sp, struct problems *p) {
    real len = (cp->x-base->x)*v->x+ (cp->y-base->y)*v->y;
    real xoff, yoff;
    char *msg=NULL;

    if ( len<0 || len>1 || (len==0 && &sp->me!=base) || (len==1 && &sp->me==base)) {
	xoff = cp->x-sp->me.x; yoff = cp->y-sp->me.y;
	if ( fabs(yoff)>fabs(xoff) )
	    msg = yoff>0?_("The control point above the selected point is outside the spline segment"):_("The control point below the selected point is outside the spline segment");
	else
	    msg = xoff>0?_("The control point right of the selected point is outside the spline segment"):_("The control point left of the selected point is outside the spline segment");
	sp->selected = true;
	ExplainIt(p,p->sc,msg, 0,0);
return( true );
    }
return( false );
}

static int Hint3Check(struct problems *p,StemInfo *h) {
    StemInfo *h2, *h3;

    /* Must be three hints to be interesting */
    if ( h==NULL || (h2=h->next)==NULL || (h3=h2->next)==NULL )
return(false);
    if ( h3->next!=NULL ) {
	StemInfo *bad, *goods[3];
	if ( h3->next->next!=NULL )	/* Don't try to find a subset with 5 */
return(false);
	if ( h->width==h2->width || h->width==h3->width ) {
	    goods[0] = h;
	    if ( h->width==h2->width ) {
		goods[1] = h2;
		if ( h->width==h3->width && h->width!=h3->next->width ) {
		    goods[2] = h3;
		    bad = h3->next;
		} else if ( h->width!=h3->width && h->width==h3->next->width ) {
		    goods[2] = h3->next;
		    bad = h3;
		} else
return(false);
	    } else if ( h->width==h3->width && h->width==h3->next->width ) {
		goods[1] = h3;
		goods[2] = h3->next;
		bad = h2;
	    } else
return(false);
	} else if ( h2->width == h3->width && h2->width==h3->next->width ) {
	    bad = h;
	    goods[0] = h2; goods[1] = h3; goods[2] = h3->next;
	} else
return(false);
	if ( goods[2]->start-goods[1]->start == goods[1]->start-goods[0]->start ) {
	    bad->active = true;
	    ExplainIt(p,p->sc,_("This glyph has four hints, but if this one were omitted it would fit a stem3 hint"),0,0);
	    if ( !missinghint(p->sc->hstem,bad) || !missinghint(p->sc->vstem,bad))
		bad->active = false;
	    if ( p->ignorethis )
		p->stem3 = false;
return( true );
	}
return(false);
    }

    if ( h->width==h2->width && h->width==h3->width &&
	    h2->start-h->start == h3->start-h2->start ) {
	if ( p->showexactstem3 ) {
	    ExplainIt(p,p->sc,_("This glyph can use a stem3 hint"),0,0);
	    if ( p->ignorethis )
		p->showexactstem3 = false;
	}
return( false );		/* It IS a stem3, so don't complain */
    }

    if ( h->width==h2->width && h->width==h3->width ) {
	if ( h2->start-h->start+p->near > h3->start-h2->start &&
		h2->start-h->start-p->near < h3->start-h2->start ) {
	    ExplainIt(p,p->sc,_("The counters between these hints are not the same size, bad for a stem3 hint"),0,0);
	    if ( p->ignorethis )
		p->stem3 = false;
return( true );
	}
return( false );
    }

    if ( (h2->start-h->start+p->near > h3->start-h2->start &&
	     h2->start-h->start-p->near < h3->start-h2->start ) ||
	    (h2->start-h->start-h->width+p->near > h3->start-h2->start-h2->width &&
	     h2->start-h->start-h->width-p->near < h3->start-h2->start-h2->width )) {
	if ( h->width==h2->width ) {
	    if ( h->width+p->near > h3->width && h->width-p->near < h3->width ) {
		h3->active = true;
		ExplainIt(p,p->sc,_("This hint has the wrong width for a stem3 hint"),0,0);
		if ( !missinghint(p->sc->hstem,h3) || !missinghint(p->sc->vstem,h3))
		    h3->active = false;
		if ( p->ignorethis )
		    p->stem3 = false;
return( true );
	    } else
return( false );
	}
	if ( h->width==h3->width ) {
	    if ( h->width+p->near > h2->width && h->width-p->near < h2->width ) {
		h2->active = true;
		ExplainIt(p,p->sc,_("This hint has the wrong width for a stem3 hint"),0,0);
		if ( !missinghint(p->sc->hstem,h2) || !missinghint(p->sc->vstem,h2))
		    h2->active = false;
		if ( p->ignorethis )
		    p->stem3 = false;
return( true );
	    } else
return( false );
	}
	if ( h2->width==h3->width ) {
	    if ( h2->width+p->near > h->width && h2->width-p->near < h->width ) {
		h->active = true;
		ExplainIt(p,p->sc,_("This hint has the wrong width for a stem3 hint"),0,0);
		if ( !missinghint(p->sc->hstem,h) || !missinghint(p->sc->vstem,h))
		    h->active = false;
		if ( p->ignorethis )
		    p->stem3 = false;
return( true );
	    } else
return( false );
	}
    }
return( false );
}

static int probRefDepth(RefChar *r,int layer) {
    RefChar *ref;
    int cur, max=0;

    for ( ref= r->sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	cur = probRefDepth(ref,layer);
	if ( cur>max ) max = cur;
    }
return( max+1 );
}

static int SCRefDepth(SplineChar *sc,int layer) {
    RefChar *ref;
    int cur, max=0;

    for ( ref= sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	cur = probRefDepth(ref,layer);
	if ( cur>max ) max = cur;
    }
return( max );
}

static int SPLPointCnt(SplinePointList *spl) {
    SplinePoint *sp;
    int cnt=0;

    for ( ; spl!=NULL; spl = spl->next ) {
	for ( sp = spl->first; ; ) {
	    ++cnt;
	    if ( sp->prev!=NULL && !sp->prev->knownlinear ) {
		if ( sp->prev->order2 )
		    ++cnt;
		else
		    cnt += 2;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
return( cnt );
}

static RefChar *FindRefOfSplineInLayer(Layer *layer,Spline *spline) {
    RefChar *r;
    SplineSet *ss;
    Spline *s, *first;

    for ( r=layer->refs; r!=NULL; r=r->next ) {
	for ( ss=r->layers[0].splines; ss!=NULL; ss=ss->next ) {
	    first = NULL;
	    for ( s=ss->first->next ; s!=NULL && s!=first; s=s->to->next ) {
		if ( first==NULL ) first = s;
		if ( s==spline )
return( r );
	    }
	}
    }
return( NULL );
}

static int SCProblems(CharView *cv,SplineChar *sc,struct problems *p) {
    SplineSet *spl, *test;
    Spline *spline, *first;
    Layer *cur;
    SplinePoint *sp, *nsp;
    int needsupdate=false, changed=false;
    StemInfo *h;
    RefChar *r1, *r2;
    int uni;
    DBounds bb;

  restart:
    if ( cv!=NULL ) {
	needsupdate = CVClearSel(cv);
	cur = cv->b.layerheads[cv->b.drawmode];
	spl = cur->splines;
	sc = cv->b.sc;
    } else {
	for ( spl = sc->layers[p->layer].splines; spl!=NULL; spl = spl->next ) {
	    if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( spline->to->selected )
		    { needsupdate = true; spline->to->selected = false; }
		if ( first==NULL ) first = spline;
	    }
	}
	cur = &sc->layers[p->layer];
	spl = cur->splines;
    }
    p->sc = sc;
    if (( p->ptnearhint || p->hintwidthnearval || p->hintwithnopt ) &&
	    sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,p->layer,NULL);

    if ( p->openpaths ) {
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
	    /* I'm also including in "open paths" the special case of a */
	    /*  singleton point with connects to itself */
	    if ( test->first!=NULL && ( test->first->prev==NULL ||
		    ( test->first->prev == test->first->next &&
			test->first->noprevcp && test->first->nonextcp))) {
		changed = true;
		test->first->selected = test->last->selected = true;
		ExplainIt(p,sc,_("The two selected points are the endpoints of an open path"),0,0);
		if ( p->ignorethis ) {
		    p->openpaths = false;
	break;
		}
		if ( missing(p,test,NULL))
      goto restart;
	    }
	}
    }

    if ( p->intersectingpaths && !p->finish ) {
	Spline *s, *s2;
	int found;
	spl = LayerAllSplines(cur);
	found = SplineSetIntersect(spl,&s,&s2);
	spl = LayerUnAllSplines(cur);
	if ( found ) {
	    changed = true;
	    if ( (r1 = FindRefOfSplineInLayer(cur,s))!=NULL )
		r1->selected = true;
	    else {
		s->from->selected = true; s->to->selected = true;
	    }
	    if ( (r2 = FindRefOfSplineInLayer(cur,s2))!=NULL )
		r2->selected = true;
	    else {
		s2->from->selected = true; s2->to->selected = true;
	    }
	    ExplainIt(p,sc,_("The paths that make up this glyph intersect one another"),0,0);
	    if ( p->ignorethis ) {
		p->intersectingpaths = false;
    /* break; */
	    }
	}
    }

    if ( p->nonintegral && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->nonintegral; test=test->next ) {
	    sp = test->first;
	    do {
		int interp = SPInterpolate(sp);
		int badme = interp
			? (rint(2*sp->me.x)!=2*sp->me.x || rint(2*sp->me.y)!=2*sp->me.y)
			: (rint(sp->me.x)!=sp->me.x || rint(sp->me.y)!=sp->me.y);
		if ( badme ||
			rint(sp->nextcp.x)!=sp->nextcp.x || rint(sp->nextcp.y)!=sp->nextcp.y ||
			rint(sp->prevcp.x)!=sp->prevcp.x || rint(sp->prevcp.y)!=sp->prevcp.y ) {
		    changed = true;
		    sp->selected = true;
		    if ( badme )
			ExplainIt(p,sc,_("The selected point is not at integral coordinates"),0,0);
		    else
			ExplainIt(p,sc,_("The selected point does not have integral control points"),0,0);
		    if ( p->ignorethis ) {
			p->nonintegral = false;
	    break;
		    }
		    if ( missing(p,test,nsp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->nonintegral )
	break;
	}
    }

    if ( p->pointstoofar && !p->finish ) {
	SplinePoint *lastsp=NULL;
	BasePoint lastpt;

	memset(&lastpt,0,sizeof(lastpt));
	for ( test=spl; test!=NULL && !p->finish && p->pointstoofar; test=test->next ) {
	    sp = test->first;
	    do {
		if ( BPTooFar(&lastpt,&sp->prevcp) ||
			BPTooFar(&sp->prevcp,&sp->me) ||
			BPTooFar(&sp->me,&sp->nextcp)) {
		    changed = true;
		    sp->selected = true;
		    if ( lastsp==NULL ) {
			ExplainIt(p,sc,_("The selected point is too far from the origin"),0,0);
		    } else {
			lastsp->selected = true;
			ExplainIt(p,sc,_("The selected points (or the intermediate control points) are too far apart"),0,0);
		    }
		    if ( p->ignorethis ) {
			p->pointstoofar = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		memcpy(&lastpt,&sp->nextcp,sizeof(lastpt));
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==test->first ) {
		    memcpy(&lastpt,&sp->me,sizeof(lastpt));
	    break;
		}
	    } while ( !p->finish );
	    if ( !p->pointstoofar )
	break;
	}
    }

    if ( p->pointstooclose && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->pointstooclose; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->next==NULL )
	    break;
		nsp = sp->next->to;
		if ( (nsp->me.x-sp->me.x)*(nsp->me.x-sp->me.x) + (nsp->me.y-sp->me.y)*(nsp->me.y-sp->me.y) < 2*2 ) {
		    changed = true;
		    sp->selected = nsp->selected = true;
		    ExplainIt(p,sc,_("The selected points are too close to each other"),0,0);
		    if ( p->ignorethis ) {
			p->pointstooclose = false;
	    break;
		    }
		    if ( missing(p,test,nsp))
  goto restart;
		}
		sp = nsp;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->pointstooclose )
	break;
	}
    }

    if ( p->xnearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->xnearval; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.x-p->xval<p->near && p->xval-sp->me.x<p->near &&
			sp->me.x!=p->xval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_("The x coord of the selected point is near the specified value"),sp->me.x,p->xval);
		    if ( p->ignorethis ) {
			p->xnearval = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->xnearval )
	break;
	}
    }

    if ( p->ynearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->ynearval; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.y-p->yval<p->near && p->yval-sp->me.y<p->near &&
			sp->me.y != p->yval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_("The y coord of the selected point is near the specified value"),sp->me.y,p->yval);
		    if ( p->ignorethis ) {
			p->ynearval = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ynearval )
	break;
	}
    }

    if ( p->ynearstd && !p->finish ) {
	real expected;
	char *msg;
	for ( test=spl; test!=NULL && !p->finish && p->ynearstd; test=test->next ) {
	    sp = test->first;
	    do {
		if (( sp->me.y-p->xheight<p->near && p->xheight-sp->me.y<p->near && sp->me.y!=p->xheight ) ||
			( sp->me.y-p->caph<p->near && p->caph-sp->me.y<p->near && sp->me.y!=p->caph && sp->me.y!=p->ascent ) ||
			( sp->me.y-p->ascent<p->near && p->ascent-sp->me.y<p->near && sp->me.y!=p->caph && sp->me.y!=p->ascent ) ||
			( sp->me.y-p->descent<p->near && p->descent-sp->me.y<p->near && sp->me.y!=p->descent ) ||
			( sp->me.y<p->near && -sp->me.y<p->near && sp->me.y!=0 ) ) {
		    changed = true;
		    sp->selected = true;
		    if ( sp->me.y<p->near && -sp->me.y<p->near ) {
			msg = _("The y coord of the selected point is near the baseline");
			expected = 0;
		    } else if ( sp->me.y-p->xheight<p->near && p->xheight-sp->me.y<p->near ) {
			msg = _("The y coord of the selected point is near the xheight");
			expected = p->xheight;
		    } else if ( sp->me.y-p->caph<p->near && p->caph-sp->me.y<p->near ) {
			msg = _("The y coord of the selected point is near the cap height");
			expected = p->caph;
		    } else if ( sp->me.y-p->ascent<p->near && p->ascent-sp->me.y<p->near ) {
			msg = _("The y coord of the selected point is near the ascender height");
			expected = p->ascent;
		    } else {
			msg = _("The y coord of the selected point is near the descender height");
			expected = p->descent;
		    }
		    ExplainIt(p,sc,msg,sp->me.y,expected);
		    if ( p->ignorethis ) {
			p->ynearstd = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ynearstd )
	break;
	}
    }

    if ( p->linenearstd && !p->finish ) {
	real ia = (90-p->fv->b.sf->italicangle)*(FF_PI/180);
	int hasia = p->fv->b.sf->italicangle!=0;
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( spline->knownlinear ) {
		    if ( HVITest(p,&spline->to->me,&spline->from->me,spline,
			    hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->linenearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->linenearstd )
	break;
	}
    }

    if ( p->cpnearstd && !p->finish ) {
	real ia = (90-p->fv->b.sf->italicangle)*(FF_PI/180);
	int hasia = p->fv->b.sf->italicangle!=0;
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( !spline->knownlinear ) {
		    if ( !spline->from->nonextcp &&
			    HVITest(p,&spline->from->nextcp,&spline->from->me,spline,
				hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpnearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		    if ( !spline->to->noprevcp &&
			    HVITest(p,&spline->to->me,&spline->to->prevcp,spline,
				hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpnearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->cpnearstd )
	break;
	}
    }

    if ( p->cpodd && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( !spline->knownlinear ) {
		    BasePoint v; real len;
		    v.x = spline->to->me.x-spline->from->me.x;
		    v.y = spline->to->me.y-spline->from->me.y;
		    len = /*sqrt*/(v.x*v.x+v.y*v.y);
		    v.x /= len; v.y /= len;
		    if ( !spline->from->nonextcp &&
			    OddCPCheck(&spline->from->nextcp,&spline->from->me,&v,
			     spline->from,p)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpodd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		    if ( !spline->to->noprevcp &&
			    OddCPCheck(&spline->to->prevcp,&spline->from->me,&v,
			     spline->to,p)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpodd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->cpodd )
	break;
	}
    }

    if ( p->irrelevantcontrolpoints && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->irrelevantcontrolpoints; test = test->next ) {
	    for ( sp=test->first; !p->finish && p->irrelevantcontrolpoints; ) {
		int either = false;
		if ( sp->prev!=NULL ) {
		    double len = sqrt((sp->me.x-sp->prev->from->me.x)*(sp->me.x-sp->prev->from->me.x) +
			    (sp->me.y-sp->prev->from->me.y)*(sp->me.y-sp->prev->from->me.y));
		    double cplen = sqrt((sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x) +
			    (sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y));
		    if ( cplen!=0 && cplen<p->irrelevantfactor*len )
			either = true;
		}
		if ( sp->next!=NULL ) {
		    double len = sqrt((sp->me.x-sp->next->to->me.x)*(sp->me.x-sp->next->to->me.x) +
			    (sp->me.y-sp->next->to->me.y)*(sp->me.y-sp->next->to->me.y));
		    double cplen = sqrt((sp->me.x-sp->nextcp.x)*(sp->me.x-sp->nextcp.x) +
			    (sp->me.y-sp->nextcp.y)*(sp->me.y-sp->nextcp.y));
		    if ( cplen!=0 && cplen<p->irrelevantfactor*len )
			either = true;
		}
		if ( either ) {
		    sp->selected = true;
		    ExplainIt(p,sc,_("This glyph contains control points which are probably too close to the main points to alter the look of the spline"),0,0);
		    if ( p->ignorethis ) {
			p->irrelevantcontrolpoints = false;
	    break;
		    }
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==test->first )
	    break;
	    }
	}
    }

    if ( p->hintwithnopt && !p->finish ) {
	int anys, anye;
      restarthhint:
	for ( h=sc->hstem; h!=NULL ; h=h->next ) {
	    anys = anye = false;
	    for ( test=spl; test!=NULL && !p->finish && (!anys || !anye); test=test->next ) {
		sp = test->first;
		do {
		    if (sp->me.y==h->start )
			anys = true;
		    if (sp->me.y==h->start+h->width )
			anye = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		} while ( sp!=test->first && !p->finish );
	    }
	    if ( h->ghost && ( anys || anye ))
		/* Ghost hints only define one edge */;
	    else if ( !anys || !anye ) {
		h->active = true;
		changed = true;
		ExplainIt(p,sc,_("This hint does not control any points"),0,0);
		if ( !missinghint(sc->hstem,h))
		    h->active = false;
		if ( p->ignorethis ) {
		    p->hintwithnopt = false;
	break;
		}
		if ( missinghint(sc->hstem,h))
      goto restarthhint;
	    }
	}
      restartvhint:
	for ( h=sc->vstem; h!=NULL && p->hintwithnopt && !p->finish; h=h->next ) {
	    anys = anye = false;
	    for ( test=spl; test!=NULL && !p->finish && (!anys || !anye); test=test->next ) {
		sp = test->first;
		do {
		    if (sp->me.x==h->start )
			anys = true;
		    if (sp->me.x==h->start+h->width )
			anye = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		} while ( sp!=test->first && !p->finish );
	    }
	    if ( !anys || !anye ) {
		h->active = true;
		changed = true;
		ExplainIt(p,sc,_("This hint does not control any points"),0,0);
		if ( p->ignorethis ) {
		    p->hintwithnopt = false;
	break;
		}
		if ( missinghint(sc->vstem,h))
      goto restartvhint;
		h->active = false;
	    }
	}
    }

    if ( p->ptnearhint && !p->finish ) {
	real found, expected;
	h = NULL;
	for ( test=spl; test!=NULL && !p->finish && p->ptnearhint; test=test->next ) {
	    sp = test->first;
	    do {
		int hs = false, vs = false;
		for ( h=sc->hstem; h!=NULL; h=h->next ) {
		    if (( sp->me.y-h->start<p->near && h->start-sp->me.y<p->near &&
				sp->me.y!=h->start ) ||
			    ( sp->me.y-h->start+h->width<p->near && h->start+h->width-sp->me.y<p->near &&
				sp->me.y!=h->start+h->width )) {
			found = sp->me.y;
			if ( sp->me.y-h->start<p->near && h->start-sp->me.y<p->near )
			    expected = h->start;
			else
			    expected = h->start+h->width;
			h->active = true;
			hs = true;
		break;
		    }
		}
		if ( !hs ) {
		    for ( h=sc->vstem; h!=NULL; h=h->next ) {
			if (( sp->me.x-h->start<p->near && h->start-sp->me.x<p->near &&
				    sp->me.x!=h->start ) ||
				( sp->me.x-h->start+h->width<p->near && h->start+h->width-sp->me.x<p->near &&
				    sp->me.x!=h->start+h->width )) {
			    found = sp->me.x;
			    if ( sp->me.x-h->start<p->near && h->start-sp->me.x<p->near )
				expected = h->start;
			    else
				expected = h->start+h->width;
			    h->active = true;
			    vs = true;
		    break;
			}
		    }
		}
		if ( hs || vs ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,hs?_("The selected point is near a horizontal stem hint"):_("The selected point is near a vertical stem hint"),found,expected);
		    if ( h!=NULL )
			h->active = false;
		    if ( p->ignorethis ) {
			p->ptnearhint = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ptnearhint )
	break;
	}
    }

    if ( p->overlappedhints && !p->finish && !cur->order2 && spl!=NULL ) {
	int anyhm=0;
	for ( test=spl; test!=NULL && !p->finish && p->overlappedhints; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->hintmask!=NULL ) {
		    anyhm = true;
		    h = SCHintOverlapInMask(sc,sp->hintmask);
		    if ( h!=NULL ) {
			sp->selected = true;
			h->active = true;
			changed = true;
			ExplainIt(p,sc,_("The hint mask of the selected point contains overlapping hints"),0,0);
			if ( p->ignorethis )
			    p->overlappedhints = false;
			if ( missing(p,test,sp))
  goto restart;
			if ( missingschint(h,sc))
  goto restart;
			h->active = false;
			sp->selected = false;
			if ( !p->overlappedhints )
	    break;
		    }
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->overlappedhints )
	break;
	}
	if ( p->overlappedhints && !anyhm ) {
	    h = SCHintOverlapInMask(sc,NULL);
	    if ( h!=NULL ) {
		h->active = true;
		changed = true;
		ExplainIt(p,sc,_("There are no hint masks in this layer but there are overlapping hints."),0,0);
		if ( missingschint(h,sc))
  goto restart;
		h->active = false;
	    }
	}
    }

    if ( p->hintwidthnearval && !p->finish ) {
	StemInfo *hs = NULL, *vs = NULL;
	for ( h=sc->hstem; h!=NULL; h=h->next ) {
	    if ( h->width-p->widthval<p->near && p->widthval-h->width<p->near &&
		    h->width!=p->widthval ) {
		h->active = true;
		hs = h;
	break;
	    }
	}
	for ( h=sc->vstem; h!=NULL; h=h->next ) {
	    if ( h->width-p->widthval<p->near && p->widthval-h->width<p->near &&
		    h->width!=p->widthval ) {
		h->active = true;
		vs = h;
	break;
	    }
	}
	if ( hs || vs ) {
	    changed = true;
	    ExplainIt(p,sc,hs?_("This glyph contains a horizontal hint near the specified width"):_("This glyph contains a vertical hint near the specified width"),
		    hs?hs->width:vs->width,p->widthval);
	    if ( hs!=NULL && !missinghint(sc->hstem,hs)) hs->active = false;
	    if ( vs!=NULL && !missinghint(sc->vstem,vs)) vs->active = false;
	    if ( p->ignorethis )
		p->hintwidthnearval = false;
	    else if ( (hs!=NULL && missinghint(sc->hstem,hs)) &&
		    ( vs!=NULL && missinghint(sc->vstem,vs)))
      goto restart;
	}
    }

    if ( p->stem3 && !p->finish )
	changed |= Hint3Check(p,sc->hstem);
    if ( p->stem3 && !p->finish )
	changed |= Hint3Check(p,sc->vstem);

    if ( p->direction && !p->finish ) {
	SplineSet **base, *ret, *ss;
	Spline *s, *s2;
	Layer *layer;
	int lastscan= -1;
	int self_intersects, dir;

	if ( cv!=NULL )
	    layer = cv->b.layerheads[cv->b.drawmode];
	else
	    layer = &sc->layers[p->layer];

	ss = LayerAllSplines(layer);
	self_intersects = SplineSetIntersect(ss,&s,&s2);
	LayerUnAllSplines(layer);

	if ( self_intersects )
	    ff_post_error(_("This glyph self-intersects"),_("This glyph self-intersects. Checking for correct direction is meaningless until that is fixed"));
	else {
	    base = &layer->splines;

	    while ( !p->finish && (ret=SplineSetsDetectDir(base,&lastscan))!=NULL ) {
		sp = ret->first;
		changed = true;
		while ( 1 ) {
		    sp->selected = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==ret->first )
		break;
		}
		dir = SplinePointListIsClockwise(ret);
		if ( dir==-1 )
		    ExplainIt(p,sc,_("This path probably intersects itself (though I could not find that when\n I checked for intersections), look closely at the corners"),0,0);
		else if ( dir==1 )
		    ExplainIt(p,sc,_("This path should have been drawn in a counter-clockwise direction"),0,0);
		else
		    ExplainIt(p,sc,_("This path should have been drawn in a clockwise direction"),0,0);
		if ( p->ignorethis ) {
		    p->direction = false;
	    break;
		}
	    }
	}
    }

    if ( p->missingextrema && !p->finish ) {
	SplineSet *ss;
	Spline *s, *first;
	double len2, bound2 = p->sc->parent->extrema_bound;
	double x,y;
	extended extrema[4];

	if ( bound2<=0 )
	    bound2 = (p->sc->parent->ascent + p->sc->parent->descent)/32.0;

	bound2 *= bound2;
      ae_restart:
	for ( ss = sc->layers[p->layer].splines; ss!=NULL && !p->finish; ss=ss->next ) {
	  ae2_restart:
	    first = NULL;
	    for ( s=ss->first->next ; s!=NULL && s!=first && !p->finish; s=s->to->next ) {
		if ( first==NULL )
		    first = s;
		if ( s->acceptableextrema )
	    continue;		/* If marked as good, don't check it */
		/* rough approximation to spline's length */
		x = (s->to->me.x-s->from->me.x);
		y = (s->to->me.y-s->from->me.y);
		len2 = x*x + y*y;
		/* short splines (serifs) need not to have points at their extrema */
		/*  But how do we define "short"? */
		if ( len2>bound2 && Spline2DFindExtrema(s,extrema)>0 ) {
		    s->from->selected = true;
		    s->to->selected = true;
		    ExplainIt(p,sc,_("The selected spline attains its extrema somewhere other than its endpoints"),0,0);
		    if ( !SSExistsInLayer(ss,sc->layers[p->layer].splines) )
      goto ae_restart;
		    if ( !SplineExistsInSS(s,ss))
	  goto ae2_restart;
		    if ( !SplineExistsInSS(first,ss))
			first = s;
		    if ( p->ignorethis ) {
			p->missingextrema = false;
	    break;
		    }
		}
	    }
	    if ( !p->missingextrema )
	break;
	}
    }

    if ( p->flippedrefs && !p->finish && ( cv==NULL || cv->b.drawmode==dm_fore )) {
	RefChar *ref;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	for ( ref = sc->layers[p->layer].refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->transform[0]*ref->transform[3]<0 ||
		    (ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_("This reference has been flipped, so the paths in it are drawn backwards"),0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->flippedrefs = false;
	break;
		}
	    }
	}
    }

    if ( p->refsbadtransformttf && !p->finish ) {
	RefChar *ref;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	for ( ref = sc->layers[p->layer].refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->transform[0]>=2 || ref->transform[0]<-2 ||
		    ref->transform[1]>=2 || ref->transform[1]<-2 ||
		    ref->transform[2]>=2 || ref->transform[2]<-2 ||
		    ref->transform[3]>=2 || ref->transform[3]<-2 ||
		    rint(ref->transform[4])!=ref->transform[4] ||
		    rint(ref->transform[5])!=ref->transform[5]) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_("This reference has a transformation matrix which cannot be expressed in truetype.\nAll entries (except translation) must be between [-2.0,2.0).\nTranslation must be integral."),0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->refsbadtransformttf = false;
	break;
		}
	    }
	}
    }

    if ( p->mixedcontoursrefs && !p->finish ) {
	RefChar *ref;
	int hasref=0, hascontour = sc->layers[p->layer].splines!=NULL;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next ) {
	    ref->selected = false;
	    if ( ref->transform[0]>=2 || ref->transform[0]<-2 ||
		    ref->transform[1]>=2 || ref->transform[1]<-2 ||
		    ref->transform[2]>=2 || ref->transform[2]<-2 ||
		    ref->transform[3]>=2 || ref->transform[3]<-2 )
		hascontour = true;
	    else
		hasref = true;
	    if ( hascontour && hasref ) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_("This glyph contains both contours and references.\n(or contains a reference which has a bad transformation matrix and counts as a contour).\nThis cannot be expressed in the TrueType glyph format."),0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->mixedcontoursrefs = false;
	break;
		}
	    }
	}
    }

    if ( p->refsbadtransformps && !p->finish ) {
	RefChar *ref;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	for ( ref = sc->layers[p->layer].refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->transform[0]!=1.0 ||
		    ref->transform[1]!=0 ||
		    ref->transform[2]!=0 ||
		    ref->transform[3]!= 1.0 ) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_("This reference has a transformation matrix which cannot be expressed in Type1/2 fonts.\nNo scaling or rotation allowed."),0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->refsbadtransformps = false;
	break;
		}
	    }
	}
    }

    if ( p->multusemymetrics && !p->finish ) {
	RefChar *ref, *found;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	found = NULL;
	for ( ref = sc->layers[p->layer].refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->use_my_metrics ) {
		if ( found==NULL )
		    found = ref;
		else {
		    changed = true;
		    ref->selected = true;
		    found->selected = true;
		    ExplainIt(p,sc,_("Both selected references have use-my-metrics set"),0,0);
		    ref->selected = false;
		    found->selected = false;
		    if ( p->ignorethis ) {
			p->multusemymetrics = false;
	break;
		    }
		}
	    }
	}
    }

    if ( p->ptmatchrefsoutofdate && !p->finish ) {
	RefChar *ref;
	for ( ref = sc->layers[p->layer].refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	for ( ref = sc->layers[p->layer].refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->point_match_out_of_date ) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_("This reference uses point-matching but it refers to a glyph\n(or a previous reference refers to a glyph)\nwhose points have been renumbered."),0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->ptmatchrefsoutofdate = false;
	break;
		}
	    }
	}
    }

    if ( p->toodeeprefs && !p->finish ) {
	int cnt=SCRefDepth(sc,p->layer);
	if ( cnt>p->refdepthmax ) {
	    changed = true;
	    ExplainIt(p,sc,_("References are nested more deeply in this glyph than the maximum allowed"),cnt,p->refdepthmax);
	    if ( p->ignorethis )
		p->toodeeprefs = false;
	}
    }

    if ( p->toomanypoints && !p->finish ) {
	int cnt=0;
	RefChar *r;
	cnt = SPLPointCnt(sc->layers[p->layer].splines);
	for ( r=sc->layers[p->layer].refs; r!=NULL ; r=r->next )
	    cnt += SPLPointCnt(r->layers[0].splines);
	if ( cnt>p->pointsmax ) {
	    changed = true;
	    ExplainIt(p,sc,_("There are more points in this glyph than the maximum allowed"),cnt,p->pointsmax);
	    if ( p->ignorethis )
		p->toomanypoints = false;
	}
    }

    if ( p->toomanyhints && !p->finish ) {
	int cnt=0;
	for ( h=sc->hstem; h!=NULL; h=h->next )
	    ++cnt;
	for ( h=sc->vstem; h!=NULL; h=h->next )
	    ++cnt;
	if ( cnt>p->hintsmax ) {
	    changed = true;
	    ExplainIt(p,sc,_("There are more hints in this glyph than the maximum allowed"),cnt,p->hintsmax);
	    if ( p->ignorethis )
		p->toomanyhints = false;
	}
    }

    if ( p->bitmaps && !p->finish && SCWorthOutputting(sc)) {
	BDFFont *bdf;

	for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sc->orig_pos>=bdf->glyphcnt || bdf->glyphs[sc->orig_pos]==NULL ) {
		changed = true;
		ExplainIt(p,sc,_("This outline glyph is missing a bitmap version"),0,0);
		if ( p->ignorethis )
		    p->bitmaps = false;
	break;
	    }
	}
    }

    if ( p->bitmapwidths && !p->finish && SCWorthOutputting(sc)) {
	BDFFont *bdf;
	double em = (sc->parent->ascent+sc->parent->descent);

	for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sc->orig_pos<bdf->glyphcnt && bdf->glyphs[sc->orig_pos]!=NULL ) {
		BDFChar *bc = bdf->glyphs[sc->orig_pos];
		if ( bc->width!= (int) rint( (sc->width*bdf->pixelsize)/em ) ) {
		    changed = true;
		    ExplainIt(p,sc,_("This outline glyph's advance width is different from that of the bitmap's"),
			    bc->width,rint( (sc->width*bdf->pixelsize)/em ));
		    if ( p->ignorethis )
			p->bitmapwidths = false;
	break;
		}
	    }
	}
    }

    if ( p->advancewidth && !p->finish && SCWorthOutputting(sc)) {
	if ( sc->width!=p->advancewidthval ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph's advance width is different from the standard width"),sc->width,p->advancewidthval);
	    if ( p->ignorethis )
		p->advancewidth = false;
	}
    }

    if ( p->vadvancewidth && !p->finish && SCWorthOutputting(sc)) {
	if ( sc->vwidth!=p->vadvancewidthval ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph's vertical advance is different from the standard width"),sc->vwidth,p->vadvancewidthval);
	    if ( p->ignorethis )
		p->vadvancewidth = false;
	}
    }

    if ( (p->bbymax || p->bbxmax || p->bbymin || p->bbxmin) && !p->finish &&
	    SCWorthOutputting(sc)) {
	SplineCharFindBounds(sc,&bb);
	if ( p->bbymax && bb.maxy > p->bbymax_val ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph is taller than desired"),bb.maxy,p->bbymax_val);
	    if ( p->ignorethis )
		p->bbymax = false;
	}
	if ( p->bbymin && bb.miny < p->bbymin_val ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph extends further below the baseline than desired"),bb.miny,p->bbymin_val);
	    if ( p->ignorethis )
		p->bbymin = false;
	}
	if ( p->bbxmax && bb.maxx > p->bbxmax_val ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph is wider than desired"),bb.maxx,p->bbxmax_val);
	    if ( p->ignorethis )
		p->bbxmax = false;
	}
	if ( p->bbxmin && bb.minx < p->bbxmin_val ) {
	    changed = true;
	    ExplainIt(p,sc,_("This glyph extends left further than desired"),bb.minx,p->bbxmin_val);
	    if ( p->ignorethis )
		p->bbxmin = false;
	}
    }

    if ( p->badsubs && !p->finish ) {
	PST *pst;
	char *pt, *end; int ch;
	for ( pst = sc->possub ; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_substitution || pst->type==pst_alternate ||
		    pst->type==pst_multiple || pst->type==pst_ligature ) {
		for ( pt=pst->u.subs.variant; *pt!='\0' ; pt=end ) {
		    end = strchr(pt,' ');
		    if ( end==NULL ) end=pt+strlen(pt);
		    ch = *end;
		    *end = '\0';
		    if ( !SCWorthOutputting(SFGetChar(sc->parent,-1,pt)) ) {
			changed = true;
			p->badsubsname = copy(pt);
			*end = ch;
			p->badsubs_lsubtable = pst->subtable;
			ExplainIt(p,sc,_("This glyph contains a substitution or ligature entry which refers to an empty char"),0,0);
			free(p->badsubsname);
			if ( p->ignorethis )
			    p->badsubs = false;
		    } else
			*end = ch;
		    while ( *end==' ' ) ++end;
		    if ( !p->badsubs )
		break;
		}
		if ( !p->badsubs )
	    break;
	    }
	}
    }

    if ( p->missinganchor && !p->finish ) {
	for (;;) {
	    p->missinganchor_class = SCValidateAnchors(sc);
	    if ( p->missinganchor_class == NULL )
	break;
	    ExplainIt(p,sc,_("This glyph contains anchor points from some, but not all anchor classes in a subtable"),0,0);
	    if ( p->ignorethis )
		p->missinganchor = false;
	break;
	}
    }

    if ( p->multuni && !p->finish && sc->unicodeenc!=-1 ) {
	SplineFont *sf = sc->parent;
	int i;
	for ( i=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && sf->glyphs[i]!=sc ) {
	    if ( sf->glyphs[i]->unicodeenc == sc->unicodeenc ) {
		changed = true;
		p->glyphname = sf->glyphs[i]->name;
		ExplainIt(p,sc,_("Two glyphs share the same unicode code point.\nChange the encoding to \"Glyph Order\" and use\nEdit->Select->Wildcard with the following code point"),0,0);
		if ( p->ignorethis )
		    p->multuni = false;
	    }
	}
    }

    if ( p->multname && !p->finish ) {
	SplineFont *sf = sc->parent;
	int i;
	for ( i=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && sf->glyphs[i]!=sc ) {
	    if ( strcmp(sf->glyphs[i]->name, sc->name)==0 ) {
		changed = true;
		p->glyphenc = i;
		ExplainIt(p,sc,_("Two glyphs have the same name.\nChange the encoding to \"Glyph Order\" and use\nEdit->Select->Wildcard with the following name"),0,0);
		if ( p->ignorethis )
		    p->multname = false;
	    }
	}
    }

    if ( p->uninamemismatch && !p->finish &&
		strcmp(sc->name,".notdef")!=0 &&
		strcmp(sc->name,".null")!=0 &&
		strcmp(sc->name,"nonmarkingreturn")!=0 &&
		(uni = UniFromName(sc->name,sc->parent->uni_interp,p->fv->b.map->enc))!= -1 &&
		sc->unicodeenc != uni ) {
	changed = true;
	p->glyphenc = sc->orig_pos;
	if ( sc->unicodeenc==-1 )
	    ExplainIt(p,sc,_("This glyph is not mapped to any unicode code point, but its name should be."),0,0);
	else if ( strcmp(sc->name,"alefmaksurainitialarabic")==0 ||
                  strcmp(sc->name,"alefmaksuramedialarabic")==0 )
	    ExplainIt(p,sc,_("The use of names 'alefmaksurainitialarabic' and 'alefmaksuramedialarabic' is discouraged."),0,0);
	else
	    ExplainIt(p,sc,_("This glyph is mapped to a unicode code point which is different from its name."),0,0);
	if ( p->ignorethis )
	    p->uninamemismatch = false;
    }


    if ( needsupdate || changed )
	SCUpdateAll(sc);
return( changed );
}

static int CIDCheck(struct problems *p,int cid) {
    int found = false;

    if ( (p->cidmultiple || p->cidblank) && !p->finish ) {
	SplineFont *csf = p->fv->b.cidmaster;
	int i, cnt;
	for ( i=cnt=0; i<csf->subfontcnt; ++i )
	    if ( cid<csf->subfonts[i]->glyphcnt &&
		    SCWorthOutputting(csf->subfonts[i]->glyphs[cid]) )
		++cnt;
	if ( cnt>1 && p->cidmultiple ) {
	    _ExplainIt(p,cid,_("This glyph is defined in more than one of the CID subfonts"),cnt,1);
	    if ( p->ignorethis )
		p->cidmultiple = false;
	    found = true;
	} else if ( cnt==0 && p->cidblank ) {
	    _ExplainIt(p,cid,_("This glyph is not defined in any of the CID subfonts"),0,0);
	    if ( p->ignorethis )
		p->cidblank = false;
	    found = true;
	}
    }
return( found );
}

static char *missinglookup(struct problems *p,char *str) {
    int i;

    for ( i=0; i<p->rpl_cnt; ++i )
	if ( strcmp(str,p->mg[i].search)==0 )
return( p->mg[i].rpl );

return( NULL );
}

static void mgreplace(char **base, char *str,char *end, const char *new_str, SplineChar *sc, PST *pst) {
    PST *p, *ps;

    if ( new_str==NULL || *new_str=='\0' ) {
	if ( *base==str && *end=='\0' && sc!=NULL ) {
	    /* We deleted the last name from the pst, it is meaningless, remove it */
	    if ( sc->possub==pst )
		sc->possub = pst->next;
	    else {
		for ( p = sc->possub, ps=p->next; ps!=NULL && ps!=pst; p=ps, ps=ps->next );
		if ( ps!=NULL )
		    p->next = pst->next;
	    }
	    pst->next = NULL;
	    PSTFree(pst);
	} else if ( *end=='\0' )
	    *str = '\0';
	else
	    strcpy(str,end+1);	/* Skip the space */
    } else {
	char *res = (char *) malloc(strlen(*base)+strlen(new_str)-(end-str)+1);
	strncpy(res,*base,str-*base);
	strcpy(res+(str-*base),new_str);
	strcat(res,end);
	free(*base);
	*base = res;
    }
}

static void ClearMissingState(struct problems *p) {
    int i;

    if ( p->mg!=NULL ) {
	for ( i=0; i<p->rpl_cnt; ++i ) {
	    free(p->mg[i].search);
	    free(p->mg[i].rpl);
	}
	free(p->mg);
    } else
	free(p->mlt);
    p->mlt = NULL;
    p->mg = NULL;
    p->rpl_cnt = p->rpl_max = 0;
}

enum missingglyph_type { mg_pst, mg_fpst, mg_kern, mg_vkern, mg_asm };
struct mgask_data {
    GWindow gw;
    uint8_t done, skipped;
    uint32_t tag;
    char **_str, *start, *end;
    SplineChar *sc;
    PST *pst;
    struct problems *p;
};

static void mark_to_replace(struct problems *p,struct mgask_data *d, const char *rpl) {
    int ch;

    if ( p->rpl_cnt >= p->rpl_max ) {
	if ( p->rpl_max == 0 )
	    p->mg = (struct mgrpl *) malloc((p->rpl_max = 30)*sizeof(struct mgrpl));
	else
	    p->mg = (struct mgrpl *) realloc(p->mg,(p->rpl_max += 30)*sizeof(struct mgrpl));
    }
    ch = *d->end; *d->end = '\0';
    p->mg[p->rpl_cnt].search = copy( d->start );
    p->mg[p->rpl_cnt++].rpl = copy( rpl );
    *d->end = ch;
}

#define CID_Always	1001
#define CID_RplText	1002
#define CID_Ignore	1003
#define CID_Rpl		1004
#define CID_Skip	1005
#define CID_Delete	1006

static int MGA_RplChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct mgask_data *d = (struct mgask_data *) GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *rpl = _GGadgetGetTitle(g);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Rpl),*rpl!=0);
    }
return( true );
}

static int MGA_Rpl(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mgask_data *d = (struct mgask_data *) GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *_rpl = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_RplText));
	char *rpl = cu_copy(_rpl);
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_Always)))
	    mark_to_replace(d->p,d,rpl);
	mgreplace(d->_str,d->start,d->end,rpl,d->sc,d->pst);
	free(rpl);
	d->done = true;
    }
return( true );
}

static int MGA_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mgask_data *d = (struct mgask_data *) GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_Always)))
	    mark_to_replace(d->p,d,"");
	mgreplace(d->_str,d->start,d->end,"",d->sc,d->pst);
	d->done = true;
    }
return( true );
}

static int MGA_Skip(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mgask_data *d = (struct mgask_data *) GDrawGetUserData(GGadgetGetWindow(g));
	d->done = d->skipped = true;
    }
return( true );
}

static int mgask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mgask_data *d = (struct mgask_data *) GDrawGetUserData(gw);
	d->done = d->skipped = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/problems.html", NULL);
return( true );
	}
return( false );
    }
return( true );
}

static int mgAsk(struct problems *p,char **_str,char *str, char *end,uint32_t tag,
	SplineChar *sc,enum missingglyph_type which,void *data) {
    char buffer[200];
    static const char *pstnames[] = { "", N_("position"), N_("pair"), N_("substitution"),
	N_("alternate subs"), N_("multiple subs"), N_("ligature"), NULL };
    static const char *fpstnames[] = { N_("Contextual position"), N_("Contextual substitution"),
	N_("Chaining position"), N_("Chaining substitution"), N_("Reverse chaining subs"), NULL };
    static const char *asmnames[] = { N_("Indic reordering"), N_("Contextual substitution"),
	N_("Lig"), NULL, N_("Simple"), N_("Contextual insertion"), NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	N_("Kerning"), NULL };
    PST *pst = (PST *) data;
    FPST *fpst = (FPST *) data;
    ASM *sm = (ASM *) data;
    KernClass *kc = (KernClass *) data;
    char end_ch;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    struct mgask_data d;
    int blen = GIntGetResource(_NUM_Buttonsize), ptwidth;
    int k;

    end_ch = *end; *end = '\0';

    if ( which == mg_pst ) {
	snprintf(buffer,sizeof(buffer),
		_("Glyph %1$.50s with a %2$s from lookup subtable %3$.50s"),
		sc->name, _(pstnames[pst->type]),
		pst->subtable->subtable_name );
    } else if ( which == mg_fpst )
	snprintf(buffer,sizeof(buffer),
		_("%1$s from lookup subtable %2$.50s"),
		_(fpstnames[fpst->type-pst_contextpos]),
		fpst->subtable->subtable_name );
    else if ( which == mg_asm )
	snprintf(buffer,sizeof(buffer),
		_("%1$s from lookup subtable %2$.50s"),
		_(asmnames[sm->type]),
		sm->subtable->subtable_name );
    else
	snprintf(buffer,sizeof(buffer),
		_("%1$s from lookup subtable %2$.50s"),
		which==mg_kern ? _("Kerning Class"): _("Vertical Kerning Class"),
		kc->subtable->subtable_name );

    memset(&d,'\0',sizeof(d));
    d._str = _str;
    d.start = str;
    d.end = end;
    d.sc = sc;
    d.pst = which==mg_pst ? (PST *) data : NULL;
    d.p = p;
    d.tag = tag;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = (enum window_attr_mask)(wam_events|wam_cursor|wam_utf8_wtitle|wam_centered|wam_restrict|wam_isdlg);
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = 1;
    wattrs.restrict_input_to_me = 1;
    wattrs.centered = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Check for missing glyph names");
    pos.x = pos.y = 0;
    ptwidth = 3*blen+GGadgetScale(80);
    pos.width =GDrawPointsToPixels(NULL,ptwidth);
    pos.height = GDrawPointsToPixels(NULL,180);
    d.gw = gw = GDrawCreateTopWindow(NULL,&pos,mgask_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k=0;
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _(" refers to a missing glyph");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) str;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Replace With:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13; gcd[k].gd.pos.width = ptwidth-20;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k].gd.cid = CID_RplText;
    gcd[k].gd.handle_controlevent = MGA_RplChange;
    gcd[k++].creator = GTextFieldCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    label[k].text = (unichar_t *) _("Always");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_Always;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) _("Ignore this problem in the future");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+20;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k].gd.cid = CID_Ignore;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30 -3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_but_default);
    label[k].text = (unichar_t *) _("Replace");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = MGA_Rpl;
    gcd[k].gd.cid = CID_Rpl;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 10+blen+(ptwidth-3*blen-GGadgetScale(20))/2;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    label[k].text = (unichar_t *) _("Remove");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = MGA_Delete;
    gcd[k].gd.cid = CID_Delete;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_but_cancel);
    label[k].text = (unichar_t *) _("Skip");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = MGA_Skip;
    gcd[k].gd.cid = CID_Skip;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_pos_in_pixels);
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    *end = end_ch;
    GDrawSetVisible(gw,true);

    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Ignore)))
	p->missingglyph = false;
    GDrawDestroyWindow(gw);
return( !d.skipped );
}

static int StrMissingGlyph(struct problems *p,char **_str,SplineChar *sc,enum missingglyph_type which, void *data) {
    char *end, ch, *str = *_str, *new_str;
    int off;
    int found = false;
    SplineFont *sf = p->fv!=NULL ? p->fv->b.sf : p->cv!=NULL ? p->cv->b.sc->parent : p->msc->parent;
    SplineChar *ssc;
    int changed=false;

    if ( str==NULL )
return( false );

    while ( *str ) {
	if ( p->finish || !p->missingglyph )
    break;
	while ( *str==' ' ) ++str;
	for ( end=str; *end!='\0' && *end!=' '; ++end );
	ch = *end; *end='\0';
	if ( strcmp(str,MAC_DELETED_GLYPH_NAME)==0 )
	    ssc = (SplineChar *) 1;
	else
	    ssc = SFGetChar(sf,-1,str);
	*end = ch;
	if ( ssc==NULL ) {
	    off = end-*_str;
	    if ( (new_str = missinglookup(p,str))!=NULL ) {
		mgreplace(_str, str,end, new_str, sc, which==mg_pst ? (PST*)data : NULL);
		changed = true;
		off += (strlen(new_str)-(end-str));
	    } else {
		if ( mgAsk(p,_str,str,end,0,sc,which,data)) {
		    changed = true;
		    off = 0;
		}
		found = true;
	    }
	    if ( changed ) {
		PST *test;
		if ( which==mg_pst ) {
		    for ( test = sc->possub; test!=NULL && test!=data; test=test->next );
		    if ( test==NULL )		/* Entire pst was removed */
return( true );
		    *_str = test->u.subs.variant;
		}
		end = *_str+off;
	    }
	}
	str = end;
    }
return( found );
}

static int SCMissingGlyph(struct problems *p,SplineChar *sc) {
    PST *pst, *next;
    int found = false;

    if ( !p->missingglyph || p->finish || sc==NULL )
return( false );

    for ( pst=sc->possub; pst!=NULL; pst=next ) {
	next = pst->next;
	switch ( pst->type ) {
	  case pst_pair:
	    found |= StrMissingGlyph(p,&pst->u.pair.paired,sc,mg_pst,pst);
	  break;
	  case pst_substitution:
	  case pst_alternate:
	  case pst_multiple:
	  case pst_ligature:
	    found |= StrMissingGlyph(p,&pst->u.subs.variant,sc,mg_pst,pst);
	  break;
	}
    }
return( found );
}

static int KCMissingGlyph(struct problems *p,KernClass *kc,int isv) {
    int i;
    int found = false;
    enum missingglyph_type which = isv ? mg_vkern : mg_kern;

    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL )
	found |= StrMissingGlyph(p,&kc->firsts[i],NULL,which,kc);
    for ( i=1; i<kc->second_cnt; ++i )
	found |= StrMissingGlyph(p,&kc->seconds[i],NULL,which,kc);
return( found );
}

static int FPSTMissingGlyph(struct problems *p,FPST *fpst) {
    int i,j;
    int found = false;

    switch ( fpst->format ) {
      case pst_glyphs:
	for ( i=0; i<fpst->rule_cnt; ++i )
	    for ( j=0; j<3; ++j )
		found |= StrMissingGlyph(p,&(&fpst->rules[i].u.glyph.names)[j],
			NULL,mg_fpst,fpst);
      break;
      case pst_class:
	for ( i=1; i<3; ++i )
	    for ( j=0; j<(&fpst->nccnt)[i]; ++j )
		found |= StrMissingGlyph(p,&(&fpst->nclass)[i][j],NULL,mg_fpst,fpst);
      break;
      case pst_reversecoverage:
	found |= StrMissingGlyph(p,&fpst->rules[0].u.rcoverage.replacements,NULL,mg_fpst,fpst);
	/* fall through */;
      case pst_coverage:
	for ( i=1; i<3; ++i )
	    for ( j=0; j<(&fpst->rules[0].u.coverage.ncnt)[i]; ++j )
		found |= StrMissingGlyph(p,&(&fpst->rules[0].u.coverage.ncovers)[i][j],NULL,mg_fpst,fpst);
      break;
    }
return( found );
}

static int ASMMissingGlyph(struct problems *p,ASM *sm) {
    int j;
    int found = false;

    for ( j=4; j<sm->class_cnt; ++j )
	found |= StrMissingGlyph(p,&sm->classes[j],NULL,mg_asm,sm);
return( found );
}

static int LookupFeaturesMissScript(struct problems *p,OTLookup *otl,OTLookup *nested,
	uint32_t script, SplineFont *sf, char *glyph_name) {
    OTLookup *invokers, *any;
    struct lookup_subtable *subs;
    int i,l, ret;
    int found = false;
    FeatureScriptLangList *fsl;
    struct scriptlanglist *sl;
    char buffer[400];
    const char *buts[4];

    if ( script==DEFAULT_SCRIPT )
return( false );

    if ( otl->features == NULL ) {
	/* No features invoke us, so presume that we are to be invoked by a */
	/*  contextual lookup, and check its scripts rather than ours */
	if ( nested!=NULL ) {
	    /* There is no need to have a nested contextual lookup */
	    /*  so we don't support them */
return(false);
	}
	any = NULL;
	for ( invokers=otl->lookup_type>=gpos_start?sf->gpos_lookups:sf->gsub_lookups;
		invokers!=NULL ; invokers = invokers->next ) {
	    for ( subs=invokers->subtables; subs!=NULL; subs=subs->next ) {
		if ( subs->fpst!=NULL ) {
		    FPST *fpst = subs->fpst;
		    for ( i=0; i<fpst->rule_cnt; ++i ) {
			struct fpst_rule *r = &fpst->rules[i];
			for ( l=0; l<r->lookup_cnt; ++l )
			    if ( r->lookups[l].lookup == otl ) {
				found |= LookupFeaturesMissScript(p,invokers,otl,script,sf,glyph_name);
			        any = invokers;
			    }
		    }
		}
	    }
	}
	if ( any==NULL ) {
	    /* No opentype contextual lookup uses this lookup with no features*/
	    /*  so it appears totally useless. But a mac feature might I guess*/
	    /*  so don't complain */
	}
    } else {
	for ( fsl = otl->features; fsl!=NULL; fsl=fsl->next ) {
	    for ( sl=fsl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script==script )
	    break;
	    }
	    if ( sl!=NULL )
	break;
	}
	if ( fsl==NULL ) {
	    buffer[0]='\0';
	    if ( nested!=NULL )
		snprintf(buffer,sizeof(buffer),
		  _("The lookup %.30s which invokes lookup %.30s is active "
		    "for glyph %.30s which has script '%c%c%c%c', yet this script does not "
		    "appear in any of the features which apply the lookup.\n"
		    "Would you like to add this script to one of those features?"),
			otl->lookup_name, nested->lookup_name,
			glyph_name,
			script>>24, script>>16, script>>8, script);
	    else
		snprintf(buffer,sizeof(buffer),
		  _("The lookup %.30s is active for glyph %.30s which has script "
		    "'%c%c%c%c', yet this script does not appear in any of the features which "
		    "apply the lookup.\n\n"
		    "Would you like to add this script to one of those features?"),
			otl->lookup_name, glyph_name,
			script>>24, script>>16, script>>8, script);
	    buts[0] = _("_OK"); buts[1] = _("_Skip"); buts[2]="_Ignore"; buts[3] = NULL;
	    ret = ff_ask(_("Missing Script"),(const char **) buts,0,1,buffer);
	    if ( ret==0 ) {
		sl = (struct scriptlanglist*)chunkalloc(sizeof(struct scriptlanglist));
		sl->script = script;
		sl->lang_cnt = 1;
		sl->langs[0] = DEFAULT_LANG;
		sl->next = otl->features->scripts;
		otl->features->scripts = sl;
		sf->changed = true;
	    } else if ( ret==2 )
		p->missingscriptinfeature = false;
return( true );
	}
    }
return( found );
}

static int SCMissingScriptFeat(struct problems *p,SplineFont *sf,SplineChar *sc) {
    PST *pst;
    int found = false;
    uint32_t script;
    AnchorPoint *ap;

    if ( !p->missingscriptinfeature || p->finish || sc==NULL )
return( false );
    script = SCScriptFromUnicode(sc);

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->subtable!=NULL )
	found |= LookupFeaturesMissScript(p,pst->subtable->lookup,NULL,script,sf,sc->name);
    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->anchor->subtable!=NULL )
	found |= LookupFeaturesMissScript(p,ap->anchor->subtable->lookup,NULL,script,sf,sc->name);

return( found );
}

static int StrMissingScript(struct problems *p,SplineFont *sf,OTLookup *otl,char *class_name) {
    char *pt, *start;
    int ch;
    SplineChar *sc;
    uint32_t script;
    int found = 0;

    if ( class_name==NULL )
return( false );

    for ( pt=class_name; *pt && p->missingscriptinfeature; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( start=pt; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt='\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	if ( sc!=NULL ) {
	    script = SCScriptFromUnicode(sc);
	    found |= LookupFeaturesMissScript(p,otl,NULL,script,sf,sc->name);
	}
    }
return( found );
}

static int KCMissingScriptFeat(struct problems *p,SplineFont *sf, KernClass *kc,int isv) {
    int i;
    int found = false;
    OTLookup *otl = kc->subtable->lookup;

    for ( i=0; i<kc->first_cnt; ++i )
	found |= StrMissingScript(p,sf,otl,kc->firsts[i]);
return( found );
}

static int FPSTMissingScriptFeat(struct problems *p,SplineFont *sf,FPST *fpst) {
    int i,j;
    int found = false;
    OTLookup *otl = fpst->subtable->lookup;

    switch ( fpst->format ) {
      case pst_glyphs:
	for ( i=0; i<fpst->rule_cnt; ++i )
	    for ( j=0; j<3; ++j )
		found |= StrMissingScript(p,sf,otl,(&fpst->rules[i].u.glyph.names)[j]);
      break;
      case pst_class:
	for ( i=1; i<3; ++i )
	    for ( j=0; j<(&fpst->nccnt)[i]; ++j )
		found |= StrMissingScript(p,sf,otl,(&fpst->nclass)[i][j]);
      break;
      case pst_reversecoverage:
		found |= StrMissingScript(p,sf,otl,fpst->rules[0].u.rcoverage.replacements);
	/* fall through */;
      case pst_coverage:
	for ( i=1; i<3; ++i )
	    for ( j=0; j<(&fpst->rules[0].u.coverage.ncnt)[i]; ++j )
		found |= StrMissingScript(p,sf,otl,(&fpst->rules[0].u.coverage.ncovers)[i][j]);
      break;
    }
return( found );
}

static int CheckForATT(struct problems *p) {
    int found = false;
    int i,k;
    FPST *fpst;
    ASM *sm;
    KernClass *kc;
    SplineFont *_sf, *sf;

    _sf = p->fv->b.sf;
    if ( _sf->cidmaster ) _sf = _sf->cidmaster;

    if ( p->missingglyph && !p->finish ) {
	if ( p->cv!=NULL )
	    found = SCMissingGlyph(p,p->cv->b.sc);
	else if ( p->msc!=NULL )
	    found = SCMissingGlyph(p,p->msc);
	else {
	    k=0;
	    do {
		if ( _sf->subfonts==NULL ) sf = _sf;
		else sf = _sf->subfonts[k++];
		for ( i=0; i<sf->glyphcnt && !p->finish; ++i ) if ( sf->glyphs[i]!=NULL )
		    found |= SCMissingGlyph(p,sf->glyphs[i]);
	    } while ( k<_sf->subfontcnt && !p->finish );
	    for ( kc=_sf->kerns; kc!=NULL && !p->finish; kc=kc->next )
		found |= KCMissingGlyph(p,kc,false);
	    for ( kc=_sf->vkerns; kc!=NULL && !p->finish; kc=kc->next )
		found |= KCMissingGlyph(p,kc,true);
	    for ( fpst=_sf->possub; fpst!=NULL && !p->finish && p->missingglyph; fpst=fpst->next )
		found |= FPSTMissingGlyph(p,fpst);
	    for ( sm=_sf->sm; sm!=NULL && !p->finish && p->missingglyph; sm=sm->next )
		found |= ASMMissingGlyph(p,sm);
	}
	ClearMissingState(p);
    }

    if ( p->missingscriptinfeature && !p->finish ) {
	if ( p->cv!=NULL )
	    found = SCMissingScriptFeat(p,_sf,p->cv->b.sc);
	else if ( p->msc!=NULL )
	    found = SCMissingScriptFeat(p,_sf,p->msc);
	else {
	    k=0;
	    do {
		if ( _sf->subfonts==NULL ) sf = _sf;
		else sf = _sf->subfonts[k++];
		for ( i=0; i<sf->glyphcnt && !p->finish; ++i ) if ( sf->glyphs[i]!=NULL )
		    found |= SCMissingScriptFeat(p,_sf,sf->glyphs[i]);
	    } while ( k<_sf->subfontcnt && !p->finish );
	    for ( kc=_sf->kerns; kc!=NULL && !p->finish; kc=kc->next )
		found |= KCMissingScriptFeat(p,_sf,kc,false);
	    for ( kc=_sf->vkerns; kc!=NULL && !p->finish; kc=kc->next )
		found |= KCMissingScriptFeat(p,_sf,kc,true);
	    for ( fpst=_sf->possub; fpst!=NULL && !p->finish && p->missingglyph; fpst=fpst->next )
		found |= FPSTMissingScriptFeat(p,_sf,fpst);
	    /* Apple's state machines don't have the concept of "script" */
	    /*  for their feature/settings */
	}
    }

return( found );
}

static void DoProbs(struct problems *p) {
    int i, ret=false, gid;
    SplineChar *sc;
    BDFFont *bdf;

    ret = CheckForATT(p);
    if ( p->cv!=NULL ) {
	ret |= SCProblems(p->cv,NULL,p);
	ret |= CIDCheck(p,p->cv->b.sc->orig_pos);
    } else if ( p->msc!=NULL ) {
	ret |= SCProblems(NULL,p->msc,p);
	ret |= CIDCheck(p,p->msc->orig_pos);
    } else {
	for ( i=0; i<p->fv->b.map->enccount && !p->finish; ++i )
	    if ( p->fv->b.selected[i] ) {
		sc = NULL;
		if ( (gid=p->fv->b.map->map[i])!=-1 && (sc = p->fv->b.sf->glyphs[gid])!=NULL ) {
		    if ( SCProblems(NULL,sc,p)) {
			if ( sc!=p->lastcharopened ) {
			    if ( (CharView *) (sc->views)!=NULL )
				GDrawRaise(((CharView *) (sc->views))->gw);
			    else
				CharViewCreate(sc,p->fv,-1);
			    p->lastcharopened = sc;
			}
			ret = true;
		    }
		}
		if ( !p->finish && p->bitmaps && !SCWorthOutputting(sc)) {
		    for ( bdf=p->fv->b.sf->bitmaps; bdf!=NULL; bdf=bdf->next )
			if ( i<bdf->glyphcnt && bdf->glyphs[i]!=NULL ) {
			    sc = SFMakeChar(p->fv->b.sf,p->fv->b.map,i);
			    ExplainIt(p,sc,_("This blank outline glyph has an unexpected bitmap version"),0,0);
			    ret = true;
			}
		}
		ret |= CIDCheck(p,i);
	    }
    }
    if ( !ret )
	ff_post_error(_("No problems found"),_("No problems found"));
}

static void FigureStandardHeights(struct problems *p) {
    BlueData bd;

    QuickBlues(p->fv->b.sf,p->layer,&bd);
    p->xheight = bd.xheight;
    p->caph = bd.caph;
    p->ascent = bd.ascent;
    p->descent = bd.descent;
}

static void show_selected_problems(struct problems *p) {
	p->explain = true;
	if ( doynearstd )
	    FigureStandardHeights(p);
	if ( openpaths || intersectingpaths || pointstooclose  || doxnear || doynear ||
		doynearstd || linestd || hintnopt || ptnearhint || hintwidth ||
		direction || p->cidmultiple || p->cidblank || p->flippedrefs ||
		p->bitmaps || p->advancewidth || p->vadvancewidth || p->stem3 ||
		p->bitmapwidths || p->missinganchor ||
		p->irrelevantcontrolpoints || p->badsubs || p->missingglyph ||
		p->missingscriptinfeature || nonintegral || pointstoofar ||
		p->toomanypoints || p->toomanyhints || p->missingextrema ||
		p->toodeeprefs || multuni || multname || uninamemismatch ||
		p->ptmatchrefsoutofdate || p->refsbadtransformttf ||
		p->multusemymetrics || p->overlappedhints ||
		p->mixedcontoursrefs || p->refsbadtransformps ||
		p->bbymax || p->bbxmax || p->bbymin || p->bbxmin ) {
	    DoProbs(p);
	}
}

static void DummyFindProblems(CharView *cv) {
    struct problems p;

    memset(&p,0,sizeof(p));
    p.fv = (FontView *) (cv->b.fv);
    p.cv=cv;
    p.layer = CVLayer((CharViewBase *) cv);
    p.map = cv->b.fv->map;
    p.lastcharopened = cv->b.sc;

    p.openpaths = true;
    p.intersectingpaths = true;
    p.direction = true;
    p.flippedrefs = true;
    p.missingextrema = true;
    p.toomanypoints = true;
    p.toomanyhints = true;
    p.pointstoofar = true;
    p.nonintegral = true;
    p.missinganchor = true;
    p.overlappedhints = true;

    p.pointsmax = 1500;
    p.hintsmax = 96;

    p.explain = true;

    DoProbs(&p);
    if ( p.explainw!=NULL )
	GDrawDestroyWindow(p.explainw);
}

static std::vector<ProblemRecord> pr_points = {
    {CID_NonIntegral, N_("Non-_integral coordinates"),
     N_("The coordinates of all points and control points in truetype "
        "must be integers (if they are not integers then FontForge will "
        "round them when it outputs them, potentially causing havoc). "
        "Even in PostScript fonts it is generally a good idea to use "
        "integral values."),
     false},
    {CID_XNear, N_("_X near¹"),
     N_("Allows you to check that vertical stems in several characters start "
        "at the same location."),
     false, 0.0},
    {CID_YNear, N_("_Y near¹"),
     N_("Allows you to check that horizontal stems in several characters "
        "start at the same location."),
     false, 0.0},
    {CID_YNearStd, N_("Y near¹ _standard heights"),
     N_("Allows you to find points which are slightly off from the baseline, "
        "xheight, cap height, ascender, descender heights."),
     false},
    {CID_CpStd, N_("_Control points near horizontal/vertical"),
     N_("Allows you to find control points which are almost, but not quite "
        "horizontal or vertical from their base point (or at the italic "
        "angle)."),
     false},
    {CID_CpOdd, N_("Control points _beyond spline"),
     N_("Allows you to find control points which when projected onto the line "
        "segment between the two end points lie outside of those end points"),
     false},
    {CID_IrrelevantCP,
     N_("Check for _irrelevant control points with factor (%)"),
     N_("Control points are irrelevant if they are too close to the main point "
        "to make a significant difference in the shape of the curve. The "
        "minimum relevant distance is computed as a factor of distance between "
        "the main points."),
     false, 0.5},
    {CID_PointsTooClose, N_("Poin_ts too close"),
     N_("If two adjacent points on the same path are less than a few emunits "
        "apart they will cause problems for some of FontForge's commands. "
        "PostScript shouldn't care though."),
     false},
    {CID_PointsTooFar, N_("_Points too far"),
     N_("Most font formats cannot specify adjacent points (or control "
        "points) which are more than 32767 em-units apart in either the x or "
        "y direction"),
     false}};

static std::vector<ProblemRecord> pr_paths = {
    {CID_OpenPaths, N_("O_pen paths"),
     N_("All paths should be closed loops, there "
        "should be no exposed endpoints"),
     false},
    {CID_IntersectingPaths, N_("Intersecting paths"),
     N_("No paths with within a glyph should intersect"), false},
    {CID_LineStd, N_("_Edges near horizontal/vertical"),
     N_("Allows you to find lines which are almost, but not quite horizontal "
        "or vertical (or at the italic angle)."),
     false},
    {CID_Direction, N_("Check _outermost paths clockwise"),
     N_("FontForge internally uses paths drawn in a clockwise direction. This "
        "lets you check that they are. Before doing this test insure that no "
        "paths self-intersect."),
     false},
    {CID_MissingExtrema, N_("Check _missing extrema"),
     N_("PostScript and TrueType require that when a path reaches its maximum "
        "or minimum position there must be a point at that location."),
     false},
    {CID_TooManyPoints, N_("_More points than:"),
     N_("The PostScript Language Reference Manual (Appendix B) says that an "
        "interpreter need not support paths with more than 1500 points. I "
        "think this count includes control points. From PostScript's point of "
        "view, all the contours in a character make up one path. "
        "Modern interpreters tend to support paths with more points than this "
        "limit. (Note a truetype font after conversion to PS will "
        "contain twice as many control points)"),
     false, 1500}};

static std::vector<ProblemRecord> pr_refs = {
    {CID_FlippedRefs, N_("Check _flipped references"),
     N_("PostScript and TrueType require that paths be drawn in a clockwise "
        "direction. If you have a reference that has been flipped then the "
        "paths in that reference will probably be counter-clockwise. You "
        "should unlink it and do Element->Correct direction on it."),
     false},
    {CID_RefBadTransformTTF,
     N_("References with bad TrueType transformation matrices"),
     N_("TrueType requires that all scaling and rotational entries in a "
        "transformation matrix be between -2 and 2"),
     false},
    {CID_MixedContoursRefs, N_("Mixed contours and references"),
     N_("TrueType glyphs can either contain references or contours. Not "
        "both."),
     false},
    {CID_RefBadTransformPS,
     N_("References with bad PostScript transformation matrices"),
     N_("Type1 and 2 fonts only support translation of references. The first "
        "four entries of the transformation matrix should be [1 0 0 1]."),
     false},
    {CID_TooDeepRefs, N_("References neste_d deeper than:"),
     N_("The Type 2 Charstring Reference (Appendix B) says that subroutines "
        "may not be nested more than 10 deep. Each nesting level for "
        "references requires one subroutine level, and hints may require "
        "another level."),
     false, 9},
    {CID_PtMatchRefsOutOfDate, N_("References with out of date point matching"),
     N_("If a glyph has been edited so that it has a different number of "
        "points now, then any references which use point matching and "
        "depended on that glyph's point count will be incorrect."),
     false},
    {CID_MultUseMyMetrics, N_("Multiple references with use-my-metrics"),
     N_("There may be at most one reference with the use-my-metrics bit set"),
     false}};

static std::vector<ProblemRecord> pr_hints = {
    {CID_HintNoPt, N_("_Hints controlling no points"),
     N_("Ghostview (perhaps other interpreters) has a problem when a hint "
        "exists without any points that lie on it."),
     false},
    {CID_PtNearHint, N_("_Points near¹ hint edges"),
     N_("Often if a point is slightly off from a hint it is because a stem is "
        "made up of several segments, and one of them has the wrong width."),
     false},
    {CID_HintWidthNear, N_("Hint _width near¹"),
     N_("Allows you to check that stems have consistent widths.."), false,
     50.0},
    /* GT: The _3 is used to mark an accelerator */
    {CID_Stem3, N_("Almost stem_3 hint"),
     N_("This checks if the character almost, but not exactly, conforms to "
        "the requirements for a stem3 hint. That is, either vertically or "
        "horizontally, there must be exactly three hints, and they must have "
        "the same width and they must be evenly spaced."),
     false},
    {CID_ShowExactStem3,
     N_("_Show exact *stem3"),
     N_("Shows when this character is exactly a stem3 hint"),
     false,
     {},
     .parent_cid = CID_Stem3},
    {CID_TooManyHints, N_("_More hints than:"),
     N_("The Type 2 Charstring Reference (Appendix B) says that there may be "
        "at most 96 horizontal and vertical stem hints in a character."),
     false, 96},
    {CID_OverlappedHints, N_("_Overlapped hints"),
     N_("Either a glyph should have no overlapping hints, or a glyph with "
        "hint masks should have no overlapping hints within a hint mask."),
     false}};

static std::vector<ProblemRecord> pr_att = {
    {CID_MissingGlyph, N_("Check for missing _glyph names"),
     N_("Check whether a substitution, kerning class, etc. uses a glyph name "
        "which does not match any glyph in the font"),
     false},
    {CID_MissingScriptInFeature, N_("Check for missing _scripts in features"),
     N_("In every lookup that uses a glyph, check that at "
        "least one feature is active for the glyph's script."),
     false},
    {CID_BadSubs, N_("Check substitutions for empty chars"),
     N_("Check for characters which contain 'GSUB' entries which refer to "
        "empty characters"),
     false},
    {CID_MissingAnchor, N_("Check for incomplete mark to base subtables"),
     N_("The OpenType documentation suggests in a rather confusing way "
        "that if a base glyph (or base mark) contains an anchor point "
        "for one class in a lookup subtable, then it should contain "
        "anchors for all classes in the subtable"),
     false}};

static std::vector<ProblemRecord> pr_cid = {
    {CID_CIDMultiple, N_("Check for CIDs defined _twice"),
     N_("Check whether a CID is defined in more than one sub-font"), false},
    {CID_CIDBlank, N_("Check for _undefined CIDs"),
     N_("Check whether a CID is undefined in all sub-fonts"), false}};

static std::vector<ProblemRecord> pr_bb = {
    {CID_BBYMax, N_("Glyph bounding box above"),
     N_("Are there any glyph's whose bounding boxes extend above this number?"),
     false, 0},
    {CID_BBYMin, N_("Glyph bounding box below"),
     N_("Are there any glyph's whose bounding boxes extend below this number?"),
     false, 0},
    {CID_BBXMax, N_("Glyph bounding box right of"),
     N_("Are there any glyphs whose bounding boxes extend to the right of this "
        "number?"),
     false, 0},
    {CID_BBXMin, N_("Glyph bounding box left of"),
     N_("Are there any glyph's whose bounding boxes extend to the left of this "
        "number?"),
     false, 0},
    {CID_AdvanceWidth, N_("Check advance:"),
     N_("Check for characters whose advance width is not the displayed value."),
     false, 0},
    {CID_VAdvanceWidth, N_("Check vertical advance:"),
     N_("Check for characters whose vertical advance width is not the "
        "displayed value."),
     false, 0}};

static std::vector<ProblemRecord> pr_random = {
    {CID_Bitmaps, N_("Check missing _bitmaps"),
     N_("Are there any outline characters which don't have a bitmap version in "
        "one of the bitmap fonts? Conversely are there any bitmap characters "
        "without a corresponding outline character?"),
     false},
    {CID_BitmapWidths, N_("Bitmap/outline _advance mismatch"),
     N_("Are there any bitmap glyphs whose advance width is not is expected "
        "from scaling and rounding the outline's advance width?"),
     false},
    {CID_MultUni, N_("Check multiple Unicode"),
     N_("Check for multiple characters with the same Unicode code point"),
     false},
    {CID_MultName, N_("Check multiple names"),
     N_("Check for multiple characters with the same name"), false},
    {CID_UniNameMisMatch, N_("Check Unicode/name mismatch"),
     N_("Check for characters whose name maps to a unicode code point which "
        "does not map the character's assigned code point."),
     false}};

static std::vector<ProblemTab> pr_tabs = {
    {N_("Points"), pr_points},   {N_("Paths"), pr_paths},
    {N_("References"), pr_refs}, {N_("Hints"), pr_hints},
    {N_("ATT"), pr_att},         {N_("CID"), pr_cid},
    {N_("Bounding Box"), pr_bb}, {N_("Random"), pr_random}};

static void adjust_problem_records(FontView* fv,
                                   std::vector<ProblemTab>& problem_tabs) {
    static SplineFont* lastsf = NULL;
    static bool loc_initialized = false;
    SplineFont* sf = fv->b.sf;

    if (lastsf != sf) {
        SplineChar* ssc = SFGetChar(sf, ' ', NULL);

        assert(problem_tabs[6].records[0].cid == CID_BBYMax);
        problem_tabs[6].records[0].value = sf->ascent;

        assert(problem_tabs[6].records[1].cid == CID_BBYMin);
        problem_tabs[6].records[1].value = -sf->descent;

        assert(problem_tabs[6].records[2].cid == CID_BBXMax);
        problem_tabs[6].records[2].value = sf->ascent + sf->descent;

        assert(problem_tabs[6].records[4].cid == CID_AdvanceWidth);
        problem_tabs[6].records[4].value = ssc ? ssc->width : 0;

        assert(problem_tabs[6].records[5].cid == CID_VAdvanceWidth);
        problem_tabs[6].records[5].value = sf->ascent + sf->descent;

        lastsf = sf;
    }

    assert(problem_tabs[0].records[4].cid == CID_CpStd);
    problem_tabs[0].records[4].label =
        (sf->italicangle == 0
             ? _("_Control points near horizontal/vertical")
             : _("Control points near horizontal/vertical/italic"));

    assert(problem_tabs[1].records[2].cid == CID_LineStd);
    problem_tabs[1].records[2].label =
        (sf->italicangle == 0 ? _("_Edges near horizontal/vertical")
                              : _("Edges near horizontal/vertical/italic"));

    assert(problem_tabs[5].records[0].cid == CID_CIDMultiple);
    problem_tabs[5].records[0].disabled = (fv->b.cidmaster == NULL);

    assert(problem_tabs[5].records[1].cid == CID_CIDBlank);
    problem_tabs[5].records[1].disabled = (fv->b.cidmaster == NULL);

    assert(problem_tabs[6].records[5].cid == CID_VAdvanceWidth);
    problem_tabs[6].records[5].disabled = (!sf->hasvmetrics);
}

static void apply_dialog_results(const std::vector<ProblemTab>& problem_tabs,
                                 struct problems& p) {
    near = p.near;

    for (const ProblemTab& tab : problem_tabs) {
        for (const ProblemRecord& rec : tab.records) {
            /* Points */
            if (rec.cid == CID_NonIntegral)
                nonintegral = p.nonintegral = rec.active;
            if (rec.cid == CID_XNear) {
                doxnear = p.xnearval = rec.active;
                if (doxnear) p.xval = xval = std::get<double>(rec.value);
            }
            if (rec.cid == CID_YNear) {
                doynear = p.ynearval = rec.active;
                if (doynear) p.yval = yval = std::get<double>(rec.value);
            }
            if (rec.cid == CID_YNearStd) doynearstd = p.ynearstd = rec.active;
            if (rec.cid == CID_CpStd) cpstd = p.cpnearstd = rec.active;
            if (rec.cid == CID_CpOdd) cpodd = p.cpodd = rec.active;
            if (rec.cid == CID_IrrelevantCP) {
                irrelevantcp = p.irrelevantcontrolpoints = rec.active;
                if (irrelevantcp)
                    p.irrelevantfactor = irrelevantfactor =
                        std::get<double>(rec.value) / 100.0;
            }
            if (rec.cid == CID_PointsTooClose)
                pointstooclose = p.pointstooclose = rec.active;
            if (rec.cid == CID_PointsTooFar)
                pointstoofar = p.pointstoofar = rec.active;

            /* Paths */
            if (rec.cid == CID_OpenPaths) openpaths = p.openpaths = rec.active;
            if (rec.cid == CID_IntersectingPaths)
                intersectingpaths = p.intersectingpaths = rec.active;
            if (rec.cid == CID_LineStd) linestd = p.linenearstd = rec.active;
            if (rec.cid == CID_Direction) direction = p.direction = rec.active;
            if (rec.cid == CID_MissingExtrema)
                missingextrema = p.missingextrema = rec.active;
            if (rec.cid == CID_TooManyPoints) {
                toomanypoints = p.toomanypoints = rec.active;
                if (toomanypoints)
                    p.pointsmax = pointsmax = std::get<int>(rec.value);
            }

            /* Refs */
            if (rec.cid == CID_FlippedRefs)
                flippedrefs = p.flippedrefs = rec.active;
            if (rec.cid == CID_RefBadTransformTTF)
                refsbadtransformttf = p.refsbadtransformttf = rec.active;
            if (rec.cid == CID_MixedContoursRefs)
                mixedcontoursrefs = p.mixedcontoursrefs = rec.active;
            if (rec.cid == CID_RefBadTransformPS)
                refsbadtransformps = p.refsbadtransformps = rec.active;
            if (rec.cid == CID_TooDeepRefs) {
                toodeeprefs = p.toodeeprefs = rec.active;
                if (toodeeprefs)
                    p.refdepthmax = refdepthmax = std::get<int>(rec.value);
            }
            if (rec.cid == CID_PtMatchRefsOutOfDate)
                ptmatchrefsoutofdate = p.ptmatchrefsoutofdate = rec.active;
            if (rec.cid == CID_MultUseMyMetrics)
                multusemymetrics = p.multusemymetrics = rec.active;

            /* Hints */
            if (rec.cid == CID_HintNoPt) hintnopt = p.hintwithnopt = rec.active;
            if (rec.cid == CID_PtNearHint)
                ptnearhint = p.ptnearhint = rec.active;
            if (rec.cid == CID_HintWidthNear) {
                hintwidth = p.hintwidthnearval = rec.active;
                if (hintwidth)
                    widthval = p.widthval = std::get<double>(rec.value);
            }
            if (rec.cid == CID_Stem3) stem3 = p.stem3 = rec.active;
            if (rec.cid == CID_ShowExactStem3 && stem3)
                showexactstem3 = p.showexactstem3 = rec.active;
            if (rec.cid == CID_TooManyHints) {
                toomanyhints = p.toomanyhints = rec.active;
                if (toomanyhints)
                    p.hintsmax = hintsmax = std::get<int>(rec.value);
            }
            if (rec.cid == CID_OverlappedHints)
                overlappedhints = p.overlappedhints = rec.active;

            /* ATT */
            if (rec.cid == CID_MissingGlyph)
                missingglyph = p.missingglyph = rec.active;
            if (rec.cid == CID_MissingScriptInFeature)
                missingscriptinfeature = p.missingscriptinfeature = rec.active;
            if (rec.cid == CID_BadSubs) badsubs = p.badsubs = rec.active;
            if (rec.cid == CID_MissingAnchor)
                missinganchor = p.missinganchor = rec.active;

            /* CID */
            if (p.fv->b.cidmaster != NULL) {
                if (rec.cid == CID_CIDMultiple)
                    cidmultiple = p.cidmultiple = rec.active;
                if (rec.cid == CID_CIDBlank) cidblank = p.cidblank = rec.active;
            }

            /* Bounding Box */
            if (rec.cid == CID_BBYMax) {
                bbymax = p.bbymax = rec.active;
                if (bbymax)
                    bbymax_val = p.bbymax_val = std::get<int>(rec.value);
            }
            if (rec.cid == CID_BBYMin) {
                bbymin = p.bbymin = rec.active;
                if (bbymin)
                    bbymin_val = p.bbymin_val = std::get<int>(rec.value);
            }
            if (rec.cid == CID_BBXMax) {
                bbxmax = p.bbxmax = rec.active;
                if (bbxmax)
                    bbxmax_val = p.bbxmax_val = std::get<int>(rec.value);
            }
            if (rec.cid == CID_BBXMin) {
                bbxmin = p.bbxmin = rec.active;
                if (bbxmin)
                    bbxmin_val = p.bbxmin_val = std::get<int>(rec.value);
            }
            if (rec.cid == CID_AdvanceWidth) {
                advancewidth = p.advancewidth = rec.active;
                if (advancewidth)
                    advancewidthval = p.advancewidthval =
                        std::get<int>(rec.value);
            }
            if (p.fv->b.sf->hasvmetrics && rec.cid == CID_VAdvanceWidth) {
                vadvancewidth = p.vadvancewidth = rec.active;
                if (vadvancewidth)
                    vadvancewidthval = p.vadvancewidthval =
                        std::get<int>(rec.value);
            }

            /* Random */
            if (rec.cid == CID_Bitmaps) bitmaps = p.bitmaps = rec.active;
            if (rec.cid == CID_BitmapWidths)
                bitmapwidths = p.bitmapwidths = rec.active;
            if (rec.cid == CID_MultUni) multuni = p.multuni = rec.active;
            if (rec.cid == CID_MultName) multname = p.multname = rec.active;
            if (rec.cid == CID_UniNameMisMatch)
                uninamemismatch = p.uninamemismatch = rec.active;
        }
    }
}

void FindProblems(FontView *fv,CharView *cv, SplineChar *sc) {
    struct problems p;
    bool do_apply = false;

    memset(&p,0,sizeof(p));
    if ( fv==NULL ) fv = (FontView *) (cv->b.fv);
    p.fv = fv; p.cv=cv; p.msc = sc;
    p.near = near;
    if ( cv!=NULL )
	p.lastcharopened = cv->b.sc;
    if ( fv!=NULL ) {
	p.map = fv->b.map;
	p.layer = fv->b.active_layer;
    } else if ( cv!=NULL ) {
	p.map = cv->b.fv->map;
	p.layer = CVLayer((CharViewBase *) cv);
    } else {
	p.map = sc->parent->fv->map;
	p.layer = sc->parent->fv->active_layer;
    }

    adjust_problem_records(fv, pr_tabs);
    do_apply = find_problems_dialog(fv->gw, pr_tabs, p.near);

    if (do_apply) {
	apply_dialog_results(pr_tabs, p);
	show_selected_problems(&p);
    }

    if ( p.explainw!=NULL )
	GDrawDestroyWindow(p.explainw);
}

/* ************************************************************************** */
/* ***************************** Validation code **************************** */
/* ************************************************************************** */

struct val_data {
    GWindow gw;
    GWindow v;
    GGadget *vsb;
    int lcnt;
    int loff_top;
    int vlcnt;
    SplineFont *sf;
    int cidmax;
    enum validation_state mask;
    int need_to_check_with_user_on_mask;
    int needs_blue;
    GTimer *recheck;
    int laststart;
    int finished_first_pass;
    int as,fh;
    GFont *font;
    SplineChar *sc;		/* used by popup menu */
    int lastgid;
    CharView *lastcv;
    int layer;
};

static const char *vserrornames[] = {
    N_("Open Contour"),
    N_("Self Intersecting"),
    N_("Wrong Direction"),
    N_("Flipped References"),
    N_("Missing Points at Extrema"),
    N_("Unknown glyph referenced in GSUB/GPOS/MATH"),
    N_("Too Many Points"),
    N_("Too Many Hints"),
    N_("Bad Glyph Name"),
    NULL,		/* Maxp too many points */
    NULL,		/* Maxp too many paths */
    NULL,		/* Maxp too many component points */
    NULL,		/* Maxp too many component paths */
    NULL,		/* Maxp instructions too long */
    NULL,		/* Maxp too many references */
    NULL,		/* Maxp references too deep */
    NULL,		/* prep or fpgm too long */
    N_("Distance between adjacent points is too big"),
    N_("Non-integral coordinates"),
    N_("Contains anchor points for some, but not all, classes in a subtable"),
    N_("There is another glyph in the font with this name"),
    N_("There is another glyph in the font with this unicode code point"),
    N_("Glyph contains overlapped hints (in the same hintmask)")
};

static const char *privateerrornames[] = {
    N_("Odd number of elements in BlueValues/OtherBlues array."),
    N_("Elements in BlueValues/OtherBlues array are disordered."),
    N_("Too many elements in BlueValues/OtherBlues array."),
    N_("Elements in BlueValues/OtherBlues array are too close (Change BlueFuzz)."),
    N_("Elements in BlueValues/OtherBlues array are not integers."),
    N_("Alignment zone height in BlueValues/OtherBlues array is too big for BlueScale."),
    NULL,
    NULL,
    N_("Odd number of elements in FamilyBlues/FamilyOtherBlues array."),
    N_("Elements in FamilyBlues/FamilyOtherBlues array are disordered."),
    N_("Too many elements in FamilyBlues/FamilyOtherBlues array."),
    N_("Elements in FamilyBlues/FamilyOtherBlues array are too close (Change BlueFuzz)."),
    N_("Elements in FamilyBlues/FamilyOtherBlues array are not integers."),
    N_("Alignment zone height in FamilyBlues/FamilyOtherBlues array is too big for BlueScale."),
    NULL,
    NULL,
    N_("Missing BlueValues entry."),
    N_("Bad BlueFuzz entry."),
    N_("Bad BlueScale entry."),
    N_("Bad StdHW entry."),
    N_("Bad StdVW entry."),
    N_("Bad StemSnapH entry."),
    N_("Bad StemSnapV entry."),
    N_("StemSnapH does not contain StdHW value."),
    N_("StemSnapV does not contain StdVW value."),
    N_("Bad BlueShift entry."),
    NULL
};

char *VSErrorsFromMask(int mask, int private_mask) {
    int bit, m;
    int len;
    char *ret;

    len = 0;
    for ( m=0, bit=(vs_known<<1) ; bit<=vs_last; ++m, bit<<=1 )
	if ( (mask&bit) && vserrornames[m]!=NULL )
	    len += strlen( _(vserrornames[m]))+2;
    if ( private_mask != 0 )
	len += strlen( _("Bad Private Dictionary")) +2;
	ret = (char *) malloc(len+1);
    len = 0;
    for ( m=0, bit=(vs_known<<1) ; bit<=vs_last; ++m, bit<<=1 )
	if ( (mask&bit) && vserrornames[m]!=NULL ) {
	    ret[len++] =' ';
	    strcpy(ret+len,_(vserrornames[m]));
	    len += strlen( ret+len );
	    ret[len++] ='\n';
	}
    if ( private_mask != 0 ) {
	ret[len++] =' ';
	strcpy(ret+len,_("Bad Private Dictionary"));
	len += strlen( ret+len );
	ret[len++] ='\n';
    }
    ret[len] = '\0';
return( ret );
}

static int VSModMask(SplineChar *sc, struct val_data *vw) {
    int vs = 0;
    if ( sc!=NULL ) {
	vs = sc->layers[vw->layer].validation_state;
	if ( sc->unlink_rm_ovrlp_save_undo )
	    vs &= ~vs_selfintersects;
	/* if doing a truetype eval, then I'm told it's ok for references */
	/*  to overlap. And if refs can overlap, then we can't figure out */
	/*  direction properly */ /* I should really check that all the   */
	/*  refs have legal ttf transform matrices */
	if ( vw->mask==vs_maskttf &&
		sc->layers[vw->layer].splines==NULL &&
		sc->layers[vw->layer].refs!=NULL )
	    vs &= ~(vs_selfintersects|vs_wrongdirection);
    }
return( vs );
}

static int VW_FindLine(struct val_data *vw,int line, int *skips) {
    int gid,k, cidmax = vw->cidmax;
    SplineFont *sf = vw->sf;
    SplineFont *sub;
    SplineChar *sc;
    int sofar, tot;
    int bit;
    int vs;

    sofar = 0;
    for ( gid=0; gid<cidmax ; ++gid ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->glyphs[gid];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) {
		sub = sf->subfonts[k];
		if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
	    break;
	    }
	}
	/* Ignore it if it has not been validated */
	/* Ignore it if it is good */
	vs = VSModMask(sc,vw);
	if ((vs&vs_known) && (vs&vw->mask)!=0 ) {
	    tot = 1;
	    if ( sc->vs_open )
		for ( bit=(vs_known<<1) ; bit<=vs_last; bit<<=1 )
		    if ( (bit&vw->mask) && (vs&bit) )
			++tot;
	    if ( sofar+tot>line ) {
		*skips = line-sofar;
return( gid );
	    }
	    sofar += tot;
	}
    }

    vs = ValidatePrivate(sf);
    if ( !vw->needs_blue )
	vs &= ~pds_missingblue;
    if ( vs!=0 ) {
	tot = 1;
	for ( bit=1 ; bit!=0; bit<<=1 )
	    if ( vs&bit )
		++tot;
	if ( sofar+tot>line ) {
	    *skips = line-sofar;
return( -2 );
	}
    }

    *skips = 0;
return( -1 );
}

static int VW_FindSC(struct val_data *vw,SplineChar *sought) {
    int gid,k, cidmax = vw->cidmax;
    SplineFont *sf = vw->sf;
    SplineFont *sub;
    SplineChar *sc;
    int sofar;
    int bit, vs;

    sofar = 0;
    for ( gid=0; gid<cidmax ; ++gid ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->glyphs[gid];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) {
		sub = sf->subfonts[k];
		if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
	    break;
	    }
	}
	/* Ignore it if it has not been validated */
	/* Ignore it if it is good */
	vs = VSModMask(sc,vw);
	if ((vs&vs_known) && (vs&vw->mask)!=0 ) {
	    if ( sc==sought )
return( sofar );
	    ++sofar;
	    if ( sc->vs_open )
		for ( bit=(vs_known<<1) ; bit<=vs_last; bit<<=1 )
		    if ( (bit&vw->mask) && (vs&bit) )
			++sofar;
	} else if ( sc==sought )
return( -1 );
    }
return( -1 );
}

static int VW_VScroll(GGadget *g, GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(GGadgetGetWindow(g));
    int newpos = vw->loff_top;

    switch( e->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*vw->vlcnt/10;
      break;
      case et_sb_up:
        newpos -= vw->vlcnt/15;
      break;
      case et_sb_down:
        newpos += vw->vlcnt/15;
      break;
      case et_sb_downpage:
        newpos += 9*vw->vlcnt/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = e->u.control.u.sb.pos;
      break;
      case et_sb_halfup:
        newpos -= vw->vlcnt/30;
      break;
      case et_sb_halfdown:
        newpos += vw->vlcnt/30;
      break;
    }
    if ( newpos + vw->vlcnt > vw->lcnt )
	newpos = vw->lcnt-vw->vlcnt;
    if ( newpos<0 )
	newpos = 0;
    if ( vw->loff_top!=newpos ) {
	vw->loff_top = newpos;
	GScrollBarSetPos(vw->vsb,newpos);
	GDrawRequestExpose(vw->v,NULL,false);
    }
return( true );
}

static void VW_SetSb(struct val_data *vw) {
    if ( vw->loff_top + vw->vlcnt > vw->lcnt )
	vw->loff_top = vw->lcnt-vw->vlcnt;
    if ( vw->loff_top<0 )
	vw->loff_top = 0;
    GScrollBarSetBounds(vw->vsb,0,vw->lcnt,vw->vlcnt);
    GScrollBarSetPos(vw->vsb,vw->loff_top);
}

static void VW_Remetric(struct val_data *vw) {
    int gid,k, cidmax = vw->cidmax;
    SplineFont *sub, *sf = vw->sf;
    SplineChar *sc;
    int sofar, tot;
    int bit, vs;

    sofar = 0;
    for ( gid=0; gid<cidmax ; ++gid ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->glyphs[gid];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) {
		sub = sf->subfonts[k];
		if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
	    break;
	    }
	}
	/* Ignore it if it has not been validated */
	/* Ignore it if it is good */
	vs = VSModMask(sc,vw);
	if ((vs&vs_known) && (vs&vw->mask)!=0 ) {
	    tot = 1;
	    if ( sc->vs_open )
		for ( bit=(vs_known<<1) ; bit<=vs_last; bit<<=1 )
		    if ( (bit&vw->mask) && (vs&bit) )
			++tot;
	    sofar += tot;
	}
    }
    vs = ValidatePrivate(sf);
    if ( !vw->needs_blue )
	vs &= ~pds_missingblue;
    if ( vs!=0 ) {
	tot = 1;
	for ( bit=1 ; bit!=0; bit<<=1 )
	    if ( vs&bit )
		++tot;
	sofar += tot;
    }
    if ( vw->lcnt!=sofar ) {
	vw->lcnt = sofar;
	VW_SetSb(vw);
    }
    GDrawRequestExpose(vw->v,NULL,false);
}

static void VWMenuConnect(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;
    int changed = false;
    SplineSet *ss;

    for ( ss=sc->layers[vw->layer].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL && ss->first->next!=NULL ) {
	    if ( !changed ) {
		SCPreserveLayer(sc,vw->layer,false);
		changed = true;
	    }
	    SplineMake(ss->last,ss->first,sc->layers[vw->layer].order2);
	    ss->last = ss->first;
	}
    }
    if ( changed ) {
	SCCharChangedUpdate(sc,vw->layer);
	SCValidate(vw->sc,vw->layer,true);
	if ( vs != vw->sc->layers[vw->layer].validation_state )
	    VW_Remetric(vw);
    }
}

static void VWMenuInlineRefs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;
    int changed = false;
    RefChar *ref, *refnext;

    for ( ref= sc->layers[vw->layer].refs; ref!=NULL; ref=refnext ) {
	refnext = ref->next;
	if ( !changed )
	    SCPreserveLayer(sc,vw->layer,false);
	changed = true;
	SCRefToSplines(sc,ref,vw->layer);
    }
    if ( changed ) {
	SCCharChangedUpdate(sc,vw->layer);

	SCValidate(vw->sc,vw->layer,true);
	if ( vs != vw->sc->layers[vw->layer].validation_state )
	    VW_Remetric(vw);
    }
}

static void VWMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;

    if ( !SCRoundToCluster(sc,ly_all,false,.03,.12))
	SCPreserveLayer(sc,vw->layer,false);
    sc->layers[vw->layer].splines = SplineSetRemoveOverlap(sc,sc->layers[vw->layer].splines,over_remove);
    SCCharChangedUpdate(sc,vw->layer);

    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWMenuMark(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;

    sc->unlink_rm_ovrlp_save_undo = true;

    VW_Remetric(vw);
}

static void VWMenuInlineFlippedRefs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;
    int changed = false;
    RefChar *ref, *refnext;

    for ( ref= sc->layers[vw->layer].refs; ref!=NULL; ref=refnext ) {
	refnext = ref->next;
	if ( ref->transform[0]*ref->transform[3]<0 ||
		(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
	    if ( !changed )
		SCPreserveLayer(sc,vw->layer,false);
	    changed = true;
	    SCRefToSplines(sc,ref,vw->layer);
	}
    }
    if ( changed ) {
	SCCharChangedUpdate(sc,vw->layer);

	SCValidate(vw->sc,vw->layer,true);
	if ( vs != vw->sc->layers[vw->layer].validation_state )
	    VW_Remetric(vw);
    }
}

static void VWMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;
    int changed = false;

    SCPreserveLayer(sc,vw->layer,false);
    sc->layers[vw->layer].splines = SplineSetsCorrect(sc->layers[vw->layer].splines,&changed);
    SCCharChangedUpdate(sc,vw->layer);

    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWMenuGoodExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineFont *sf = vw->sf;
    int emsize = sf->ascent+sf->descent;
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;

    SCPreserveLayer(sc,vw->layer,false);
    SplineCharAddExtrema(sc,sc->layers[vw->layer].splines,ae_only_good,emsize);
    SCCharChangedUpdate(sc,vw->layer);

    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWMenuAllExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineFont *sf = vw->sf;
    int emsize = sf->ascent+sf->descent;
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;

    SCPreserveLayer(sc,vw->layer,false);
    SplineCharAddExtrema(sc,sc->layers[vw->layer].splines,ae_all,emsize);
    SCCharChangedUpdate(sc,vw->layer);

    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc = vw->sc;
    int vs = sc->layers[vw->layer].validation_state;
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.05, 0, -1, 0, 0 };

    SCPreserveLayer(sc,vw->layer,false);
    sc->layers[vw->layer].splines = SplineCharSimplify(sc,sc->layers[vw->layer].splines,&smpl);
    SCCharChangedUpdate(sc,vw->layer);

    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWMenuRevalidateAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	    sc->layers[vw->layer].validation_state = 0;
	    sc->layers[vw->layer].old_vs = 2;
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
    VW_Remetric(vw);
}

static void VWMenuRevalidate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    int vs = vw->sc->layers[vw->layer].validation_state;
    SCValidate(vw->sc,vw->layer,true);
    if ( vs != vw->sc->layers[vw->layer].validation_state )
	VW_Remetric(vw);
}

static void VWReuseCV(struct val_data *vw, SplineChar *sc) {
    int k;
    SplineChar *sctest;
    SplineFont *sf = vw->sf;
    CharView *cv;

    /* See if the last cv we used is still open. This is a little complex as */
    /*  we must make sure that the splinechar is still in the font, and then */
    /*  that the cv is still attached to it */
    cv = NULL;
    if ( vw->lastgid!=-1 && vw->lastcv!=NULL ) {
	sctest = NULL;
	if ( sf->subfontcnt==0 ) {
	    if ( vw->lastgid<sf->glyphcnt )
		sctest = sf->glyphs[vw->lastgid];
	} else {
	    for ( k = 0; k<sf->subfontcnt; ++k )
		if ( vw->lastgid<sf->subfonts[k]->glyphcnt )
		    if ( (sctest = sf->subfonts[k]->glyphs[vw->lastgid])!=NULL )
	    break;
	}
	if ( sctest!=NULL )
	    for ( cv=(CharView *) (sctest->views); cv!=NULL && cv!=vw->lastcv; cv=(CharView *) (cv->b.next) );
    }
    if ( cv==NULL )
	cv = CharViewCreate(sc,(FontView *) (vw->sf->fv),vw->sf->fv->map->backmap[sc->orig_pos]);
    else {
	CVChangeSC(cv,sc);
	GDrawSetVisible(cv->gw,true);
	GDrawRaise(cv->gw);
    }
    if ( CVLayer((CharViewBase *) cv)!=vw->layer )
	CVSetLayer(cv,vw->layer);
    vw->lastgid = sc->orig_pos;
    vw->lastcv = cv;

    if ( sc->layers[vw->layer].validation_state & vs_maskfindproblems & vw->mask )
	DummyFindProblems(cv);
}

static void VWMenuOpenGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    VWReuseCV(vw,vw->sc);
}

static void VWMenuGotoGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    FontView *fv = (FontView *) (vw->sf->fv);
    int enc = GotoChar(vw->sf,fv->b.map,NULL);
    int gid, line;
    SplineChar *sc;

    if ( enc==-1 )
return;
    gid = fv->b.map->map[enc];
    if ( gid==-1 || (sc=vw->sf->glyphs[gid])==NULL ) {
	ff_post_error(_("Glyph not in font"), _("Glyph not in font"));
return;
    } else if ( (SCValidate(sc,vw->layer,true)&vw->mask)==0 ) {
	ff_post_notice(_("Glyph Valid"), _("No problems detected in %s"),
		sc->name );
return;
    }

    line = VW_FindSC(vw,sc);
    if ( line==-1 )
	IError("Glyph doesn't exist?");

    if ( line + vw->vlcnt > vw->lcnt )
	line = vw->lcnt-vw->vlcnt;
    if ( line<0 )
	line = 0;
    if ( vw->loff_top!=line ) {
	vw->loff_top = line;
	GScrollBarSetPos(vw->vsb,line);
	GDrawRequestExpose(vw->v,NULL,false);
    }
}


#define MID_SelectOpen	102
#define MID_SelectRO	103
#define MID_SelectDir	104
#define MID_SelectExtr	105
#define MID_SelectErrors	106

static void VWMenuSelect(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    FontView *fv = (FontView *) (vw->sf->fv);
    int mask = mi->mid == MID_SelectErrors ? vw->mask :
	    mi->mid == MID_SelectOpen ? vs_opencontour :
	    mi->mid == MID_SelectRO ? vs_selfintersects :
	    mi->mid == MID_SelectDir ? vs_wrongdirection :
	    mi->mid == MID_SelectExtr ? vs_missingextrema : 0;
    EncMap *map = fv->b.map;
    int i, gid;
    SplineChar *sc;

    for ( i=0; i<map->enccount; ++i ) {
	fv->b.selected[i] = false;
	gid = map->map[i];
	if ( gid!=-1 && (sc=vw->sf->glyphs[gid])!=NULL &&
		(SCValidate(sc,vw->layer,true) & mask) )
	    fv->b.selected[i] = true;
    }
    GDrawSetVisible(fv->gw,true);
    GDrawRaise(fv->gw);
    GDrawRequestExpose(fv->v,NULL,false);
}

static GMenuItem vw_subselect[] = {
    { { (unichar_t *) N_("problselect|Errors"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSelect, MID_SelectErrors },
    { { (unichar_t *) N_("problselect|Open Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSelect, MID_SelectOpen },
    { { (unichar_t *) N_("problselect|Bad Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSelect, MID_SelectDir },
    { { (unichar_t *) N_("problselect|Self Intersections"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSelect, MID_SelectRO },
    { { (unichar_t *) N_("problselect|Missing Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSelect, MID_SelectExtr },
    GMENUITEM_EMPTY
};

static void VWMenuManyConnect(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_opencontour) ) {
	    int vs = sc->layers[vw->layer].validation_state;
	    int changed = false;
	    SplineSet *ss;

	    for ( ss=sc->layers[vw->layer].splines; ss!=NULL; ss=ss->next ) {
		if ( ss->first->prev==NULL && ss->first->next!=NULL ) {
		    if ( !changed ) {
			SCPreserveLayer(sc,vw->layer,false);
			changed = true;
		    }
		    SplineMake(ss->last,ss->first,sc->layers[vw->layer].order2);
		    ss->last = ss->first;
		}
	    }
	    if ( changed ) {
		SCCharChangedUpdate(sc,vw->layer);
		SCValidate(vw->sc,vw->layer,true);
		if ( vs != vw->sc->layers[vw->layer].validation_state )
		    VW_Remetric(vw);
	    }
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManyOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_selfintersects) ) {
	    int vs = sc->layers[vw->layer].validation_state;

	    /* If it's only got references, I could inline them, since the */
	    /*  intersection would occur between two refs. But that seems */
	    /*  to extreme to do to an unsuspecting user */
	    if ( !SCRoundToCluster(sc,ly_all,false,.03,.12))
		SCPreserveLayer(sc,vw->layer,false);
	    sc->layers[vw->layer].splines = SplineSetRemoveOverlap(sc,sc->layers[vw->layer].splines,over_remove);
	    SCCharChangedUpdate(sc,vw->layer);
	    SCValidate(vw->sc,vw->layer,true);
	    if ( vs != vw->sc->layers[vw->layer].validation_state )
		VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManyMark(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL &&
		(sc->layers[vw->layer].validation_state&vs_selfintersects) &&
		sc->layers[vw->layer].refs!=NULL &&
		sc->layers[vw->layer].refs->next!=NULL &&
		sc->layers[vw->layer].splines==NULL ) {
	    sc->unlink_rm_ovrlp_save_undo = true;
	    VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManyCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;
    RefChar *ref, *refnext;
    int changed;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_wrongdirection) ) {
	    int vs = sc->layers[vw->layer].validation_state;

	    SCPreserveLayer(sc,vw->layer,false);
	    /* But a flipped reference is just wrong so I have no compunctions*/
	    /*  about inlining it and then correcting its direction */

	    for ( ref= sc->layers[vw->layer].refs; ref!=NULL; ref=refnext ) {
		refnext = ref->next;
		if ( ref->transform[0]*ref->transform[3]<0 ||
			(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		    SCRefToSplines(sc,ref,vw->layer);
		}
	    }
	    sc->layers[vw->layer].splines = SplineSetsCorrect(sc->layers[vw->layer].splines,&changed);
	    SCCharChangedUpdate(sc,vw->layer);
	    SCValidate(vw->sc,vw->layer,true);
	    if ( vs != vw->sc->layers[vw->layer].validation_state )
		VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManyGoodExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf = vw->sf;
    int emsize = sf->ascent+sf->descent;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_missingextrema) ) {
	    int vs = sc->layers[vw->layer].validation_state;

	    SCPreserveLayer(sc,vw->layer,false);
	    SplineCharAddExtrema(sc,sc->layers[vw->layer].splines,ae_only_good,emsize);
	    SCCharChangedUpdate(sc,vw->layer);
	    SCValidate(vw->sc,vw->layer,true);
	    if ( vs != vw->sc->layers[vw->layer].validation_state )
		VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManyAllExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf = vw->sf;
    int emsize = sf->ascent+sf->descent;

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_missingextrema) ) {
	    int vs = sc->layers[vw->layer].validation_state;

	    SCPreserveLayer(sc,vw->layer,false);
	    SplineCharAddExtrema(sc,sc->layers[vw->layer].splines,ae_all,emsize);
	    SCCharChangedUpdate(sc,vw->layer);
	    SCValidate(vw->sc,vw->layer,true);
	    if ( vs != vw->sc->layers[vw->layer].validation_state )
		VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static void VWMenuManySimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);
    SplineChar *sc;
    int k, gid;
    SplineFont *sf;
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.05, 0, -1, 0, 0 };

    k=0;
    do {
	sf = k<vw->sf->subfontcnt ? vw->sf->subfonts[k] : vw->sf;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && (sc->layers[vw->layer].validation_state&vs_toomanypoints) ) {
	    int vs = sc->layers[vw->layer].validation_state;

	    SCPreserveLayer(sc,vw->layer,false);
	    sc->layers[vw->layer].splines = SplineCharSimplify(sc,sc->layers[vw->layer].splines,&smpl);
	    SCCharChangedUpdate(sc,vw->layer);
	    SCValidate(vw->sc,vw->layer,true);
	    if ( vs != vw->sc->layers[vw->layer].validation_state )
		VW_Remetric(vw);
	}
	++k;
    } while ( k<vw->sf->subfontcnt );
}

static GMenuItem vw_subfixup[] = {
    { { (unichar_t *) N_("problfixup|Open Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyConnect, 0 },
    { { (unichar_t *) N_("problfixup|Self Intersections"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyOverlap, 0 },
    { { (unichar_t *) N_("problfixup|Mark for Overlap fix before Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyMark, 0 },
    { { (unichar_t *) N_("problfixup|Bad Directions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyCorrectDir, 0 },
    { { (unichar_t *) N_("problfixup|Missing Extrema (cautiously)"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyGoodExtrema, 0 },
    { { (unichar_t *) N_("problfixup|Missing Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManyAllExtrema, 0 },
    { { (unichar_t *) N_("problfixup|Too Many Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuManySimplify, 0 },
    GMENUITEM_EMPTY
};

static GMenuItem vw_popuplist[] = {
    { { (unichar_t *) N_("Close Open Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuConnect, 0 },
    { { (unichar_t *) N_("Inline All References"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuInlineRefs, 0 },
    { { (unichar_t *) N_("Remove Overlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuOverlap, 0 },
    { { (unichar_t *) N_("Mark for Overlap fix before Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuMark, 0 },
    { { (unichar_t *) N_("Inline Flipped References"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuInlineFlippedRefs, 0 },
    { { (unichar_t *) N_("Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuCorrectDir, 0 },
    { { (unichar_t *) N_("Add Good Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0',0, NULL, NULL, VWMenuGoodExtrema, 0 },
    { { (unichar_t *) N_("Add All Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuAllExtrema, 0 },
    { { (unichar_t *) N_("Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 0 }, '\0', 0, NULL, NULL, VWMenuSimplify, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Revalidate All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuRevalidateAll, 0 },
    { { (unichar_t *) N_("Revalidate"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuRevalidate, 0 },
    { { (unichar_t *) N_("Open Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuOpenGlyph, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Scroll To Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, NULL, NULL, VWMenuGotoGlyph, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Select Glyphs With"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, vw_subselect, NULL, NULL, 0 },
    { { (unichar_t *) N_("Try To Fix Glyphs With"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, '\0', 0, vw_subfixup, NULL, NULL, 0 },
    GMENUITEM_EMPTY
};

static void VWMouse(struct val_data *vw, GEvent *e) {
    int skips;
    int gid = VW_FindLine(vw,vw->loff_top + e->u.mouse.y/vw->fh, &skips);
    SplineChar *sc;
    int k=0;

    if ( gid==-2 && e->u.mouse.clicks==2 && e->type==et_mouseup ) {
	FontInfo(vw->sf,vw->layer,4,false);	/* Bring up the Private Dict */
return;
    }
    if ( gid<0 )
return;
    if ( vw->sf->subfontcnt==0 ) {
	if ( (sc = vw->sf->glyphs[gid])==NULL )
return;
    } else {
	sc = NULL;
	for ( k=0; k<vw->sf->subfontcnt; ++k ) {
	    if ( gid<vw->sf->subfonts[k]->glyphcnt &&
		    (sc=vw->sf->subfonts[k]->glyphs[gid])!=NULL )
	break;
	}
	if ( sc==NULL )
return;
    }

    if ( e->u.mouse.clicks==2 && e->type==et_mouseup ) {
	VWReuseCV(vw,sc);
    } else if ( e->type==et_mouseup && e->u.mouse.x<10+vw->as && skips==0 ) {
	sc->vs_open = !sc->vs_open;
	VW_Remetric(vw);
    } else if ( e->type==et_mousedown && e->u.mouse.button==3 ) {
	static int initted = false;
	if ( !initted ) {
	    int i;
	    initted = true;

	    for ( i=0; vw_popuplist[i].ti.text!=NULL || vw_popuplist[i].ti.line; ++i )
		if ( vw_popuplist[i].ti.text!=NULL )
		    vw_popuplist[i].ti.text = (unichar_t *) _( (char *)vw_popuplist[i].ti.text);

	    for (i=0; vw_subfixup[i].ti.text!=NULL || vw_subfixup[i].ti.line; ++i )
		if ( vw_subfixup[i].ti.text!=NULL )
		    vw_subfixup[i].ti.text = (unichar_t *) S_( (char *)vw_subfixup[i].ti.text);

	    for (i=0; vw_subselect[i].ti.text!=NULL || vw_subselect[i].ti.line; ++i )
		if ( vw_subselect[i].ti.text!=NULL )
		    vw_subselect[i].ti.text = (unichar_t *) S_( (char *)vw_subselect[i].ti.text);
	}
	vw_popuplist[0].ti.disabled = (sc->layers[vw->layer].validation_state&vs_opencontour)?0:1;
	vw_popuplist[1].ti.disabled = (SCValidate(sc,vw->layer,true)&vs_selfintersects)?0:1;
	vw_popuplist[2].ti.disabled = (SCValidate(sc,vw->layer,true)&vs_selfintersects)?0:1;
	vw_popuplist[3].ti.disabled = (SCValidate(sc,vw->layer,true)&vs_selfintersects) &&
		    sc->layers[vw->layer].refs!=NULL?0:1;
	vw_popuplist[4].ti.disabled = (sc->layers[vw->layer].validation_state&vs_flippedreferences)?0:1;
	vw_popuplist[5].ti.disabled = (sc->layers[vw->layer].validation_state&vs_wrongdirection)?0:1;
	vw_popuplist[6].ti.disabled = (sc->layers[vw->layer].validation_state&vs_missingextrema)?0:1;
	vw_popuplist[7].ti.disabled = (sc->layers[vw->layer].validation_state&vs_missingextrema)?0:1;
	vw_popuplist[8].ti.disabled = (sc->layers[vw->layer].validation_state&vs_toomanypoints)?0:1;
	vw->sc = sc;
	GMenuCreatePopupMenu(vw->v,e, vw_popuplist);
    }
}

static void VWDrawWindow(GWindow pixmap,struct val_data *vw, GEvent *e) {
    int gid,k, cidmax = vw->cidmax;
    SplineFont *sub, *sf=vw->sf;
    SplineChar *sc;
    int sofar;
    int bit, skips, vs, y, m;
    GRect old, r;

    Color fg = GDrawGetDefaultForeground(NULL);

    GDrawPushClip(pixmap,&e->u.expose.rect,&old);
    GDrawSetFont(pixmap,vw->font);
    gid = VW_FindLine(vw,vw->loff_top, &skips);
    if ( gid==-1 ) {
	GDrawDrawText8(pixmap,2,(vw->vlcnt-1)*vw->fh/2 + vw->as,
		vw->finished_first_pass ? _("Passed Validation") : _("Thinking..."),
		-1,fg );
	GDrawPopClip(pixmap,&old);
return;
    }

    y = vw->as - skips*vw->fh;
    sofar = -skips;
    r.width = r.height = vw->as;
    if ( gid!=-2 ) {
	for ( ; gid<cidmax && sofar<vw->vlcnt ; ++gid ) {
	    if ( sf->subfontcnt==0 )
		sc = sf->glyphs[gid];
	    else {
		for ( k=0; k<sf->subfontcnt; ++k ) {
		    sub = sf->subfonts[k];
		    if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
		break;
		}
	    }
	    /* Ignore it if it has not been validated */
	    /* Ignore it if it is good */
	    vs = VSModMask(sc,vw);
	    if ((vs&vs_known) && (vs&vw->mask)!=0 ) {
		r.x = 2;   r.y = y-vw->as+1;
		GDrawDrawRect(pixmap,&r,fg);
		GDrawDrawLine(pixmap,r.x+2,r.y+vw->as/2,r.x+vw->as-2,r.y+vw->as/2,fg);
		if ( !sc->vs_open )
		    GDrawDrawLine(pixmap,r.x+vw->as/2,r.y+2,r.x+vw->as/2,r.y+vw->as-2,fg);
		GDrawDrawText8(pixmap,r.x+r.width+2,y,sc->name,-1,fg);
		y += vw->fh;
		++sofar;
		if ( sc->vs_open ) {
		    for ( m=0, bit=(vs_known<<1) ; bit<=vs_last; ++m, bit<<=1 )
			if ( (bit&vw->mask) && (vs&bit) && vserrornames[m]!=NULL ) {
			    GDrawDrawText8(pixmap,10+r.width+r.x,y,_(vserrornames[m]),-1,
			                   GDrawGetWarningForeground(NULL) );
			    y += vw->fh;
			    ++sofar;
			}
		}
	    }
	}
    }
    if ( sofar<vw->vlcnt ) {
	vs = ValidatePrivate(sf);
	if ( !vw->needs_blue )
	    vs &= ~pds_missingblue;
	if ( vs!=0 ) {
	    /* GT: "Private" is a keyword (sort of) in PostScript. Perhaps it */
	    /* GT: should remain untranslated? */
	    GDrawDrawText8(pixmap,r.x+r.width+2,y,_("Private Dictionary"),-1,fg );
	    y += vw->fh;
	    for ( m=0, bit=1 ; bit!=0; ++m, bit<<=1 ) {
		if ( vs&bit ) {
		    GDrawDrawText8(pixmap,10+r.width+r.x,y,_(privateerrornames[m]),-1,
		                   GDrawGetWarningForeground(NULL));
		    y += vw->fh;
		}
	    }
	}
    }
    GDrawPopClip(pixmap,&old);
}

static int VWCheckup(struct val_data *vw) {
    /* Check some glyphs to see what their validation state is or if they have*/
    /*  changed */
    int gid, k, cnt=0;
    int max;
    SplineFont *sf=vw->sf, *sub;
    int cntmax = vw->finished_first_pass ? 40 : 60;
    SplineChar *sc;
    int a_change = false;
    int firstv = true;
    char *buts[4];

    if ( sf->subfontcnt==0 )
	max = vw->cidmax = sf->glyphcnt;
    else
	max = vw->cidmax;

    for ( gid=vw->laststart; gid<max && cnt<cntmax && gid<vw->laststart+2000; ++gid ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->glyphs[gid];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) {
		sub = sf->subfonts[k];
		if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
	    break;
	    }
	}
	if ( sc!=NULL && !(sc->layers[vw->layer].validation_state&vs_known)) {
	    if ( firstv ) {
		GDrawSetCursor(vw->v,ct_watch);
		GDrawSync(NULL);
		firstv = false;
	    }
	    SCValidate(sc,vw->layer,true);
	    ++cnt;
	}
	if ( sc!=NULL && vw->need_to_check_with_user_on_mask &&
		(sc->layers[vw->layer].validation_state&vs_nonintegral )) {
	    vw->need_to_check_with_user_on_mask = false;
	    buts[0] = _("Report as Error"); buts[1]=_("Ignore"); buts[2] = NULL;
	    if ( ff_ask(_("Not sure if this is an error..."),(const char **) buts,0,1,
		    _("This font contains non-integral coordinates. That's OK\n"
			"in PostScript and SVG but causes problems in TrueType.\n"
			"Should I consider that an error here?"))==0 ) {
		a_change = true;
		vw->mask = (enum validation_state) (vw->mask | vs_nonintegral);
	    }
	}
	if ( sc!=NULL && sc->layers[vw->layer].validation_state!=sc->layers[vw->layer].old_vs ) {
	    a_change = true;
	    sc->layers[vw->layer].old_vs = sc->layers[vw->layer].validation_state;
	}
    }
    if ( gid<max )
	vw->laststart = gid;
    else {
	vw->laststart = 0;
	if ( !vw->finished_first_pass ) {
	    vw->finished_first_pass = true;
	    GDrawCancelTimer(vw->recheck);
	    /* Check less frequently now we've completed a full scan */
	    vw->recheck = GDrawRequestTimer(vw->v,3000,3000,NULL);
	}
    }
    if ( a_change )
	VW_Remetric(vw);
    if ( !firstv )
	GDrawSetCursor(vw->v,ct_mypointer);
return( a_change );
}

static int VW_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct val_data *vw = (struct val_data *) GDrawGetUserData(GGadgetGetWindow(g));
	GDrawDestroyWindow(vw->gw);
    }
return( true );
}

static int vwv_e_h(GWindow gw, GEvent *event) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	if ( vw->recheck==NULL ) {
	    vw->recheck = GDrawRequestTimer(vw->v,500,500,NULL);
	}
	VWDrawWindow(gw,vw,event);
      break;
      case et_mouseup:
      case et_mousedown:
      case et_mousemove:
	if (( event->type==et_mouseup || event->type==et_mousedown ) &&
		(event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(vw->vsb,event));
	}
	VWMouse(vw,event);
      break;
      case et_char:
return( false );
      break;
      case et_resize: {
	int vlcnt = event->u.resize.size.height/vw->fh;
	vw->vlcnt = vlcnt;
	VW_SetSb(vw);
	GDrawRequestExpose(vw->v,NULL,false);
      } break;
      case et_timer:
	VWCheckup(vw);
      break;
      default: break;
    }
return( true );
}

static int vw_e_h(GWindow gw, GEvent *event) {
    struct val_data *vw = (struct val_data *) GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	GDrawDestroyWindow(gw);
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_destroy ) {
	if ( vw->sf!=NULL )
	    vw->sf->valwin = NULL;
	chunkfree(vw,sizeof(*vw));
    }
return( true );
}

void SFValidationWindow(SplineFont *sf,int layer,enum fontformat format) {
    GWindowAttrs wattrs;
    GRect pos;
    GWindow gw;
    GGadgetCreateData gcd[4], boxes[4], *harray[4], *butarray[8], *varray[3];
    GTextInfo label[4];
    struct val_data *valwin;
    char buffer[200];
    int k, gid;
    int cidmax;
    SplineFont *sub;
    SplineChar *sc;
    int as, ds, ld;
    int needs_blue;
    enum validation_state mask;

    if ( sf->cidmaster )
	sf = sf->cidmaster;
    mask = VSMaskFromFormat(sf,layer,format);
    needs_blue = (mask==vs_maskps || mask==vs_maskcid);

    if ( sf->valwin!=NULL ) {
	/* Don't need to force a revalidation because if the window exists */
	/*  it's been doing that all by itself */
	if ( mask!=(sf->valwin->mask&~vs_nonintegral) ) {
	    /* But if we go from postscript to truetype the types of errors */
	    /*  change, so what we display might be different */
	    sf->valwin->mask = mask;
	    sf->valwin->needs_blue = needs_blue;
	    sf->valwin->layer = layer;
	    VW_Remetric(sf->valwin);
	}
	GDrawSetVisible(sf->valwin->gw,true);
	GDrawRaise(sf->valwin->gw);
return;
    }

    if ( sf->subfontcnt!=0 ) {
	cidmax = 0;
	for ( k=0; k<sf->subfontcnt; ++k )
	    if ( sf->subfonts[k]->glyphcnt > cidmax )
		cidmax = sf->subfonts[k]->glyphcnt;
    } else
	cidmax = sf->glyphcnt;

    /* Init all glyphs as undrawn */
    for ( gid=0; gid<cidmax ; ++gid ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->glyphs[gid];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) {
		sub = sf->subfonts[k];
		if ( gid<sub->glyphcnt && (sc = sub->glyphs[gid])!=NULL )
	    break;
	    }
	}
	if ( sc!=NULL ) {
	    sc->layers[layer].old_vs = 0;
	    sc->vs_open = true;		/* should this default to false? */
	}
    }

    valwin = (struct val_data *)chunkalloc(sizeof(struct val_data));
    valwin->sf = sf;
    valwin->mask = mask;
    valwin->needs_blue = needs_blue;
    valwin->cidmax = cidmax;
    valwin->lastgid = -1;
    valwin->layer = layer;
    valwin->need_to_check_with_user_on_mask = (format==ff_none && !sf->layers[layer].order2 );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = (enum window_attr_mask)(wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg);
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    sprintf( buffer, _("Validation of %.100s"), sf->fontname );
    wattrs.utf8_window_title = buffer;
    wattrs.is_dlg = true;
    wattrs.undercursor = 1;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,300);
    valwin->gw = gw = GDrawCreateTopWindow(NULL,&pos,vw_e_h,valwin,&wattrs);

    valwin->font = validate_font.fi;
    GDrawWindowFontMetrics(valwin->gw,valwin->font,&as,&ds,&ld);
    valwin->fh = as+ds;
    valwin->as = as;

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled);
    gcd[k].gd.u.drawable_e_h = vwv_e_h;
    gcd[k++].creator = GDrawableCreate;

    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_sb_vert);
    gcd[k].gd.handle_controlevent = VW_VScroll;
    gcd[k++].creator = GScrollBarCreate;
    harray[0] = &gcd[k-2]; harray[1] = &gcd[k-1]; harray[2] = NULL; harray[3] = NULL;

    gcd[k].gd.flags = (enum gg_flags) (gg_visible | gg_enabled | gg_but_default);
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = VW_OK;
    gcd[k++].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = &gcd[k-1]; butarray[2] = GCD_Glue; butarray[3] = NULL;

    boxes[2].gd.flags = (enum gg_flags) (gg_enabled|gg_visible);
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHVGroupCreate;

    boxes[3].gd.flags = (enum gg_flags) (gg_enabled|gg_visible);
    boxes[3].gd.u.boxelements = butarray;
    boxes[3].creator = GHBoxCreate;
    varray[0] = &boxes[2]; varray[1] = &boxes[3]; varray[2] = NULL;

    boxes[0].gd.flags = (enum gg_flags) (gg_enabled|gg_visible);
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);
    valwin->vsb = gcd[1].ret;
    valwin->v = GDrawableGetWindow(gcd[0].ret);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,0);
    GHVBoxSetPadding(boxes[2].ret,0,0);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
}

void ValidationDestroy(SplineFont *sf) {
    if ( sf->valwin!=NULL ) {
	sf->valwin->sf = NULL;
	GDrawDestroyWindow(sf->valwin->gw);
	sf->valwin = NULL;
    }
}
