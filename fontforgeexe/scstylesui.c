/* -*- coding: utf-8 -*- */
/* Copyright (C) 2007-2012 by George Williams */
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

#include "autohint.h"
#include "cvundoes.h"
#include "dumppfa.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "scstyles.h"
#include "ustring.h"
#include "utype.h"

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
    enum glyphchange_type gc;
    bigreal scale;
} StyleDlg;

#define CID_C_Factor	1001
#define CID_C_Add	1002
#define CID_SB_Factor	1003
#define CID_SB_Add	1004
#define CID_CorrectItalic	1005

static struct counterinfo last_ci = {
    90, 0, 90, 0, true,
    /* initializers below are dummy... will be set later */
    BLUEDATA_EMPTY, 0, NULL, 0, DBOUNDS_EMPTY, 0.0, 0.0, 0.0, 0,
    { 0, 0 }, /* cnts */
    { 0, 0 }, /* maxes */
    { NULL, NULL } /* zones */
};

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
	    help("ui/dialogs/Styles.html", NULL);
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
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[1] = &gcd[k-1];
    hvarray[2] = GCD_Glue;

    label[k].text = (unichar_t *) _("Add");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[3] = &gcd[k-1];
    hvarray[4] = NULL;

    label[k].text = (unichar_t *) _("Counters:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[5] = &gcd[k-1];

    sprintf( c_factor, "%g", last_ci.c_factor );
    label[k].text = (unichar_t *) c_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_C_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[6] = &gcd[k-1];

    label[k].text = (unichar_t *) " % + ";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[7] = &gcd[k-1];

    sprintf( c_add, "%g", last_ci.c_add );
    label[k].text = (unichar_t *) c_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
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
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[10] = &gcd[k-1];

    sprintf( sb_factor, "%g", last_ci.sb_factor );
    label[k].text = (unichar_t *) sb_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SB_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[11] = &gcd[k-1];

    label[k].text = (unichar_t *) " % + ";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[12] = &gcd[k-1];

    sprintf( sb_add, "%g", last_ci.sb_add );
    label[k].text = (unichar_t *) sb_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
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
    gcd[k].gd.flags = gg_enabled | gg_visible| (last_ci.correct_italic?gg_cb_on:0);
    gcd[k].gd.cid = CID_CorrectItalic;
    gcd[k].gd.popup_msg = _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
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
/* ************************* Generic Change Dialog ************************** */
/* ************************************************************************** */

#undef CID_Letter_Ext
#undef CID_Symbol_Ext
#undef CID_Symbols_Too

#undef CID_HScale
#undef CID_VScale

#define CID_Feature		1001
#define CID_Extension		1002
#define CID_StemsUniform	1003
#define CID_Stems_H_V		1004
#define CID_Stems_by_Width	1005
#define CID_StemThreshold	1006
#define CID_StemHeight		1007
#define CID_StemHeightLabel	1008
#define CID_StemWidth		1009
#define CID_StemWidthLabel	1010
#define CID_StemHeightAdd	1011
#define CID_StemWidthAdd	1012
#define CID_DStemOn		1013

#define CID_Counter_SameAdvance	1020
#define CID_Counter_PropAdvance	1021
#define CID_Counter_is_SideB	1022
#define CID_Counter_isnt_SideB	1023
#define CID_CounterPercent	1024
#define CID_CounterAdd		1025
#define CID_LSBPercent		1026
#define CID_LSBAdd		1027
#define CID_RSBPercent		1028
#define CID_RSBAdd		1029

#define CID_UseVerticalCounters	1040
#define CID_VCounterPercent	1041
#define CID_VCounterAdd		1042
#define CID_UseVerticalMappings	1043
#define CID_VerticalScale	1044
#define CID_VMappings		1045

#define CID_VerticalOff		1060

#define CID_Letter_Ext		1081
#define CID_Symbol_Ext		1082
#define CID_Symbols_Too		1083
#define CID_SmallCaps		1084
#define CID_PetiteCaps		1085

#define CID_TabSet		1100

static GTextInfo ss_features[] = {
    { (unichar_t *) N_("Superscript"), NULL, 0, 0, (void *) CHR('s','u','p','s'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Scientific Inferiors"), NULL, 0, 0, (void *) CHR('s','i','n','f'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Subscript"), NULL, 0, 0, (void *) CHR('s','u','b','s'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Denominators"), NULL, 0, 0, (void *) CHR('d','n','o','m'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Numerators"), NULL, 0, 0, (void *) CHR('n','u','m','r'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* Not translated */
static char *ss_extensions[] = {
    "superior",
    "inferior",
    "subscript",
    "denominator",
    "numerator",
    NULL
};
static int ss_percent_xh_up[] = {
    90,
    -100,
    -70,
    -50,
    50
};

static double GuessStemThreshold(SplineFont *sf) {
    double stdvw = 0, stdhw = 0, avg;
    char *ret;

    if ( sf->private_dict!=NULL ) {
	if ((ret=PSDictHasEntry(sf->private_dict,"StdVW"))!=NULL ) {
	    if ( ret[0] == '[' ) ret++;
	    stdvw = strtod(ret,NULL);
	}
	if ((ret=PSDictHasEntry(sf->private_dict,"StdHW"))!=NULL ) {
	    if ( ret[0] == '[' ) ret++;
	    stdhw = strtod(ret,NULL);
	}
    }
    avg = (stdvw + stdhw)/2;
    if ( avg<=0 )
	avg = (sf->ascent+sf->descent)/25;
return( avg );
}

static int SS_Feature_Changed(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged &&
	    e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	int index = e->u.control.u.tf_changed.from_pulldown;
	uint32_t tag = (intptr_t) ss_features[index].userdata;
	char tagbuf[5], offset[40];

	tagbuf[0] = tag>>24; tagbuf[1] = tag>>16; tagbuf[2] = tag>>8; tagbuf[3] = tag; tagbuf[4] = 0;
	GGadgetSetTitle8(g,tagbuf);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_Extension), ss_extensions[index]);

	sprintf( offset, "%g", rint( ed->small->xheight*ss_percent_xh_up[index]/100.0 ));
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_VerticalOff), offset);
    }
return( true );
}

