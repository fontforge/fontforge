/* Copyright (C) 2007,2008 by George Williams */
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
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include <math.h>

/* Code for various stylistic changes embolden/thin, condense/extend, oblique */


/* ************************************************************************** */
/* ***************************** Condense/Extend **************************** */
/* ************************************************************************** */

static void CVCondenseExtend(CharView *cv,struct counterinfo *ci) {
    SplineChar *sc = cv->b.sc;

    if ( cv->b.drawmode == dm_grid )
return;

    SCCondenseExtend(ci, sc, CVLayer((CharViewBase *) cv),true);

    free( ci->zones[0]);
    free( ci->zones[1]);
}

typedef struct styledlg {
    int done;
    GWindow gw;
    CharView *cv;
    FontView *fv;
    SplineFont *sf;
    int layer;
    struct smallcaps *small;
} StyleDlg;

#define CID_C_Factor	1001
#define CID_C_Add	1002
#define CID_SB_Factor	1003
#define CID_SB_Add	1004
#define CID_CorrectItalic	1005

static struct counterinfo last_ci = { 90, 0, 90, 0, true };

static int CondenseExtend_OK(GGadget *g, GEvent *e) {
    struct counterinfo ci;
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	memset(&ci,0,sizeof(ci));
	err = false;
	ci.c_factor  = GetReal8(ew,CID_C_Factor,_("Counter Expansion Factor"),&err);
	ci.c_add     = GetReal8(ew,CID_C_Add,_("Counter Addition"),&err);
	ci.sb_factor = GetReal8(ew,CID_SB_Factor,_("Side Bearing Expansion Factor"),&err);
	ci.sb_add    = GetReal8(ew,CID_SB_Add,_("Side Bearing Addition"),&err);
	ci.correct_italic= GGadgetIsChecked( GWidgetGetControl(ew,CID_CorrectItalic));
	if ( err )
return( true );

	last_ci = ci;

	CI_Init(&ci,ed->sf);
	if ( ed->fv!=NULL )
	    FVCondenseExtend((FontViewBase *) ed->fv, &ci);
	else
	    CVCondenseExtend(ed->cv, &ci);
	ed->done = true;
    }
return( true );
}

static int CondenseExtend_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	ed->done = true;
    }
return( true );
}

static int style_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	StyleDlg *ed = GDrawGetUserData(gw);
	ed->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("Styles.html");
return( true );
	}
return( false );
    }
return( true );
}

