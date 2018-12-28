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
#include "bvedit.h"
#include "splineutil.h"
#include "fontforgeui.h"
#include <math.h>
#include <ustring.h>
#include "fvmetrics.h"

typedef struct createwidthdlg {
    CreateWidthData wd;
    GWindow gw;
} CreateWidthDlg;

#define CID_Set		1001
#define CID_Incr	1002
#define CID_Scale	1003
#define CID_SetVal	1011
#define CID_IncrVal	1012
#define CID_ScaleVal	1013

static char *rb1[] = { N_("Set Width To:"), N_("Set LBearing To:"), N_("Set RBearing To:"), N_("Set Bearings To:"), N_("Set Vert. Advance To:") };
static char *rb2[] = { N_("Increment Width By:"), N_("Increment LBearing By:"), N_("Increment RBearing By:"), N_("Increment Bearings By:"), N_("Increment V. Adv. By:") };
static char *rb3[] = { N_("Scale Width By:"), N_("Scale LBearing By:"), N_("Scale RBearing By:"), N_("Scale Bearings By:"), N_("Scale VAdvance By:") };
static char *info[] = { N_("Left Side Bearing does not change."), N_("Advance Width does not change."), N_("Left Side Bearing does not change."), N_("ThisSpaceIntentionallyLeftBlank-PleaseDoNotTranslate-LeaveThisOut|"), N_("Top Bearing does not change.") };

static int CW_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err = false;
	CreateWidthDlg *wd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(wd->gw,CID_Set)) ) {
	    wd->wd.type = st_set;
	    wd->wd.setto = GetReal8(wd->gw,CID_SetVal,rb1[wd->wd.wtype],&err);
	    if ( wd->wd.setto<0 && wd->wd.wtype==wt_width ) {
		char *yesno[3];
		yesno[0] = _("_Yes");
		yesno[1] = _("_No");
		yesno[2] = NULL;
		if ( gwwv_ask(_("Negative Width"), (const char **) yesno, 0, 1, _("Negative glyph widths are not allowed in TrueType\nDo you really want a negative width?") )==1 )
return( true );
	    }
	} else if ( GGadgetIsChecked(GWidgetGetControl(wd->gw,CID_Incr)) ) {
	    wd->wd.type = st_incr;
	    wd->wd.increment = GetReal8(wd->gw,CID_IncrVal,rb2[wd->wd.wtype],&err);
	} else {
	    wd->wd.type = st_scale;
	    wd->wd.scale = GetReal8(wd->gw,CID_ScaleVal,rb2[wd->wd.wtype],&err);
	}
	if ( err )
return(true);
	(wd->wd.doit)((CreateWidthData *) wd);
    }
return( true );
}

static int CW_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateWidthDlg *wd = GDrawGetUserData(GGadgetGetWindow(g));
	wd->wd.done = true;
    }
return( true );
}

static int CW_FocusChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	CreateWidthDlg *wd = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intpt) GGadgetGetUserData(g);
	GGadgetSetChecked(GWidgetGetControl(wd->gw,cid),true);
    }
return( true );
}

static int CW_RadioChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CreateWidthDlg *wd = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intpt) GGadgetGetUserData(g);
	GWidgetIndicateFocusGadget(GWidgetGetControl(wd->gw,cid));
	GTextFieldSelect(GWidgetGetControl(wd->gw,cid),0,-1);
    }
return( true );
}

