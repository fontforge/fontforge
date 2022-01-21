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

#include "cvundoes.h"
#include "dlist.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "lookups.h"
#include "parsettf.h"
#include "spiro.h"
#include "splineorder2.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

#define RAD2DEG	(180/FF_PI)
#define TCnt	3

typedef struct gidata {
    struct dlistnode ln;
    CharView *cv;
    SplineChar *sc;
    RefChar *rf;
    ImageList *img;
    AnchorPoint *ap;
    SplinePoint *cursp;
    spiro_cp *curcp;
    SplinePointList *curspl;
    SplinePointList *oldstate;
    AnchorPoint *oldaps;
    GWindow gw;
    int done, first, changed;
    int prevchanged, nextchanged;
    int normal_start, normal_end;
    int interp_start, interp_end;
    GGadgetCreateData* gcd;
    GGadget *group1ret, *group2ret;
} GIData;

#define CID_BaseX	2001
#define CID_BaseY	2002
#define CID_NextXOff	2003
#define CID_NextYOff	2004
#define CID_NextPos	2005
#define CID_PrevXOff	2006
#define CID_PrevYOff	2007
#define CID_PrevPos	2008
#define CID_NextDef	2009
#define CID_PrevDef	2010
#define CID_NextR	2014
#define CID_NextTheta	2015
#define CID_PrevR	2016
#define CID_PrevTheta	2017
#define CID_HintMask	2020
#define CID_ActiveHints	2030
#define CID_NextX	2031
#define CID_NextY	2032
#define CID_PrevX	2033
#define CID_PrevY	2034
#define CID_BasePos	2035
#define CID_Normal	2036
#define CID_Interpolated 2037
#define CID_NeverInterpolate	2038
/* Also use CID_Next, CID_Prev below */
#define CID_NextC	2041
#define CID_PrevC	2042
#define CID_PrevCurvature	2043
#define CID_NextCurvature	2044
#define CID_DeltaCurvature	2045
#define CID_Curve	2050		/* Next four must be in order */
#define CID_Corner	2051
#define CID_Tangent	2052
#define CID_HVCurve	2053
#define CID_SpiroLeft	2054
#define CID_SpiroRight	2055
#define CID_TabSet	2100

#define CID_X		3001
#define CID_Y		3002
#define CID_NameList	3003
#define CID_Mark	3004
#define CID_BaseChar	3005
#define CID_BaseLig	3006
#define CID_BaseMark	3007
#define CID_CursEntry	3008
#define CID_CursExit	3009
#define CID_LigIndex	3010
#define CID_Next	3011
#define CID_Prev	3012
#define CID_Delete	3013
#define CID_New		3014
#define CID_MatchPt	3015

#define RI_Width	225
#define RI_Height	246
#define CID_Match_Pt_Base	1010
#define CID_Match_Pt_Ref	1011

#define II_Width	130
#define II_Height	70

#define PI_Width	228
#define PI_Height	434

#define AI_Width	160
#define AI_Height	258

static int GI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
    }
return( true );
}

static int GI_TransChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->changed = true;
    }
return( true );
}

static int GI_MatchPtChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *t1 = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Match_Pt_Base));
	const unichar_t *t2 = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Match_Pt_Ref));
	while ( *t1==' ' ) ++t1;
	while ( *t2==' ' ) ++t2;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,1004),*t1=='\0' && *t2=='\0' );
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,1005),*t1=='\0' && *t2=='\0' );
	if ( isdigit(*t1) && isdigit(*t2)) {
	    BasePoint inbase, inref;
	    int basept, refpt;
	    basept = u_strtol(t1,NULL,10);
	    refpt = u_strtol(t2,NULL,10);
	    if ( ttfFindPointInSC(ci->cv->b.sc,CVLayer((CharViewBase *) ci->cv),basept,&inbase,ci->rf)==-1 &&
		    ttfFindPointInSC(ci->rf->sc,CVLayer((CharViewBase *) ci->cv),refpt,&inref,NULL)==-1 ) {
		char buffer[40];
		sprintf(buffer,"%g",(double) (inbase.x-inref.x));
		GGadgetSetTitle8(GWidgetGetControl(ci->gw,1004),buffer);
		sprintf(buffer,"%g",(double) (inbase.y-inref.y));
		GGadgetSetTitle8(GWidgetGetControl(ci->gw,1005),buffer);
	    }
	}
    }
return( true );
}

static int GI_ROK_Do(GIData *ci) {
    int errs=false,i;
    real trans[6];
    SplinePointList *spl, *new;
    RefChar *ref = ci->rf, *subref;
    int usemy = GGadgetIsChecked(GWidgetGetControl(ci->gw,6+1000));
    int round = GGadgetIsChecked(GWidgetGetControl(ci->gw,7+1000));
    int basept=-1, refpt=-1;
    BasePoint inbase, inref;

    for ( i=0; i<6; ++i ) {
	trans[i] = GetReal8(ci->gw,1000+i,_("Transformation Matrix"),&errs);
	if ( !errs &&
		((i<4 && (trans[i]>30 || trans[i]<-30)) ||
		 (i>=4 && (trans[i]>16000 || trans[i]<-16000))) ) {
	    /* Don't want the user to insert an enormous scale factor or */
	    /*  it will move points outside the legal range. */
	    GTextFieldSelect(GWidgetGetControl(ci->gw,1000+i),0,-1);
	    ff_post_error(_("Value out of range"),_("Value out of range"));
	    errs = true;
	}
	if ( errs )
return( false );
    }
    if ( !ci->cv->b.sc->layers[ly_fore].order2 )
	/* No point matching */;
    else {
	const unichar_t *txt;
	txt = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Match_Pt_Base));
	while ( isspace(*txt)) ++txt;
	if ( *txt!='\0' )
	    basept = GetInt8(ci->gw,CID_Match_Pt_Base,_("_Base:"),&errs);
	txt = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Match_Pt_Ref));
	while ( isspace(*txt)) ++txt;
	if ( *txt!='\0' )
	    refpt = GetInt8(ci->gw,CID_Match_Pt_Ref,_("Ref:"),&errs);
	if ( errs )
return( false );
	if ( (basept!=-1) ^ (refpt!=-1) ) {
	    ff_post_error(_("Bad Point Match"),_("Both points must be specified, or neither"));
	}
	if ( basept!=-1 ) {
	    if ( ttfFindPointInSC(ci->cv->b.sc,CVLayer((CharViewBase *) ci->cv),basept,&inbase,ci->rf)!=-1 ) {
		ff_post_error(_("Bad Point Match"),_("Couldn't find base point"));
return( false );
	    } else if ( ttfFindPointInSC(ci->rf->sc,CVLayer((CharViewBase *) ci->cv),refpt,&inref,NULL)!=-1 ) {
		ff_post_error(_("Bad Point Match"),_("Couldn't find point in reference"));
return( false );
	    }
	    /* Override user specified value */
	    trans[4] = inbase.x-inref.x;
	    trans[5] = inbase.y-inref.y;
	}
    }

    for ( i=0; i<6 && ref->transform[i]==trans[i]; ++i );
    if ( i==6 &&
	    usemy==ref->use_my_metrics &&
	    round==ref->round_translation_to_grid &&
	    (basept!=-1)==ref->point_match &&
	    (basept==-1 ||
		(ref->match_pt_base==basept && ref->match_pt_ref==refpt))) {
	ref->point_match_out_of_date = false;
return( true );		/* Didn't really change */
    }

    for ( i=0; i<6; ++i )
	ref->transform[i] = trans[i];
    SplinePointListsFree(ref->layers[0].splines);
    ref->layers[0].splines = SplinePointListTransform(SplinePointListCopy(ref->sc->layers[ly_fore].splines),trans,tpt_AllPoints);
    spl = NULL;
    if ( ref->layers[0].splines!=NULL )
	for ( spl = ref->layers[0].splines; spl->next!=NULL; spl = spl->next );
    for ( subref = ref->sc->layers[ly_fore].refs; subref!=NULL; subref=subref->next ) {
	new = SplinePointListTransform(SplinePointListCopy(subref->layers[0].splines),trans,tpt_AllPoints);
	if ( spl==NULL )
	    ref->layers[0].splines = new;
	else
	    spl->next = new;
	if ( new!=NULL )
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
    }
    ref->use_my_metrics = usemy;
    ref->round_translation_to_grid = round;
    ref->point_match = basept!=-1;
    ref->match_pt_base = basept; ref->match_pt_ref = refpt;
    ref->point_match_out_of_date = false;

    SplineSetFindBounds(ref->layers[0].splines,&ref->bb);
    CVCharChangedUpdate(&ci->cv->b);
return( true );
}

static int GI_ROK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GI_ROK_Do(ci))
	    ci->done = true;
    }
return( true );
}

static int GI_Show(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( ci->changed ) {
	    char *buts[4];
	    int ans;
	    buts[0] = _("C_hange");
	    buts[1] = _("_Retain");
	    buts[2] = _("_Cancel");
	    buts[3] = NULL;
	    ans = gwwv_ask(_("Transformation Matrix Changed"),(const char **)buts,0,2,_("You have changed the transformation matrix, do you wish to use the new version?"));
	    if ( ans==2 )
return( true );
	    else if ( ans==0 ) {
		if ( !GI_ROK_Do(ci))
return( true );
	    }
	}
	ci->done = true;
	CharViewCreate(ci->rf->sc,(FontView *) (ci->cv->b.fv),-1);
    }
return( true );
}