void CondenseExtendDlg(FontView *fv, CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->b.sf : cv->b.sc->parent;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[15], boxes[6], *barray[8], *hvarray[31];
    GTextInfo label[15];
    int k;
    char c_factor[40], c_add[40], sb_factor[40], sb_add[40];

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Condense/Extend");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    hvarray[0] = GCD_Glue;

    label[k].text = (unichar_t *) _("Scale By");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[1] = &gcd[k-1];
    hvarray[2] = GCD_Glue;

    label[k].text = (unichar_t *) _("Add");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[3] = &gcd[k-1];
    hvarray[4] = NULL;

    label[k].text = (unichar_t *) _("Counters:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[5] = &gcd[k-1];

    sprintf( c_factor, "%g", last_ci.c_factor );
    label[k].text = (unichar_t *) c_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_C_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[6] = &gcd[k-1];

    label[k].text = (unichar_t *) " % + ";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[7] = &gcd[k-1];

    sprintf( c_add, "%g", last_ci.c_add );
    label[k].text = (unichar_t *) c_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_C_Add;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[8] = &gcd[k-1];
    hvarray[9] = NULL;

    label[k].text = (unichar_t *) _("Side Bearings:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[10] = &gcd[k-1];

    sprintf( sb_factor, "%g", last_ci.sb_factor );
    label[k].text = (unichar_t *) sb_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SB_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[11] = &gcd[k-1];

    label[k].text = (unichar_t *) " % + ";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[12] = &gcd[k-1];

    sprintf( sb_add, "%g", last_ci.sb_add );
    label[k].text = (unichar_t *) sb_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SB_Add;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[13] = &gcd[k-1];
    hvarray[14] = NULL;

    label[k].text = (unichar_t *) _("Correct for Italic Angle");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | (last_ci.correct_italic?gg_cb_on:0);
    gcd[k].gd.cid = CID_CorrectItalic;
    gcd[k].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[15] = &gcd[k-1];
    hvarray[16] = hvarray[17] = hvarray[18] = GCD_ColSpan; hvarray[19] = NULL;
    if ( sf->italicangle==0 )
	gcd[k-1].gd.flags = ~gg_enabled;

    hvarray[20] = hvarray[21] = hvarray[22] = hvarray[23] = GCD_ColSpan; hvarray[24] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = CondenseExtend_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = CondenseExtend_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = barray;
    boxes[3].creator = GHBoxCreate;
    hvarray[25] = &boxes[3];
    hvarray[26] = hvarray[27] = hvarray[28] = GCD_ColSpan; hvarray[29] = NULL;
    hvarray[30] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

/* ************************************************************************** */
/* *************************** Small Caps Dialog **************************** */
/* ************************************************************************** */
#define CID_SCH_Is_XH	1001
#define CID_SCH_Lab	1002
#define CID_SCH		1003
#define CID_Cap_Lab	1004
#define CID_Cap		1005
#define CID_LC_Stem_Width	1006
#define CID_UC_Stem_Width	1007
#define CID_Symbols_Too	1008
#define CID_Letter_Ext	1009
#define CID_Symbol_Ext	1010

static int SmallCaps_OK(GGadget *g, GEvent *e) {
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	struct smallcaps *small = ed->small;
	if ( !GGadgetIsChecked(GWidgetGetControl(ew,CID_SCH_Is_XH))) {
	    small->scheight  = GetReal8(ew,CID_SCH,_("Small Caps Height"),&err);
	    small->capheight = GetReal8(ew,CID_Cap,_("Capital Height"),&err);
	    if ( err )
return( true );
	}
	small->lc_stem_width = GetReal8(ew,CID_LC_Stem_Width,_("Primary stem width for lower case letters"),&err);
	small->uc_stem_width = GetReal8(ew,CID_UC_Stem_Width,_("Primary stem width for upper case letters"),&err);
	if ( err )
return( true );
	small->dosymbols = GGadgetIsChecked(GWidgetGetControl(ew,CID_Symbols_Too));
	small->extension_for_letters = GGadgetGetTitle8(GWidgetGetControl(ew,CID_Letter_Ext));
	small->extension_for_symbols = GGadgetGetTitle8(GWidgetGetControl(ew,CID_Symbol_Ext));
	if ( *small->extension_for_letters=='\0' || (*small->extension_for_symbols=='\0' && small->dosymbols )) {
	    free( small->extension_for_letters );
	    free( small->extension_for_symbols );
	    ff_post_error(_("Missing extension"),_("You must provide a glyph extension"));
return( true );
	}

	FVAddSmallCaps( (FontViewBase *) ed->fv, small );
	free( small->extension_for_letters );
	free( small->extension_for_symbols );
	ed->done = true;
    }
return( true );
}

static int SC_Def_Changed(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow ew = GGadgetGetWindow(g);
	int on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_SCH), !on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_SCH_Lab), !on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_Cap), !on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_Cap_Lab), !on);
    }
return( true );
}