static int cwd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CreateWidthDlg *wd = GDrawGetUserData(gw);
	wd->wd.done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static void FVCreateWidth( void *_fv,SplineChar* _sc,void (*doit)(CreateWidthData *),
			   enum widthtype wtype, char *def) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11], boxes[2], topbox[2], *hvs[17], *varray[8], *buttons[6];
    GTextInfo label[11];
    static CreateWidthDlg cwd;
    static GWindow winds[5] = {NULL, NULL, NULL, NULL, NULL};
    static char *title[] = { N_("Set Width..."), N_("Set LBearing..."), N_("Set RBearing..."), N_("Set Both Side Bearings..."), N_("Set Vertical Advance...") };

    cwd.wd.done = false;
    cwd.wd._fv = _fv;
    cwd.wd._sc = _sc;
    cwd.wd.wtype = wtype;
    cwd.wd.doit = doit;
    cwd.gw = winds[wtype];

    if ( cwd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _(title[wtype]);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,180));
	pos.height = GDrawPointsToPixels(NULL,100);
	cwd.gw = winds[wtype] = GDrawCreateTopWindow(NULL,&pos,cwd_e_h,&cwd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _(rb1[wtype]);
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].gd.cid = CID_Set;
	gcd[0].gd.handle_controlevent = CW_RadioChange;
	gcd[0].data = (void *) CID_SetVal;
	gcd[0].creator = GRadioCreate;

	label[1].text = (unichar_t *) _(rb2[wtype]);
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.flags = gg_enabled|gg_visible|gg_rad_continueold ;
	gcd[1].gd.cid = CID_Incr;
	gcd[1].gd.handle_controlevent = CW_RadioChange;
	gcd[1].data = (void *) CID_IncrVal;
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) _(rb3[wtype]);
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.flags = gg_enabled|gg_visible|gg_rad_continueold ;
	gcd[2].gd.cid = CID_Scale;
	gcd[2].gd.handle_controlevent = CW_RadioChange;
	gcd[2].data = (void *) CID_ScaleVal;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) def;
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.width = 60;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.cid = CID_SetVal;
	gcd[3].gd.handle_controlevent = CW_FocusChange;
	gcd[3].data = (void *) CID_Set;
	gcd[3].creator = GTextFieldCreate;

	label[4].text = (unichar_t *) "0";
	label[4].text_is_1byte = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.width = 60;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_IncrVal;
	gcd[4].gd.handle_controlevent = CW_FocusChange;
	gcd[4].data = (void *) CID_Incr;
	gcd[4].creator = GTextFieldCreate;

	label[5].text = (unichar_t *) "100";
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.width = 60;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = CID_ScaleVal;
	gcd[5].gd.handle_controlevent = CW_FocusChange;
	gcd[5].data = (void *) CID_Scale;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[6].text = (unichar_t *) _("_OK");
	label[6].text_is_1byte = true;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'O';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.handle_controlevent = CW_OK;
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[7].text = (unichar_t *) _("_Cancel");
	label[7].text_is_1byte = true;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.mnemonic = 'C';
	gcd[7].gd.handle_controlevent = CW_Cancel;
	gcd[7].creator = GButtonCreate;

	label[8].text = (unichar_t *) S_(info[wtype]);
	label[8].text_is_1byte = true;
	gcd[8].gd.label = &label[8];
	gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = 59; 
	gcd[8].gd.flags = gg_enabled|gg_visible ;
	gcd[8].creator = GLabelCreate;

	label[9].text = (unichar_t *) "%";
	label[9].text_is_1byte = true;
	gcd[9].gd.label = &label[9];
	gcd[9].gd.flags = gg_enabled|gg_visible;
	gcd[9].creator = GLabelCreate;

	hvs[0] = &gcd[0]; hvs[1] = &gcd[3]; hvs[2] = GCD_Glue; hvs[3] = NULL;
	hvs[4] = &gcd[1]; hvs[5] = &gcd[4]; hvs[6] = GCD_Glue; hvs[7] = NULL;
	hvs[8] = &gcd[2]; hvs[9] = &gcd[5]; hvs[10] = &gcd[9]; hvs[11] = NULL;
	hvs[12] = &gcd[8]; hvs[13] = GCD_ColSpan; hvs[14] = GCD_Glue; hvs[15] = NULL;
	hvs[16] = NULL;

	buttons[0] = buttons[2] = buttons[4] = GCD_Glue; buttons[5] = NULL;
	buttons[1] = &gcd[6]; buttons[3] = &gcd[7];

	varray[0] = &boxes[1]; varray[1] = NULL;
	varray[2] = GCD_Glue; varray[3] = NULL;
	varray[4] = &boxes[0]; varray[5] = NULL;
	varray[6] = NULL;

	memset(boxes,0,sizeof(boxes));
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = buttons;
	boxes[0].creator = GHBoxCreate;

	boxes[1].gd.flags = gg_enabled|gg_visible;
	boxes[1].gd.u.boxelements = hvs;
	boxes[1].creator = GHVBoxCreate;

	memset(topbox,0,sizeof(topbox));
	topbox[0].gd.pos.x = topbox[0].gd.pos.y = 2;
	topbox[0].gd.pos.width = pos.width-4; topbox[0].gd.pos.height = pos.height-4;
	topbox[0].gd.flags = gg_enabled|gg_visible;
	topbox[0].gd.u.boxelements = varray;
	topbox[0].creator = GHVGroupCreate;

	GGadgetsCreate(cwd.gw,topbox);
	GHVBoxSetExpandableRow(topbox[0].ret,1);
	GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxes[1].ret,1);
	GWidgetIndicateFocusGadget(GWidgetGetControl(cwd.gw,CID_SetVal));
	GTextFieldSelect(GWidgetGetControl(cwd.gw,CID_SetVal),0,-1);
	GHVBoxFitWindow(topbox[0].ret);
    } else {
	unichar_t *temp = uc_copy(def);
	GGadgetSetTitle(GWidgetGetControl(cwd.gw,CID_SetVal),temp);
	free( temp );
    }

    GWidgetHidePalettes();
    GDrawSetVisible(cwd.gw,true);
    while ( !cwd.wd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(cwd.gw,false);
}