static int GlyphChange_OK(GGadget *g, GEvent *e) {
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	struct genericchange genchange;
	int stem_xy_same = GGadgetIsChecked(GWidgetGetControl(ew,CID_StemsUniform));
	int stem_bywidth = GGadgetIsChecked(GWidgetGetControl(ew,CID_Stems_by_Width));
	enum glyphchange_type gc = ed->gc;

	memset(&genchange,0,sizeof(genchange));
	genchange.gc = gc;
	genchange.small = ed->small;
	genchange.stem_height_scale = GetReal8(ew,CID_StemHeight,_("Horizontal Stem Height Scale"),&err)/100.;
	genchange.stem_height_add   = GetReal8(ew,CID_StemHeightAdd,_("Horizontal Stem Height Add"),&err);
	genchange.stem_threshold = stem_bywidth ? GetReal8(ew,CID_StemThreshold,_("Threshold between Thin and Thick Stems"),&err) : 0;
	if ( stem_xy_same ) {
	    genchange.stem_width_scale = genchange.stem_height_scale;
	    genchange.stem_width_add   = genchange.stem_height_add;
	} else {
	    genchange.stem_width_scale = GetReal8(ew,CID_StemWidth,_("Vertical Stem Width Scale"),&err)/100.;
	    genchange.stem_width_add   = stem_bywidth ? genchange.stem_height_add : GetReal8(ew,CID_StemWidthAdd,_("Vertical Stem Width Add"),&err);
	}
	genchange.dstem_control        = GGadgetIsChecked(GWidgetGetControl(ew,CID_DStemOn));
	if ( err )
return( true );
	if (stem_bywidth && genchange.stem_threshold <= 0) 
	    ff_post_error(_("Unlikely stem threshold"), _("Stem threshold should be positive"));
	if ( genchange.stem_width_scale<.03 || genchange.stem_width_scale>10 ||
		genchange.stem_height_scale<.03 || genchange.stem_height_scale>10 ) {
	    ff_post_error(_("Unlikely scale factor"), _("Scale factors must be between 3 and 1000 percent"));
return( true );
	}
	if ( genchange.stem_height_add!=genchange.stem_width_add ) {
	    if (( genchange.stem_height_add==0 && genchange.stem_width_add!=0 ) ||
		    ( genchange.stem_height_add!=0 && genchange.stem_width_add==0 )) {
		ff_post_error(_("Bad stem add"), _("The horizontal and vertical stem add amounts must either both be zero, or neither may be 0"));
return( true );
	    }
	    /* if width_add has a different sign than height_add that's also */
	    /*  a problem, but this test will catch that too */
	    if (( genchange.stem_height_add/genchange.stem_width_add>4 ) ||
		    ( genchange.stem_height_add/genchange.stem_width_add<.25 )) {
		ff_post_error(_("Bad stem add"), _("The horizontal and vertical stem add amounts may not differ by more than a factor of 4"));
return( true );
	    }
	}
	if ( gc==gc_subsuper ) {
	    const unichar_t *tag_str = _GGadgetGetTitle(GWidgetGetControl(ew,CID_Feature));
	    char tag[4];

	    memset(tag,' ',sizeof(tag));
	    if ( *tag_str=='\0' )
		genchange.feature_tag = 0;		/* Perfectly valid to have no tag */
	    else {
		tag[0] = *tag_str;
		if ( tag_str[1]!='\0' ) {
		    tag[1] = tag_str[1];
		    if ( tag_str[2]!='\0' ) {
			tag[2] = tag_str[2];
			if ( tag_str[3]!='\0' ) {
			    tag[3] = tag_str[3];
			    if ( tag_str[4]!='\0' ) {
				ff_post_error(_("Bad tag"), _("Feature tags are limited to 4 letters"));
return( true );
			    }
			}
		    }
		}
		genchange.feature_tag = (tag[0]<<24) | (tag[1]<<16) | (tag[2]<<8) | tag[3];
	    }
	    genchange.glyph_extension = GGadgetGetTitle8(GWidgetGetControl(ew,CID_Extension));
	    if ( *genchange.glyph_extension=='\0' ) {
		ff_post_error(_("Missing glyph extension"),_("You must specify a glyph extension"));
		free(genchange.glyph_extension);
return( true );
	    }
	    genchange.vertical_offset = GetReal8(ew,CID_VerticalOff,_("Vertical Offset"),&err);
	    if ( err )
return( true );
	} else if ( gc==gc_smallcaps ) {
	    genchange.do_smallcap_symbols = GGadgetIsChecked(GWidgetGetControl(ew,CID_Symbols_Too));
	    genchange.petite              = GGadgetIsChecked(GWidgetGetControl(ew,CID_PetiteCaps));
	    genchange.extension_for_letters = GGadgetGetTitle8(GWidgetGetControl(ew,CID_Letter_Ext));
	    genchange.extension_for_symbols = GGadgetGetTitle8(GWidgetGetControl(ew,CID_Symbol_Ext));
	    if ( *genchange.extension_for_letters=='\0' || (*genchange.extension_for_symbols=='\0' && genchange.do_smallcap_symbols )) {
		free( genchange.extension_for_letters );
		free( genchange.extension_for_symbols );
		ff_post_error(_("Missing extension"),_("You must provide a glyph extension"));
return( true );
	    }
	}

	if (GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_SameAdvance)))
	    genchange.center_in_hor_advance = 1;
	else if (GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_PropAdvance)))
	    genchange.center_in_hor_advance = 2;
	else
	    genchange.center_in_hor_advance = 0;
	genchange.hcounter_scale = GetReal8(ew,CID_CounterPercent,_("Horizontal Counter Scale"),&err)/100.;
	genchange.hcounter_add   = GetReal8(ew,CID_CounterAdd,_("Horizontal Counter Add"),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_is_SideB)) ) {
	    genchange.lsb_scale = genchange.hcounter_scale;
	    genchange.lsb_add   = genchange.hcounter_add;
	    genchange.rsb_scale = genchange.hcounter_scale;
	    genchange.rsb_add   = genchange.hcounter_add;
	} else {
	    genchange.lsb_scale = GetReal8(ew,CID_LSBPercent,_("Left Side Bearing Scale"),&err)/100.;
	    genchange.lsb_add   = GetReal8(ew,CID_LSBAdd,_("Left Side Bearing Add"),&err);
	    genchange.rsb_scale = GetReal8(ew,CID_RSBPercent,_("Right Side Bearing Scale"),&err)/100.;
	    genchange.rsb_add   = GetReal8(ew,CID_RSBAdd,_("Right Side Bearing Add"),&err);
	}
	if ( err )
return( true );

	genchange.use_vert_mapping = GGadgetIsChecked(GWidgetGetControl(ew,CID_UseVerticalMappings));
	if ( genchange.use_vert_mapping ) {
	    GGadget *map = GWidgetGetControl(ew,CID_VMappings);
	    int i,j;
	    int rows, cols = GMatrixEditGetColCnt(map);
	    struct matrix_data *mappings = GMatrixEditGet(map, &rows);

	    genchange.v_scale = GetReal8(ew,CID_VerticalScale,_("Vertical Scale"),&err)/100.;
	    if ( err )
return( true );
	    genchange.m.cnt = rows;
	    genchange.m.maps = malloc(rows*sizeof(struct position_maps));
	    for ( i=0; i<rows; ++i ) {
		genchange.m.maps[i].current   = mappings[cols*i+0].u.md_real;
		genchange.m.maps[i].desired   = mappings[cols*i+2].u.md_real;
		genchange.m.maps[i].cur_width = mappings[cols*i+1].u.md_real;
	    }
	    /* Order maps */
	    for ( i=0; i<rows; ++i ) for ( j=i+1; j<rows; ++j ) {
		if ( genchange.m.maps[i].current > genchange.m.maps[j].current ) {
		    struct position_maps temp;
		    temp = genchange.m.maps[i];
		    genchange.m.maps[i] = genchange.m.maps[j];
		    genchange.m.maps[j] =  temp;
		}
	    }
	} else {
	    genchange.vcounter_scale = GetReal8(ew,CID_VCounterPercent,_("Vertical Counter Scale"),&err)/100.;
	    genchange.vcounter_add   = GetReal8(ew,CID_VCounterAdd,_("Vertical Counter Add"),&err);
	    if ( err )
return( true );
	}
	if ( ed->gc == gc_smallcaps )
	    FVAddSmallCaps( (FontViewBase *) ed->fv, &genchange );
	else if ( ed->fv!=NULL )
	    FVGenericChange( (FontViewBase *) ed->fv, &genchange );
	else
	    CVGenericChange( (CharViewBase *) ed->cv, &genchange );
	free(genchange.glyph_extension);
	free(genchange.m.maps);
	free( genchange.extension_for_letters );
	free( genchange.extension_for_symbols );
	ed->done = true;
    }