void AddSmallCapsDlg(FontView *fv) {
    StyleDlg ed;
    SplineFont *sf = fv->b.sf;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[23], boxes[6], *barray[8], *hvarray[37], *swarray[6],
	    *exarray[6];
    GTextInfo label[23];
    int k;
    char sch[40], caph[40], lcsw[40], ucsw[40];
    struct smallcaps small;

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.sf = sf;
    ed.small = &small;

    SmallCapsFindConstants(&small,fv->b.sf,fv->b.active_layer);    

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Add Small Caps");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    label[k].text = (unichar_t *) _(
	"Unlike most commands this one does not work directly on the\n"
	"selected glyphs. Instead, if you select an \"A\" (or an \"a\")\n"
	"FontForge will create (or reuse) a glyph named \"a.sc\", and\n"
	"it will copied a modified version of the \"A\" glyph into this one.");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0] = &gcd[k-1]; hvarray[1] = GCD_ColSpan; hvarray[2] = NULL;

    gcd[k].gd.pos.width = 10; gcd[k].gd.pos.height = 10;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GSpacerCreate;
    hvarray[3] = &gcd[k-1]; hvarray[4] = GCD_ColSpan; hvarray[5] = NULL;

    label[k].text = (unichar_t *) _("Small Caps letters should be as tall as the x-height");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_cb_on;
    gcd[k].gd.cid = CID_SCH_Is_XH;
    gcd[k].gd.handle_controlevent = SC_Def_Changed;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[6] = &gcd[k-1]; hvarray[7] = GCD_ColSpan; hvarray[8] = NULL;

    label[k].text = (unichar_t *) _("Small Cap Height:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible ;
    gcd[k].gd.cid = CID_SCH_Lab;
    gcd[k++].creator = GLabelCreate;

    sprintf( sch, "%g", rint( small.scheight ));
    label[k].text = (unichar_t *) sch;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_SCH;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[9] = &gcd[k-2]; hvarray[10] = &gcd[k-1]; hvarray[11] = NULL;

    label[k].text = (unichar_t *) _("Capital Height:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible ;
    gcd[k].gd.cid = CID_Cap_Lab;
    gcd[k++].creator = GLabelCreate;

    sprintf( caph, "%g", rint( small.capheight ));
    label[k].text = (unichar_t *) caph;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_Cap;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[12] = &gcd[k-2]; hvarray[13] = &gcd[k-1]; hvarray[14] = NULL;

    label[k].text = (unichar_t *) _("Primary Stem Widths");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    hvarray[15] = &gcd[k-1]; hvarray[16] = GCD_ColSpan; hvarray[17] = NULL;

    label[k].text = (unichar_t *) _("Lower Case:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( lcsw, "%g", rint( small.lc_stem_width ));
    label[k].text = (unichar_t *) lcsw;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 50;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_LC_Stem_Width;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Upper Case:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( ucsw, "%g", rint( small.uc_stem_width ));
    label[k].text = (unichar_t *) ucsw;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 50;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_UC_Stem_Width;
    gcd[k++].creator = GTextFieldCreate;
    swarray[0] = &gcd[k-4]; swarray[1] = &gcd[k-3]; swarray[2] = &gcd[k-2]; swarray[3] = &gcd[k-1];
    swarray[4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = swarray;
    boxes[3].creator = GHBoxCreate;
    hvarray[18] = &boxes[3]; hvarray[19] = GCD_ColSpan; hvarray[20] = NULL;

    label[k].text = (unichar_t *) _("Create small caps variants for symbols as well as letters");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Symbols_Too;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[21] = &gcd[k-1]; hvarray[22] = GCD_ColSpan; hvarray[23] = NULL;

    gcd[k].gd.pos.width = 10; gcd[k].gd.pos.height = 10;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GSpacerCreate;
    hvarray[24] = &gcd[k-1]; hvarray[25] = GCD_ColSpan; hvarray[26] = NULL;

    label[k].text = (unichar_t *) _("Glyph Extensions");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    hvarray[27] = &gcd[k-1]; hvarray[28] = GCD_ColSpan; hvarray[29] = NULL;

    label[k].text = (unichar_t *) _("Letters:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "sc";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Letter_Ext;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Symbols:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "taboldstyle";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Symbol_Ext;
    gcd[k++].creator = GTextFieldCreate;
    exarray[0] = &gcd[k-4]; exarray[1] = &gcd[k-3]; exarray[2] = &gcd[k-2]; exarray[3] = &gcd[k-1];
    exarray[4] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = exarray;
    boxes[4].creator = GHBoxCreate;
    hvarray[30] = &boxes[4]; hvarray[31] = GCD_ColSpan; hvarray[32] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = SmallCaps_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = CondenseExtend_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = barray;
    boxes[5].creator = GHBoxCreate;
    hvarray[33] = &boxes[5]; hvarray[34] = GCD_ColSpan; hvarray[35] = NULL;
    hvarray[36] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

/* ************************************************************************** */
/* ***************************** Embolden Dialog **************************** */
/* ************************************************************************** */

#define CID_EmBdWidth	1001
#define CID_LCG		1002
#define CID_CJK		1003
#define CID_Auto	1004
#define CID_Custom	1005
#define CID_TopZone	1006
#define CID_BottomZone	1007
#define CID_CleanupSelfIntersect	1008
#define CID_TopHint	1009
#define CID_BottomHint	1010
#define CID_Squish	1011
#define CID_Retain	1012
#define CID_CounterAuto	1013
#define CID_SerifHeight	1014
#define CID_SerifHFuzz	1015

static SplineFont *lastsf = NULL;
static enum embolden_type last_type = embolden_auto;
static struct lcg_zones last_zones;
static int last_width;
static int last_overlap = true;

static int Embolden_OK(GGadget *g, GEvent *e) {
    enum embolden_type type;
    struct lcg_zones zones;
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	memset(&zones,0,sizeof(zones));
	err = false;
	zones.stroke_width = GetReal8(ew,CID_EmBdWidth,_("Embolden by"),&err);
	type = GGadgetIsChecked( GWidgetGetControl(ew,CID_LCG)) ? embolden_lcg :
		GGadgetIsChecked( GWidgetGetControl(ew,CID_CJK)) ? embolden_cjk :
		GGadgetIsChecked( GWidgetGetControl(ew,CID_Auto)) ? embolden_auto :
			embolden_custom;
	zones.serif_height = GetReal8(ew,CID_SerifHeight,_("Serif Height"),&err);
	zones.serif_fuzz = GetReal8(ew,CID_SerifHFuzz,_("Serif Height Fuzz"),&err);
	if ( type == embolden_custom ) {
	    zones.top_zone = GetReal8(ew,CID_TopZone,_("Top Zone"),&err);
	    zones.bottom_zone = GetReal8(ew,CID_BottomZone,_("Bottom Zone"),&err);
	    zones.top_bound = GetReal8(ew,CID_TopHint,_("Top Hint"),&err);
	    zones.bottom_bound = GetReal8(ew,CID_BottomHint,_("Bottom Hint"),&err);
	}
	if ( err )
return( true );
	zones.counter_type = GGadgetIsChecked( GWidgetGetControl(ew,CID_Squish)) ? ct_squish :
		GGadgetIsChecked( GWidgetGetControl(ew,CID_Retain)) ? ct_retain :
			ct_auto;

	lastsf = ed->sf;
	last_type = type;
	last_width = zones.stroke_width;
	last_overlap = zones.removeoverlap = GGadgetIsChecked( GWidgetGetControl(ew,CID_CleanupSelfIntersect));
	if ( type == embolden_custom )
	    last_zones = zones;

	if ( ed->fv!=NULL )
	    FVEmbolden((FontViewBase *) ed->fv, type, &zones);
	else
	    CVEmbolden((CharViewBase *) ed->cv, type, &zones);
	ed->done = true;
    }
return( true );
}

static int Embolden_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	ed->done = true;
    }
return( true );
}

static int Embolden_Radio(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	int en;
	en = GGadgetIsChecked( GWidgetGetControl(ed->gw,CID_Custom));
	GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_TopZone),en);
	GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_BottomZone),en);
	GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_TopHint),en);
	GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_BottomHint),en);
    }