static int gi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GIData *ci = GDrawGetUserData(gw);
	ci->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/getinfo.html", NULL);
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static void RefGetInfo(CharView *cv, RefChar *ref) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[33], boxes[7];
    GGadgetCreateData *varray[19], *hvarray[16], *harray1[6], *harray2[4],
	    *harray3[7], *hvarray2[4][6];
    GTextInfo label[33];
    char tbuf[6][40], bbbuf[4][40];
    char basebuf[20], refbuf[20];
    char namebuf[100];
    char ubuf[40];
    int i,j,l;

    gi.cv = cv;
    gi.sc = cv->b.sc;
    gi.rf = ref;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Reference Info");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,RI_Width));
	pos.height = GDrawPointsToPixels(NULL,
		ref->sc->unicodeenc!=-1?RI_Height+12:RI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,gi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	memset(&boxes,0,sizeof(boxes));

	snprintf( namebuf, sizeof(namebuf),
		_("Reference to character %1$.20s at %2$d"),
		ref->sc->name, (int) cv->b.fv->map->backmap[ref->sc->orig_pos]);
	label[0].text = (unichar_t *) namebuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	l = 0;
	varray[l++] = &gcd[0];
	j = 1;

	if ( ref->sc->unicodeenc!=-1 ) {
	    sprintf( ubuf, " Unicode: U+%04x", ref->sc->unicodeenc );
	    label[1].text = (unichar_t *) ubuf;
	    label[1].text_is_1byte = true;
	    gcd[1].gd.label = &label[1];
	    gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 17;
	    gcd[1].gd.flags = gg_enabled|gg_visible;
	    gcd[1].creator = GLabelCreate;
	    varray[l++] = &gcd[1];
	    j=2;
	}

	label[j].text = (unichar_t *) _("Transformed by:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.popup_msg = _("The transformation matrix specifies how the points in\nthe source glyph should be transformed before\nthey are drawn in the current glyph.\n x(new) = tm[1,1]*x + tm[2,1]*y + tm[3,1]\n y(new) = tm[1,2]*x + tm[2,2]*y + tm[3,2]");
	gcd[j].creator = GLabelCreate;
	varray[l++] = &gcd[j];
	++j;

	for ( i=0; i<6; ++i ) {
	    if ( !(i&1) ) hvarray[5*(i/2)] = GCD_Glue;
	    sprintf(tbuf[i],"%g", (double) ref->transform[i]);
	    label[i+j].text = (unichar_t *) tbuf[i];
	    label[i+j].text_is_1byte = true;
	    gcd[i+j].gd.label = &label[i+j];
	    gcd[i+j].gd.pos.x = 20+((i&1)?85:0); gcd[i+j].gd.pos.width=75;
	    gcd[i+j].gd.pos.y = gcd[j-1].gd.pos.y+14+(i/2)*26;
	    gcd[i+j].gd.flags = gg_enabled|gg_visible;
	    gcd[i+j].gd.cid = i+1000;
	    gcd[i+j].gd.handle_controlevent = GI_TransChange;
	    gcd[i+j].creator = (i>=4 ? GNumericFieldCreate : GTextFieldCreate);
	    hvarray[5*(i/2)+1+(i&1)] = &gcd[i+j];
	    if ( (i&1) ) { hvarray[5*(i/2)+3] = GCD_Glue; hvarray[5*(i/2)+4] = NULL; }
	}
	if ( ref->point_match )
	    gcd[4+j].gd.flags = gcd[5+j].gd.flags = gg_visible;
	hvarray[15] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = hvarray;
	boxes[2].creator = GHVBoxCreate;
	varray[l++] = &boxes[2];

	label[6+j].text = (unichar_t *) _("_Use My Metrics");
	label[6+j].text_in_resource = true;
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.pos.x = 5; gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+21;
	gcd[6+j].gd.flags = gg_enabled|gg_visible| (ref->use_my_metrics?gg_cb_on:0);
	gcd[i+j].gd.cid = 6+1000;
	gcd[6+j].gd.popup_msg = _("Only relevant in a truetype font, this flag indicates that the width\nof the composite glyph should be the same as the width of this reference.");
	varray[l++] = &gcd[6+j];
	gcd[6+j++].creator = GCheckBoxCreate;

	label[6+j].text = (unichar_t *) _("_Round To Grid");
	label[6+j].text_in_resource = true;
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.pos.x = 5; gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+14;
	gcd[6+j].gd.flags = gg_enabled|gg_visible| (ref->round_translation_to_grid?gg_cb_on:0);
	gcd[i+j].gd.cid = 7+1000;
	gcd[6+j].gd.popup_msg = _("Only relevant in a truetype font, this flag indicates that if the reference\nis translated, then the translation should be rounded during grid fitting.");
	varray[l++] = &gcd[6+j];
	gcd[6+j++].creator = GCheckBoxCreate;

	label[6+j].text = (unichar_t *) _("TrueType Point _Matching:");
	label[6+j].text_in_resource = true;
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.pos.x = 5; gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+17;
	gcd[6+j].gd.flags = cv->b.sc->layers[ly_fore].order2 ? (gg_enabled|gg_visible) : (gg_visible);
	gcd[6+j].gd.popup_msg = _("Only relevant in a truetype font, this flag indicates that this\nreference should not be translated normally, but rather its position\nshould be determined by moving the reference so that the indicated\npoint in the reference falls on top of the indicated point in the base\ncharacter.");
	varray[l++] = &gcd[6+j];
	gcd[6+j++].creator = GLabelCreate;

	label[6+j].text = (unichar_t *) _("_Base:");
	label[6+j].text_is_1byte = true;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.pos.x = 8; gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+19;
	gcd[6+j].gd.flags = cv->b.sc->layers[ly_fore].order2 ? (gg_enabled|gg_visible) : (gg_visible);
	gcd[6+j].gd.popup_msg = _("Only relevant in a truetype font, this flag indicates that this\nreference should not be translated normally, but rather its position\nshould be determined by moving the reference so that the indicated\npoint in the reference falls on top of the indicated point in the base\ncharacter.");
	harray1[0] = &gcd[6+j];
	gcd[6+j++].creator = GLabelCreate;

	if ( ref->point_match ) {
	    sprintf(basebuf,"%d", ref->match_pt_base);
	    label[6+j].text = (unichar_t *) basebuf;
	    label[6+j].text_is_1byte = true;
	    gcd[6+j].gd.label = &label[6+j];
	}
	gcd[6+j].gd.pos.x = 40; gcd[6+j].gd.pos.width=50;
	gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y-4;
	gcd[6+j].gd.flags = gcd[6+j-1].gd.flags;
	gcd[6+j].gd.cid = CID_Match_Pt_Base;
	gcd[6+j].gd.handle_controlevent = GI_MatchPtChange;
	harray1[1] = &gcd[6+j];
	gcd[6+j++].creator = GTextFieldCreate;

	label[6+j].text = (unichar_t *) _("Ref:");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.pos.x = 95; gcd[6+j].gd.pos.y = gcd[6+j-2].gd.pos.y;
	gcd[6+j].gd.flags = gcd[6+j-1].gd.flags;
	gcd[6+j].gd.popup_msg = _("Only relevant in a truetype font, this flag indicates that this\nreference should not be translated normally, but rather its position\nshould be determined by moving the reference so that the indicated\npoint in the reference falls on top of the indicated point in the base\ncharacter.");
	harray1[2] = &gcd[6+j];
	gcd[6+j++].creator = GLabelCreate;

	if ( ref->point_match ) {
	    sprintf(refbuf,"%d", ref->match_pt_ref);
	    label[6+j].text = (unichar_t *) refbuf;
	    label[6+j].text_is_1byte = true;
	    gcd[6+j].gd.label = &label[6+j];
	}
	gcd[6+j].gd.pos.x = 127; gcd[6+j].gd.pos.width=50;
	gcd[6+j].gd.pos.y = gcd[6+j-2].gd.pos.y;
	gcd[6+j].gd.flags = gcd[6+j-1].gd.flags;
	gcd[6+j].gd.cid = CID_Match_Pt_Ref;
	gcd[6+j].gd.handle_controlevent = GI_MatchPtChange;
	harray1[3] = &gcd[6+j];
	gcd[6+j++].creator = GTextFieldCreate;
	harray1[4] = GCD_Glue; harray1[5] = NULL;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray1;
	boxes[3].creator = GHBoxCreate;
	varray[l++] = &boxes[3];

	varray[l++] = GCD_Glue;

	gcd[6+j].gd.pos.x = 5; gcd[6+j].gd.pos.y = RI_Height+(j==10?12:0)-70;
	gcd[6+j].gd.pos.width = RI_Width-10;
	gcd[6+j].gd.flags = gg_visible | gg_enabled;
	varray[l++] = &gcd[6+j];
	gcd[6+j++].creator = GLineCreate;

	label[6+j].text = (unichar_t *) _("Bounding Box:");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.flags = gg_enabled|gg_visible;
	gcd[6+j].creator = GLabelCreate;
	varray[l++] = &gcd[6+j];
	++j;

	hvarray2[0][0] = GCD_Glue; hvarray2[0][1] = GCD_Glue;

	label[6+j].text = (unichar_t *) _("Min");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.flags = gg_enabled|gg_visible;
	gcd[6+j].creator = GLabelCreate;
	hvarray2[0][2] = &gcd[6+j++];

	label[6+j].text = (unichar_t *) _("Max");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.flags = gg_enabled|gg_visible;
	gcd[6+j].creator = GLabelCreate;
	hvarray2[0][3] = &gcd[6+j++]; hvarray2[0][4] = GCD_Glue; hvarray2[0][5] = NULL;

	hvarray2[1][0] = hvarray2[1][4] = GCD_Glue; hvarray2[1][5] = NULL;
	hvarray2[2][0] = hvarray2[2][4] = GCD_Glue; hvarray2[2][5] = NULL;

	label[6+j].text = (unichar_t *) _("X:");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.flags = gg_enabled|gg_visible;
	gcd[6+j].creator = GLabelCreate;
	hvarray2[1][1] = &gcd[6+j++];

	label[6+j].text = (unichar_t *) _("Y:");
	label[6+j].text_is_1byte = true;
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.flags = gg_enabled|gg_visible;
	gcd[6+j].creator = GLabelCreate;
	hvarray2[2][1] = &gcd[6+j++];

	for ( i=0; i<4; ++i ) {
	    sprintf(bbbuf[i],"%g", (double) ((&ref->bb.minx)[i]));
	    label[6+j].text = (unichar_t *) bbbuf[i];
	    label[6+j].text_is_1byte = true;
	    gcd[6+j].gd.label = &label[6+j];
	    gcd[6+j].gd.flags = gg_enabled|gg_visible;
	    gcd[6+j].creator = GLabelCreate;
	    hvarray2[1+i/2][2+(i&1)] = &gcd[6+j++];
	}
	hvarray2[3][0] = NULL;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = hvarray2[0];
	boxes[4].creator = GHVBoxCreate;
	varray[l++] = &boxes[4];

	gcd[6+j].gd.pos.x = 5; gcd[6+j].gd.pos.y = RI_Height+(j==10?12:0)-70;
	gcd[6+j].gd.pos.width = RI_Width-10;
	gcd[6+j].gd.flags = gg_visible | gg_enabled;
	varray[l++] = &gcd[6+j];
	gcd[6+j++].creator = GLineCreate;

	gcd[6+j].gd.pos.x = (RI_Width-GIntGetResource(_NUM_Buttonsize))/2;
	gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+6;
	gcd[6+j].gd.pos.width = -1; gcd[6+j].gd.pos.height = 0;
	gcd[6+j].gd.flags = gg_visible | gg_enabled ;
	label[6+j].text = (unichar_t *) _("_Show");
	label[6+j].text_is_1byte = true;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.mnemonic = 'S';
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.handle_controlevent = GI_Show;
	harray2[0] = GCD_Glue; harray2[1] = &gcd[6+j]; harray2[2] = GCD_Glue; harray2[3] = NULL;
	gcd[6+j++].creator = GButtonCreate;

	boxes[5].gd.flags = gg_enabled|gg_visible;
	boxes[5].gd.u.boxelements = harray2;
	boxes[5].creator = GHBoxCreate;
	varray[l++] = &boxes[5];

	gcd[6+j] = gcd[6+j-2];
	varray[l++] = &gcd[6+j++];

	gcd[6+j].gd.pos.x = 30-3; gcd[6+j].gd.pos.y = RI_Height+(j==13?12:0)-30-3;
	gcd[6+j].gd.pos.width = -1; gcd[6+j].gd.pos.height = 0;
	gcd[6+j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[6+j].text = (unichar_t *) _("_OK");
	label[6+j].text_is_1byte = true;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.mnemonic = 'O';
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.handle_controlevent = GI_ROK;
	gcd[6+j++].creator = GButtonCreate;

	gcd[6+j].gd.pos.x = -30; gcd[6+j].gd.pos.y = gcd[6+j-1].gd.pos.y+3;
	gcd[6+j].gd.pos.width = -1; gcd[6+j].gd.pos.height = 0;
	gcd[6+j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[6+j].text = (unichar_t *) _("_Cancel");
	label[6+j].text_is_1byte = true;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.mnemonic = 'C';
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.handle_controlevent = GI_Cancel;
	gcd[6+j].creator = GButtonCreate;

	harray3[0] = GCD_Glue; harray3[1] = &gcd[6+j-1]; harray3[2] = GCD_Glue;
	 harray3[3] = GCD_Glue; harray3[4] = &gcd[6+j]; harray3[5] = GCD_Glue;
	 harray3[6] = NULL;
	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = harray3;
	boxes[6].creator = GHBoxCreate;
	varray[l++] = &boxes[6];
	varray[l] = NULL;

	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GVBoxCreate;

	GGadgetsCreate(gi.gw,boxes);
	GWidgetIndicateFocusGadget(gcd[j].ret);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetPadding(boxes[3].ret,6,2);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static void ImgGetInfo(CharView *cv, ImageList *img) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12], boxes[3], *varray[11], *harray[6];
    GTextInfo label[12];
    char posbuf[100], scalebuf[100], sizebuf[100];
    struct _GImage *base = img->image->list_len==0?
	    img->image->u.image:img->image->u.images[0];

    gi.cv = cv;
    gi.sc = cv->b.sc;
    gi.img = img;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Image Info");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,II_Width));
	pos.height = GDrawPointsToPixels(NULL,II_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,gi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	sprintf( posbuf, _("Image at:      (%.0f,%.0f)"), (double) img->xoff,
		(double) (img->yoff-GImageGetHeight(img->image)*img->yscale));
	label[0].text = (unichar_t *) posbuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	varray[0] = &gcd[0]; varray[1] = NULL;

	sprintf( scalebuf, _("Scaled by:    (%.2f,%.2f)"), (double) img->xscale, (double) img->yscale );
	label[1].text = (unichar_t *) scalebuf;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 19;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;
	varray[2] = &gcd[1]; varray[3] = NULL;

	sprintf( sizebuf, _("Image Size:  %d x %d  pixels"), (int) base->width, (int) base->height );
	label[2].text = (unichar_t *) sizebuf;
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 19;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	varray[4] = &gcd[2]; varray[5] = NULL;
	varray[6] = GCD_Glue; varray[7] = NULL;

	gcd[3].gd.pos.x = (II_Width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-6)/2; gcd[3].gd.pos.y = II_Height-32-3;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
	label[3].text = (unichar_t *) _("_OK");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.mnemonic = 'O';
	gcd[3].gd.label = &label[3];
	gcd[3].gd.handle_controlevent = GI_Cancel;
	gcd[3].creator = GButtonCreate;
	harray[0] = GCD_Glue; harray[1] = &gcd[3]; harray[2] = GCD_Glue; harray[3] = NULL;
	varray[8] = &boxes[2]; varray[9] = NULL;
	varray[10] = NULL;

	memset(boxes,0,sizeof(boxes));
	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;

	GGadgetsCreate(gi.gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static AnchorClass *_AnchorClassUnused(SplineChar *sc,int *waslig, int classmatch) {
    AnchorClass *an, *maybe;
    int val, maybelig;
    SplineFont *sf;
    int ismarkglyph, isligatureglyph;
    PST *pst;
    /* Are there any anchors with this name? If so can't reuse it */
    /*  unless they are ligature anchors */
    /*  or 'curs' anchors, which allow exactly two points (entry, exit) */
    /*  or 'mkmk' anchors, which allow a mark to be both a base and an attach */

    ismarkglyph = (sc->width==0) || sc->glyph_class==(3+1) ||
	    ( sc->unicodeenc!=-1 && iscombining(sc->unicodeenc)) ||
	    ( sc->anchor!=NULL && (sc->anchor->type == at_mark || sc->anchor->type == at_basemark));
    for ( pst = sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
    isligatureglyph = (pst!=NULL) || sc->glyph_class==2+1;

    *waslig = false; maybelig = false; maybe = NULL;
    sf = sc->parent;
    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    for ( an=sf->anchor; an!=NULL; an=an->next ) {
	if ( classmatch ) {
	    if (( an->type == act_mklg && !isligatureglyph && !ismarkglyph ) ||
		    ( an->type == act_mkmk && !ismarkglyph ))
    continue;
	}
	val = IsAnchorClassUsed(sc,an);
	if ( val>=0 ) {
	    *waslig = val;
return( an );
	} else if ( val!=-1 && maybe==NULL ) {
	    maybe = an;
	    maybelig = val;
	}
    }
    *waslig = maybelig;
return( maybe );
}

AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig) {
return( _AnchorClassUnused(sc,waslig,false));
}

static AnchorPoint *AnchorPointNew(CharView *cv) {
    AnchorClass *an;
    AnchorPoint *ap;
    int waslig;
    SplineChar *sc = cv->b.sc;
    PST *pst;

    an = _AnchorClassUnused(sc,&waslig,true);
    if ( an==NULL )
	an = AnchorClassUnused(sc,&waslig);

    if ( an==NULL )
return(NULL);
    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = an;
    ap->me.x = cv->p.cx; /* cv->p.cx = 0; */
    ap->me.y = cv->p.cy; /* cv->p.cy = 0; */
    ap->type = an->type==act_mark ? at_basechar :
		an->type==act_mkmk ? at_basemark :
		an->type==act_mklg ? at_baselig :
		an->type==act_curs ? at_centry :
                                     at_basechar;
    for ( pst = cv->b.sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
    if ( waslig<-1 && an->type==act_mkmk ) {
	ap->type = waslig==-2 ? at_basemark : at_mark;
    } else if ( waslig==-2  && an->type==act_curs )
	ap->type = at_cexit;
    else if ( waslig==-3 || an->type==act_curs )
	ap->type = at_centry;
    else if (( sc->unicodeenc!=-1 && iscombining(sc->unicodeenc)) ||
	    sc->width==0 || sc->glyph_class==(3+1) /* mark class+1 */)
	ap->type = at_mark;
    else if ( an->type==act_mkmk )
	ap->type = at_basemark;
    else if (( pst!=NULL || waslig || sc->glyph_class==2+1) && an->type==act_mklg )
	ap->type = at_baselig;
    if (( ap->type==at_basechar || ap->type==at_baselig ) && an->type==act_mkmk )
	ap->type = at_basemark;
    ap->next = sc->anchor;
    if ( waslig>=0 )
	ap->lig_index = waslig;
    sc->anchor = ap;
return( ap );
}

static void AI_SelectList(GIData *ci,AnchorPoint *ap) {
    int i;
    AnchorClass *an;
    SplineFont *sf = ci->sc->parent;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    for ( i=0, an=sf->anchor; an!=ap->anchor; ++i, an=an->next );
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_NameList),i);
}

static void AI_DisplayClass(GIData *ci,AnchorPoint *ap) {
    AnchorClass *ac = ap->anchor;
    AnchorPoint *aps;
    int saw[at_max];

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),ac->type==act_mark || ac->type==act_unknown);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseLig),ac->type==act_mklg);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),ac->type==act_mkmk);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),ac->type==act_curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),ac->type==act_curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),ac->type!=act_curs);

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);

    if ( ac->type==act_mkmk && (ap->type==at_basechar || ap->type==at_baselig)) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
	ap->type = at_basemark;
    } else if ( ac->type==act_mark && ap->type==at_basemark ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
	ap->type = at_basechar;
    } else if ( ac->type==act_curs && ap->type!=at_centry && ap->type!=at_cexit ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursEntry),true);
	ap->type = at_centry;
    }

    memset(saw,0,sizeof(saw));
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) if ( aps!=ap ) {
	if ( aps->anchor==ac ) saw[aps->type] = true;
    }
    if ( ac->type==act_curs ) {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),!saw[at_centry]);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),!saw[at_cexit]);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),!saw[at_mark]);
	if ( saw[at_basechar]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),false);
	if ( saw[at_basemark]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),false);
    }
}