return( true );
}

static GTextInfo stemwidth[] = {
    { (unichar_t *) N_("Width of Vertical Stems:"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Width/Height of Thick Stems:"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo stemheight[] = {
    { (unichar_t *) N_("Height of Horizontal Stems:"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Width/Height of Thin Stems:"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

static int CG_SameAs_Changed(GGadget *g, GEvent *e) {

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	GWindow ew = GGadgetGetWindow(g);
	int uniform = GGadgetIsChecked(GWidgetGetControl(ew,CID_StemsUniform));
	int by_dir = GGadgetIsChecked(GWidgetGetControl(ew,CID_Stems_H_V));
	int by_width = GGadgetIsChecked(GWidgetGetControl(ew,CID_Stems_by_Width));
	unichar_t *v_label = by_width ? stemwidth[1].text : stemwidth[0].text;
	unichar_t *h_label = by_width ? stemheight[1].text : stemheight[0].text;
	
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_StemWidth), !uniform);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_StemWidthAdd), by_dir);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_StemThreshold), by_width);
	if ( uniform ) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidth),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeight)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidthAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeightAdd)));
	} else if ( by_width ) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidthAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeightAdd)));
	}

	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemWidthLabel), (char *) v_label);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemHeightLabel), (char *) h_label);
    }
return( true );
}

static int CG_VStem_Changed(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow ew = GGadgetGetWindow(g);
	if ( GGadgetIsChecked(GWidgetGetControl(ew,CID_StemsUniform))) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidth),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeight)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidthAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeightAdd)));

	} else if ( GGadgetIsChecked(GWidgetGetControl(ew,CID_Stems_by_Width))) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_StemWidthAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_StemHeightAdd)));

	}
    }
return( true );
}

static int CG_CounterSameAs_Changed(GGadget *g, GEvent *e) {

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	GWindow ew = GGadgetGetWindow(g);
	int on = GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_is_SideB));
	int enabled = GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_isnt_SideB));
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_LSBPercent), enabled);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_LSBAdd), enabled);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_RSBPercent), enabled);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_RSBAdd), enabled);
	if ( on ) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_LSBPercent),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterPercent)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_LSBAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterAdd)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_RSBPercent),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterPercent)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_RSBAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterAdd)));
	}
    }
return( true );
}

static int CG_Counter_Changed(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow ew = GGadgetGetWindow(g);
	if ( GGadgetIsChecked(GWidgetGetControl(ew,CID_Counter_is_SideB))) {
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_LSBPercent),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterPercent)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_LSBAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterAdd)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_RSBPercent),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterPercent)));
	    GGadgetSetTitle(GWidgetGetControl(ew,CID_RSBAdd),
		    _GGadgetGetTitle(GWidgetGetControl(ew,CID_CounterAdd)));
	}
    }
return( true );
}

static int CG_UseVCounters(GGadget *g, GEvent *e) {

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	GWindow ew = GGadgetGetWindow(g);
	int on = GGadgetIsChecked(GWidgetGetControl(ew,CID_UseVerticalCounters));
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_VCounterPercent), on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_VCounterAdd), on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_VerticalScale), !on);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_VMappings), !on);
    }
return( true );
}

static int CG_VScale_Changed(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	int err=0;
	bigreal scale;
	GGadget *map = GWidgetGetControl(ew,CID_VMappings);
	int rows, cols = GMatrixEditGetColCnt(map);
	struct matrix_data *mappings = GMatrixEditGet(map, &rows);
	int i;

	scale = GetCalmReal8(ew,CID_VerticalScale,"unused",&err)/100.0;
	if ( err || scale<=0 || RealNear(ed->scale,scale) )
return( true );
	for ( i=0; i<rows; ++i ) {
	    bigreal offset = mappings[cols*i+2].u.md_real - rint(ed->scale*mappings[cols*i+0].u.md_real);
	    mappings[cols*i+2].u.md_real =
		    rint(scale * mappings[cols*i+0].u.md_real) + offset;
	}
	ed->scale = scale;
	GGadgetRedraw(map);
    }
return( true );
}

static int CG_PetiteCapsChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow ew = GGadgetGetWindow(g);
	int petite = GGadgetIsChecked(GWidgetGetControl(ew,CID_PetiteCaps));
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_Letter_Ext), petite ? "pc" : "sc" );
    }
return( true );
}

static int CG_SmallCapSymbols(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow ew = GGadgetGetWindow(g);
	int on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(ew,CID_Symbol_Ext), on);
    }
return( true );
}

static int ParseBlue(double blues[14],struct psdict *private,char *key) {
    int i;
    char *val, *end;

    if ( private==NULL )
return( 0 );
    if ( (val = PSDictHasEntry(private,key))==NULL )
return( 0 );
    while ( isspace( *val ) || *val=='[' ) ++val;

    for ( i=0; i<14; ++i ) {
	while ( isspace( *val )) ++val;
	if ( *val==']' || *val=='\0' )
return( i );
	blues[i] = strtod(val,&end);
	if ( end==val )		/* Not a number */
return( 0 );
	val = end;
    }
return( i );
}

static struct col_init mapci[5] = {
    { me_real, NULL, NULL, NULL, N_("Original Y Position") },
    { me_real, NULL, NULL, NULL, N_("Extent") },
    { me_real, NULL, NULL, NULL, N_("Resultant Y Position") },
};
static void MappingMatrixInit(struct matrixinit *mi,SplineFont *sf,
	double xheight, double capheight, double scale) {
    struct matrix_data *md;
    int cnt;
    double blues[14], others[14];
    int b=0,o=0,i,j;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 3;
    mi->col_init = mapci;

    if ( sf->private_dict!=NULL ) {
	b = ParseBlue(blues,sf->private_dict,"BlueValues");
	o = ParseBlue(others,sf->private_dict,"OtherBlues");
    }

    if ( (b>1 && (b&1)==0) || (o>1 && (o&1)==0)) {
	b>>=1; o>>=1;
	md = calloc(3*(b+o),sizeof(struct matrix_data));
	mi->initial_row_cnt = b+o;
	mi->matrix_data = md;

	for ( i=0; i<o; ++i ) {
	    md[3*i+0].u.md_real = others[2*i+1];
	    md[3*i+1].u.md_real = others[2*i] - others[2*i+1];
	    md[3*i+2].u.md_real = rint(scale*md[3*i+0].u.md_real);
	}
	for ( j=0; j<b; ++j ) {
	    if ( j==0 ) {
		md[3*(i+j)+0].u.md_real = blues[1];
		md[3*(i+j)+1].u.md_real = blues[0] - blues[1];
	    } else {
		md[3*(i+j)+0].u.md_real = blues[2*j];
		md[3*(i+j)+1].u.md_real = blues[2*j+1] - blues[2*j];
	    }
	    md[3*(i+j)+2].u.md_real = rint(scale*md[3*(i+j)+0].u.md_real);
	}
    } else if ( xheight==0 && capheight==0 ) {
	md = calloc(3,sizeof(struct matrix_data));
	mi->initial_row_cnt = 1;
	mi->matrix_data = md;
    } else {
	cnt = 1;	/* For the baseline */
	if ( xheight!=0 )
	    ++cnt;
	if ( capheight!=0 )
	    ++cnt;
	md = calloc(3*cnt,sizeof(struct matrix_data));
	mi->initial_row_cnt = cnt;
	mi->matrix_data = md;
	md[3*0+1].u.md_real = -1;
	cnt = 1;
	if ( xheight!=0 ) {
	    md[3*cnt+0].u.md_real =       xheight;
	    md[3*cnt+1].u.md_real =       1;
	    md[3*cnt+2].u.md_real = scale*xheight;
	    ++cnt;
	}
	if ( capheight!=0 ) {
	    md[3*cnt+0].u.md_real =       capheight;
	    md[3*cnt+1].u.md_real =       1;
	    md[3*cnt+2].u.md_real = scale*capheight;
	    ++cnt;
	}
    }
}

