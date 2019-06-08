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

#ifndef FONTFORGE_CHARVIEW_PRIVATE_H
#define FONTFORGE_CHARVIEW_PRIVATE_H

#include "gdraw.h"
#include "views.h"


#define MID_Fit		2001
#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_HidePoints	2004
#define MID_HideControlPoints	2005
#define MID_Fill	2006
#define MID_Next	2007
#define MID_Prev	2008
#define MID_HideRulers	2009
#define MID_Preview     2010
#define MID_DraggingComparisonOutline 2011
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_DisplayCompositions	2014
#define MID_MarkExtrema	2015
#define MID_Goto	2016
#define MID_FindInFontView	2017
#define MID_KernPairs	2018
#define MID_AnchorPairs	2019
#define MID_ShowGridFit 2020
#define MID_PtsNone	2021
#define MID_PtsTrue	2022
#define MID_PtsPost	2023
#define MID_PtsSVG	2024
#define MID_Ligatures	2025
#define MID_Former	2026
#define MID_MarkPointsOfInflection	2027
#define MID_ShowCPInfo	2028
#define MID_ShowTabs	2029
#define MID_AnchorGlyph	2030
#define MID_AnchorControl 2031
#define MID_ShowSideBearings	2032
#define MID_Bigger	2033
#define MID_Smaller	2034
#define MID_GridFitAA	2035
#define MID_GridFitOff	2036
#define MID_ShowHHints	2037
#define MID_ShowVHints	2038
#define MID_ShowDHints	2039
#define MID_ShowBlueValues	2040
#define MID_ShowFamilyBlues	2041
#define MID_ShowAnchors		2042
#define MID_ShowHMetrics	2043
#define MID_ShowVMetrics	2044
#define MID_ShowRefNames	2045
#define MID_ShowAlmostHV	2046
#define MID_ShowAlmostHVCurves	2047
#define MID_DefineAlmost	2048
#define MID_SnapOutlines	2049
#define MID_ShowDebugChanges	2050
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_Merge	2105
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyWidth	2111
#define MID_RemoveUndoes	2112
#define MID_CopyFgToBg	2115
#define MID_NextPt	2116
#define MID_PrevPt	2117
#define MID_FirstPt	2118
#define MID_NextCP	2119
#define MID_PrevCP	2120
#define MID_SelNone	2121
#define MID_SelectWidth	2122
#define MID_SelectVWidth	2123
#define MID_CopyLBearing	2124
#define MID_CopyRBearing	2125
#define MID_CopyVWidth	2126
#define MID_Join	2127
#define MID_CopyGridFit	2128
/*#define MID_Elide	2129*/
#define MID_SelectAllPoints	2130
#define MID_SelectAnchors	2131
#define MID_FirstPtNextCont	2132
#define MID_Contours	2133
#define MID_SelectHM	2134
#define MID_SelInvert	2136
#define MID_CopyBgToFg	2137
#define MID_SelPointAt	2138
#define MID_CopyLookupData	2139
#define MID_SelectOpenContours	2140
#define MID_MergeToLine	2141
#define MID_Clockwise	2201
#define MID_Counter	2202
#define MID_GetInfo	2203
#define MID_Correct	2204
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Stroke	2205
#define MID_RmOverlap	2206
#define MID_Simplify	2207
#define MID_BuildAccent	2208
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_Embolden	2217
#define MID_Condense	2218
#define MID_Average	2219
#define MID_SpacePts	2220
#define MID_SpaceRegion	2221
#define MID_MakeParallel	2222
#define MID_ShowDependentRefs	2223
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_Exclude	2228
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Styles	2231
#define MID_SimplifyMore	2232
#define MID_First	2233
#define MID_Earlier	2234
#define MID_Later	2235
#define MID_Last	2236
#define MID_CharInfo	2240
#define MID_ShowDependentSubs	2241
#define MID_CanonicalStart	2242
#define MID_CanonicalContours	2243
#define MID_RemoveBitmaps	2244
#define MID_RoundToCluster	2245
#define MID_Align		2246
#define MID_FontInfo		2247
#define MID_FindProblems	2248
#define MID_InsertText		2249
#define MID_Italic		2250
#define MID_ChangeXHeight	2251
#define MID_ChangeGlyph		2252
#define MID_CheckSelf		2253
#define MID_GlyphSelfIntersects	2254
#define MID_ReverseDir		2255
#define MID_Corner	2301
#define MID_Tangent	2302
#define MID_Curve	2303
#define MID_MakeFirst	2304
#define MID_MakeLine	2305
#define MID_CenterCP	2306
#define MID_ImplicitPt	2307
#define MID_NoImplicitPt	2308
#define MID_InsertPtOnSplineAt	2309
#define MID_AddAnchor	2310
#define MID_HVCurve	2311
#define MID_SpiroG4	2312
#define MID_SpiroG2	2313
#define MID_SpiroCorner	2314
#define MID_SpiroLeft	2315
#define MID_SpiroRight	2316
#define MID_SpiroMakeFirst 2317
#define MID_NamePoint	2318
#define MID_NameContour	2319
#define MID_AcceptableExtrema 2320
#define MID_MakeArc	2321
#define MID_ClipPath	2322

#define MID_AutoHint	2400
#define MID_ClearHStem	2401
#define MID_ClearVStem	2402
#define MID_ClearDStem	2403
#define MID_AddHHint	2404
#define MID_AddVHint	2405
#define MID_AddDHint	2406
#define MID_ReviewHints	2407
#define MID_CreateHHint	2408
#define MID_CreateVHint	2409
#define MID_MinimumDistance	2410
#define MID_AutoInstr	2411
#define MID_ClearInstr	2412
#define MID_EditInstructions 2413
#define MID_Debug	2414
#define MID_HintSubsPt	2415
#define MID_AutoCounter	2416
#define MID_DontAutoHint	2417
#define MID_Deltas	2418
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_DockPalettes	2503
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_RemoveKerns	2605
#define MID_SetVWidth	2606
#define MID_RemoveVKerns	2607
#define MID_KPCloseup	2608
#define MID_AnchorsAway	2609
#define MID_SetBearings	2610
#define MID_OpenBitmap	2700
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_RevertGlyph	2707
#define MID_Open	2708
#define MID_New		2709
#define MID_Close	2710
#define MID_Quit	2711
#define MID_CloseTab	2712
#define MID_GenerateTTC	2713
#define MID_VKernClass  2715
#define MID_VKernFromHKern 2716

#define MID_ShowGridFitLiveUpdate 2720

#define MID_MMReblend	2800
#define MID_MMAll	2821
#define MID_MMNone	2822
#define MID_NextLineInWordList  2823
#define MID_PrevLineInWordList  2824

#define MID_Warnings	3000

extern void CVMenuPointType(GWindow gw, struct gmenuitem *mi, GEvent *e);
extern void CVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void CVMergeToLine(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void CVLSelectLayer(CharView *cv, int layer);


#endif /* FONTFORGE_CHARVIEW_PRIVATE_H */
