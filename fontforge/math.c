/* Copyright (C) 2007 by George Williams */
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
#include <stddef.h>
#include <gkeysym.h>

#ifdef FONTFORGE_CONFIG_DEVICETABLES
#define MCD(ui_name,name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -1,msg,np }
#define MCDD(ui_name,name,devtab_name,msg,np) { ui_name, #name, offsetof(struct MATH,name), offsetof(struct MATH,devtab_name),msg,np }
#else
#define MCD(ui_name,name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -1,msg,np }
#define MCDD(ui_name,name,devtab_name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -2,msg,np }
#endif

struct math_constants_descriptor math_constants_descriptor[] = {
    MCD(N_("ScriptPercentScaleDown:"),ScriptPercentScaleDown,N_("Percentage scale down for script level 1"),0),
    MCD(N_("ScriptScriptPercentScaleDown:"),ScriptScriptPercentScaleDown,N_("Percentage scale down for script level 2"),0),
    MCD(N_("DelimitedSubFormulaMinHeight:"),DelimitedSubFormulaMinHeight,N_("Minimum height at which to treat a delimited\nexpression as a subformula"),0),
    MCD(N_("DisplayOperatorMinHeight:"),DisplayOperatorMinHeight,N_("Minimum height of n-ary operators (integration, summation, etc.)"),0),
    MCDD(N_("MathLeading:"),MathLeading,MathLeading_adjust,N_("White space to be left between math formulae\nto ensure proper line spacing."),0),
    MCDD(N_("AxisHeight:"),AxisHeight,AxisHeight_adjust,N_("Axis height of the font"),0),
    MCDD(N_("AccentBaseHeight:"),AccentBaseHeight,AccentBaseHeight_adjust,N_("Maximum (ink) height of accent base that\ndoes not require raising the accents."),0),
    MCDD(N_("FlattenedAccentBaseHeight:"),FlattenedAccentBaseHeight,FlattenedAccentBaseHeight_adjust,N_("Maximum (ink) height of accent base that\ndoes not require flattening the accents."),0),
    MCDD(N_("SubscriptShiftDown:"),SubscriptShiftDown,SubscriptShiftDown_adjust,N_("The standard shift down applied to subscript elements.\nPositive for moving downward."),1),
    MCDD(N_("SubscriptTopMax:"),SubscriptTopMax,SubscriptTopMax_adjust,N_("Maximum height of the (ink) top of subscripts\nthat does not require moving\nubscripts further down."),0),
    MCDD(N_("SubscriptBaselineDropMin:"),SubscriptBaselineDropMin,SubscriptBaselineDropMin_adjust,N_("Maximum allowed drop of the baseline of\nsubscripts realtive to the bottom of the base.\nUsed for bases that are treated as a box\nor extended shape. Positive for subscript\nbaseline dropped below base bottom."),0),
    MCDD(N_("SuperscriptShiftUp:"),SuperscriptShiftUp,SuperscriptShiftUp_adjust,N_("Standard shift up applied to superscript elements."),0),
    MCDD(N_("SuperscriptShiftUpCramped:"),SuperscriptShiftUpCramped,SuperscriptShiftUpCramped_adjust,N_("Standard shift of superscript relative\nto base in cramped mode."),0),
    MCDD(N_("SuperscriptBottomMin:"),SuperscriptBottomMin,SuperscriptBottomMin_adjust,N_("Minimum allowed hieght of the bottom\nof superscripts that does not require moving\nthem further up."),0),
    MCDD(N_("SuperscriptBaselineDropMax:"),SubscriptBaselineDropMin,SubscriptBaselineDropMin_adjust,N_("Maximum allowed drop of the baseline of\nsuperscripts realtive to the top of the base.\nUsed for bases that are treated as a box\nor extended shape. Positive for superscript\nbaseline below base top."),0),
    MCDD(N_("SubSuperscriptGapMin:"),SubSuperscriptGapMin,SubSuperscriptGapMin_adjust,N_("Minimum gap between the supersecript and subscript ink."),0),
    MCDD(N_("SuperscriptBottomMaxWithSubscript:"),SuperscriptBottomMaxWithSubscript,SuperscriptBottomMaxWithSubscript_adjust,N_("The maximum level to which the (ink) bottom\nof superscript can be pushed to increase the\ngap between superscript and subscript, before\nsubscript starts being moved down."),0),
    MCDD(N_("SpaceAfterScript:"),SpaceAfterScript,SpaceAfterScript_adjust,N_("Extra white space to be added after each\nub/superscript."),0),
    MCDD(N_("UpperLimitGapMin:"),UpperLimitGapMin,UpperLimitGapMin_adjust,N_("Minimum gap between the bottom of the\nupper limit, and the top fo the base operator."),1),
    MCDD(N_("UpperLimitBaselineRiseMin:"),UpperLimitBaselineRiseMin,UpperLimitBaselineRiseMin_adjust,N_("Minimum distance between the baseline of an upper\nlimit and the bottom of the base operator."),0),
    MCDD(N_("LowerLimitGapMin:"),LowerLimitGapMin,LowerLimitGapMin_adjust,N_("Standard shift up applied to the top element of\na stack."),0),
    MCDD(N_("LowerLimitBaselineDropMin:"),LowerLimitBaselineDropMin,LowerLimitBaselineDropMin_adjust,N_("Minimum distance between the baseline of the\nlower limit and bottom of the base operator."),0),
    MCDD(N_("StackTopShiftUp:"),StackTopShiftUp,StackTopShiftUp_adjust,N_("Standard shift up applied to the top element of a stack."),1),
    MCDD(N_("StackTopDisplayStyleShiftUp:"),StackTopDisplayStyleShiftUp,StackTopDisplayStyleShiftUp_adjust,N_("Standard shift up applied to the top element of\na stack in display style."),0),
    MCDD(N_("StackBottomShiftDown:"),StackBottomShiftDown,StackBottomShiftDown_adjust,N_("Standard shift down applied to the bottom element of a stack.\nPositive values indicate downward motion."),0),
    MCDD(N_("StackBottomDisplayStyleShiftDown:"),StackBottomDisplayStyleShiftDown,StackBottomDisplayStyleShiftDown_adjust,N_("Standard shift down applied to the bottom\nelement of a stack in display style.\nPositive values indicate downward motion."),0),
    MCDD(N_("StackGapMin:"),StackGapMin,StackGapMin_adjust,N_("Minimum gap between bottom of the top\nelement of a stack, and the top of the bottom element."),0),
    MCDD(N_("StackDisplayStyleGapMin:"),StackDisplayStyleGapMin,StackDisplayStyleGapMin_adjust,N_("Minimum gap between bottom of the top\nelement of a stack and the top of the bottom\nelement in display style."),0),
    MCDD(N_("StretchStackTopShiftUp:"),StretchStackTopShiftUp,StretchStackTopShiftUp_adjust,N_("Standard shift up applied to the top element of the stretch stack."),0),
    MCDD(N_("StretchStackBottomShiftDown:"),StretchStackBottomShiftDown,StretchStackBottomShiftDown_adjust,N_("Standard shift down applied to the bottom\nelement of the stretch stack.\nPositive values indicate downward motion."),0),
    MCDD(N_("StretchStackGapAboveMin:"),StretchStackGapAboveMin,StretchStackGapAboveMin_adjust,N_("Minimum gap between the ink of the stretched\nelement and the ink bottom of the element\nabove.."),0),
    MCDD(N_("StretchStackGapBelowMin:"),StretchStackGapBelowMin,StretchStackGapBelowMin_adjust,N_("Minimum gap between the ink of the stretched\nelement and the ink top of the element below."),0),
    MCDD(N_("FractionNumeratorShiftUp:"),FractionNumeratorShiftUp,FractionNumeratorShiftUp_adjust,N_("Standard shift up applied to the numerator."),1),
    MCDD(N_("FractionNumeratorDisplayStyleShiftUp:"),FractionNumeratorDisplayStyleShiftUp,FractionNumeratorDisplayStyleShiftUp_adjust,N_("Standard shift up applied to the\nnumerator in display style."),0),
    MCDD(N_("FractionDenominatorShiftDown:"),FractionDenominatorShiftDown,FractionDenominatorShiftDown_adjust,N_("Standard shift down applied to the denominator.\nPostive values indicate downward motion."),0),
    MCDD(N_("FractionDenominatorDisplayStyleShiftDown:"),FractionDenominatorDisplayStyleShiftDown,FractionDenominatorDisplayStyleShiftDown_adjust,N_("Standard shift down applied to the\ndenominator in display style.\nPostive values indicate downward motion."),0),
    MCDD(N_("FractionNumeratorGapMin:"),FractionNumeratorGapMin,FractionNumeratorGapMin_adjust,N_("Minimum tolerated gap between the ink\nbottom of the numerator and the ink of the fraction bar."),0),
    MCDD(N_("FractionNumeratorDisplayStyleGapMin:"),FractionNumeratorDisplayStyleGapMin,FractionNumeratorDisplayStyleGapMin_adjust,N_("Minimum tolerated gap between the ink\nbottom of the numerator and the ink of the fraction\nbar in display style."),0),
    MCDD(N_("FractionRuleThickness:"),FractionRuleThickness,FractionRuleThickness_adjust,N_("Thickness of the fraction bar."),0),
    MCDD(N_("FractionDenominatorGapMin:"),FractionDenominatorGapMin,FractionDenominatorGapMin_adjust,N_("Minimum tolerated gap between the ink top of the denominator\nand the ink of the fraction bar.."),0),
    MCDD(N_("FractionDenominatorDisplayStyleGapMin:"),FractionDenominatorDisplayStyleGapMin,FractionDenominatorDisplayStyleGapMin_adjust,N_("Minimum tolerated gap between the ink top of the denominator\nand the ink of the fraction bar in display style."),0),
    MCDD(N_("SkewedFractionHorizontalGap:"),SkewedFractionHorizontalGap,SkewedFractionHorizontalGap_adjust,N_("Horizontal distance between the top\nand bottom elemnts of a skewed fraction."),0),
    MCDD(N_("SkewedFractionVerticalGap:"),SkewedFractionVerticalGap,SkewedFractionVerticalGap_adjust,N_("Vertical distance between the ink of the top and\nbottom elements of a skewed fraction."),0),
    MCDD(N_("OverbarVerticalGap:"),OverbarVerticalGap,OverbarVerticalGap_adjust,N_("Distance between the overbar and\nthe ink top of the base."),1),
    MCDD(N_("OverbarRuleThickness:"),OverbarRuleThickness,OverbarRuleThickness_adjust,N_("Thickness of the overbar."),0),
    MCDD(N_("OverbarExtraAscender:"),OverbarExtraAscender,OverbarExtraAscender_adjust,N_("Extra white space reserved above the overbar."),0),
    MCDD(N_("UnderbarVerticalGap:"),UnderbarVerticalGap,UnderbarVerticalGap_adjust,N_("Distance between underbar and\nthe (ink) bottom of the base."),0),
    MCDD(N_("UnderbarRuleThickness:"),UnderbarRuleThickness,UnderbarRuleThickness_adjust,N_("Thickness of the underbar."),0),
    MCDD(N_("UnderbarExtraDescender:"),UnderbarExtraDescender,UnderbarExtraDescender_adjust,N_("Extra white space resevered below the underbar."),0),
    MCDD(N_("RadicalVerticalGap:"),RadicalVerticalGap,RadicalVerticalGap_adjust,N_("Space between the ink to of the\nexpression and the bar over it."),1),
    MCDD(N_("RadicalDisplayStyleVerticalGap:"),RadicalDisplayStyleVerticalGap,RadicalDisplayStyleVerticalGap_adjust,N_("Space between the ink top of the\nexpression and the bar over it in display\nstyle."),0),
    MCDD(N_("RadicalRuleThickness:"),RadicalRuleThickness,RadicalRuleThickness_adjust,N_("Thickness of the radical rule in\ndesigned or constructed radical\nsigns."),0),
    MCDD(N_("RadicalExtraAscender:"),RadicalExtraAscender,RadicalExtraAscender_adjust,N_("Extra white space reserved above the radical."),0),
    MCDD(N_("RadicalKernBeforeDegree:"),RadicalKernBeforeDegree,RadicalKernBeforeDegree_adjust,N_("Extra horizontal kern before the degree of a\nradical if such be present."),0),
    MCDD(N_("RadicalKernAfterDegree:"),RadicalKernAfterDegree,RadicalKernAfterDegree_adjust,N_("Negative horizontal kern after the degree of a\nradical if such be present."),0),
    MCD(N_("RadicalDegreeBottomRaisePercent:"),RadicalDegreeBottomRaisePercent,N_("Height of the bottom of the radical degree, if\nsuch be present, in proportion to the ascender\nof the radical sign."),0),
    MCD(N_("MinConnectorOverlap:"),MinConnectorOverlap,N_("Minimum overlap of connecting glyphs during\nglyph construction."),1),
    { NULL }
};

