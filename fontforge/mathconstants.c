/* Copyright (C) 2007-2009 by George Williams */
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

#include "fontforgevw.h"
#ifdef __need_size_t
/* This is a bug on the mac, someone defines this and leaves it defined */
/*  that means when I load stddef.h it only defines size_t and doesn't */
/*  do offset_of, which is what I need */
# undef __need_size_t
#endif
#include <stddef.h>

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
    MCDD(N_("SuperscriptBaselineDropMax:"),SuperscriptBaselineDropMax,SuperscriptBaselineDropMax_adjust,N_("Maximum allowed drop of the baseline of\nsuperscripts realtive to the top of the base.\nUsed for bases that are treated as a box\nor extended shape. Positive for superscript\nbaseline below base top."),0),
    MCDD(N_("SubSuperscriptGapMin:"),SubSuperscriptGapMin,SubSuperscriptGapMin_adjust,N_("Minimum gap between the supersecript and subscript ink."),0),
    MCDD(N_("SuperscriptBottomMaxWithSubscript:"),SuperscriptBottomMaxWithSubscript,SuperscriptBottomMaxWithSubscript_adjust,N_("The maximum level to which the (ink) bottom\nof superscript can be pushed to increase the\ngap between superscript and subscript, before\nsubscript starts being moved down."),0),
    MCDD(N_("SpaceAfterScript:"),SpaceAfterScript,SpaceAfterScript_adjust,N_("Extra white space to be added after each\nub/superscript."),0),
    MCDD(N_("UpperLimitGapMin:"),UpperLimitGapMin,UpperLimitGapMin_adjust,N_("Minimum gap between the bottom of the\nupper limit, and the top of the base operator."),1),
    MCDD(N_("UpperLimitBaselineRiseMin:"),UpperLimitBaselineRiseMin,UpperLimitBaselineRiseMin_adjust,N_("Minimum distance between the baseline of an upper\nlimit and the bottom of the base operator."),0),
    MCDD(N_("LowerLimitGapMin:"),LowerLimitGapMin,LowerLimitGapMin_adjust,N_("Minimum gap between (ink) top of the lower limit,\nand (ink) bottom of the base operator."),0),
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