static void AI_DisplayIndex(GIData *ci,AnchorPoint *ap) {
    char buffer[12];

    sprintf(buffer,"%d", ap->lig_index );

    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_LigIndex),buffer);
}

static void AI_DisplayRadio(GIData *ci,enum anchor_type type) {
    switch ( type ) {
      case at_mark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Mark),true);
      break;
      case at_basechar:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
      break;
      case at_baselig:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseLig),true);
      break;
      case at_basemark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
      break;
      case at_centry:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursEntry),true);
      break;
      case at_cexit:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursExit),true);
      break;
      case at_max: break;
    }
}

static void AI_Display(GIData *ci,AnchorPoint *ap) {
    char val[40];
    unichar_t uval[40];
    AnchorPoint *aps;

    if ( ap==NULL) {
	SCUpdateAll(ci->sc);
return;
    }

    ci->ap = ap;
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next )
	aps->selected = false;
    ap->selected = true;
    sprintf(val,"%g",(double) ap->me.x);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_X),uval);
    sprintf(val,"%g",(double) ap->me.y);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Y),uval);
    sprintf(val,"%d",ap->type==at_baselig?ap->lig_index:0);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigIndex),uval);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Next),ap->next!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Prev),ci->sc->anchor!=ap);
    if ( ap->has_ttf_pt )
	sprintf(val,"%d",ap->ttf_pt_index);
    else
	val[0] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_MatchPt),val);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_X),!ap->has_ttf_pt);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Y),!ap->has_ttf_pt);

    AI_DisplayClass(ci,ap);
    AI_DisplayRadio(ci,ap->type);

    AI_SelectList(ci,ap);
    SCUpdateAll(ci->sc);
}

static int AI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));

	if ( ci->ap->next==NULL )
return( true );
	AI_Display(ci,ci->ap->next);
    }
return( true );
}

static int AI_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev;

	prev = NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap = ap->next )
	    prev = ap;
	if ( prev==NULL )
return( true );
	AI_Display(ci,prev);
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e);
static int AI_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev, *delete_it;

	prev=NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap=ap->next )
	    prev = ap;
	if ( prev==NULL && ci->ap->next==NULL ) {
	    static char *buts[3];
	    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	    if ( gwwv_ask(_("Last Anchor Point"),(const char **) buts,0,1, _("You are deleting the last anchor point in this character.\nDoing so will cause this dialog to close, is that what you want?"))==1 ) {
		AI_Ok(g,e);
return( true );
	    }
	}

	delete_it = ci->ap;

	if ((prev == NULL) && (ci->ap->next == NULL)) {
	    ci->sc->anchor = NULL;
	    AnchorPointsFree(delete_it);
	    AI_Ok(g,e);
	    SCUpdateAll(ci->sc);
	}
	else if (ci->ap->next == NULL) {
	    prev->next = NULL;
	    AnchorPointsFree(delete_it);
	    AI_Display(ci,prev);
	}
	else if (prev == NULL) {
	    ci->sc->anchor = delete_it->next;
	    delete_it->next = NULL;
	    AnchorPointsFree(delete_it);
	    AI_Display(ci,ci->sc->anchor);
	}
	else {
	    prev->next = delete_it->next;
	    delete_it->next = NULL;
	    AnchorPointsFree(delete_it);
	    AI_Display(ci,prev->next);
	}

	_CVCharChangedUpdate(&ci->cv->b,2);
    }
return( true );
}

static GTextInfo **AnchorClassesLList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo **ti;

    if ( sf->cidmaster ) sf=sf->cidmaster;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = calloc(cnt+1,sizeof(GTextInfo*));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt] = calloc(1,sizeof(GTextInfo));
	ti[cnt]->text = utf82u_copy(an->name);
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->userdata = an;
    }
    ti[cnt] = calloc(1,sizeof(GTextInfo));
return( ti );
}

static int AI_NewClass(GGadget *g, GEvent *e) {
    GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
    SplineFont *sf = ci->sc->parent;
    AnchorClass *ac;
    GTextInfo **ti;
    int j;
    char *name = gwwv_ask_string(_("Anchor Class Name"),"",_("Please enter the name of a Anchor point class to create"));
    if ( name==NULL )
return( true );
    ac = SFFindOrAddAnchorClass(sf,name,NULL);
    GGadgetSetList(GWidgetGetControl(ci->gw,CID_NameList),
    ti = AnchorClassesLList(sf),false);
    for ( j=0; ti[j]->text!=NULL && ti[j]->userdata!=ac; ++j )
        GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_NameList),j);
return( true );
}

static int AI_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int waslig;
	AnchorPoint *ap;
	SplineFont *sf = ci->sc->parent;

	if ( sf->cidmaster ) sf = sf->cidmaster;

	if ( AnchorClassUnused(ci->sc,&waslig)==NULL ) {
            if ( !AI_NewClass(g, e) )
return( false );
	}
	ap = AnchorPointNew(ci->cv);
	if ( ap==NULL )
return( true );
	AI_Display(ci,ap);
    }
return( true );
}

static int AI_TypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap = ci->ap;

	if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Mark)) )
	    ap->type = at_mark;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseChar)) )
	    ap->type = at_basechar;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseLig)) )
	    ap->type = at_baselig;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseMark)) )
	    ap->type = at_basemark;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_CursEntry)) )
	    ap->type = at_centry;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_CursExit)) )
	    ap->type = at_cexit;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
	_CVCharChangedUpdate(&ci->cv->b,2);
    }
return( true );
}

static void AI_TestOrdering(GIData *ci,real x) {
    AnchorPoint *aps, *ap=ci->ap;
    AnchorClass *ac = ap->anchor;
    int isr2l;

    isr2l = SCRightToLeft(ci->sc);
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	if ( aps->anchor==ac && aps!=ci->ap ) {
	    if (( aps->lig_index<ap->lig_index &&
		    ((!isr2l && aps->me.x>x) ||
		     ( isr2l && aps->me.x<x))) ||
		( aps->lig_index>ap->lig_index &&
		    (( isr2l && aps->me.x>x) ||
		     (!isr2l && aps->me.x<x))) ) {
		ff_post_error(_("Out Of Order"),_("Marks within a ligature should be ordered with the direction of writing.\nThis one and %d are out of order."),aps->lig_index);
return;
	    }
	}
    }
return;
}

static int AI_LigIndexChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int index, max;
	int err=false;
	AnchorPoint *ap = ci->ap, *aps;

	index = GetCalmReal8(ci->gw,CID_LigIndex,_("Lig Index:"),&err);
	if ( index<0 || err )
return( true );
	if ( *_GGadgetGetTitle(g)=='\0' )
return( true );
	max = 0;
	AI_TestOrdering(ci,ap->me.x);
	for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	    if ( aps->anchor==ap->anchor && aps!=ap ) {
		if ( aps->lig_index==index ) {
		    ff_post_error(_("Index in use"),_("This ligature index is already in use"));
return( true );
		} else if ( aps->lig_index>max )
		    max = aps->lig_index;
	    }
	}
	if ( index>max+10 ) {
	    char buf[20];
	    ff_post_error(_("Too Big"),_("This index is much larger than the closest neighbor"));
	    sprintf(buf,"%d", max+1);
	    GGadgetSetTitle8(g,buf);
	    index = max+1;
	}
	ap->lig_index = index;
	_CVCharChangedUpdate(&ci->cv->b,2);
    }
return( true );
}

static int AI_ANameChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap;
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	AnchorClass *an = ti->userdata;
	int change=0, max=0;
	int ntype;
	int sawentry, sawexit;

	if ( an==ci->ap->anchor )
return( true );			/* No op */

	ntype = ci->ap->type;
	if ( an->type==act_curs ) {
	    if ( ntype!=at_centry && ntype!=at_cexit )
		ntype = at_centry;
	} else if ( an->type==act_mkmk ) {
	    if ( ntype!=at_basemark && ntype!=at_mark )
		ntype = at_basemark;
	} else if ( ntype==at_centry || ntype==at_cexit || ntype==at_basemark ||
		ntype==at_baselig || ntype == at_basechar ) {
	    PST *pst;
	    for ( pst = ci->sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
	    if (ci->sc->glyph_class==3+1 ||
		    (ci->sc->unicodeenc!=-1 && iscombining(ci->sc->unicodeenc)))
		ntype = at_mark;
	    else if (( pst!=NULL || ci->sc->glyph_class==2+1 ) && an->type==act_mklg )
		ntype = at_baselig;
	    else
		ntype = at_basechar;
	}

	sawentry = sawexit = false;
	for ( ap=ci->sc->anchor; ap!=NULL; ap = ap->next ) {
	    if ( ap!=ci->ap && ap->anchor==an ) {
		if ( an->type==act_curs ) {
		    if ( ap->type == at_centry ) sawentry = true;
		    else if ( ap->type== at_cexit ) sawexit = true;
		} else if ( an->type==act_mkmk ) {
		    if ( ap->type == at_mark ) sawentry = true;
		    else if ( ap->type== at_basemark ) sawexit = true;
		} else {
		    if ( ap->type!=at_baselig )
	break;
		    if ( ap->lig_index==ci->ap->lig_index )
			change = true;
		    else if ( ap->lig_index>max )
			max = ap->lig_index;
		}
	    } else if ( ap!=ci->ap && ap->anchor->subtable==an->subtable &&
		    ap->type==at_mark )
	break;
	}
	if ( ap!=NULL || (sawentry && sawexit)) {
	    AI_SelectList(ci,ci->ap);
	    ff_post_error(_("Class already used"),_("This anchor class already is associated with a point in this character"));
	} else {
	    ci->ap->anchor = an;
	    if ( an->type==act_curs ) {
		if ( sawentry ) ntype = at_cexit;
		else if ( sawexit ) ntype = at_centry;
	    } else if ( an->type==act_mkmk ) {
		if ( sawentry ) ntype = at_basemark;
		else if ( sawexit ) ntype = at_mark;
	    }
	    if ( ci->ap->type!=ntype ) {
		ci->ap->type = ntype;
		AI_DisplayRadio(ci,ntype);
	    }
	    if ( ci->ap->type!=at_baselig )
		ci->ap->lig_index = 0;
	    else if ( change ) {
		ci->ap->lig_index = max+1;
		AI_DisplayIndex(ci,ci->ap);
	    }
	    AI_DisplayClass(ci,ci->ap);
	    AI_TestOrdering(ci,ci->ap->me.x);
	}
	_CVCharChangedUpdate(&ci->cv->b,2);
    }
return( true );
}

static int AI_PosChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	AnchorPoint *ap = ci->ap;

	if ( GGadgetGetCid(g)==CID_X ) {
	    dx = GetCalmReal8(ci->gw,CID_X,_("_X"),&err)-ap->me.x;
	    AI_TestOrdering(ci,ap->me.x+dx);
	} else
	    dy = GetCalmReal8(ci->gw,CID_Y,_("_Y"),&err)-ap->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	ap->me.x += dx;
	ap->me.y += dy;
	_CVCharChangedUpdate(&ci->cv->b,2);
    }
return( true );
}

static int AI_MatchChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *t1 = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_MatchPt));
	unichar_t *end;
	AnchorPoint *ap = ci->ap;

	while ( *t1==' ' ) ++t1;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_X),*t1=='\0');
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Y),*t1=='\0');
	if ( isdigit(*t1)) {
	    BasePoint here;
	    int pt;
	    pt = u_strtol(t1,&end,10);
	    if ( *end=='\0' && ttfFindPointInSC(ci->cv->b.sc,CVLayer((CharViewBase *) ci->cv),pt,&here,NULL)==-1 ) {
		char buffer[40];
		sprintf(buffer,"%g",(double) here.x);
		GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_X),buffer);
		sprintf(buffer,"%g",(double) here.y);
		GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_Y),buffer);
		ap->me = here;
		ap->has_ttf_pt = true;
		ap->ttf_pt_index = pt;
		_CVCharChangedUpdate(&ci->cv->b,2);
	    }
	} else
	    ap->has_ttf_pt = false;
    }
return( true );
}

static void AI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    AnchorPointsFree(cv->b.sc->anchor);
    cv->b.sc->anchor = ci->oldaps;
    ci->oldaps = NULL;
    CVRemoveTopUndo(&cv->b);
    SCUpdateAll(cv->b.sc);
}

static int ai_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	AI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/getinfo.html", NULL);
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int AI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	/* All the work has been done as we've gone along */
	/* Well, we should reorder the list... */
	SCOrderAP(ci->cv->b.sc);
    }
return( true );
}