static char *aspectnames[] = {
    N_("Constants"),
    N_("Sub/Superscript"),
    N_("Limits"),
    N_("Stacks"),
    N_("Fractions"),
    N_("Over/Underbars"),
    N_("Radicals"),
    N_("Connectors"),
    NULL
};

void MathInit(void) {
    int i;
    static int inited = false;

    if ( inited )
return;

    for ( i=0; aspectnames[i]!=NULL; ++i )
	aspectnames[i] = _(aspectnames[i]);
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i )
	math_constants_descriptor[i].ui_name=_(math_constants_descriptor[i].ui_name);
    inited = true;
}

struct MATH *MathTableNew(SplineFont *sf) {
    struct MATH *math = gcalloc(1,sizeof(struct MATH));	/* Too big for chunkalloc */
    int emsize = sf->ascent+sf->descent;
    DBounds b;
    SplineChar *sc;

    math->ScriptPercentScaleDown	= 80;
    math->ScriptScriptPercentScaleDown	= 60;
    math->DelimitedSubFormulaMinHeight	= emsize*1.5;
    /* No default given for math->DisplayOperatorMinHeight */
    /* No default given for math->AxisHeight */
    sc = SFGetChar(sf,'x',NULL);
    if ( sc!=NULL ) {
	SplineCharFindBounds(sc,&b);
	math->AccentBaseHeight = b.maxy;
    }
    sc = SFGetChar(sf,'I',NULL);
    if ( sc!=NULL ) {
	SplineCharFindBounds(sc,&b);
	math->FlattenedAccentBaseHeight = b.maxy;
    }
    if ( sf->pfminfo.subsuper_set )
	math->SubscriptShiftDown = sf->pfminfo.os2_subyoff;
    math->SubscriptTopMax = math->AccentBaseHeight;		/* X-height */
    /* No default given for math->SubscriptBaselineDropMin */
    if ( sf->pfminfo.subsuper_set )
	math->SuperscriptShiftUp = sf->pfminfo.os2_supyoff;
    /* No default given for math->SuperscriptShiftUpCramped */
    math->SuperscriptBottomMin = math->AccentBaseHeight;	/* X-height */
    /* No default given for math->SuperscriptBaselineDropMax */
    math->SubSuperscriptGapMin = 4*sf->uwidth;			/* 4* default rule thickness */
    math->SuperscriptBottomMaxWithSubscript = math->AccentBaseHeight;	/* X-height */
    math->SpaceAfterScript = emsize/24;				/* .5pt at 12pt */
    math->StackGapMin = 3*sf->uwidth;				/* 3* default rule thickness */
    math->StackDisplayStyleGapMin = 7*sf->uwidth;
    math->StretchStackGapAboveMin = math->UpperLimitGapMin;
    math->StretchStackGapBelowMin = math->LowerLimitGapMin;
    math->FractionNumeratorDisplayStyleShiftUp = math->StackTopDisplayStyleShiftUp;
    math->FractionDenominatorDisplayStyleShiftDown = math->StackBottomDisplayStyleShiftDown;
    math->FractionNumeratorGapMin = sf->uwidth;
    math->FractionNumeratorDisplayStyleGapMin = 3*sf->uwidth;
    math->FractionRuleThickness = sf->uwidth;
    math->FractionDenominatorGapMin = sf->uwidth;
    math->FractionDenominatorDisplayStyleGapMin = 3*sf->uwidth;
    math->OverbarVerticalGap = 3*sf->uwidth;
    math->OverbarRuleThickness = sf->uwidth;
    math->OverbarExtraAscender = sf->uwidth;
    math->UnderbarVerticalGap = 3*sf->uwidth;
    math->UnderbarRuleThickness = sf->uwidth;
    math->UnderbarExtraDescender = sf->uwidth;
    math->RadicalVerticalGap = sf->uwidth;
    math->RadicalExtraAscender = sf->uwidth;
    math->RadicalKernBeforeDegree = 5*emsize/18;
    math->RadicalKernAfterDegree = -10*emsize/18;
    math->RadicalDegreeBottomRaisePercent = 60;