return( true );
}

void EmboldenDlg(FontView *fv, CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->b.sf : cv->b.sc->parent;
    BlueData bd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[27], boxes[6], *barray[8], *rarray[6], *carray[6], *hvarray[10][5];
    GTextInfo label[27];
    int k;
    char topzone[40], botzone[40], emb_width[40], tophint[40], bothint[40], serifh[40];

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;
    ed.layer = cv==NULL ? fv->b.active_layer : CVLayer((CharViewBase *) cv);

    QuickBlues(sf, ed.layer, &bd);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Change Weight");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));
    label[k].text = (unichar_t *) _("Embolden by:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0][0] = &gcd[k-1];

    sprintf( emb_width, "%d", sf==lastsf ? last_width : (sf->ascent+sf->descent)/20 );
    label[k].text = (unichar_t *) emb_width;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_EmBdWidth;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[0][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("em units");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0][2] = &gcd[k-1];
    hvarray[0][3] = GCD_Glue; hvarray[0][4] = NULL;

    label[k].text = (unichar_t *) _("_LCG");
    gcd[k].gd.popup_msg = (unichar_t *) _("Embolden as appropriate for Latin, Cyrillic and Greek scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_LCG;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_CJK");
    gcd[k].gd.popup_msg = (unichar_t *) _("Embolden as appropriate for Chinese, Japanese, Korean scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_CJK;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_Auto");
    gcd[k].gd.popup_msg = (unichar_t *) _("Choose the appropriate method depending on the glyph's script");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Auto;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("C_ustom");
    gcd[k].gd.popup_msg = (unichar_t *) _("User controls the emboldening with the next two fields");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Custom;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[3] = &gcd[k-1]; rarray[4] = GCD_Glue; rarray[5] = NULL;

    if ( lastsf!=sf )
	gcd[k-4 + embolden_auto].gd.flags |= gg_cb_on;
    else
	gcd[k-4 + last_type].gd.flags |= gg_cb_on;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = rarray;
    boxes[3].creator = GHBoxCreate;
    hvarray[1][0] = &boxes[3]; hvarray[1][1] = hvarray[1][2] = hvarray[1][3] = GCD_ColSpan; hvarray[1][4] = NULL;

    label[k].text = (unichar_t *) _("_Top hint:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[2][0] = &gcd[k-1];

    sprintf( tophint, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.top_bound :
			(int) rint(bd.xheight>0 ? bd.xheight : bd.caph>0 ? 2*bd.caph/3 :
			    sf->ascent/2 ));
    label[k].text = (unichar_t *) tophint;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_TopHint;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[2][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_Zone:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[2][2] = &gcd[k-1];

    sprintf( topzone, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.top_zone :
			(int) rint(bd.xheight>0 ? 2*bd.xheight/3 :
			bd.caph>0 ? 2*bd.caph/3 :
			(sf->ascent/3)) );
    label[k].text = (unichar_t *) topzone;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_TopZone;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[2][3] = &gcd[k-1]; hvarray[2][4] = NULL;

    label[k].text = (unichar_t *) _("_Bottom hint:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[3][0] = &gcd[k-1];

    sprintf( bothint, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.bottom_bound :
			0 );
    label[k].text = (unichar_t *) bothint;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_BottomHint;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[3][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Zone:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[3][2] = &gcd[k-1];

    sprintf( botzone, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.bottom_zone :
			(int) rint(bd.xheight>0 ? bd.xheight/3 :
			bd.caph>0 ? bd.caph/3 :
			(sf->ascent/4)) );
    label[k].text = (unichar_t *) botzone;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_BottomZone;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[3][3] = &gcd[k-1]; hvarray[3][4] = NULL;

    label[k].text = (unichar_t *) _("Serif Height");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Any points this high will be assumed to be on serifs,\nand will remain at that height after processing.\n(So serifs should remain the same size).\n(If you do wish the serifs to grow, set this to 0)");
    gcd[k++].creator = GLabelCreate;
    hvarray[4][0] = &gcd[k-1];

    sprintf( serifh, "%g", SFSerifHeight(sf));
    label[k].text = (unichar_t *) serifh;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_SerifHeight;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[4][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Fuzz");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Allow the height match to differ by this much");
    gcd[k++].creator = GLabelCreate;
    hvarray[4][2] = &gcd[k-1];

    label[k].text = (unichar_t *) ".9";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_SerifHFuzz;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[4][3] = &gcd[k-1];
    hvarray[4][4] = NULL;

    label[k].text = (unichar_t *) _("Counters:");
    gcd[k].gd.popup_msg = (unichar_t *) _("The simple application of this algorithm will squeeze counters\nThat is not normally seen in bold latin fonts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    carray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Squish");
    gcd[k].gd.popup_msg = (unichar_t *) _("Make the counters narrower");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Squish;
    /*gcd[k].gd.handle_controlevent = Embolden_Counter;*/
    gcd[k++].creator = GRadioCreate;
    carray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Retain");
    gcd[k].gd.popup_msg = (unichar_t *) _("Try to insure that the counters are as wide\nafterward as they were before");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Retain;
    /*gcd[k].gd.handle_controlevent = Embolden_Counter;*/
    gcd[k++].creator = GRadioCreate;
    carray[2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Auto");
    gcd[k].gd.popup_msg = (unichar_t *) _("Retain counter size for glyphs using latin algorithm\nSquish them for those using CJK." );
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | gg_cb_on;
    gcd[k].gd.cid = CID_CounterAuto;
    /* gcd[k].gd.handle_controlevent = Embolden_Counter;*/
    gcd[k++].creator = GRadioCreate;
    carray[3] = &gcd[k-1];
    carray[4] = GCD_Glue; carray[5] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = carray;
    boxes[5].creator = GHBoxCreate;
    hvarray[5][0] = &boxes[5]; hvarray[5][1] = hvarray[5][2] = hvarray[5][3] = GCD_ColSpan; hvarray[5][4] = NULL;

    label[k].text = (unichar_t *) _("Cleanup Self Intersect");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | (last_overlap?gg_cb_on:0);
    gcd[k].gd.cid = CID_CleanupSelfIntersect;
    gcd[k].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[6][0] = &gcd[k-1]; hvarray[6][1] = hvarray[6][2] = hvarray[6][3] = GCD_ColSpan; hvarray[6][4] = NULL;

    hvarray[7][0] = hvarray[7][1] = hvarray[7][2] = hvarray[7][3] = GCD_Glue; hvarray[7][4] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Embolden_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Embolden_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;
    hvarray[8][0] = &boxes[4]; hvarray[8][1] = hvarray[8][2] = hvarray[8][3] = GCD_ColSpan; hvarray[8][4] = NULL;
    hvarray[9][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

/* ************************************************************************** */
/* ***************************** Oblique Dialog ***************************** */
/* ************************************************************************** */

static ItalicInfo last_ii = {
    -13,		/* Italic angle (in degrees) */
    /* horizontal squash, lsb, stemsize, countersize, rsb */
    .91, .89, .90, .91,		/* For lower case */
    .91, .93, .93, .91,		/* For upper case */
    .91, .93, .93, .91,		/* For things which are neither upper nor lower case */
    srf_simpleslant,	/* Secondary serifs (initial, medial on "m", descender on "p", "q" */
    true,		/* Transform bottom serifs */
    true,		/* Transform serifs at x-height */
    false,		/* Transform serifs on ascenders */

    true,		/* Change the shape of an "a" to look like a "d" without ascender */
    false,		/* Change the shape of "f" so it descends below baseline (straight down no flag at end) */
    true,		/* Change the shape of "f" so the bottom looks like the top */
    true,		/* Remove serifs from the bottom of descenders */

    true,		/* Make the cyrillic "phi" glyph have a top like an "f" */
    true,		/* Make the cyrillic "i" glyph look like a latin "u" */
    true,		/* Make the cyrillic "pi" glyph look like a latin "n" */
    true,		/* Make the cyrillic "te" glyph look like a latin "m" */
    true,		/* Make the cyrillic "sha" glyph look like a latin "m" rotated 180 */
    true,		/* Make the cyrillic "dje" glyph look like a latin smallcaps T (not implemented) */
    true		/* Make the cyrillic "dzhe" glyph look like a latin "u" (same glyph used for cyrillic "i") */
};

void ObliqueDlg(FontView *fv, CharView *cv) {
    double temp;
    char def[40], *ret, *end;
    real transform[6];

    sprintf( def, "%g", last_ii.italic_angle );
    ret = gwwv_ask_string(_("Oblique Slant..."),def,_("By what angle (in degrees) do you want to slant the font?"));
    if ( ret==NULL )
return;
    temp = strtod(ret,&end);
    if ( *end || temp>90 || temp<-90 ) {
	free(ret);
	ff_post_error( _("Bad Number"),_("Bad Number") );
return;
    }

    last_ii.italic_angle = temp;
    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1;
    transform[2] = -tan( last_ii.italic_angle * 3.1415926535897932/180.0 );
    if ( cv!=NULL ) {
	CVPreserveState((CharViewBase *) cv);
	CVTransFunc(cv,transform,fvt_dontmovewidth);
	CVCharChangedUpdate(&cv->b);
    } else {
	int i, gid;
	SplineChar *sc;

	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] &&
		(gid = fv->b.map->map[i])!=-1 && (sc=fv->b.sf->glyphs[gid])!=NULL ) {
	    FVTrans((FontViewBase *) fv,sc,transform,NULL,fvt_dontmovewidth);
	}
    }
}


/* ************************************************************************** */
/* ********************************* Italic ********************************* */
/* ************************************************************************** */

#define CID_A		1001
#define CID_F		1002
#define CID_F2		1003
#define CID_P		1004
#define CID_Cyrl_I	1011
#define CID_Cyrl_Pi	1012
#define CID_Cyrl_Te	1013
#define CID_Cyrl_Phi	1014
#define CID_Cyrl_Sha	1015
#define CID_Cyrl_Dzhe	1016
#define CID_BottomSerifs	2001
#define CID_XHeightSerifs	2002
#define CID_AscenderSerifs	2003
#define CID_Flat		2004
#define CID_Slanted		2005
#define CID_PenSlant		2006
#define CID_CompressLSB		3001		/* for lc, 3011 for uc, 3021 for others */
#define CID_CompressStem	3002		/* for lc, 3012 for uc, 3022 for others */
#define CID_CompressCounter	3003		/* for lc, 3013 for uc, 3023 for others */
#define CID_CompressRSB		3004		/* for lc, 3014 for uc, 3024 for others */
#define CID_ItalicAngle		4001

static int Ital_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	ItalicInfo ii;
	int err = false, i;

	memset(&ii,0,sizeof(ii));
	for ( i=0; i<3; ++i ) {
	    struct hsquash *hs = &(&ii.lc)[i];
	    hs->lsb_percent = GetReal8(ew,CID_CompressLSB+i*10,_("LSB Compression Percent"),&err)/100.0;
	    hs->stem_percent = GetReal8(ew,CID_CompressStem+i*10,_("Stem Compression Percent"),&err)/100.0;
	    hs->counter_percent = GetReal8(ew,CID_CompressCounter+i*10,_("Counter Compression Percent"),&err)/100.0;
	    hs->rsb_percent = GetReal8(ew,CID_CompressRSB+i*10,_("RSB Compression Percent"),&err)/100.0;
	    if ( err )
return( true );
	}
	ii.italic_angle = GetReal8(ew,CID_ItalicAngle,_("Italic Angle"),&err);
	if ( err )
return( true );

	ii.secondary_serif = GGadgetIsChecked(GWidgetGetControl(ew,CID_Flat)) ? srf_flat :
		GGadgetIsChecked(GWidgetGetControl(ew,CID_Slanted)) ? srf_simpleslant :
			 srf_complexslant;

	ii.transform_bottom_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_BottomSerifs));
	ii.transform_top_xh_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_XHeightSerifs));
	ii.transform_top_as_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_AscenderSerifs));

	ii.a_from_d = GGadgetIsChecked(GWidgetGetControl(ew,CID_A));
	ii.f_rotate_top = GGadgetIsChecked(GWidgetGetControl(ew,CID_F));
	ii.f_long_tail = GGadgetIsChecked(GWidgetGetControl(ew,CID_F2));
	ii.pq_deserif = GGadgetIsChecked(GWidgetGetControl(ew,CID_P));
	if ( ii.f_rotate_top && ii.f_long_tail ) {
	    ff_post_error(_("Bad setting"),_("You may not select both variants of 'f'"));
return( true );
	}

	ii.cyrl_i = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_I));
	ii.cyrl_pi = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Pi));
	ii.cyrl_te = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Te));
	ii.cyrl_phi = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Phi));
	ii.cyrl_sha = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Sha));
	ii.cyrl_dzhe = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Dzhe));
	
	last_ii = ii;
	MakeItalic((FontViewBase *) ed->fv,(CharViewBase *) ed->cv,&ii);
	ed->done = true;
    }