static int GlyphChange_Default(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	enum glyphchange_type gc = ed->gc;
	bigreal glyph_scale = 1.0, stem_scale=1.0;
	char glyph_factor[40], stem_factor[40];
	struct matrixinit mapmi;

	if ( gc==gc_subsuper ) {
	    GGadgetSetTitle8(GWidgetGetControl(ew,CID_Feature),"");
	    GGadgetSetTitle8(GWidgetGetControl(ew,CID_Extension),"");
	    GGadgetSetTitle8(GWidgetGetControl(ew,CID_VerticalOff),"");

	    glyph_scale = 2.0/3.0;
	    stem_scale  = 3.0/4.0;
	} else if ( gc == gc_smallcaps ) {
	    GGadgetSetTitle8(GWidgetGetControl(ew,CID_Letter_Ext),"sc");
	    GGadgetSetTitle8(GWidgetGetControl(ew,CID_Symbol_Ext),"taboldstyle");
	    GGadgetSetChecked(GWidgetGetControl(ew,CID_Symbols_Too),false);
	    GGadgetSetChecked(GWidgetGetControl(ew,CID_SmallCaps),true);

	    if ( ed->small->xheight!=0 && ed->small->capheight!=0 )
		glyph_scale = ed->small->xheight/ed->small->capheight;
	    if ( ed->small->lc_stem_width!=0 && ed->small->uc_stem_width!=0 )
		stem_scale  = ed->small->lc_stem_width/ed->small->uc_stem_width;
	}
	ed->scale = glyph_scale;
	sprintf( glyph_factor, "%.2f", (double) (100*glyph_scale) );
	sprintf( stem_factor , "%.2f", (double) (100* stem_scale) );

	GGadgetSetChecked(GWidgetGetControl(ew,CID_StemsUniform),true);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemHeight),stem_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemHeightAdd),"0");
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemWidth),stem_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_StemWidthAdd),"0");
	GGadgetSetChecked(GWidgetGetControl(ew,CID_DStemOn),true);

	GGadgetSetChecked(GWidgetGetControl(ew,CID_Counter_is_SideB),true);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_CounterPercent),glyph_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_CounterAdd),"0");
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_LSBPercent),glyph_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_LSBAdd),"0");
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_RSBPercent),glyph_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_RSBAdd),"0");

	GGadgetSetChecked(GWidgetGetControl(ew,CID_UseVerticalMappings),true);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_VCounterPercent),glyph_factor);
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_VCounterAdd),"0");
	GGadgetSetTitle8(GWidgetGetControl(ew,CID_VerticalScale),glyph_factor);

	MappingMatrixInit(&mapmi,
		ed->sf,
		gc==gc_smallcaps?0:ed->small->xheight,
		ed->small->capheight,glyph_scale);
	GMatrixEditSet(GWidgetGetControl(ew,CID_VMappings),
		mapmi.matrix_data,mapmi.initial_row_cnt,false);

	CG_SameAs_Changed(GWidgetGetControl(ew,CID_StemsUniform),NULL);
	CG_CounterSameAs_Changed(GWidgetGetControl(ew,CID_Counter_is_SideB),NULL);
	CG_UseVCounters(GWidgetGetControl(ew,CID_UseVerticalMappings),NULL);
    }
return( true );
}