    math->MinConnectorOverlap = emsize/50;
return( math );
}

void MATHFree(struct MATH *math) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int i;

    if ( math==NULL )
return;

    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	if ( math_constants_descriptor[i].devtab_offset>=0 )
	    DeviceTableFree( *(DeviceTable **) (((char *) math) + math_constants_descriptor[i].devtab_offset ) );
    }
#else
    if ( math==NULL )
return;
#endif
    free(math);
}

typedef struct mathdlg {
    GWindow gw;
    SplineFont *sf;
    struct MATH *math;
    int done;
    int ok;
} MathDlg;

static void MATH_Init(MathDlg *math) {
    int i;
    char buffer[20];

    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	GGadget *tf = GWidgetGetControl(math->gw,2*i+1);
	int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );

	sprintf( buffer, "%d", *pos );
	GGadgetSetTitle8(tf,buffer);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
	    GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
	    DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );
	    char *str;

	    DevTabToString(&str,*devtab);
	    if ( str!=NULL )
		GGadgetSetTitle8(tf2,str);
	    free(str);
	}
#endif
    }
}

static int MATH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int i,high,low;

	/* Two passes. First checks that everything is parsable */
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);
	    if ( err )
return( true );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		if ( !DeviceTableOK(str,&low,&high)) {
		    gwwv_post_error(_("Bad device table"), _("Bad device table for %s"),
			    math_constants_descriptor[i].ui_name);
return( true );
		}
		free(str);
	    }