void ApGetInfo(CharView *cv, AnchorPoint *ap) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25], boxes[10], *varray[21], *harray1[5], *harray2[3],
	*hvarray[13], *harray3[4], *harray4[6], *harray5[6], *harray6[7];
    GTextInfo label[25];
    int j;
    SplineFont *sf;
    GTextInfo **ti;

    memset(&gi,0,sizeof(gi));
    gi.cv = cv;
    gi.sc = cv->b.sc;
    gi.oldaps = AnchorPointsCopy(cv->b.sc->anchor);
    CVPreserveState(&cv->b);
    if ( ap==NULL ) {
	ap = AnchorPointNew(cv);
	if ( ap==NULL )
return;
    }

    gi.ap = ap;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Anchor Point Info");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,AI_Width));
	pos.height = GDrawPointsToPixels(NULL,AI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ai_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	memset(&boxes,0,sizeof(boxes));

	j=0;
	label[j].text = (unichar_t *) ap->anchor->name;
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = 5;
	gcd[j].gd.pos.width = AI_Width-10;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NameList;
	sf = cv->b.sc->parent;
	if ( sf->cidmaster ) sf = sf->cidmaster;
	gcd[j].gd.handle_controlevent = AI_ANameChanged;
	gcd[j].creator = GListButtonCreate;
	varray[0] = &gcd[j]; varray[1] = NULL;
	++j;

	label[j].text = (unichar_t *) _("_X:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+34;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray1[0] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 23; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_X;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray1[1] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) _("_Y:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 85; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray1[2] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 103; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Y;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray1[3] = &gcd[j]; harray1[4] = NULL;
	++j;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray1;
	boxes[2].creator = GHBoxCreate;
	varray[2] = &boxes[2]; varray[3] = NULL;

	label[j].text = (unichar_t *) _("Matching TTF Point:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y+24;
	gcd[j].gd.flags = cv->b.sc->layers[ly_fore].order2 ? (gg_enabled|gg_visible) : gg_visible;
	gcd[j].creator = GLabelCreate;
	harray2[0] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 103; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gcd[j-1].gd.flags;
	gcd[j].gd.cid = CID_MatchPt;
	gcd[j].gd.handle_controlevent = AI_MatchChanged;
	gcd[j].creator = GTextFieldCreate;
	harray2[1] = &gcd[j]; harray2[2] = NULL;
	++j;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray2;
	boxes[3].creator = GHBoxCreate;
	varray[4] = &boxes[3]; varray[5] = NULL;

	label[j].text = (unichar_t *) _("Mark");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Mark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[0] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) _("Base Glyph");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 70; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseChar;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[1] = &gcd[j]; hvarray[2] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Base Lig");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseLig;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[3] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) _("Base Mark");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseMark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[4] = &gcd[j]; hvarray[5] = NULL;
	++j;

/* GT: Cursive Entry. This defines a point on the glyph that should be matched */
/* GT: with the "Cursive Exit" point of the preceding glyph.  */
/* GT: This is a special way of joining letters which was developed for Urdu */
/* GT: fonts. Essentially every glyph has an entry point and an exit point. */
/* GT: When written the glyphs in sequence are aligned so that the exit point */
/* GT: of each glyph matches the entry point of the following. It means you */
/* GT: get a join such as might be expected for script. Urdu is odd because */
/* GT: letters within a word crawl diagonally up the page, but with each word */
/* GT: the writing point starts at the baseline. */
	label[j].text = (unichar_t *) _("CursEntry");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_CursEntry;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[6] = &gcd[j];
	++j;

/* GT: Cursive Exit. This defines a point on the glyph that should be matched */
/* GT: with the "Cursive Entry" point of the following glyph. This allows */
/* GT: scripts such as Urdu to work */
	label[j].text = (unichar_t *) _("CursExit");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_CursExit;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	hvarray[7] = &gcd[j]; hvarray[8] = NULL; hvarray[9] = NULL;
	++j;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = hvarray;
	boxes[4].creator = GHVBoxCreate;
	varray[6] = &boxes[4]; varray[7] = NULL;

	label[j].text = (unichar_t *) _("Lig Index:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray3[0] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 65; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_LigIndex;
	gcd[j].gd.handle_controlevent = AI_LigIndexChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray3[1] = &gcd[j]; harray3[2] = NULL;
	++j;

	boxes[5].gd.flags = gg_enabled|gg_visible;
	boxes[5].gd.u.boxelements = harray3;
	boxes[5].creator = GHBoxCreate;
	varray[8] = &boxes[5]; varray[9] = NULL;

	label[j].text = (unichar_t *) S_("AnchorPoint|_New");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+30;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_New;
	gcd[j].gd.handle_controlevent = AI_New;
	gcd[j].creator = GButtonCreate;
	harray4[0] = &gcd[j]; harray4[1] = GCD_Glue;
	++j;

	label[j].text = (unichar_t *) S_("AnchorClass|New _Class");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 30; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_New;
	gcd[j].gd.handle_controlevent = AI_NewClass;
	gcd[j].creator = GButtonCreate;
	harray4[2] = &gcd[j]; harray4[3] = GCD_Glue;
	++j;

	label[j].text = (unichar_t *) _("_Delete");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Delete;
	gcd[j].gd.handle_controlevent = AI_Delete;
	gcd[j].creator = GButtonCreate;
	harray4[4] = &gcd[j]; harray4[5] = NULL;
	++j;

	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = harray4;
	boxes[6].creator = GHBoxCreate;
	varray[10] = &boxes[6]; varray[11] = NULL;

	label[j].text = (unichar_t *) _("< _Prev");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Prev;
	gcd[j].gd.handle_controlevent = AI_Prev;
	gcd[j].creator = GButtonCreate;
	harray5[0] = GCD_Glue; harray5[1] = &gcd[j]; harray5[2] = GCD_Glue;
	++j;

	label[j].text = (unichar_t *) _("_Next >");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Next;
	gcd[j].gd.handle_controlevent = AI_Next;
	gcd[j].creator = GButtonCreate;
	harray5[3] = &gcd[j]; harray5[4] = GCD_Glue; harray5[5] = NULL;
	++j;

	boxes[7].gd.flags = gg_enabled|gg_visible;
	boxes[7].gd.u.boxelements = harray5;
	boxes[7].creator = GHBoxCreate;
	varray[12] = &boxes[7]; varray[13] = NULL;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.pos.width = AI_Width-10;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLineCreate;
	varray[14] = GCD_Glue; varray[15] = NULL;
	varray[16] = &gcd[j]; varray[17] = NULL;
	++j;

	label[j].text = (unichar_t *) _("_OK");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+4;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_but_default;
	gcd[j].gd.handle_controlevent = AI_Ok;
	gcd[j].creator = GButtonCreate;
	harray6[0] = &gcd[j]; harray6[1] = GCD_Glue;
	++j;

	label[j].text = (unichar_t *) _("_Cancel");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+3;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
	gcd[j].gd.handle_controlevent = AI_Cancel;
	gcd[j].creator = GButtonCreate;
	harray6[2] = &gcd[j]; harray6[3] = NULL;
	++j;

	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = harray6;
	boxes[8].creator = GHBoxCreate;
	varray[18] = &boxes[8]; varray[19] = NULL;
	varray[20] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gi.gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[8].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxes[7].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

	GGadgetSetList(GWidgetGetControl(gi.gw,CID_NameList),
		    ti = AnchorClassesLList(gi.sc->parent),false);
	for ( j=0; ti[j]->text!=NULL && ti[j]->userdata!=ap->anchor; ++j )
	GGadgetSelectOneListItem(GWidgetGetControl(gi.gw,CID_NameList),j);

	AI_Display(&gi,ap);
	GWidgetIndicateFocusGadget(GWidgetGetControl(gi.gw,CID_X));

    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
    AnchorPointsFree(gi.oldaps);
}

void PI_ShowHints(SplineChar *sc, GGadget *list, int set) {
    StemInfo *h;
    int32_t i, len;

    if ( !set ) {
	for ( h = sc->hstem; h!=NULL; h=h->next )
	    h->active = false;
	for ( h = sc->vstem; h!=NULL; h=h->next )
	    h->active = false;
    } else {
	GTextInfo **ti = GGadgetGetList(list,&len);
	for ( h = sc->hstem, i=0; h!=NULL && i<len; h=h->next, ++i )
	    h->active = ti[i]->selected;
	for ( h = sc->vstem; h!=NULL && i<len; h=h->next, ++i )
	    h->active = ti[i]->selected;
    }
    SCOutOfDateBackground(sc);
    SCUpdateAll(sc);
}

static void _PI_ShowHints(GIData *ci,int set) {
    PI_ShowHints(ci->cv->b.sc,GWidgetGetControl(ci->gw,CID_HintMask),set);
}

static void PI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    if ( cv->b.drawmode==dm_fore )
	MDReplace(cv->b.sc->md,cv->b.sc->layers[ly_fore].splines,ci->oldstate);
    SplinePointListsFree(cv->b.layerheads[cv->b.drawmode]->splines);
    cv->b.layerheads[cv->b.drawmode]->splines = ci->oldstate;
    CVRemoveTopUndo(&cv->b);
    SCClearSelPt(cv->b.sc);
    _PI_ShowHints(ci,false);
    SCUpdateAll(cv->b.sc);
}

static void PIFillup(GIData *ci, int except_cid);

static void PI_FigureNext(GIData *ci) {
    if ( ci->prevchanged ) {
	SplinePoint *cursp = ci->cursp;
	if ( !ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 && (cursp->pointtype==pt_curve || cursp->pointtype==pt_hvcurve)) {
	    double dx, dy, len, len2;
	    dx = cursp->prevcp.x - cursp->me.x;
	    dy = cursp->prevcp.y - cursp->me.y;
	    len = sqrt(dx*dx+dy*dy);
	    if ( len!=0 ) {
		len2 = sqrt((cursp->nextcp.x-cursp->me.x)*(cursp->nextcp.x-cursp->me.x)+
			(cursp->nextcp.y-cursp->me.y)*(cursp->nextcp.y-cursp->me.y));
		cursp->nextcp.x=cursp->me.x-dx*len2/len;
		cursp->nextcp.y=cursp->me.y-dy*len2/len;
		if ( cursp->next!=NULL )
		    SplineRefigure(cursp->next);
		CVCharChangedUpdate(&ci->cv->b);
		PIFillup(ci,-1);
	    }
	}
    }
    ci->prevchanged = false;
}

static void PI_FigurePrev(GIData *ci) {
    if ( ci->nextchanged ) {
	SplinePoint *cursp = ci->cursp;
	if ( !ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 && (cursp->pointtype==pt_curve || cursp->pointtype==pt_hvcurve)) {
	    double dx, dy, len, len2;
	    dx = cursp->nextcp.x - cursp->me.x;
	    dy = cursp->nextcp.y - cursp->me.y;
	    len = sqrt(dx*dx+dy*dy);
	    if ( len!=0 ) {
		len2 = sqrt((cursp->prevcp.x-cursp->me.x)*(cursp->prevcp.x-cursp->me.x)+
			(cursp->prevcp.y-cursp->me.y)*(cursp->prevcp.y-cursp->me.y));
		cursp->prevcp.x=cursp->me.x-dx*len2/len;
		cursp->prevcp.y=cursp->me.y-dy*len2/len;
		if ( cursp->prev!=NULL )
		    SplineRefigure(cursp->prev);
		CVCharChangedUpdate(&ci->cv->b);
		PIFillup(ci,-1);
	    }
	}
    }
    ci->nextchanged = false;
}

static void PI_FigureHintMask(GIData *ci) {
    int32_t i, len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_HintMask),&len);

    for ( i=0; i<len; ++i )
	if ( ti[i]->selected )
    break;

    if ( i==len ) {
	chunkfree(ci->cursp->hintmask,sizeof(HintMask));
	ci->cursp->hintmask = NULL;
    } else {
	if ( ci->cursp->hintmask==NULL )
	    ci->cursp->hintmask = chunkalloc(sizeof(HintMask));
	else
	    memset(ci->cursp->hintmask,0,sizeof(HintMask));
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected )
		(*ci->cursp->hintmask)[i>>3] |= (0x80>>(i&7));
    }
}

static void PI_FixStuff(GIData *ci) {
    SplinePoint *sp = ci->cursp;

    PI_FigureHintMask(ci);
    PI_FigureNext(ci);
    PI_FigurePrev(ci);

    if ( sp->pointtype == pt_hvcurve ) {
	if (
		((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x && sp->nextcp.y!=sp->me.y) ||
		 (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y && sp->nextcp.x!=sp->me.x)))
	    /* Do Nothing */;
	else
	    sp->pointtype = pt_curve;
    } else if ( sp->pointtype == pt_tangent )
	SplinePointCategorize(sp);	/* Users can change cps so it isn't a tangent, so check */
}

void PI_Destroy(struct dlistnode *node) {
    GIData *d = (GIData *)node;
    GDrawDestroyWindow(d->gw);
    dlist_erase(&d->cv->pointInfoDialogs,(struct dlistnode *)d);
    free(d);
}

static void PI_Close(GIData *d) {
    PI_Destroy((struct dlistnode *)d);
}

static int PI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *d = GDrawGetUserData(GGadgetGetWindow(g));
	PI_DoCancel(d);
	PI_Close(d);
    }
return( true );
}

static void PI_DoOk(GIData *ci) {
	PI_FixStuff(ci);
	_PI_ShowHints(ci,false);

	ci->done = true;
	/* All the work has been done as we've gone along */
	PI_Close(ci);
}

static int PI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_DoOk(ci);
    }
    return( true );
}

