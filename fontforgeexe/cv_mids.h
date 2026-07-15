/*
 * CharView Menu IDs
 *
 * Range: 2000-2999
 * All IDs prefixed with CV_MID_ to avoid collisions with other views.
 *
 * This file is the single source of truth for CharView menu IDs.
 */

#ifndef FONTFORGEEXE_CV_MIDS_H
#define FONTFORGEEXE_CV_MIDS_H

/* ===== View Menu (2000-2099) ===== */

/* Zoom/Fit */
#define CV_MID_Fit              2000
#define CV_MID_ZoomIn           2001
#define CV_MID_ZoomOut          2002
#define CV_MID_Bigger           2003
#define CV_MID_Smaller          2004

/* Display toggles */
#define CV_MID_HidePoints       2005
#define CV_MID_HideControlPoints 2006
#define CV_MID_Fill             2007
#define CV_MID_HideRulers       2008
#define CV_MID_Preview          2009
#define CV_MID_DraggingComparisonOutline 2010
#define CV_MID_DisplayCompositions 2011
#define CV_MID_MarkExtrema      2012
#define CV_MID_MarkPointsOfInflection 2013
#define CV_MID_ShowCPInfo       2014
#define CV_MID_ShowTabs         2015
#define CV_MID_ShowSideBearings 2016
#define CV_MID_SnapOutlines     2017
#define CV_MID_ShowDebugChanges 2018

/* Navigation */
#define CV_MID_Next             2019
#define CV_MID_Prev             2020
#define CV_MID_NextDef          2021
#define CV_MID_PrevDef          2022
#define CV_MID_Goto             2023
#define CV_MID_FindInFontView   2024
#define CV_MID_Former           2025

/* Combinations */
#define CV_MID_KernPairs        2026
#define CV_MID_AnchorPairs      2027
#define CV_MID_Ligatures        2028
#define CV_MID_AnchorGlyph      2029
#define CV_MID_AnchorControl    2030

/* Grid fit */
#define CV_MID_ShowGridFit      2031
#define CV_MID_GridFitAA        2032
#define CV_MID_GridFitOff       2033
#define CV_MID_ShowGridFitLiveUpdate 2034

/* Point numbering */
#define CV_MID_PtsNone          2035
#define CV_MID_PtsTrue          2036
#define CV_MID_PtsPost          2037
#define CV_MID_PtsSVG           2038
#define CV_MID_PtsPos           2039

/* Hints display */
#define CV_MID_ShowHHints       2040
#define CV_MID_ShowVHints       2041
#define CV_MID_ShowDHints       2042
#define CV_MID_ShowBlueValues   2043
#define CV_MID_ShowFamilyBlues  2044
#define CV_MID_ShowAnchors      2045

/* Metrics display */
#define CV_MID_ShowHMetrics     2046
#define CV_MID_ShowVMetrics     2047

/* Other display */
#define CV_MID_ShowRefNames     2048
#define CV_MID_ShowAlmostHV     2049
#define CV_MID_ShowAlmostHVCurves 2050
#define CV_MID_DefineAlmost     2051

/* ===== Edit Menu (2100-2199) ===== */

/* Undo/Redo */
#define CV_MID_Undo             2100
#define CV_MID_Redo             2101
#define CV_MID_RemoveUndoes     2102

/* Clipboard */
#define CV_MID_Cut              2103
#define CV_MID_Copy             2104
#define CV_MID_Paste            2105
#define CV_MID_Clear            2106
#define CV_MID_Merge            2107
#define CV_MID_MergeToLine      2108
#define CV_MID_Join             2109

/* Copy special */
#define CV_MID_CopyRef          2110
#define CV_MID_CopyWidth        2111
#define CV_MID_CopyLBearing     2112
#define CV_MID_CopyRBearing     2113
#define CV_MID_CopyVWidth       2114
#define CV_MID_CopyFgToBg       2115
#define CV_MID_CopyBgToFg       2116
#define CV_MID_CopyGridFit      2117
#define CV_MID_CopyLookupData   2118

/* References */
#define CV_MID_UnlinkRef        2119

/* Selection */
#define CV_MID_SelAll           2120
#define CV_MID_SelNone          2121
#define CV_MID_SelInvert        2122
#define CV_MID_SelectWidth      2123
#define CV_MID_SelectVWidth     2124
#define CV_MID_SelectAllPoints  2125
#define CV_MID_SelectAnchors    2126
#define CV_MID_SelectHM         2127
#define CV_MID_SelectOpenContours 2128
#define CV_MID_Contours         2129
#define CV_MID_SelPointAt       2130

/* Point navigation */
#define CV_MID_NextPt           2131
#define CV_MID_PrevPt           2132
#define CV_MID_FirstPt          2133
#define CV_MID_NextCP           2134
#define CV_MID_PrevCP           2135
#define CV_MID_FirstPtNextCont  2136

/* ===== Element Menu (2200-2299) ===== */

/* Info */
#define CV_MID_CharInfo         2200
#define CV_MID_FontInfo         2201
#define CV_MID_GetInfo          2202

/* Direction */
#define CV_MID_Clockwise        2203
#define CV_MID_Counter          2204
#define CV_MID_Correct          2205
#define CV_MID_ReverseDir       2206

/* Path operations */
#define CV_MID_Stroke           2207
#define CV_MID_RmOverlap        2208
#define CV_MID_Simplify         2209
#define CV_MID_SimplifyMore     2210
#define CV_MID_Exclude          2211
#define CV_MID_Intersection     2212
#define CV_MID_FindInter        2213
#define CV_MID_ClipPath         2214