#endif
	}

	/* Ok, if we got this far it should be legal */
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );
	    *pos = GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);

#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );

		*devtab = DeviceTableParse(*devtab,str);
		free(str);
	    }
#endif
	}
	math->sf->MATH = math->math;
	math->done = true;
	math->ok = true;
    }
return( true );
}

static int MATH_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	math->done = true;
    }
return( true );
}

static int math_e_h(GWindow gw, GEvent *event) {
    MathDlg *math = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	math->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("math.html");
return( true );
	}
return( false );
    }
return( true );
}

#define MAX_PAGE	9
#define MAX_ROW		12

void SFMathDlg(SplineFont *sf) {
    MathDlg md;
    int i, page, row;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[MAX_PAGE][MAX_ROW][3], boxes[MAX_PAGE][2],
	    *hvarray[MAX_PAGE][MAX_ROW+1][4], *harray[7], mgcd[4],
	    *varray[6], mboxes[3];
    GTextInfo label[MAX_PAGE][MAX_ROW], mlabel[3];
    GTabInfo aspects[MAX_PAGE+1];

    MathInit();

    memset(&md,0,sizeof(md));
    if ( sf->cidmaster ) sf = sf->cidmaster;
    md.sf = sf;
    md.math = sf->MATH;
    if ( md.math==NULL )
	md.math = MathTableNew(sf);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("MATH table");
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    md.gw = gw = GDrawCreateTopWindow(NULL,&pos,math_e_h,&md,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    memset(boxes,0,sizeof(boxes));
    memset(aspects,0,sizeof(aspects));

    page = row = 0;
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	if ( math_constants_descriptor[i].new_page ) {
	    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
	    hvarray[page][row][3] = NULL;
	    hvarray[page][row+1][0] = NULL;
	    ++page;
	    if ( page>=MAX_PAGE ) {
		IError( "Too many pages" );
return;
	    }
	    row = 0;
	}

	label[page][row].text = (unichar_t *) math_constants_descriptor[i].ui_name;
	label[page][row].text_is_1byte = true;
	label[page][row].text_in_resource = true;
	gcd[page][row][0].gd.label = &label[page][row];
	gcd[page][row][0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[page][row][0].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	gcd[page][row][0].creator = GLabelCreate;
	hvarray[page][row][0] = &gcd[page][row][0];

	gcd[page][row][1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[page][row][1].gd.pos.width = 50;
	gcd[page][row][1].gd.cid = 2*i+1;
	gcd[page][row][1].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	gcd[page][row][1].creator = GTextFieldCreate;
	hvarray[page][row][1] = &gcd[page][row][1];

#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( math_constants_descriptor[i].devtab_offset>=0 ) {
	    gcd[page][row][2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    gcd[page][row][2].gd.cid = 2*i+2;
	    gcd[page][row][2].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	    gcd[page][row][2].creator = GTextFieldCreate;
	    hvarray[page][row][2] = &gcd[page][row][2];
	} else
#endif
	    hvarray[page][row][2] = GCD_Glue;
	hvarray[page][row][3] = NULL;

	if ( ++row>=MAX_ROW ) {
	    IError( "Too many rows" );
return;
	}
    }
    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
    hvarray[page][row][3] = NULL;
    hvarray[page][row+1][0] = NULL;

    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	boxes[i][0].gd.flags = gg_enabled|gg_visible;
	boxes[i][0].gd.u.boxelements = hvarray[i][0];
	boxes[i][0].creator = GHVBoxCreate;

	aspects[i].text = (unichar_t *) aspectnames[i];
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = i!=0;
	aspects[i].gcd = boxes[i];
    }
    if ( i!=page+1 ) {	/* Page never gets its final increment */
	IError( "Page miscount %d in descriptor table, but only %d names.", page+1, i );
return;
    }

    memset(mgcd,0,sizeof(mgcd));
    memset(mlabel,0,sizeof(mlabel));
    memset(mboxes,0,sizeof(mboxes));

    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    /*mgcd[0].gd.cid = CID_Tabs;*/
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = MATH_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = MATH_Cancel;
    mgcd[2].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &mgcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &mgcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &mboxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,mboxes);
    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	GHVBoxSetExpandableCol(boxes[i][0].ret,2);
	GHVBoxSetExpandableRow(boxes[i][0].ret,gb_expandglue);
    }
    MATH_Init(&md);
    GHVBoxFitWindow(mboxes[0].ret);

    GDrawSetVisible(md.gw,true);

    while ( !md.done )
	GDrawProcessOneEvent(NULL);
    if ( sf->MATH==NULL && !md.ok )
	MATHFree(md.math);

    GDrawDestroyWindow(md.gw);
}