static int pi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI_DoOk(GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/getinfo.html", NULL);
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static void mysprintf( char *buffer, char *format, real v) {
    char *pt;

    if ( v<.0001 && v>-.0001 && v!=0 )
	sprintf( buffer, "%e", (double) v );
    else if ( v<1 && v>0 )
	sprintf( buffer, "%f", (double) v );
    else if ( v<0 && v>-1 )
	sprintf( buffer, "%.5f", (double) v );
    else
	sprintf( buffer, format, (double) v );
    pt = buffer + strlen(buffer);
    while ( pt>buffer && pt[-1]=='0' )
	*--pt = '\0';
    if ( pt>buffer && pt[-1]=='.' )
	pt[-1] = '\0';
}

static void mysprintf2( char *buffer, real v1, real v2) {
    char *pt;

    mysprintf(buffer,"%.2f", v1);
    pt = buffer+strlen(buffer);
    *pt++ = ',';
    mysprintf(pt,"%.2f", v2);
}

static void PIFillup(GIData *ci, int except_cid) {
    char buffer[51];
    double dx, dy;
    double kappa, kappa2;
    int emsize;

    mysprintf(buffer, "%.2f", ci->cursp->me.x );
    if ( except_cid!=CID_BaseX )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_BaseX),buffer);

    mysprintf(buffer, "%.2f", ci->cursp->me.y );
    if ( except_cid!=CID_BaseY )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_BaseY),buffer);

    dx = ci->cursp->nextcp.x-ci->cursp->me.x;
    dy = ci->cursp->nextcp.y-ci->cursp->me.y;
    mysprintf(buffer, "%.2f", dx );
    if ( except_cid!=CID_NextXOff )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextXOff),buffer);

    mysprintf(buffer, "%.2f", dy );
    if ( except_cid!=CID_NextYOff )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextYOff),buffer);

    if ( except_cid!=CID_NextR ) {
	mysprintf(buffer, "%.2f", sqrt( dx*dx+dy*dy ));
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextR),buffer);
    }

    if ( except_cid!=CID_NextTheta ) {
	if ( ci->cursp->pointtype==pt_tangent && ci->cursp->prev!=NULL ) {
	    dx = ci->cursp->me.x-ci->cursp->prev->from->me.x;
	    dy = ci->cursp->me.y-ci->cursp->prev->from->me.y;
	}
	mysprintf(buffer, "%.1f", atan2(dy,dx)*RAD2DEG);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextTheta),buffer);
    }

    mysprintf2(buffer, ci->cursp->nextcp.x,ci->cursp->nextcp.y );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextPos),buffer);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), ci->cursp->nextcpdef );

    dx = ci->cursp->prevcp.x-ci->cursp->me.x;
    dy = ci->cursp->prevcp.y-ci->cursp->me.y;
    mysprintf(buffer, "%.2f", dx );
    if ( except_cid!=CID_PrevXOff )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevXOff),buffer);

    mysprintf(buffer, "%.2f", dy );
    if ( except_cid!=CID_PrevYOff )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevYOff),buffer);

    if ( except_cid!=CID_PrevR ) {
	mysprintf(buffer, "%.2f", sqrt( dx*dx+dy*dy ));
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevR),buffer);
    }

    if ( except_cid!=CID_PrevTheta ) {
	if ( ci->cursp->pointtype==pt_tangent && ci->cursp->next!=NULL ) {
	    dx = ci->cursp->me.x-ci->cursp->next->to->me.x;
	    dy = ci->cursp->me.y-ci->cursp->next->to->me.y;
	}
	mysprintf(buffer, "%.1f", atan2(dy,dx)*RAD2DEG);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevTheta),buffer);
    }

    mysprintf2(buffer, ci->cursp->prevcp.x,ci->cursp->prevcp.y );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevPos),buffer);


    mysprintf2(buffer, ci->cursp->me.x,ci->cursp->me.y );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_BasePos),buffer);

    mysprintf(buffer, "%.2f", ci->cursp->nextcp.x );
    if ( except_cid!=CID_NextX )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextX),buffer);

    mysprintf(buffer, "%.2f", ci->cursp->nextcp.y );
    if ( except_cid!=CID_NextY )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextY),buffer);

    mysprintf(buffer, "%.2f", ci->cursp->prevcp.x );
    if ( except_cid!=CID_PrevX )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevX),buffer);

    mysprintf(buffer, "%.2f", ci->cursp->prevcp.y );
    if ( except_cid!=CID_PrevY )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevY),buffer);


    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), ci->cursp->prevcpdef );

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Curve), ci->cursp->pointtype==pt_curve );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_HVCurve), ci->cursp->pointtype==pt_hvcurve );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Corner), ci->cursp->pointtype==pt_corner );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Tangent), ci->cursp->pointtype==pt_tangent );

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_PrevTheta), ci->cursp->pointtype!=pt_tangent );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_NextTheta), ci->cursp->pointtype!=pt_tangent );

    kappa = SplineCurvature(ci->cursp->next,0);
    kappa2 = SplineCurvature(ci->cursp->prev,1);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_PrevCurvature), kappa2!=CURVATURE_ERROR );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_NextCurvature), kappa!=CURVATURE_ERROR );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_DeltaCurvature),
	    kappa!=CURVATURE_ERROR && kappa2!=CURVATURE_ERROR );
    emsize = ci->cv->b.sc->parent->ascent + ci->cv->b.sc->parent->descent;
    /* If we normalize by the em-size, the curvature is often more */
    /*  readable */
    if ( kappa!=CURVATURE_ERROR )
	sprintf( buffer, _("Curvature: %g"), kappa*emsize );
    else
	strcpy( buffer, _("Curvature: ?"));
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_NextCurvature),buffer);
    if ( kappa2!=CURVATURE_ERROR )
	sprintf( buffer, _("Curvature: %g"), kappa2*emsize );
    else
	strncpy( buffer, _("Curvature: ?"),sizeof(buffer)-1 );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_PrevCurvature),buffer);
    if ( kappa!=CURVATURE_ERROR && kappa2!=CURVATURE_ERROR )
	sprintf( buffer, "∆: %g", (kappa-kappa2)*emsize );
    else
	strcpy( buffer, "∆: ?");
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_DeltaCurvature),buffer);
}

static void PIShowHide(GIData *ci) {
    int normal = GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Normal));
    int i;

    for ( i=ci->normal_start; i<ci->normal_end; ++i )
	if ( ci->gcd[i].ret!=NULL )
	    GGadgetSetVisible(ci->gcd[i].ret,normal);
    GGadgetSetVisible(ci->group1ret,normal);
    GGadgetSetVisible(ci->group2ret,normal);
    for ( i=ci->interp_start; i<ci->interp_end; ++i )
	if ( ci->gcd[i].ret!=NULL )
	    GGadgetSetVisible(ci->gcd[i].ret,!normal);
    GWidgetFlowGadgets(GGadgetGetWindow(GWidgetGetControl(ci->gw,CID_Normal)));
}

void PIChangePoint(GIData *ci) {
    int aspect = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_TabSet));
    GGadget *list = GWidgetGetControl(ci->gw,CID_HintMask);
    int32_t i, len;
    HintMask *hm;
    SplinePoint *sp;
    SplineSet *spl;
    int interpolate;

    GGadgetGetList(list,&len);

    PIFillup(ci,0);

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Interpolated),
	    !ci->cursp->dontinterpolate && !(ci->cursp->nonextcp && ci->cursp->noprevcp) );
    interpolate = SPInterpolate(ci->cursp) && ci->cv->b.layerheads[ci->cv->b.drawmode]->order2;
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Normal), !interpolate );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Interpolated), interpolate );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NeverInterpolate), ci->cursp->dontinterpolate );
    PIShowHide(ci);

    if ( ci->cursp->hintmask==NULL ) {
	for ( i=0; i<len; ++i )
	    GGadgetSelectListItem(list,i,false);
    } else {
	for ( i=0; i<len && i<HntMax; ++i )
	    GGadgetSelectListItem(list,i, (*ci->cursp->hintmask)[i>>3]&(0x80>>(i&7))?true:false );
    }
    _PI_ShowHints(ci,aspect==1);

    list = GWidgetGetControl(ci->gw,CID_ActiveHints);
    hm = NULL;
    /* Figure out what hintmask is active at the current point */
    /* Note: we must walk each ss backwards because we reverse the splineset */
    /*  when we output postscript */
    for ( spl = ci->sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->hintmask )
		hm = sp->hintmask;
	    if ( sp==ci->cursp )
	break;
	    if ( sp->prev==NULL )
	break;
	    sp = sp->prev->from;
	    if ( sp==spl->first )
	break;
	}
	if ( sp==ci->cursp )
    break;
    }
    if ( hm==NULL ) {
	for ( i=0; i<len; ++i )
	    GGadgetSelectListItem(list,i,false);
    } else {
	for ( i=0; i<len && i<HntMax; ++i )
	    GGadgetSelectListItem(list,i, (*hm)[i>>3]&(0x80>>(i&7))?true:false );
    }
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_NextDef),ci->cursp->next!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_PrevDef),ci->cursp->prev!=NULL);
}

static int PI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	int cid = GGadgetGetCid(g);
	SplinePointList *spl;

	PI_FixStuff(ci);

	ci->cursp->selected = false;
	if ( cid == CID_Next ) {
	    if ( ci->cursp->next!=NULL && ci->cursp->next->to!=ci->curspl->first )
		ci->cursp = ci->cursp->next->to;
	    else {
		if ( ci->curspl->next == NULL ) {
		    ci->curspl = cv->b.layerheads[cv->b.drawmode]->splines;
		    GDrawBeep(NULL);
		} else
		    ci->curspl = ci->curspl->next;
		ci->cursp = ci->curspl->first;
	    }
	} else if ( cid == CID_Prev ) {
	    if ( ci->cursp!=ci->curspl->first ) {
		ci->cursp = ci->cursp->prev->from;
	    } else {
		if ( ci->curspl==cv->b.layerheads[cv->b.drawmode]->splines ) {
		    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=NULL; spl=spl->next );
		    GDrawBeep(NULL);
		} else {
		    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=ci->curspl; spl=spl->next );
		}
		ci->curspl = spl;
		ci->cursp = spl->last;
		if ( spl->last==spl->first && spl->last->prev!=NULL )
		    ci->cursp = ci->cursp->prev->from;
	    }
	} else if ( cid==CID_NextC ) {
	    if ( ci->cursp->next!=NULL )
		ci->cursp = ci->cursp->next->to;
	    else {
		ci->cursp = ci->curspl->first;
		GDrawBeep(NULL);
	    }
	} else /* CID_PrevC */ {
	    if ( ci->cursp->prev!=NULL )
		ci->cursp = ci->cursp->prev->from;
	    else {
		ci->cursp = ci->curspl->last;
		GDrawBeep(NULL);
	    }
	}
	ci->cursp->selected = true;
	PIChangePoint(ci);
	CVShowPoint(cv,&ci->cursp->me);
	SCUpdateAll(cv->b.sc);
    }
return( true );
}

static int PI_InterpChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_Interpolated ) {
	    if ( cursp->nonextcp && cursp->noprevcp )
		/* Do Nothing */;
	    else {
		if ( cursp->nonextcp && cursp->next ) {
		    SplinePoint *n = cursp->next->to;
		    cursp->nextcp.x = rint((n->me.x+cursp->me.x)/2);
		    cursp->nextcp.y = rint((n->me.y+cursp->me.y)/2);
		    n->prevcp = cursp->nextcp;
		}
		if ( cursp->noprevcp && cursp->prev ) {
		    SplinePoint *p = cursp->prev->from;
		    cursp->prevcp.x = rint((p->me.x+cursp->me.x)/2);
		    cursp->prevcp.y = rint((p->me.y+cursp->me.y)/2);
		    p->nextcp = cursp->prevcp;
		}
		cursp->me.x = (cursp->nextcp.x + cursp->prevcp.x)/2;
		cursp->me.y = (cursp->nextcp.y + cursp->prevcp.y)/2;
		if ( cursp->pointtype==pt_tangent ) {
		    cursp->pointtype = pt_curve;
		    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Curve),true);
		}
		if ( cursp->next!=NULL )
		    SplineRefigure(cursp->next);
		if ( cursp->prev!=NULL )
		    SplineRefigure(cursp->prev);
		SplineSetSpirosClear(ci->curspl);
		CVCharChangedUpdate(&ci->cv->b);
	    }
	    PIFillup(ci,0);
	}
	PIShowHide(ci);
    }
return( true );
}

static int PI_NeverInterpChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int never = GGadgetIsChecked(g);

	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Interpolated),!never);

	if ( never )
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Normal),true);
	ci->cursp->dontinterpolate = never;
	PIShowHide(ci);
    }
return( true );
}

static int PI_BaseChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_BaseX )
	    dx = GetCalmReal8(ci->gw,CID_BaseX,_("Base X"),&err)-cursp->me.x;
	else
	    dy = GetCalmReal8(ci->gw,CID_BaseY,_("Base Y"),&err)-cursp->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->me.x += dx;
	cursp->nextcp.x += dx;
	cursp->prevcp.x += dx;
	cursp->me.y += dy;
	cursp->nextcp.y += dy;
	cursp->prevcp.y += dy;
	if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 ) {
	    SplinePointNextCPChanged2(cursp);
	    SplinePointPrevCPChanged2(cursp);
	}
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	SplineSetSpirosClear(ci->curspl);
	CVCharChangedUpdate(&ci->cv->b);
	PIFillup(ci,GGadgetGetCid(g));
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
	PI_FigurePrev(ci);
    }
return( true );
}

static int PI_NextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_NextXOff ) {
	    dx = GetCalmReal8(ci->gw,CID_NextXOff,_("Next CP X"),&err)-(cursp->nextcp.x-cursp->me.x);
	    if ( cursp->pointtype==pt_tangent && cursp->prev!=NULL ) {
		if ( cursp->prev->from->me.x==cursp->me.x ) {
		    dy = dx; dx = 0;	/* They should be constrained not to change in the x direction */
		} else
		    dy = dx*(cursp->prev->from->me.y-cursp->me.y)/(cursp->prev->from->me.x-cursp->me.x);
	    }
	} else if ( GGadgetGetCid(g)==CID_NextYOff ) {
	    dy = GetCalmReal8(ci->gw,CID_NextYOff,_("Next CP Y"),&err)-(cursp->nextcp.y-cursp->me.y);
	    if ( cursp->pointtype==pt_tangent && cursp->prev!=NULL ) {
		if ( cursp->prev->from->me.y==cursp->me.y ) {
		    dx = dy; dy = 0;	/* They should be constrained not to change in the y direction */
		} else
		    dx = dy*(cursp->prev->from->me.x-cursp->me.x)/(cursp->prev->from->me.y-cursp->me.y);
	    }
	} else {
	    double len, theta;
	    len = GetCalmReal8(ci->gw,CID_NextR,_("Next CP Dist"),&err);
	    theta = GetCalmReal8(ci->gw,CID_NextTheta,_("Next CP Angle"),&err)/RAD2DEG;
	    dx = len*cos(theta) - (cursp->nextcp.x-cursp->me.x);
	    dy = len*sin(theta) - (cursp->nextcp.y-cursp->me.y);
	}
	if ( (dx==0 && dy==0) || err )
return( true );
	if ( cursp->pointtype==pt_hvcurve ) {
	    BasePoint diff;
	    diff.x = cursp->nextcp.x+dx - cursp->me.x;
	    diff.y = cursp->nextcp.y+dy - cursp->me.y;
	    BP_HVForce(&diff);
	    dx = diff.x + (cursp->me.x - cursp->nextcp.x);
	    dy = diff.y + (cursp->me.y - cursp->nextcp.y);
	}
	cursp->nextcp.x += dx;
	cursp->nextcp.y += dy;
	SplineSetSpirosClear(ci->curspl);
	ci->nextchanged = true;
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->nextcpdef ) {
	    cursp->nextcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), false );
	}
	if (( cursp->pointtype==pt_curve || cursp->pointtype==pt_hvcurve) && cursp->prev!=NULL ) {
	    double plen, ntheta;
	    double ndx, ndy;
	    ndx = ci->cursp->nextcp.x-ci->cursp->me.x;
	    ndy = ci->cursp->nextcp.y-ci->cursp->me.y;
	    ntheta = atan2(ndy,ndx);
	    plen = GetCalmReal8(ci->gw,CID_PrevR,_("Prev CP Dist"),&err);
	    ci->cursp->prevcp.x = ci->cursp->me.x - plen*cos(ntheta);
	    ci->cursp->prevcp.y = ci->cursp->me.y - plen*sin(ntheta);
	    if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
		SplinePointPrevCPChanged2(cursp);
	    SplineRefigure(cursp->prev);
	}
	if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
	    SplinePointNextCPChanged2(cursp);
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	CVCharChangedUpdate(&ci->cv->b);
	PIFillup(ci,GGadgetGetCid(g));
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
    }
return( true );
}