return( true );
}

void ItalicDlg(FontView *fv, CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->b.sf : cv->b.sc->parent;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[50], boxes[7], *forms[25], *compress[5][6], *sarray[5],
	    *iaarray[4], *barray[10], *varray[35];
    GTextInfo label[50];
    int k,f,r,i;
    char lsb[3][40], stems[3][40], counters[3][40], rsb[3][40], ia[40];

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Italic");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);

    k=f=r=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    label[k].image = &GIcon_aItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.a_from_d ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_A;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_fItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.f_rotate_top ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_F;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_f2Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.f_long_tail ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_F2;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_Glue; forms[f++] = NULL;

    label[k].image = &GIcon_pItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.pq_deserif ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_P;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_ColSpan; forms[f++] = GCD_ColSpan;
    forms[f++] = GCD_Glue; forms[f++] = NULL;

    label[k].image = &GIcon_u438Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_i ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_I;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_u43fItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_pi ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Pi;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_u442Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_te ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Te;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_Glue; forms[f++] = NULL;

    label[k].image = &GIcon_u444Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_phi ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Phi;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_u448Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_sha ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Sha;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];

    label[k].image = &GIcon_u45fItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_dzhe ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Dzhe;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_Glue; forms[f++] = NULL;
    forms[f++] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = forms;
    boxes[2].creator = GHVBoxCreate;
    varray[r++] = &boxes[2]; varray[r++] = NULL;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.pos.height = 5; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Transform baseline serifs");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_BottomSerifs;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.transform_bottom_serifs ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_BottomSerifs;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Transform x-height serifs");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_TopSerifs;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.transform_top_xh_serifs ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_XHeightSerifs;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Transform ascender serifs");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_TopSerifs;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.transform_top_as_serifs ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_AscenderSerifs;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("When serifs are removed (as first two in \"m\"), replace with:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Flat");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.secondary_serif==srf_flat ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Flat;
    gcd[k++].creator = GRadioCreate;
    sarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Slanted");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.secondary_serif==srf_simpleslant ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Slanted;
    gcd[k++].creator = GRadioCreate;
    sarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Pen Slanted");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.secondary_serif==srf_complexslant ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_PenSlant;
    gcd[k++].creator = GRadioCreate;
    sarray[2] = &gcd[k-1]; sarray[3] = GCD_Glue; sarray[4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = sarray;
    boxes[3].creator = GHBoxCreate;
    varray[r++] = &boxes[3]; varray[r++] = NULL;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.pos.height = 5; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Compress (as a percentage)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    compress[0][0] = GCD_Glue;

    label[k].text = (unichar_t *) _("LSB");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Left Side Bearing");
    gcd[k++].creator = GLabelCreate;
    compress[0][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Stems");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    compress[0][2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Counters");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    compress[0][3] = &gcd[k-1];

    label[k].text = (unichar_t *) _("RSB");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Right Side Bearing");
    gcd[k++].creator = GLabelCreate;
    compress[0][4] = &gcd[k-1]; compress[0][5] = NULL;

    for ( i=0; i<3; ++i ) {
	struct hsquash *hs = &(&last_ii.lc)[i];

	label[k].text = (unichar_t *) (i==0 ? _("Lower Case") : i==1 ? _("Upper Case") : _("Others"));
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
	gcd[k++].creator = GLabelCreate;
	compress[i+1][0] = &gcd[k-1];

	sprintf( lsb[i], "%g", 100.0* hs->lsb_percent );
	label[k].text = (unichar_t *) lsb[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[k].gd.cid = CID_CompressLSB+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][1] = &gcd[k-1];

	sprintf( stems[i], "%g", 100.0* hs->stem_percent );
	label[k].text = (unichar_t *) stems[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[k].gd.cid = CID_CompressStem+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][2] = &gcd[k-1];

	sprintf( counters[i], "%g", 100.0* hs->counter_percent );
	label[k].text = (unichar_t *) counters[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[k].gd.cid = CID_CompressCounter+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][3] = &gcd[k-1];

	sprintf( rsb[i], "%g", 100.0* hs->rsb_percent );
	label[k].text = (unichar_t *) rsb[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[k].gd.cid = CID_CompressRSB+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][4] = &gcd[k-1]; compress[i+1][5] = NULL;
    }
    compress[i+1][0] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = compress[0];
    boxes[4].creator = GHVBoxCreate;
    varray[r++] = &boxes[4]; varray[r++] = NULL;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.pos.height = 5; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Italic Angle:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    iaarray[0] = &gcd[k-1];

    sprintf( ia, "%g", last_ii.italic_angle );
    label[k].text = (unichar_t *) ia;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 50;
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[k].gd.cid = CID_ItalicAngle;
    gcd[k++].creator = GTextFieldCreate;
    iaarray[1] = &gcd[k-1]; iaarray[2] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = iaarray;
    boxes[5].creator = GHBoxCreate;
    varray[r++] = &boxes[5]; varray[r++] = NULL;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 17+31+16;
    gcd[k].gd.pos.width = 190-10;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLineCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) U_(
	"This italic conversion will be incomplete!\n"
	"You will probably want to do manual fixups on e, g, k, and v-z\n"
	"And on в, г, д, е, ж, л, м, ц, щ, ъ, ђ\n"
	"And on all Greek lower case letters");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k++].creator = GLabelCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 17+31+16;
    gcd[k].gd.pos.width = 190-10;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLineCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = Ital_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = CondenseExtend_Cancel;
    gcd[k++].creator = GButtonCreate;

    if ( k>sizeof(gcd)/sizeof(gcd[0]) )
	IError( "Too many options in Italic Dlg");

    barray[0] = GCD_Glue; barray[1] = &gcd[k-2]; barray[2] = GCD_Glue;
    barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = barray;
    boxes[6].creator = GHBoxCreate;
    varray[r++] = &boxes[6]; varray[r++] = NULL;
    varray[r++] = NULL;
    if ( r>sizeof(varray)/sizeof(varray[0]) )
	IError( "Too many rows in Italic Dlg");

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
}