static void BCDefWidthVal(char *buf,BDFChar *bc, FontView *fv, enum widthtype wtype) {
    IBounds bb;

    if ( wtype==wt_width )
	sprintf( buf, "%d", bc->width );
    else if ( wtype==wt_vwidth )
	sprintf( buf, "%d", fv->show->pixelsize );
    else {
	BDFCharFindBounds(bc,&bb);
	if ( wtype==wt_lbearing )
	    sprintf( buf, "%d", bb.minx );
	else if ( wtype==wt_rbearing )
	    sprintf( buf, "%d", bc->width-bb.maxx-1 );
	else
	    sprintf( buf, "%d", (int) rint( (bc->width-bb.maxx-1 + bb.minx)/2 ));
    }
}

static void SCDefWidthVal(char *buf,SplineChar *sc, enum widthtype wtype) {
    DBounds bb;

    if ( wtype==wt_width )
	sprintf( buf, "%d", sc->width );
    else if ( wtype==wt_vwidth )
	sprintf( buf, "%d", sc->vwidth );
    else {
	SplineCharFindBounds(sc,&bb);
	if ( wtype==wt_lbearing )
	    sprintf( buf, "%.4g", (double) bb.minx );
	else if ( wtype==wt_rbearing )
	    sprintf( buf, "%.4g", sc->width-(double) bb.maxx );
	else
	    sprintf( buf, "%.4g", rint( (sc->width-(double) bb.maxx + (double) bb.minx)/2 ) );
    }
}

void FVSetWidth(FontView *fv,enum widthtype wtype) {
    char buffer[12];
    int em = fv->b.sf->ascent + fv->b.sf->descent;
    int i, gid;

    if ( !fv->b.sf->onlybitmaps || fv->b.sf->bitmaps==NULL ) {
	sprintf(buffer,"%d",wtype==wt_width?6*em/10:wtype==wt_vwidth?em: em/10 );
	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 && fv->b.sf->glyphs[gid]!=NULL ) {
	    SCDefWidthVal(buffer,fv->b.sf->glyphs[gid],wtype);
	break;
	}
    } else {
	int size = fv->show->pixelsize;
	sprintf(buffer,"%d",wtype==wt_width?6*size/10:wtype==wt_vwidth?size: size/10 );
	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 && fv->show->glyphs[gid]!=NULL ) {
	    BCDefWidthVal(buffer,fv->show->glyphs[gid],fv,wtype);
	break;
	}
    }
    FVCreateWidth(fv,0,FVDoit,wtype,buffer);
}

void CVSetWidth(CharView *cv,enum widthtype wtype) {
    char buf[10];

    SCDefWidthVal(buf,cv->b.sc,wtype);
    FVCreateWidth(cv,cv->b.sc,CVDoit,wtype,buf);
}


void GenericVSetWidth(FontView *fv,SplineChar* sc,enum widthtype wtype) {
    char buf[10];

    SCDefWidthVal(buf,sc,wtype);
    FVCreateWidth(fv,sc,GenericVDoit,wtype,buf);
}