static int PI_PrevChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_PrevXOff ) {
	    dx = GetCalmReal8(ci->gw,CID_PrevXOff,_("Prev CP X"),&err)-(cursp->prevcp.x-cursp->me.x);
	    if ( cursp->pointtype==pt_tangent && cursp->next!=NULL ) {
		if ( cursp->next->to->me.x==cursp->me.x ) {
		    dy = dx; dx = 0;	/* They should be constrained not to change in the x direction */
		} else
		    dy = dx*(cursp->next->to->me.y-cursp->me.y)/(cursp->next->to->me.x-cursp->me.x);
	    }
	} else if ( GGadgetGetCid(g)==CID_PrevYOff ) {
	    dy = GetCalmReal8(ci->gw,CID_PrevYOff,_("Prev CP Y"),&err)-(cursp->prevcp.y-cursp->me.y);
	    if ( cursp->pointtype==pt_tangent && cursp->next!=NULL ) {
		if ( cursp->next->to->me.y==cursp->me.y ) {
		    dx = dy; dy = 0;	/* They should be constrained not to change in the y direction */
		} else
		    dx = dy*(cursp->next->to->me.x-cursp->me.x)/(cursp->next->to->me.y-cursp->me.y);
	    }
	} else {
	    double len, theta;
	    len = GetCalmReal8(ci->gw,CID_PrevR,_("Prev CP Dist"),&err);
	    theta = GetCalmReal8(ci->gw,CID_PrevTheta,_("Prev CP Angle"),&err)/RAD2DEG;
	    dx = len*cos(theta) - (cursp->prevcp.x-cursp->me.x);
	    dy = len*sin(theta) - (cursp->prevcp.y-cursp->me.y);
	}
	if ( (dx==0 && dy==0) || err )
return( true );
	if ( cursp->pointtype==pt_hvcurve ) {
	    BasePoint diff;
	    diff.x = cursp->prevcp.x+dx - cursp->me.x;
	    diff.y = cursp->prevcp.y+dy - cursp->me.y;
	    BP_HVForce(&diff);
	    dx = diff.x - (cursp->prevcp.x - cursp->me.x);
	    dy = diff.y - (cursp->prevcp.y - cursp->me.y);
	}
	cursp->prevcp.x += dx;
	cursp->prevcp.y += dy;
	ci->prevchanged = true;
	SplineSetSpirosClear(ci->curspl);
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->prevcpdef ) {
	    cursp->prevcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), false );
	}
	if (( cursp->pointtype==pt_curve || cursp->pointtype==pt_hvcurve) && cursp->next!=NULL ) {
	    double nlen, ptheta;
	    double pdx, pdy;
	    pdx = ci->cursp->prevcp.x-ci->cursp->me.x;
	    pdy = ci->cursp->prevcp.y-ci->cursp->me.y;
	    ptheta = atan2(pdy,pdx);
	    nlen = GetCalmReal8(ci->gw,CID_NextR,_("Next CP Dist"),&err);
	    ci->cursp->nextcp.x = ci->cursp->me.x - nlen*cos(ptheta);
	    ci->cursp->nextcp.y = ci->cursp->me.y - nlen*sin(ptheta);
	    if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
		SplinePointNextCPChanged2(cursp);
	    SplineRefigure(cursp->next);
	}
	if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
	    SplinePointPrevCPChanged2(cursp);
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(&ci->cv->b);
	PIFillup(ci,GGadgetGetCid(g));
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigurePrev(ci);
    }
return( true );
}

static int PI_NextIntChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real x=0, y=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	x = GetCalmReal8(ci->gw,CID_NextX,_("Next CP X"),&err);
	y = GetCalmReal8(ci->gw,CID_NextY,_("Next CP Y"),&err);
	if ( err || (x==cursp->nextcp.x && y==cursp->nextcp.y) )
return( true );
	cursp->nextcp.x = x;
	cursp->nextcp.y = y;
	cursp->me.x = (cursp->nextcp.x + cursp->prevcp.x)/2;
	cursp->me.y = (cursp->nextcp.y + cursp->prevcp.y)/2;
	SplineSetSpirosClear(ci->curspl);
	if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
	    SplinePointNextCPChanged2(cursp);
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	CVCharChangedUpdate(&ci->cv->b);
	PIFillup(ci,GGadgetGetCid(g));
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
    }
return( true );
}

static int PI_PrevIntChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real x=0, y=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	x = GetCalmReal8(ci->gw,CID_PrevX,_("Prev CP X"),&err);
	y = GetCalmReal8(ci->gw,CID_PrevY,_("Prev CP Y"),&err);
	if ( err || (x==cursp->prevcp.x && y==cursp->prevcp.y) )
return( true );
	cursp->prevcp.x = x;
	cursp->prevcp.y = y;
	cursp->me.x = (cursp->nextcp.x + cursp->prevcp.x)/2;
	cursp->me.y = (cursp->nextcp.y + cursp->prevcp.y)/2;
	SplineSetSpirosClear(ci->curspl);
	if ( ci->cv->b.layerheads[ci->cv->b.drawmode]->order2 )
	    SplinePointPrevCPChanged2(cursp);
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(&ci->cv->b);
	PIFillup(ci,GGadgetGetCid(g));
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
    }
return( true );
}

static int PI_NextDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->nextcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->nextcpdef ) {
	    BasePoint temp = cursp->prevcp;
	    SplineCharDefaultNextCP(cursp);
	    if ( !cursp->prevcpdef ) {
		cursp->prevcp = temp;
		SplineSetSpirosClear(ci->curspl);
	    }
	    CVCharChangedUpdate(&ci->cv->b);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_PrevDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->prevcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->prevcpdef ) {
	    BasePoint temp = cursp->nextcp;
	    SplineCharDefaultPrevCP(cursp);
	    if ( !cursp->nextcpdef ) {
		cursp->nextcp = temp;
		SplineSetSpirosClear(ci->curspl);
	    }
	    CVCharChangedUpdate(&ci->cv->b);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_PTypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;
	enum pointtype pt = GGadgetGetCid(g)-CID_Curve + pt_curve;

	if ( pt==cursp->pointtype ) {
	    /* Can't happen */
	} else if ( pt==pt_corner ) {
	    cursp->pointtype = pt_corner;
	    CVCharChangedUpdate(&ci->cv->b);
	} else {
	    SPChangePointType(cursp,pt);
	    SplineSetSpirosClear(ci->curspl);
	    CVCharChangedUpdate(&ci->cv->b);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int aspect = GTabSetGetSel(g);

	_PI_ShowHints(ci,aspect==1);
    }
return( true );
}

static int PI_HintSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));

	_PI_ShowHints(ci,true);

	if ( GGadgetIsListItemSelected(g,e->u.control.u.list.changed_index)) {
	    /* If we just selected something, check to make sure it doesn't */
	    /*  conflict with any other hints. */
	    /* Adobe says this is an error, but in "three" in AdobeSansMM */
	    /*  (in black,extended) we have a hintmask which contains two */
	    /*  overlapping hints */
	    /* So just make it a warning */
	    int i,j;
	    StemInfo *h, *h2=NULL;
	    for ( i=0, h=ci->cv->b.sc->hstem; h!=NULL && i!=e->u.control.u.list.changed_index; h=h->next, ++i );
	    if ( h!=NULL ) {
		for ( h2 = ci->cv->b.sc->hstem, i=0 ; h2!=NULL; h2=h2->next, ++i ) {
		    if ( h2!=h && GGadgetIsListItemSelected(g,i)) {
			if (( h2->start<h->start && h2->start+h2->width>h->start ) ||
			    ( h2->start>=h->start && h->start+h->width>h2->start ))
		break;
		    }
		}
	    } else {
		j = i;
		for ( h=ci->cv->b.sc->vstem; h!=NULL && i!=e->u.control.u.list.changed_index; h=h->next, ++i );
		if ( h==NULL )
		    IError("Failed to find hint");
		else {
		    for ( h2 = ci->cv->b.sc->vstem, i=j ; h2!=NULL; h2=h2->next, ++i ) {
			if ( h2!=h && GGadgetIsListItemSelected(g,i)) {
			    if (( h2->start<h->start && h2->start+h2->width>h->start ) ||
				( h2->start>=h->start && h->start+h->width>h2->start ))
		    break;
			}
		    }
		}
	    }
	    if ( h2!=NULL )
		ff_post_error(_("Overlapped Hints"),_("The hint you have just selected overlaps with <%.2f,%.2f>. You should deselect one of the two."),
			h2->start,h2->width);
	}
    }
return( true );
}

GTextInfo *SCHintList(SplineChar *sc,HintMask *hm) {
    StemInfo *h;
    int i;
    GTextInfo *ti;
    char buffer[100];

    for ( h=sc->hstem, i=0; h!=NULL; h=h->next, ++i );
    for ( h=sc->vstem     ; h!=NULL; h=h->next, ++i );
    ti = calloc(i+1,sizeof(GTextInfo));

    for ( h=sc->hstem, i=0; h!=NULL; h=h->next, ++i ) {
	ti[i].fg = ti[i].bg = COLOR_DEFAULT;
	ti[i].userdata = h;
	if ( h->ghost && h->width>0 )
	    sprintf( buffer, "H<%g,%g>",
		    rint(h->start*100)/100+rint(h->width*100)/100, -rint(h->width*100)/100 );
	else
	    sprintf( buffer, "H<%g,%g>",
		    rint(h->start*100)/100, rint(h->width*100)/100 );
	ti[i].text = uc_copy(buffer);
	if ( hm!=NULL && ((*hm)[i>>3]&(0x80>>(i&7))))
	    ti[i].selected = true;
    }

    for ( h=sc->vstem    ; h!=NULL; h=h->next, ++i ) {
	ti[i].fg = ti[i].bg = COLOR_DEFAULT;
	ti[i].userdata = h;
	if ( h->ghost && h->width>0 )
	    sprintf( buffer, "V<%g,%g>",
		    rint(h->start*100)/100+rint(h->width*100)/100, -rint(h->width*100)/100 );
	else
	    sprintf( buffer, "V<%g,%g>",
		    rint(h->start*100)/100, rint(h->width*100)/100 );
	ti[i].text = uc_copy(buffer);
	if ( hm!=NULL && ((*hm)[i>>3]&(0x80>>(i&7))))
	    ti[i].selected = true;
    }
return( ti );
}