void GlyphChangeDlg(FontView *fv,CharView *cv, enum glyphchange_type gc) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->b.sf : cv->b.sc->parent;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[64], boxes[19], *barray[11], *stemarray[22], *stemtarray[6],
	    *stemarrayhc[20], *stemarrayvc[8], *varrayi[16], *varrays[15],
	    *varrayhc[14], *varrayvc[12], *pcarray[4],
	    *varray[7], *harray[6], *voarray[6], *extarray[7], *exarray[6];
    GTextInfo label[64];
    GTabInfo aspects[5];
    int i,k,l,s, a;
    struct smallcaps small;
    struct matrixinit mapmi;
    bigreal glyph_scale = 1.0, stem_scale=1.0;
    char glyph_factor[40], stem_factor[40], stem_threshold[10];
    int layer = fv!=NULL ? fv->b.active_layer : CVLayer((CharViewBase *) cv);
    static GWindow last_dlg[gc_max] = { NULL };
    static SplineFont *last_sf[gc_max] = { NULL };
    static int intldone = false;

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;
    ed.small = &small;
    ed.gc = gc;

    if (!intldone) {
	intldone = true;
	for ( i=0; stemwidth[i].text; ++i )
	    stemwidth[i].text = (unichar_t *) _((char *) stemwidth[i].text);
	for ( i=0; stemheight[i].text; ++i )
	    stemheight[i].text = (unichar_t *) _((char *) stemheight[i].text);
    }

    SmallCapsFindConstants(&small,sf,layer); /* I want to know the xheight... */

    if ( last_dlg[gc] == NULL || last_sf[gc] != sf ) {
	if ( last_dlg[gc]!=NULL )
	    GDrawDestroyWindow(last_dlg[gc]);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  gc==gc_subsuper  ? _("Create Subscript/Superscript") :
				    gc==gc_smallcaps ? _("Create Small Caps") :
							_("Change Glyphs");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = 100;
	pos.height = 100;
	ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);


	k=l=s=a=0;

	memset(aspects,0,sizeof(aspects));
	memset(gcd,0,sizeof(gcd));
	memset(boxes,0,sizeof(boxes));
	memset(label,0,sizeof(label));
	memset(stemarray,0,sizeof(stemarray));

	if ( gc==gc_subsuper ) {
	    label[k].text = (unichar_t *) _(
		"Unlike most commands this one does not work directly on the\n"
		"selected glyphs. Instead, if you select a glyph FontForge will\n"
		"create (or reuse) another glyph named by appending the extension\n"
		"to the original name, and it will copy a modified version of\n"
		"the original glyph into the new one.");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GLabelCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;


	    gcd[k].gd.pos.width = 10; gcd[k].gd.pos.height = 10;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GSpacerCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;

	    label[k].text = (unichar_t *) _("Feature Tag:");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GLabelCreate;
	    extarray[0] = &gcd[k-1];

	    label[k].text = (unichar_t *) "";
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k].gd.cid = CID_Feature;
	    gcd[k].gd.u.list = ss_features;
	    gcd[k].gd.handle_controlevent = SS_Feature_Changed;
	    gcd[k++].creator = GListFieldCreate;
	    extarray[1] = &gcd[k-1];

	    label[k].text = (unichar_t *) _("Glyph Extension:");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GLabelCreate;
	    extarray[2] = &gcd[k-1];

	    label[k].text = (unichar_t *) "";
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k].gd.cid = CID_Extension;
	    gcd[k++].creator = GTextFieldCreate;
	    extarray[3] = &gcd[k-1]; extarray[4] = NULL;

	    boxes[2].gd.flags = gg_enabled|gg_visible;
	    boxes[2].gd.u.boxelements = extarray;
	    boxes[2].creator = GHBoxCreate;
	    varrayi[l++] = &boxes[2]; varrayi[l++] = NULL;

	    label[k].text = (unichar_t *) _("Vertical Offset:");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GLabelCreate;
	    voarray[0] = &gcd[k-1]; 

	    label[k].text = (unichar_t *) "0";
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k].gd.pos.width = 60;
	    gcd[k].gd.cid = CID_VerticalOff;
	    gcd[k++].creator = GTextFieldCreate;
	    voarray[1]= &gcd[k-1]; voarray[2] = GCD_Glue; voarray[3] = NULL;

	    boxes[3].gd.flags = gg_enabled|gg_visible;
	    boxes[3].gd.u.boxelements = voarray;
	    boxes[3].creator = GHBoxCreate;
	    varrayi[l++] = &boxes[3]; varrayi[l++] = NULL;
	    varrayi[l++] = GCD_Glue; varrayi[l++] = NULL; varrayi[l++] = NULL;

	    boxes[4].gd.flags = gg_enabled|gg_visible;
	    boxes[4].gd.u.boxelements = varrayi;
	    boxes[4].creator = GHVBoxCreate;

	    aspects[a].text = (unichar_t *) _("Introduction");
	    aspects[a].text_is_1byte = true;
	    aspects[a++].gcd = &boxes[4];

	    glyph_scale = 2.0/3.0;
	    stem_scale  = 3.0/4.0;
	} else if ( gc == gc_smallcaps ) {
	    label[k].text = (unichar_t *) _(
		"Unlike most commands this one does not work directly on the\n"
		"selected glyphs. Instead, if you select an \"A\" (or an \"a\")\n"
		"FontForge will create (or reuse) a glyph named \"a.sc\", and\n"
		"it will copy a modified version of the \"A\" glyph into \"a.sc\".");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GLabelCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;

	    gcd[k].gd.pos.width = 10; gcd[k].gd.pos.height = 10;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k++].creator = GSpacerCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;

	    label[k].text = (unichar_t *) _("Small Caps");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	    gcd[k].gd.handle_controlevent = CG_PetiteCapsChange;
	    gcd[k].gd.cid = CID_SmallCaps;
	    gcd[k++].creator = GRadioCreate;
	    pcarray[0] = &gcd[k-1];

	    label[k].text = (unichar_t *) _("Petite Caps");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_visible | gg_enabled ;
	    gcd[k].gd.handle_controlevent = CG_PetiteCapsChange;
	    gcd[k].gd.cid = CID_PetiteCaps;
	    gcd[k++].creator = GRadioCreate;
	    pcarray[1] = &gcd[k-1]; pcarray[2] = GCD_Glue; pcarray[3] = NULL;

	    boxes[2].gd.flags = gg_enabled|gg_visible;
	    boxes[2].gd.u.boxelements = pcarray;
	    boxes[2].creator = GHBoxCreate;
	    varrayi[l++] = &boxes[2]; varrayi[l++] = NULL;

	    label[k].text = (unichar_t *) _("Glyph Extensions");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_visible | gg_enabled;
	    gcd[k++].creator = GLabelCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;

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
	    gcd[k].gd.flags = gg_visible;
	    gcd[k].gd.cid = CID_Symbol_Ext;
	    gcd[k++].creator = GTextFieldCreate;
	    exarray[0] = &gcd[k-4]; exarray[1] = &gcd[k-3]; exarray[2] = &gcd[k-2]; exarray[3] = &gcd[k-1];
	    exarray[4] = NULL;

	    boxes[3].gd.flags = gg_enabled|gg_visible;
	    boxes[3].gd.u.boxelements = exarray;
	    boxes[3].creator = GHBoxCreate;
	    varrayi[l++] = &boxes[3]; varrayi[l++] = NULL;

	    label[k].text = (unichar_t *) _("Create small caps variants for symbols as well as letters");
	    label[k].text_is_1byte = true;
	    label[k].text_in_resource = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	    gcd[k].gd.flags = gg_enabled | gg_visible;
	    gcd[k].gd.handle_controlevent = CG_SmallCapSymbols;
	    gcd[k].gd.cid = CID_Symbols_Too;
	    gcd[k++].creator = GCheckBoxCreate;
	    varrayi[l++] = &gcd[k-1]; varrayi[l++] = NULL;
	    varrayi[l++] = GCD_Glue; varrayi[l++] = NULL; varrayi[l++] = NULL;

	    boxes[4].gd.flags = gg_enabled|gg_visible;
	    boxes[4].gd.u.boxelements = varrayi;
	    boxes[4].creator = GHVBoxCreate;

	    aspects[a].text = (unichar_t *) _("Introduction");
	    aspects[a].text_is_1byte = true;
	    aspects[a++].gcd = &boxes[4];

	    if ( small.xheight!=0 && small.capheight!=0 )
		glyph_scale = small.xheight/small.capheight;
	    if ( small.lc_stem_width!=0 && small.uc_stem_width!=0 )
		stem_scale  = small.lc_stem_width/small.uc_stem_width;
	}

	l = 0;

	ed.scale = glyph_scale;
	sprintf( glyph_factor, "%.2f", (double) (100*glyph_scale) );
	sprintf( stem_factor , "%.2f", (double) (100* stem_scale) );

	label[k].text = (unichar_t *) _("Uniform scaling for stems of any width and direction");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[k].gd.cid = CID_StemsUniform;
	gcd[k].gd.handle_controlevent = CG_SameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrays[l++] = &gcd[k-1]; varrays[l++] = NULL;

	label[k].text = (unichar_t *) _("Separate ratios for thin and thick stems");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_Stems_by_Width;
	gcd[k].gd.handle_controlevent = CG_SameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrays[l++] = &gcd[k-1]; varrays[l++] = NULL;

	label[k].text = (unichar_t *) _("Threshold between \"thin\" and \"thick\":");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemtarray[s++] = &gcd[k-1];

	sprintf(stem_threshold,"%.0f",GuessStemThreshold(sf));
	label[k].text = (unichar_t *) stem_threshold;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_StemThreshold;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemtarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemtarray[s++] = &gcd[k-1];
	stemtarray[s++] = NULL;
	stemtarray[s++] = NULL;

	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = stemtarray;
	boxes[6].creator = GHVBoxCreate;
	varrays[l++] = &boxes[6]; varrays[l++] = NULL;
	
	label[k].text = (unichar_t *) _("Separate ratios for horizontal and vertical stems");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_rad_continueold;
	gcd[k].gd.cid = CID_Stems_H_V;
	gcd[k].gd.handle_controlevent = CG_SameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrays[l++] = &gcd[k-1]; varrays[l++] = NULL;

	s = 0;
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_StemHeightLabel;
	gcd[k].gd.u.list = stemheight;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) stem_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_StemHeight;
	gcd[k].gd.handle_controlevent = CG_VStem_Changed;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_StemHeightAdd;
	gcd[k].gd.handle_controlevent = CG_VStem_Changed;
	gcd[k++].creator = GTextFieldCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];
	stemarray[s++] = NULL;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_StemWidthLabel;
	gcd[k].gd.u.list = stemwidth;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) stem_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_StemWidth;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_StemWidthAdd;
	gcd[k++].creator = GTextFieldCreate;
	stemarray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarray[s++] = &gcd[k-1];
	stemarray[s++] = NULL;
	stemarray[s++] = NULL;

	boxes[7].gd.flags = gg_enabled|gg_visible;
	boxes[7].gd.u.boxelements = stemarray;
	boxes[7].creator = GHVBoxCreate;
	varrays[l++] = &boxes[7]; varrays[l++] = NULL;

	label[k].text = (unichar_t *) _("Activate diagonal stem processing");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[k].gd.cid = CID_DStemOn;
	gcd[k++].creator = GCheckBoxCreate;
	varrays[l++] = &gcd[k-1]; varrays[l++] = NULL;
	varrays[l++] = GCD_Glue;  varrays[l++] = NULL; varrays[l++] = NULL;

	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = varrays;
	boxes[8].creator = GHVBoxCreate;

	aspects[a].text = (unichar_t *) _("Stems");
	aspects[a].text_is_1byte = true;
	aspects[a++].gcd = &boxes[8];

	l=s=0;

	label[k].text = (unichar_t *) _("Retain current advance width, center glyph within that width");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_Counter_SameAdvance;
	gcd[k].gd.handle_controlevent = CG_CounterSameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrayhc[l++] = &gcd[k-1]; varrayhc[l++] = NULL;

	label[k].text = (unichar_t *) _("Retain current advance width, scale side bearings proportionally");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_Counter_PropAdvance;
	gcd[k].gd.handle_controlevent = CG_CounterSameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrayhc[l++] = &gcd[k-1]; varrayhc[l++] = NULL;

	label[k].text = (unichar_t *) _("Uniform scaling for horizontal counters and side bearings");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[k].gd.cid = CID_Counter_is_SideB;
	gcd[k].gd.handle_controlevent = CG_CounterSameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrayhc[l++] = &gcd[k-1]; varrayhc[l++] = NULL;

	label[k].text = (unichar_t *) _("Non uniform scaling for horizontal counters and side bearings");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_Counter_isnt_SideB;
	gcd[k].gd.handle_controlevent = CG_CounterSameAs_Changed;
	gcd[k++].creator = GRadioCreate;
	varrayhc[l++] = &gcd[k-1]; varrayhc[l++] = NULL;

	label[k].text = (unichar_t *) _("Counter Size:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) glyph_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_CounterPercent;
	gcd[k].gd.handle_controlevent = CG_Counter_Changed;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_CounterAdd;
	gcd[k].gd.handle_controlevent = CG_Counter_Changed;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];
	stemarrayhc[s++] = NULL;

	label[k].text = (unichar_t *) _("Left Side Bearing:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) glyph_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_LSBPercent;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_LSBAdd;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];
	stemarrayhc[s++] = NULL;

	label[k].text = (unichar_t *) _("Right Side Bearing:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) glyph_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_RSBPercent;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_RSBAdd;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayhc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayhc[s++] = &gcd[k-1];
	stemarrayhc[s++] = NULL;
	stemarrayhc[s++] = NULL;

	boxes[10].gd.flags = gg_enabled|gg_visible;
	boxes[10].gd.u.boxelements = stemarrayhc;
	boxes[10].creator = GHVBoxCreate;
	varrayhc[l++] = &boxes[10]; varrayhc[l++] = NULL;
	varrayhc[l++] = GCD_Glue; varrayhc[l++] = NULL; varrayhc[l++] = NULL;

	boxes[11].gd.flags = gg_enabled|gg_visible;
	boxes[11].gd.u.boxelements = varrayhc;
	boxes[11].creator = GHVBoxCreate;

	aspects[a].text = (unichar_t *) _("Horizontal");
	aspects[a].text_is_1byte = true;
	aspects[a++].gcd = &boxes[11];

	l=s=0;

	label[k].text = (unichar_t *) _("Control Vertical Counters (use for CJK)");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.handle_controlevent = CG_UseVCounters;
	gcd[k].gd.cid = CID_UseVerticalCounters;
	gcd[k++].creator = GRadioCreate;
	varrayvc[l++] = &gcd[k-1]; varrayvc[l++] = NULL;

	label[k].text = (unichar_t *) _("Vertical Counters:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayvc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) glyph_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_VCounterPercent;
	gcd[k].gd.pos.width = 60;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayvc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("% +");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayvc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) "0";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.cid = CID_VCounterAdd;
	gcd[k++].creator = GTextFieldCreate;
	stemarrayvc[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em-units");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	stemarrayvc[s++] = &gcd[k-1];
	stemarrayvc[s++] = NULL; stemarrayvc[s++] = NULL;

	if ( s > sizeof(stemarrayvc)/sizeof(stemarrayvc[0]) )
	    IError( "Increase size of stemarrayvc" );

	boxes[13].gd.flags = gg_enabled|gg_visible;
	boxes[13].gd.u.boxelements = stemarrayvc;
	boxes[13].creator = GHBoxCreate;
	varrayvc[l++] = &boxes[13]; varrayvc[l++] = NULL;

	label[k].text = (unichar_t *) _("Control Vertical Mapping (use for Latin, Greek, Cyrillic)");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible| gg_cb_on | gg_rad_continueold;
	gcd[k].gd.popup_msg = _("These mappings may be used to fix certain standard heights.");
	gcd[k].gd.cid = CID_UseVerticalMappings;
	gcd[k].gd.handle_controlevent = CG_UseVCounters;
	gcd[k++].creator = GRadioCreate;
	varrayvc[l++] = &gcd[k-1]; varrayvc[l++] = NULL;

	s = 0;
	label[k].text = (unichar_t *) _("Vertical Scale:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	harray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) glyph_factor;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_VerticalScale;
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.handle_controlevent = CG_VScale_Changed;
	gcd[k++].creator = GTextFieldCreate;
	harray[s++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("%");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	harray[s++] = &gcd[k-1]; harray[s++] = GCD_Glue; harray[s++] = NULL;

	boxes[14].gd.flags = gg_enabled|gg_visible;
	boxes[14].gd.u.boxelements = harray;
	boxes[14].creator = GHBoxCreate;
	varrayvc[l++] = &boxes[14]; varrayvc[l++] = NULL;


	MappingMatrixInit(&mapmi,
		sf,
		gc==gc_smallcaps?0:small.xheight,
		small.capheight,glyph_scale);

	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k].gd.cid = CID_VMappings;
	gcd[k].gd.u.matrix = &mapmi;
	gcd[k++].creator = GMatrixEditCreate;
	varrayvc[l++] = &gcd[k-1]; varrayvc[l++] = NULL; varrayvc[l++] = NULL;

	boxes[15].gd.flags = gg_enabled|gg_visible;
	boxes[15].gd.u.boxelements = varrayvc;
	boxes[15].creator = GHVBoxCreate;

	aspects[a].text = (unichar_t *) _("Vertical");
	aspects[a].text_is_1byte = true;
	aspects[a++].gcd = &boxes[15];

	l=0;

	gcd[k].gd.u.tabs = aspects;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_tabset_scroll;
	gcd[k].gd.cid = CID_TabSet;
	gcd[k++].creator = GTabSetCreate;
	varray[l++] = &gcd[k-1]; varray[l++] = NULL;


	gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = GlyphChange_OK;
	gcd[k++].creator = GButtonCreate;
	barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.popup_msg = _("Everything to its default value");
	label[k].text = (unichar_t *) _("Reset");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = GlyphChange_Default;
	gcd[k++].creator = GButtonCreate;
	barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue;

	gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = CondenseExtend_Cancel;
	gcd[k].creator = GButtonCreate;
	barray[6] = GCD_Glue; barray[7] = &gcd[k]; barray[8] = GCD_Glue;
	barray[9] = NULL;

	boxes[17].gd.flags = gg_enabled|gg_visible;
	boxes[17].gd.u.boxelements = barray;
	boxes[17].creator = GHBoxCreate;
	varray[l++] = &boxes[17]; varray[l++] = NULL; varray[l++] = NULL;

	if ( l>=sizeof(varray)/sizeof(varray[0]))
	    IError("Increase size of varray" );
	if ( k>=sizeof(gcd)/sizeof(gcd[0]))
	    IError("Increase size of gcd" );
	if ( k>=sizeof(label)/sizeof(label[0]))
	    IError("Increase size of label" );

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableCol(boxes[13].ret,gb_expandglue);
	if ( boxes[2].ret!=NULL )
	    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	if ( boxes[3].ret!=NULL )
	    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	if ( boxes[4].ret!=NULL )
	    GHVBoxSetExpandableRow(boxes[4].ret,gb_expandglue);
	GHVBoxSetExpandableRow(boxes[8].ret,gb_expandglue);
	GHVBoxSetExpandableRow(boxes[11].ret,gb_expandglue);
	GHVBoxSetExpandableRow(boxes[15].ret,4);
	GHVBoxSetExpandableCol(boxes[17].ret,gb_expandgluesame);
	GHVBoxSetExpandableRow(boxes[0].ret,0);
	GHVBoxFitWindow(boxes[0].ret);
	/* if ( gc==gc_subsuper ) */
	    /* GMatrixEditShowColumn(GWidgetGetControl(gw,CID_VMappings),2,false);*/
	last_dlg[gc] = gw;
	last_sf[gc] = sf;
    } else {
	int err = false;
	ed.gw = gw = last_dlg[gc];
	ed.scale = GetCalmReal8(gw,CID_VerticalScale,"unused",&err)/100.0;
	if ( err )
	    ed.scale = 1;
	GDrawSetUserData(last_dlg[gc],&ed);
	GDrawSetTransientFor(last_dlg[gc],(GWindow) -1);
    }
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
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
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0][0] = &gcd[k-1];

    sprintf( emb_width, "%d", sf==lastsf ? last_width : (sf->ascent+sf->descent)/20 );
    label[k].text = (unichar_t *) emb_width;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
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
    gcd[k].gd.popup_msg = _("Embolden as appropriate for Latin, Cyrillic and Greek scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_LCG;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_CJK");
    gcd[k].gd.popup_msg = _("Embolden as appropriate for Chinese, Japanese, Korean scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_CJK;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_Auto");
    gcd[k].gd.popup_msg = _("Choose the appropriate method depending on the glyph's script");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Auto;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("C_ustom");
    gcd[k].gd.popup_msg = _("User controls the emboldening with the next two fields");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
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
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.popup_msg = _("Any points this high will be assumed to be on serifs,\nand will remain at that height after processing.\n(So serifs should remain the same size).\n(If you do wish the serifs to grow, set this to 0)");
    gcd[k++].creator = GLabelCreate;
    hvarray[4][0] = &gcd[k-1];

    sprintf( serifh, "%g", SFSerifHeight(sf));
    label[k].text = (unichar_t *) serifh;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SerifHeight;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[4][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Fuzz");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.popup_msg = _("Allow the height match to differ by this much");
    gcd[k++].creator = GLabelCreate;
    hvarray[4][2] = &gcd[k-1];

    label[k].text = (unichar_t *) ".9";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SerifHFuzz;
    gcd[k].gd.popup_msg = gcd[k-1].gd.popup_msg;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[4][3] = &gcd[k-1];
    hvarray[4][4] = NULL;

    label[k].text = (unichar_t *) _("Counters:");
    gcd[k].gd.popup_msg = _("The simple application of this algorithm will squeeze counters\nThat is not normally seen in bold latin fonts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    carray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Squish");
    gcd[k].gd.popup_msg = _("Make the counters narrower");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Squish;
    /*gcd[k].gd.handle_controlevent = Embolden_Counter;*/
    gcd[k++].creator = GRadioCreate;
    carray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Retain");
    gcd[k].gd.popup_msg = _("Try to insure that the counters are as wide\nafterward as they were before");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Retain;
    /*gcd[k].gd.handle_controlevent = Embolden_Counter;*/
    gcd[k++].creator = GRadioCreate;
    carray[2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Auto");
    gcd[k].gd.popup_msg = _("Retain counter size for glyphs using latin algorithm\nSquish them for those using CJK." );
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible| gg_cb_on;
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
    gcd[k].gd.flags = gg_enabled | gg_visible| (last_overlap?gg_cb_on:0);
    gcd[k].gd.cid = CID_CleanupSelfIntersect;
    gcd[k].gd.popup_msg = _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
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
    .95,		/* xheight percent */
    /* horizontal squash, lsb, stemsize, countersize, rsb */
    { .91, .89, .90, .91 },	/* For lower case */
    { .91, .93, .93, .91 },	/* For upper case */
    { .91, .93, .93, .91 },	/* For things which are neither upper nor lower case */
    srf_flat,		/* Secondary serifs (initial, medial on "m", descender on "p", "q" */
    true,		/* Transform bottom serifs */
    true,		/* Transform serifs at x-height */
    false,		/* Transform serifs on ascenders */
    true,		/* Transform serifs on diagonal stems at baseline and x-height */

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
    true,		/* Make the cyrillic "dzhe" glyph look like a latin "u" (same glyph used for cyrillic "i") */

    ITALICINFO_REMAINDER
};

void ObliqueDlg(FontView *fv, CharView *cv) {
    bigreal temp;
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
    free(ret);
    last_ii.italic_angle = temp;
    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1;
    transform[2] = -tan( last_ii.italic_angle * FF_PI/180.0 );
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
#define CID_Cyrl_Dje	1016
#define CID_Cyrl_Dzhe	1017
#define CID_BottomSerifs	2001
#define CID_XHeightSerifs	2002
#define CID_AscenderSerifs	2003
#define CID_DiagSerifs		2004
#define CID_Flat		2011
#define CID_Slanted		2012
#define CID_PenSlant		2013
#define CID_CompressLSB		3001		/* for lc, 3011 for uc, 3021 for others */
#define CID_CompressStem	3002		/* for lc, 3012 for uc, 3022 for others */
#define CID_CompressCounter	3003		/* for lc, 3013 for uc, 3023 for others */
#define CID_CompressRSB		3004		/* for lc, 3014 for uc, 3024 for others */
#define CID_XHeightPercent	4001
#define CID_ItalicAngle		4002

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
	ii.xheight_percent = GetReal8(ew,CID_XHeightPercent,_("XHeight Percent"),&err)/100;
	ii.italic_angle = GetReal8(ew,CID_ItalicAngle,_("Italic Angle"),&err);
	if ( err )
return( true );

	ii.secondary_serif = GGadgetIsChecked(GWidgetGetControl(ew,CID_Flat)) ? srf_flat :
		GGadgetIsChecked(GWidgetGetControl(ew,CID_Slanted)) ? srf_simpleslant :
			 srf_complexslant;

	ii.transform_bottom_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_BottomSerifs));
	ii.transform_top_xh_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_XHeightSerifs));
	ii.transform_top_as_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_AscenderSerifs));
	ii.transform_diagon_serifs= GGadgetIsChecked(GWidgetGetControl(ew,CID_DiagSerifs));

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
	ii.cyrl_dje = GGadgetIsChecked(GWidgetGetControl(ew,CID_Cyrl_Dje));
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
    GGadgetCreateData gcd[54], boxes[7], *forms[30], *compress[5][6], *sarray[5],
	    *iaarray[3][3], *barray[10], *varray[39];
    GTextInfo label[54];
    int k,f,r,i;
    char lsb[3][40], stems[3][40], counters[3][40], rsb[3][40], ia[40], xp[40];

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

    gcd[k].gd.pos.width = 10; gcd[k].gd.pos.height = 10;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GSpacerCreate;
    forms[f++] = GCD_Glue;
    forms[f++] = GCD_Glue; forms[f++] = GCD_Glue;
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

    label[k].image = &GIcon_u452Italic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_dje ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Dje;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_Glue; forms[f++] = NULL;

    label[k].image = &GIcon_u45fItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.cyrl_dzhe ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Cyrl_Dzhe;
    gcd[k++].creator = GCheckBoxCreate;
    forms[f++] = &gcd[k-1];
    forms[f++] = GCD_Glue; forms[f++] = GCD_Glue; forms[f++] = GCD_Glue; forms[f++] = NULL;
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

    label[k].text = (unichar_t *) _("Transform descender serifs");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_pItalic;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.pq_deserif ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_P;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r++] = &gcd[k-1]; varray[r++] = NULL;

    label[k].text = (unichar_t *) _("Transform diagonal serifs");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_DiagSerifs;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.transform_diagon_serifs ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_DiagSerifs;
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
    label[k].image_precedes = true;
    label[k].image = &GIcon_FlatSerif;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.secondary_serif==srf_flat ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Flat;
    gcd[k++].creator = GRadioCreate;
    sarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Slanted");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_SlantSerif;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    if ( last_ii.secondary_serif==srf_simpleslant ) gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Slanted;
    gcd[k++].creator = GRadioCreate;
    sarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Pen Slanted");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    label[k].image_precedes = true;
    label[k].image = &GIcon_PenSerif;
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
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.popup_msg = _("Left Side Bearing");
    gcd[k++].creator = GLabelCreate;
    compress[0][1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Stems");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    compress[0][2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Counters");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    compress[0][3] = &gcd[k-1];

    label[k].text = (unichar_t *) _("RSB");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.popup_msg = _("Right Side Bearing");
    gcd[k++].creator = GLabelCreate;
    compress[0][4] = &gcd[k-1]; compress[0][5] = NULL;

    for ( i=0; i<3; ++i ) {
	struct hsquash *hs = &(&last_ii.lc)[i];

	label[k].text = (unichar_t *) (i==0 ? _("Lower Case") : i==1 ? _("Upper Case") : _("Others"));
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible;
	gcd[k++].creator = GLabelCreate;
	compress[i+1][0] = &gcd[k-1];

	sprintf( lsb[i], "%g", 100.0* hs->lsb_percent );
	label[k].text = (unichar_t *) lsb[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_CompressLSB+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][1] = &gcd[k-1];

	sprintf( stems[i], "%g", 100.0* hs->stem_percent );
	label[k].text = (unichar_t *) stems[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_CompressStem+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][2] = &gcd[k-1];

	sprintf( counters[i], "%g", 100.0* hs->counter_percent );
	label[k].text = (unichar_t *) counters[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_CompressCounter+10*i;
	gcd[k++].creator = GTextFieldCreate;
	compress[i+1][3] = &gcd[k-1];

	sprintf( rsb[i], "%g", 100.0* hs->rsb_percent );
	label[k].text = (unichar_t *) rsb[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
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

    label[k].text = (unichar_t *) _("XHeight Percent:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    iaarray[0][0] = &gcd[k-1];

    sprintf( xp, "%g", rint(last_ii.xheight_percent*100) );
    label[k].text = (unichar_t *) xp;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 50;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_XHeightPercent;
    gcd[k].gd.popup_msg = _("Traditionally the x-height of an italic face is slightly less\nthan the x-height of the companion roman");
    gcd[k++].creator = GTextFieldCreate;
    iaarray[0][1] = &gcd[k-1]; iaarray[0][2] = NULL;

    label[k].text = (unichar_t *) _("Italic Angle:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    iaarray[1][0] = &gcd[k-1];

    sprintf( ia, "%g", last_ii.italic_angle );
    label[k].text = (unichar_t *) ia;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 50;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_ItalicAngle;
    gcd[k++].creator = GTextFieldCreate;
    iaarray[1][1] = &gcd[k-1]; iaarray[1][2] = NULL; iaarray[2][0] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = iaarray[0];
    boxes[5].creator = GHVBoxCreate;
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
	"And on all Greek lower case letters. And maybe everything else.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
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

/* ************************************************************************** */
/* ************************* Change XHeight Dialog ************************** */
/* ************************************************************************** */
#define CID_XHeight_Current	1001
#define CID_XHeight_Desired	1002
#define CID_Serif_Height	1003

static int XHeight_OK(GGadget *g, GEvent *e) {
    int err = false;
    struct xheightinfo xi;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	xi.xheight_current = GetReal8(ew,CID_XHeight_Current,_("Current X-Height"),&err);
	xi.xheight_desired = GetReal8(ew,CID_XHeight_Desired,_("Desired X-Height"),&err);
	xi.serif_height    = GetReal8(ew,CID_Serif_Height   ,_("Serif Height"),&err);
	if ( err )
return( true );

	ChangeXHeight( (FontViewBase *) ed->fv, (CharViewBase *) ed->cv, &xi );
	ed->done = true;
    }
return( true );
}

void ChangeXHeightDlg(FontView *fv,CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->b.sf : cv->b.sc->parent;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[31], boxes[7], *barray[8], *hvarray[40];
    GTextInfo label[31];
    int k;
    char xh_c[40], xh_d[40], sh[40];
    struct xheightinfo xi;

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;

    InitXHeightInfo(sf,fv!=NULL ? fv->b.active_layer: CVLayer((CharViewBase *) cv),&xi);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Change XHeight");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,style_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    label[k].text = (unichar_t *) _("Current x-height:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    sprintf( xh_c, "%g", rint( xi.xheight_current ));
    label[k].text = (unichar_t *) xh_c;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_XHeight_Current;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[0] = &gcd[k-2]; hvarray[1] = &gcd[k-1]; hvarray[2] = NULL;

    label[k].text = (unichar_t *) _("Desired x-height:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible  | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( xh_d, "%g", rint( xi.xheight_current ));
    label[k].text = (unichar_t *) xh_d;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_XHeight_Desired;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[3] = &gcd[k-2]; hvarray[4] = &gcd[k-1]; hvarray[5] = NULL;

    label[k].text = (unichar_t *) _("Serif height:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_visible  | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( sh, "%g", rint( xi.xheight_desired ));
    label[k].text = (unichar_t *) sh;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Serif_Height;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[6] = &gcd[k-2]; hvarray[7] = &gcd[k-1]; hvarray[8] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = XHeight_OK;
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
    hvarray[9] = &boxes[3]; hvarray[10] = GCD_ColSpan; hvarray[11] = NULL;
    hvarray[12] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