/* Build */
#define CV_MID_BuildAccent      2215
#define CV_MID_BuildComposite   2216

/* Bitmap */
#define CV_MID_AvailBitmaps     2217
#define CV_MID_RegenBitmaps     2218
#define CV_MID_RemoveBitmaps    2219
#define CV_MID_Autotrace        2220

/* Point operations */
#define CV_MID_Round            2221
#define CV_MID_RoundToCluster   2222
#define CV_MID_AddExtrema       2223
#define CV_MID_AddInflections   2224
#define CV_MID_Balance          2225
#define CV_MID_Harmonize        2226
#define CV_MID_CleanupGlyph     2227
#define CV_MID_CanonicalStart   2228
#define CV_MID_CanonicalContours 2229
#define CV_MID_TilePath         2230

/* Arrange */
#define CV_MID_Average          2231
#define CV_MID_SpacePts         2232
#define CV_MID_SpaceRegion      2233
#define CV_MID_MakeParallel     2234
#define CV_MID_Align            2235

/* Order */
#define CV_MID_First            2236
#define CV_MID_Earlier          2237
#define CV_MID_Later            2238
#define CV_MID_Last             2239

/* Styles */
#define CV_MID_Styles           2240
#define CV_MID_Embolden         2241
#define CV_MID_Condense         2242
#define CV_MID_Italic           2243
#define CV_MID_ChangeXHeight    2244
#define CV_MID_ChangeGlyph      2245

/* Dependencies */
#define CV_MID_ShowDependentRefs 2246
#define CV_MID_ShowDependentSubs 2247

/* Validation */
#define CV_MID_FindProblems     2248
#define CV_MID_CheckSelf        2249
#define CV_MID_GlyphSelfIntersects 2250

/* Other */
#define CV_MID_InsertText       2251

/* ===== Point Menu (2300-2399) ===== */

/* Point types */
#define CV_MID_Corner           2300
#define CV_MID_Tangent          2301
#define CV_MID_Curve            2302
#define CV_MID_HVCurve          2303

/* Spiro point types */
#define CV_MID_SpiroG4          2304
#define CV_MID_SpiroG2          2305
#define CV_MID_SpiroCorner      2306
#define CV_MID_SpiroLeft        2307
#define CV_MID_SpiroRight       2308
#define CV_MID_SpiroMakeFirst   2309

/* Point operations */
#define CV_MID_MakeFirst        2310
#define CV_MID_MakeLine         2311
#define CV_MID_MakeArc          2312
#define CV_MID_CenterCP         2313
#define CV_MID_ImplicitPt       2314
#define CV_MID_NoImplicitPt     2315
#define CV_MID_InsertPtOnSplineAt 2316
#define CV_MID_AddAnchor        2317
#define CV_MID_AcceptableExtrema 2318
#define CV_MID_NamePoint        2319
#define CV_MID_NameContour      2320

/* ===== Hints Menu (2400-2499) ===== */

/* Auto hint */
#define CV_MID_AutoHint         2400
#define CV_MID_HintSubsPt       2401
#define CV_MID_AutoCounter      2402
#define CV_MID_DontAutoHint     2403

/* Clear hints */
#define CV_MID_ClearHStem       2404
#define CV_MID_ClearVStem       2405
#define CV_MID_ClearDStem       2406

/* Add hints */
#define CV_MID_AddHHint         2407
#define CV_MID_AddVHint         2408
#define CV_MID_AddDHint         2409
#define CV_MID_CreateHHint      2410
#define CV_MID_CreateVHint      2411
#define CV_MID_MinimumDistance  2412

/* Instructions */
#define CV_MID_AutoInstr        2413
#define CV_MID_ClearInstr       2414
#define CV_MID_EditInstructions 2415
#define CV_MID_Debug            2416
#define CV_MID_Deltas           2417

/* Review */
#define CV_MID_ReviewHints      2418

/* ===== Metrics Menu (2500-2549) ===== */

#define CV_MID_Center           2500
#define CV_MID_Thirds           2501
#define CV_MID_SetWidth         2502
#define CV_MID_SetLBearing      2503
#define CV_MID_SetRBearing      2504
#define CV_MID_SetBearings      2505
#define CV_MID_SetVWidth        2506
#define CV_MID_RemoveKerns      2507
#define CV_MID_RemoveVKerns     2508
#define CV_MID_KPCloseup        2509
#define CV_MID_AnchorsAway      2510
#define CV_MID_VKernClass       2511
#define CV_MID_VKernFromHKern   2512

/* ===== View Palettes (2550-2599) ===== */

#define CV_MID_Tools            2550
#define CV_MID_Layers           2551
#define CV_MID_DockPalettes     2552

/* ===== File Menu (2600-2649) ===== */

#define CV_MID_New              2600
#define CV_MID_Open             2601
#define CV_MID_Close            2602
#define CV_MID_CloseTab         2603
#define CV_MID_Quit             2604
#define CV_MID_OpenBitmap       2605
#define CV_MID_Revert           2606
#define CV_MID_RevertGlyph      2607
#define CV_MID_Recent           2608
#define CV_MID_GenerateTTC      2609

/* ===== MM Menu (2700-2749) ===== */

#define CV_MID_MMReblend        2700
#define CV_MID_MMAll            2701
#define CV_MID_MMNone           2702

/* ===== Other (2900-2999) ===== */

#define CV_MID_Warnings         2900
#define CV_MID_NextLineInWordList 2901
#define CV_MID_PrevLineInWordList 2902

#endif /* FONTFORGEEXE_CV_MIDS_H */