static void PointGetInfo(CharView *cv, SplinePoint *sp, SplinePointList *spl) {
    CharViewTab* tab = CVGetActiveTab(cv);
    GIData* gi = 0;
    GRect pos;
    GWindowAttrs wattrs;
    const int gcdcount = 54;
    GGadgetCreateData gcd[gcdcount], hgcd[2], h2gcd[2], mgcd[11];
    GGadgetCreateData mb[5], pb[9];
    GGadgetCreateData *marray[11], *marray2[5], *marray3[5], *marray4[7],
	*varray[11], *harray1[4], *harray2[6], *hvarray1[25], *hvarray2[25],
	*hvarray3[16], *harray3[13];
    GTextInfo label[54], mlabel[11];
    GTabInfo aspects[4];
    static GBox cur, nextcp, prevcp;
    extern Color nextcpcol, prevcpcol;
    GWindow root;
    GRect screensize;
    GPoint pt;
    int j, defxpos, nextstarty, k, l;

    gi = calloc(1,sizeof(GIData));

    cur.main_background = nextcp.main_background = prevcp.main_background = COLOR_DEFAULT;
    cur.main_foreground = 0xff0000;
    nextcp.main_foreground = nextcpcol;
    prevcp.main_foreground = prevcpcol;
    gi->cv = cv;
    gi->sc = cv->b.sc;
    gi->cursp = sp;
    gi->curspl = spl;
    gi->oldstate = SplinePointListCopy(cv->b.layerheads[cv->b.drawmode]->splines);
    gi->done = false;
    CVPreserveState(&cv->b);

    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.positioned = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Point Info");
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,PI_Width));
	pos.height = GDrawPointsToPixels(NULL,PI_Height);
	pt.x = tab->xoff + rint(sp->me.x*tab->scale);
	pt.y = -tab->yoff + cv->height - rint(sp->me.y*tab->scale);
	GDrawTranslateCoordinates(cv->v,root,&pt);
	if ( pt.x+20+pos.width<=screensize.width )
	    pos.x = pt.x+20;
	else if ( (pos.x = pt.x-10-screensize.width)<0 )
	    pos.x = 0;
	pos.y = pt.y;
	if ( pos.y+pos.height+20 > screensize.height )
	    pos.y = screensize.height - pos.height - 20;
	if ( pos.y<0 ) pos.y = 0;
	gi->gw = GDrawCreateTopWindow(NULL,&pos,pi_e_h,gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	memset(&hgcd,0,sizeof(hgcd));
	memset(&h2gcd,0,sizeof(h2gcd));
	memset(&mgcd,0,sizeof(mgcd));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,0,sizeof(aspects));
	memset(&pb,0,sizeof(pb));

	j=k=0;
	gi->gcd = malloc( gcdcount*sizeof(GGadgetCreateData) );
	memcpy( gi->gcd, gcd, gcdcount*sizeof(GGadgetCreateData) );

	label[j].text = (unichar_t *) _("_Normal");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = 5;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[j].gd.cid = CID_Normal;
	gcd[j].gd.handle_controlevent = PI_InterpChanged;
	gcd[j].creator = GRadioCreate;
	harray1[0] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) _("_Interpolated");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 70; gcd[j].gd.pos.y = 5;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_Interpolated;
	gcd[j].gd.handle_controlevent = PI_InterpChanged;
	gcd[j].creator = GRadioCreate;
	harray1[1] = &gcd[j]; harray1[2] = GCD_Glue; harray1[3] = NULL;
	++j;

	pb[2].gd.flags = gg_enabled|gg_visible;
	pb[2].gd.u.boxelements = harray1;
	pb[2].creator = GHBoxCreate;
	varray[k++] = &pb[2];

	label[j].text = (unichar_t *) _("N_ever Interpolate");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+16;
	gcd[j].gd.flags = cv->b.layerheads[cv->b.drawmode]->order2 ? (gg_enabled|gg_visible) : gg_visible;
	gcd[j].gd.cid = CID_NeverInterpolate;
	gcd[j].gd.handle_controlevent = PI_NeverInterpChanged;
	gcd[j].creator = GCheckBoxCreate;
	varray[k++] = &gcd[j];
	++j;

	gi->normal_start = j;
	label[j].text = (unichar_t *) _("_Base:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+17+6;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &cur;
	gcd[j].creator = GLabelCreate;
	harray2[0] = &gcd[j]; harray2[1] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-6; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseX;
	gcd[j].gd.handle_controlevent = PI_BaseChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray2[2] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseY;
	gcd[j].gd.handle_controlevent = PI_BaseChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray2[3] = &gcd[j]; harray2[4] = GCD_Glue; harray2[5] = NULL;
	++j;

	pb[3].gd.flags = gg_enabled|gg_visible;
	pb[3].gd.u.boxelements = harray2;
	pb[3].creator = GHBoxCreate;
	varray[k++] = &pb[3];

	l = 0;
	label[j].text = (unichar_t *) _("Prev CP:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+24+4;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &prevcp;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 65;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevPos;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j];
	++j;

	defxpos = 130;
	label[j].text = (unichar_t *) S_("ControlPoint|Default");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = defxpos; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-3;
	gcd[j].gd.flags = (gg_enabled|gg_visible);
	gcd[j].gd.cid = CID_PrevDef;
	gcd[j].gd.handle_controlevent = PI_PrevDefChanged;
	gcd[j].creator = GCheckBoxCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_ColSpan; hvarray1[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Offset");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+18+4;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevXOff;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray1[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevYOff;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_ColSpan; hvarray1[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Dist");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevR;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray1[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 60;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevTheta;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GTextFieldCreate;
	hvarray1[l++] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) U_("°");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-1].gd.pos.x+gcd[j-1].gd.pos.width+2; gcd[j].gd.pos.y = gcd[j-3].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Curvature: -0.00000000");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+20;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevCurvature;
	gcd[j].creator = GLabelCreate;
	hvarray1[l++] = &gcd[j]; hvarray1[l++] = GCD_ColSpan; hvarray1[l++] = GCD_ColSpan;
	  hvarray1[l++] = GCD_Glue; hvarray1[l++] = GCD_Glue; hvarray1[l++] = NULL;
	 hvarray1[l++] = NULL;
	++j;

	pb[4].gd.pos.x = pb[4].gd.pos.y = 2;
	pb[4].gd.flags = gg_enabled|gg_visible;
	pb[4].gd.u.boxelements = hvarray1;
	pb[4].creator = GHVGroupCreate;
	varray[k++] = &pb[4];

	l = 0;
	nextstarty = gcd[j-2].gd.pos.y+28;
	label[j].text = (unichar_t *) _("Next CP:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = nextstarty;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &nextcp;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = nextstarty;  gcd[j].gd.pos.width = 65;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextPos;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) S_("ControlPoint|Default");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = defxpos; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-3;
	gcd[j].gd.flags = (gg_enabled|gg_visible);
	gcd[j].gd.cid = CID_NextDef;
	gcd[j].gd.handle_controlevent = PI_NextDefChanged;
	gcd[j].creator = GCheckBoxCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_ColSpan; hvarray2[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Offset");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+18+4;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextXOff;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray2[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextYOff;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_ColSpan; hvarray2[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Dist");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextR;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray2[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 60;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextTheta;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GTextFieldCreate;
	hvarray2[l++] = &gcd[j];
	++j;

	label[j].text = (unichar_t *) U_("°");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-1].gd.pos.x+gcd[j-1].gd.pos.width+2; gcd[j].gd.pos.y = gcd[j-3].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Curvature: -0.00000000");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+20;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextCurvature;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_ColSpan; hvarray2[l++] = GCD_ColSpan;
	++j;

	label[j].text = (unichar_t *) "∆: -0.00000000";
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 130; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.popup_msg = _("This is the difference of the curvature between\nthe next and previous splines. Contours often\nlook nicer as this number approaches 0." );
	gcd[j].gd.cid = CID_DeltaCurvature;
	gcd[j].creator = GLabelCreate;
	hvarray2[l++] = &gcd[j]; hvarray2[l++] = GCD_ColSpan; hvarray2[l++] = NULL;
	hvarray2[l++] = NULL;
	++j;

	pb[5].gd.pos.x = pb[5].gd.pos.y = 2;
	pb[5].gd.flags = gg_enabled|gg_visible;
	pb[5].gd.u.boxelements = hvarray2;
	pb[5].creator = GHVGroupCreate;
	varray[k++] = &pb[5];
	gi->normal_end = j;

	gi->interp_start = j;
	l = 0;
	label[j].text = (unichar_t *) _("_Base:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[gi->normal_start].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &cur;
	gcd[j].creator = GLabelCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 65;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BasePos;
	gcd[j].creator = GLabelCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = GCD_Glue; hvarray3[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Prev CP:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14+4;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &prevcp;
	gcd[j].creator = GLabelCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevX;
	gcd[j].gd.handle_controlevent = PI_PrevIntChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray3[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevY;
	gcd[j].gd.handle_controlevent = PI_PrevIntChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = NULL;
	++j;

	label[j].text = (unichar_t *) _("Next CP:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+24+4;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &nextcp;
	gcd[j].creator = GLabelCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextX;
	gcd[j].gd.handle_controlevent = PI_NextIntChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray3[l++] = &gcd[j];
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextY;
	gcd[j].gd.handle_controlevent = PI_NextIntChanged;
	gcd[j].creator = GNumericFieldCreate;
	hvarray3[l++] = &gcd[j]; hvarray3[l++] = NULL; hvarray3[l++] = NULL;
	++j;
	gi->interp_end = j;

	pb[6].gd.flags = gg_enabled|gg_visible;
	pb[6].gd.u.boxelements = hvarray3;
	pb[6].creator = GHVBoxCreate;
	varray[k++] = &pb[6];
	varray[k++] = GCD_Glue;

	label[j].text = (unichar_t *) _("Type:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[0].gd.pos.x; gcd[j].gd.pos.y = gcd[gi->normal_end-2].gd.pos.y+26;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray3[0] = &gcd[j]; harray3[1] = GCD_Glue; harray3[2] = GCD_Glue;
	++j;

	label[j].image = &GIcon_midcurve;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Curve;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	harray3[3] = &gcd[j]; harray3[4] = GCD_Glue;
	++j;

	label[j].image = &GIcon_midhvcurve;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_HVCurve;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	harray3[5] = &gcd[j]; harray3[6] = GCD_Glue;
	++j;

	label[j].image = &GIcon_midcorner;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 100; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_Corner;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	harray3[7] = &gcd[j]; harray3[8] = GCD_Glue;
	++j;

	label[j].image = &GIcon_midtangent;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 140; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_Tangent;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	harray3[9] = &gcd[j]; harray3[10] = GCD_Glue; harray3[11] = GCD_Glue;
	harray3[12] = NULL;
	++j;

	pb[7].gd.flags = gg_enabled|gg_visible;
	pb[7].gd.u.boxelements = harray3;
	pb[7].creator = GHBoxCreate;
	varray[k++] = &pb[7];
	varray[k++] = NULL;

	pb[0].gd.flags = gg_enabled|gg_visible;
	pb[0].gd.u.boxelements = varray;
	pb[0].creator = GVBoxCreate;

	hgcd[0].gd.pos.x = 5; hgcd[0].gd.pos.y = 5;
	hgcd[0].gd.pos.width = PI_Width-20; hgcd[0].gd.pos.height = gcd[j-1].gd.pos.y+10;
	hgcd[0].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
	hgcd[0].gd.cid = CID_HintMask;
	hgcd[0].gd.u.list = SCHintList(cv->b.sc,NULL);
	hgcd[0].gd.handle_controlevent = PI_HintSel;
	hgcd[0].creator = GListCreate;

	h2gcd[0].gd.pos.x = 5; h2gcd[0].gd.pos.y = 5;
	h2gcd[0].gd.pos.width = PI_Width-20; h2gcd[0].gd.pos.height = gcd[j-1].gd.pos.y+10;
	h2gcd[0].gd.flags = gg_visible | gg_list_multiplesel;
	h2gcd[0].gd.cid = CID_ActiveHints;
	h2gcd[0].gd.u.list = SCHintList(cv->b.sc,NULL);
	h2gcd[0].creator = GListCreate;

	j = 0;

	aspects[j].text = (unichar_t *) _("Location");
	aspects[j].text_is_1byte = true;
	aspects[j++].gcd = pb;

	aspects[j].text = (unichar_t *) _("Hint Mask");
	aspects[j].text_is_1byte = true;
	aspects[j++].gcd = hgcd;

	aspects[j].text = (unichar_t *) _("Active Hints");
	aspects[j].text_is_1byte = true;
	aspects[j++].gcd = h2gcd;

	j = 0;

	mgcd[j].gd.pos.x = 4; mgcd[j].gd.pos.y = 6;
	mgcd[j].gd.pos.width = PI_Width-8;
	mgcd[j].gd.pos.height = hgcd[0].gd.pos.height+10+24;
	mgcd[j].gd.u.tabs = aspects;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mgcd[j].gd.handle_controlevent = PI_AspectChange;
	mgcd[j].gd.cid = CID_TabSet;
	mgcd[j++].creator = GTabSetCreate;

	mgcd[j].gd.pos.x = (PI_Width-2*50-10)/2; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y+mgcd[j-1].gd.pos.height+5;
	mgcd[j].gd.pos.width = 53; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _("< _Prev");
	mlabel[j].text_is_1byte = true;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.cid = CID_Prev;
	mgcd[j].gd.handle_controlevent = PI_NextPrev;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = PI_Width-50-(PI_Width-2*50-10)/2; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y;
	mgcd[j].gd.pos.width = 53; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _("_Next >");
	mlabel[j].text_is_1byte = true;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.cid = CID_Next;
	mgcd[j].gd.handle_controlevent = PI_NextPrev;
	mgcd[j].creator = GButtonCreate;
	++j;

/* Why 3? */
	mgcd[j].gd.pos.x = mgcd[j-2].gd.pos.x-50+3; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y+26;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _("Prev On Contour");
	mlabel[j].text_is_1byte = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.cid = CID_PrevC;
	mgcd[j].gd.handle_controlevent = PI_NextPrev;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = mgcd[j-2].gd.pos.x; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _("Next On Contour");
	mlabel[j].text_is_1byte = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.cid = CID_NextC;
	mgcd[j].gd.handle_controlevent = PI_NextPrev;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = 5; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y+28;
	mgcd[j].gd.pos.width = PI_Width-10;
	mgcd[j].gd.flags = gg_enabled|gg_visible;
	mgcd[j].creator = GLineCreate;
	++j;

	mgcd[j].gd.pos.x = 20-3; mgcd[j].gd.pos.y = PI_Height-33-3;
	mgcd[j].gd.pos.width = -1; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[j].text = (unichar_t *) _("_OK");
	mlabel[j].text_is_1byte = true;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.mnemonic = 'O';
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.handle_controlevent = PI_Ok;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = -20; mgcd[j].gd.pos.y = PI_Height-33;
	mgcd[j].gd.pos.width = -1; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[j].text = (unichar_t *) _("_Cancel");
	mlabel[j].text_is_1byte = true;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.mnemonic = 'C';
	mgcd[j].gd.handle_controlevent = PI_Cancel;
	mgcd[j].creator = GButtonCreate;
	++j;

	marray[0] = &mgcd[0]; marray[1] = NULL;
	marray[2] = &mb[2]; marray[3] = NULL;
	marray[4] = &mb[3]; marray[5] = NULL;
	marray[6] = &mgcd[5]; marray[7] = NULL;
	marray[8] = &mb[4]; marray[9] = NULL;
	marray[10] = NULL;
	marray2[0] = GCD_Glue; marray2[1] = &mgcd[1]; marray2[2] = &mgcd[2]; marray2[3] = GCD_Glue; marray2[4] = NULL;
	marray3[0] = GCD_Glue; marray3[1] = &mgcd[3]; marray3[2] = &mgcd[4]; marray3[3] = GCD_Glue; marray3[4] = NULL;
	marray4[0] = GCD_Glue; marray4[1] = &mgcd[6]; marray4[2] = GCD_Glue; marray4[3] = GCD_Glue; marray4[4] = &mgcd[7]; marray4[5] = GCD_Glue; marray4[6] = NULL;

	memset(mb,0,sizeof(mb));
	mb[0].gd.pos.x = mb[0].gd.pos.y = 2;
	mb[0].gd.flags = gg_enabled|gg_visible;
	mb[0].gd.u.boxelements = marray;
	mb[0].creator = GHVGroupCreate;

	mb[2].gd.flags = gg_enabled|gg_visible;
	mb[2].gd.u.boxelements = marray2;
	mb[2].creator = GHBoxCreate;

	mb[3].gd.flags = gg_enabled|gg_visible;
	mb[3].gd.u.boxelements = marray3;
	mb[3].creator = GHBoxCreate;

	mb[4].gd.flags = gg_enabled|gg_visible;
	mb[4].gd.u.boxelements = marray4;
	mb[4].creator = GHBoxCreate;

	GGadgetsCreate(gi->gw,mb);
	gi->group1ret = pb[4].ret; gi->group2ret = pb[5].ret;
	memcpy( gi->gcd, gcd, gcdcount*sizeof(GGadgetCreateData) ); // This copies pointers, but only to static things.
	GTextInfoListFree(hgcd[0].gd.u.list);
	GTextInfoListFree(h2gcd[0].gd.u.list);

	GHVBoxSetExpandableRow(mb[0].ret,0);
	GHVBoxSetExpandableCol(mb[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(mb[3].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(mb[4].ret,gb_expandgluesame);
	GHVBoxSetExpandableRow(pb[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[4].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[5].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[6].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[7].ret,gb_expandglue);

	PIChangePoint(gi);

	GHVBoxFitWindow(mb[0].ret);

	dlist_pushfront( &cv->pointInfoDialogs, (struct dlistnode *)gi );
	GDrawResize(gi->gw,
		    GGadgetScale(GDrawPointsToPixels(NULL,PI_Width)),
		    GDrawPointsToPixels(NULL,PI_Height));
	GDrawSetVisible(gi->gw,true);
}

/* ************************************************************************** */
/* ****************************** Spiro Points ****************************** */
/* ************************************************************************** */

static void SpiroFillup(GIData *ci, int except_cid) {
    char buffer[50];
    int ty;

    mysprintf(buffer, "%.2f", ci->curcp->x );
    if ( except_cid!=CID_BaseX )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_BaseX),buffer);

    mysprintf(buffer, "%.2f", ci->curcp->y );
    if ( except_cid!=CID_BaseY )
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_BaseY),buffer);

    ty = ci->curcp->ty&0x7f;
    if ( ty == SPIRO_OPEN_CONTOUR )
	ty = SPIRO_G4;
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Curve), ty==SPIRO_G4 );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Tangent), ty==SPIRO_G2 );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Corner), ty==SPIRO_CORNER );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_SpiroLeft), ty==SPIRO_LEFT );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_SpiroRight), ty==SPIRO_RIGHT );
}

static void SpiroChangePoint(GIData *ci) {

    SpiroFillup(ci,0);
}

static int PI_SpiroNextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	int cid = GGadgetGetCid(g);
	SplinePointList *spl;
	int index = ci->curcp - ci->curspl->spiros;
	BasePoint here;

	SPIRO_DESELECT(ci->curcp);
	if ( cid == CID_Next ) {
	    if ( index < ci->curspl->spiro_cnt-2 )
		ci->curcp = &ci->curspl->spiros[index+1];
	    else {
		if ( ci->curspl->next == NULL ) {
		    ci->curspl = cv->b.layerheads[cv->b.drawmode]->splines;
		    GDrawBeep(NULL);
		} else
		    ci->curspl = ci->curspl->next;
		ci->curcp = &ci->curspl->spiros[0];
	    }
	} else if ( cid == CID_Prev ) {
	    if ( index!=0 ) {
		ci->curcp = &ci->curspl->spiros[index-1];
	    } else {
		if ( ci->curspl==cv->b.layerheads[cv->b.drawmode]->splines ) {
		    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=NULL; spl=spl->next );
		    GDrawBeep(NULL);
		} else {
		    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=ci->curspl; spl=spl->next );
		}
		ci->curspl = spl;
		ci->curcp = &ci->curspl->spiros[ci->curspl->spiro_cnt-2];
	    }
	} else if ( cid==CID_NextC ) {
	    if ( index < ci->curspl->spiro_cnt-2 )
		ci->curcp = &ci->curspl->spiros[index+1];
	    else {
		ci->curcp = &ci->curspl->spiros[0];
		GDrawBeep(NULL);
	    }
	} else /* CID_PrevC */ {
	    if ( index!=0 )
		ci->curcp = &ci->curspl->spiros[index-1];
	    else {
		ci->curcp = &ci->curspl->spiros[ci->curspl->spiro_cnt-2];
		GDrawBeep(NULL);
	    }
	}
	SPIRO_SELECT(ci->curcp);
	SpiroChangePoint(ci);
	here.x = ci->curcp->x; here.y = ci->curcp->y;
	CVShowPoint(cv,&here);
	SCUpdateAll(cv->b.sc);
    }
return( true );
}

static int PI_SpiroChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent &&
	    (e->u.control.subtype == et_textchanged ||
	     e->u.control.subtype == et_radiochanged)) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	double x,y;
	int ty;
	int err=false;
	spiro_cp *curcp = ci->curcp;

	x = GetCalmReal8(ci->gw,CID_BaseX,_("X"),&err);
	y = GetCalmReal8(ci->gw,CID_BaseY,_("Y"),&err);
	ty = GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Curve))? SPIRO_G4 :
	     GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Tangent))? SPIRO_G2 :
	     GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Corner))? SPIRO_CORNER :
	     GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_SpiroLeft))? SPIRO_LEFT :
		     SPIRO_RIGHT;
	curcp->x = x;
	curcp->y = y;
	curcp->ty = ty | 0x80;
	SSRegenerateFromSpiros(ci->curspl);
	CVCharChangedUpdate(&ci->cv->b);
    }
return( true );
}

static void PI_SpiroDoOk(GIData *ci) {
	ci->done = true;
	/* All the work has been done as we've gone along */
	PI_Close(ci);
}

static int PI_SpiroOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_SpiroDoOk(ci);
    }
    return( true );
}

static int spi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI_SpiroDoOk(GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/getinfo.html", NULL);
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static void SpiroPointGetInfo(CharView *cv, spiro_cp *scp, SplinePointList *spl) {
    CharViewTab* tab = CVGetActiveTab(cv);
    GIData *gip = calloc(1, sizeof(GIData));
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GGadgetCreateData pb[9];
    GGadgetCreateData *varray[20], *harray2[10], *harray3[15], *harray4[6],
	    *harray5[6], *harray6[9];
    GTextInfo label[20];
    GWindow root;
    GRect screensize;
    GPoint pt;
    int j,k;

    gip->cv = cv;
    gip->sc = cv->b.sc;
    gip->curcp = scp;
    gip->curspl = spl;
    gip->oldstate = SplinePointListCopy(cv->b.layerheads[cv->b.drawmode]->splines);
    gip->done = false;
    CVPreserveState(&cv->b);

    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.positioned = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Spiro Point Info");
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,PI_Width));
	pos.height = GDrawPointsToPixels(NULL,PI_Height);
	pt.x = tab->xoff + rint(scp->x*tab->scale);
	pt.y = -tab->yoff + cv->height - rint(scp->y*tab->scale);
	GDrawTranslateCoordinates(cv->v,root,&pt);
	if ( pt.x+20+pos.width<=screensize.width )
	    pos.x = pt.x+20;
	else if ( (pos.x = pt.x-10-screensize.width)<0 )
	    pos.x = 0;
	pos.y = pt.y;
	if ( pos.y+pos.height+20 > screensize.height )
	    pos.y = screensize.height - pos.height - 20;
	if ( pos.y<0 ) pos.y = 0;
	gip->gw = GDrawCreateTopWindow(NULL,&pos,spi_e_h,gip,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	memset(&pb,0,sizeof(pb));

	j=k=0;
	gip->gcd = gcd;

	label[j].text = (unichar_t *) _("_X:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray2[0] = &gcd[j]; harray2[1] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-6; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseX;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray2[2] = &gcd[j]; harray2[3] = GCD_Glue; harray2[4] = GCD_Glue;
	++j;

	label[j].text = (unichar_t *) _("_Y:");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray2[5] = &gcd[j]; harray2[6] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseY;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GNumericFieldCreate;
	harray2[7] = &gcd[j]; harray2[8] = NULL;
	++j;

	pb[2].gd.flags = gg_enabled|gg_visible;
	pb[2].gd.u.boxelements = harray2;
	pb[2].creator = GHBoxCreate;
	varray[k++] = &pb[2]; varray[k++] = NULL;

	label[j].text = (unichar_t *) _("Type:");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[0].gd.pos.x;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	harray3[0] = &gcd[j]; harray3[1] = GCD_Glue; harray3[2] = GCD_Glue;
	++j;

	label[j].image = &GIcon_smallspirocurve;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Curve;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GRadioCreate;
	harray3[3] = &gcd[j]; harray3[4] = GCD_Glue;
	++j;

	label[j].image = &GIcon_smallspirog2curve;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Tangent;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GRadioCreate;
	harray3[5] = &gcd[j]; harray3[6] = GCD_Glue;
	++j;

	label[j].image = &GIcon_smallspirocorner;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 100; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_Corner;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GRadioCreate;
	harray3[7] = &gcd[j]; harray3[8] = GCD_Glue;
	++j;

	label[j].image = &GIcon_smallspiroleft;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 140; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_SpiroLeft;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GRadioCreate;
	harray3[9] = &gcd[j]; harray3[10] = GCD_Glue;
	++j;

	label[j].image = &GIcon_smallspiroright;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 140; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible | gg_rad_continueold;
	gcd[j].gd.cid = CID_SpiroRight;
	gcd[j].gd.handle_controlevent = PI_SpiroChanged;
	gcd[j].creator = GRadioCreate;
	harray3[11] = &gcd[j]; harray3[12] = GCD_Glue; harray3[13] = GCD_Glue;
	harray3[14] = NULL;
	++j;

	pb[3].gd.flags = gg_enabled|gg_visible;
	pb[3].gd.u.boxelements = harray3;
	pb[3].creator = GHBoxCreate;
	varray[k++] = &pb[3];
	varray[k++] = NULL;
	varray[k++] = GCD_Glue;
	varray[k++] = NULL;

	gcd[j].gd.flags = gg_visible | gg_enabled;
	label[j].text = (unichar_t *) _("< _Prev");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.cid = CID_Prev;
	gcd[j].gd.handle_controlevent = PI_SpiroNextPrev;
	gcd[j].creator = GButtonCreate;
	harray4[0] = GCD_Glue; harray4[1] = &gcd[j];
	++j;

	gcd[j].gd.flags = gg_visible | gg_enabled;
	label[j].text = (unichar_t *) _("_Next >");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.cid = CID_Next;
	gcd[j].gd.handle_controlevent = PI_SpiroNextPrev;
	gcd[j].creator = GButtonCreate;
	harray4[2] = &gcd[j]; harray4[3] = GCD_Glue; harray4[4] = NULL;
	++j;

	pb[4].gd.flags = gg_enabled|gg_visible;
	pb[4].gd.u.boxelements = harray4;
	pb[4].creator = GHBoxCreate;
	varray[k++] = &pb[4];
	varray[k++] = NULL;

	gcd[j].gd.flags = gg_visible | gg_enabled;
	label[j].text = (unichar_t *) _("Prev On Contour");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.cid = CID_PrevC;
	gcd[j].gd.handle_controlevent = PI_SpiroNextPrev;
	gcd[j].creator = GButtonCreate;
	harray5[0] = GCD_Glue; harray5[1] = &gcd[j];
	++j;

	gcd[j].gd.flags = gg_visible | gg_enabled;
	label[j].text = (unichar_t *) _("Next On Contour");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.cid = CID_NextC;
	gcd[j].gd.handle_controlevent = PI_SpiroNextPrev;
	gcd[j].creator = GButtonCreate;
	harray5[2] = &gcd[j]; harray5[3] = GCD_Glue; harray5[4] = NULL;
	++j;

	pb[5].gd.flags = gg_enabled|gg_visible;
	pb[5].gd.u.boxelements = harray5;
	pb[5].creator = GHBoxCreate;
	varray[k++] = &pb[5];
	varray[k++] = NULL;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.pos.width = PI_Width-10;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLineCreate;
	++j;
	varray[k++] = &gcd[j-1];
	varray[k++] = NULL;
	varray[k++] = GCD_Glue;
	varray[k++] = NULL;

	gcd[j].gd.pos.x = 20-3; gcd[j].gd.pos.y = PI_Height-33-3;
	gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[j].text = (unichar_t *) _("_OK");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.mnemonic = 'O';
	gcd[j].gd.label = &label[j];
	gcd[j].gd.handle_controlevent = PI_SpiroOk;
	gcd[j].creator = GButtonCreate;
	harray6[0] = GCD_Glue; harray6[1] = &gcd[j]; harray6[2] = GCD_Glue; harray6[3] = GCD_Glue;
	++j;

	gcd[j].gd.pos.x = -20; gcd[j].gd.pos.y = PI_Height-33;
	gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[j].text = (unichar_t *) _("_Cancel");
	label[j].text_is_1byte = true;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.mnemonic = 'C';
	gcd[j].gd.handle_controlevent = PI_Cancel;
	gcd[j].creator = GButtonCreate;
	harray6[4] = GCD_Glue; harray6[5] = &gcd[j]; harray6[6] = GCD_Glue; harray6[7] = NULL;
	++j;

	pb[6].gd.flags = gg_enabled|gg_visible;
	pb[6].gd.u.boxelements = harray6;
	pb[6].creator = GHBoxCreate;
	varray[k++] = &pb[6];
	varray[k++] = NULL;
	varray[k++] = NULL;

	pb[0].gd.pos.x = pb[0].gd.pos.y = 2;
	pb[0].gd.flags = gg_enabled|gg_visible;
	pb[0].gd.u.boxelements = varray;
	pb[0].creator = GHVGroupCreate;

	GGadgetsCreate(gip->gw,pb);

	GHVBoxSetExpandableRow(pb[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[4].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[5].ret,gb_expandglue);
	GHVBoxSetExpandableCol(pb[6].ret,gb_expandgluesame);

	SpiroChangePoint(gip);

	GHVBoxFitWindow(pb[0].ret);

    GDrawSetVisible(gip->gw,true);
    while ( !gip->done )
	GDrawProcessOneEvent(NULL);
}

void CVGetInfo(CharView *cv) {
    SplinePoint *sp;
    SplinePointList *spl;
    RefChar *ref;
    ImageList *img;
    AnchorPoint *ap;
    spiro_cp *scp;

    if ( !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap,&scp)) {
    } else if ( ref!=NULL )
	RefGetInfo(cv,ref);
    else if ( img!=NULL )
	ImgGetInfo(cv,img);
    else if ( ap!=NULL )
	ApGetInfo(cv,ap);
    else if ( scp!=NULL )
	SpiroPointGetInfo(cv,scp,spl);
    else
	PointGetInfo(cv,sp,spl);
}

void CVPGetInfo(CharView *cv) {

    if ( cv->p.ref!=NULL )
	RefGetInfo(cv,cv->p.ref);
    else if ( cv->p.img!=NULL )
	ImgGetInfo(cv,cv->p.img);
    else if ( cv->p.ap!=NULL )
	ApGetInfo(cv,cv->p.ap);
    else if ( cv->p.sp!=NULL )
	PointGetInfo(cv,cv->p.sp,cv->p.spl);
    else if ( cv->p.spiro!=NULL )
	SpiroPointGetInfo(cv,cv->p.spiro,cv->p.spl);
}

void SCRefBy(SplineChar *sc) {
    int cnt,i,tot=0;
    char **deps = NULL;
    struct splinecharlist *d;
    char *buts[3];

    buts[0] = _("_Show");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( d = sc->dependents; d!=NULL; d=d->next ) {
	    if ( deps!=NULL )
		deps[tot-cnt] = copy(d->sc->name);
	    ++cnt;
	}
	if ( cnt==0 )
return;
	if ( i==0 )
	    deps = calloc(cnt+1,sizeof(unichar_t *));
	tot = cnt-1;
    }

    i = gwwv_choose_with_buttons(_("Dependents"),(const char **) deps, cnt, 0, buts, _("Dependents") );
    if ( i!=-1 ) {
	i = tot-i;
	for ( d = sc->dependents, cnt=0; d!=NULL && cnt<i; d=d->next, ++cnt );
	CharViewCreate(d->sc,(FontView *) (sc->parent->fv),-1);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
}

static int UsedIn(char *name, char *subs) {
    int nlen = strlen( name );
    while ( *subs!='\0' ) {
	if ( strncmp(subs,name,nlen)==0 && (subs[nlen]==' ' || subs[nlen]=='\0'))
return( true );
	while ( *subs!=' ' && *subs!='\0' ) ++subs;
	while ( *subs==' ' ) ++subs;
    }
return( false );
}

int SCUsedBySubs(SplineChar *sc) {
    int k, i;
    SplineFont *_sf, *sf;
    PST *pst;

    if ( sc==NULL )
return( false );

    _sf = sc->parent;
    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k=0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			pst->type==pst_multiple || pst->type==pst_ligature )
		    if ( UsedIn(sc->name,pst->u.mult.components))
return( true );
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
return( false );
}

void SCSubBy(SplineChar *sc) {
    int i,j,k,tot;
    char **deps = NULL;
    SplineChar **depsc;
    char ubuf[200];
    SplineFont *sf, *_sf;
    PST *pst;
    char *buts[3];

    buts[0] = _("Show");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    if ( sc==NULL )
return;

    _sf = sc->parent;
    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    for ( j=0; j<2; ++j ) {
	tot = 0;
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			    pst->type==pst_multiple || pst->type==pst_ligature )
			if ( UsedIn(sc->name,pst->u.mult.components)) {
			    if ( deps!=NULL ) {
				snprintf(ubuf,sizeof(ubuf),
					_("Subtable %.60s in glyph %.60s"),
			                pst->subtable->subtable_name,
			                sf->glyphs[i]->name);
				deps[tot] = copy(ubuf);
			        depsc[tot] = sf->glyphs[i];
			    }
			    ++tot;
			}
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
	if ( tot==0 )
return;
	if ( j==0 ) {
	    deps = calloc(tot+1,sizeof(char *));
	    depsc = malloc(tot*sizeof(SplineChar *));
	}
    }

    i = gwwv_choose_with_buttons(_("Dependent Substitutions"),(const char **) deps, tot, 0, buts, _("Dependent Substitutions") );
    if ( i>-1 ) {
	CharViewCreate(depsc[i],(FontView *) (sc->parent->fv),-1);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
    free(depsc);
}
