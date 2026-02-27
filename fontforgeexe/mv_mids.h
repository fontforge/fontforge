/*
 * MetricsView Menu IDs
 *
 * Range: 3000-3999
 * All IDs prefixed with MV_MID_ to avoid collisions with other views.
 *
 * This file is the single source of truth for MetricsView menu IDs.
 */

#ifndef FONTFORGEEXE_MV_MIDS_H
#define FONTFORGEEXE_MV_MIDS_H

/* ===== View Menu (3000-3049) ===== */

/* Zoom */
#define MV_MID_ZoomIn           3000
#define MV_MID_ZoomOut          3001
#define MV_MID_Bigger           3002
#define MV_MID_Smaller          3003

/* Display options */
#define MV_MID_Outline          3004
#define MV_MID_ShowGrid         3005
#define MV_MID_HideGrid         3006
#define MV_MID_PartialGrid      3007
#define MV_MID_HideGridWhenMoving 3008
#define MV_MID_AntiAlias        3009
#define MV_MID_Vertical         3010
#define MV_MID_RenderUsingHinting 3011

/* Navigation */
#define MV_MID_Next             3012
#define MV_MID_Prev             3013
#define MV_MID_NextDef          3014
#define MV_MID_PrevDef          3015
#define MV_MID_FindInFontView   3016

/* Combinations */
#define MV_MID_Ligatures        3017
#define MV_MID_KernPairs        3018
#define MV_MID_AnchorPairs      3019

/* Other view */
#define MV_MID_Layers           3020
#define MV_MID_PointSize        3021
#define MV_MID_SizeWindow       3022

/* ===== Edit Menu (3050-3099) ===== */

/* Undo/Redo */
#define MV_MID_Undo             3050
#define MV_MID_Redo             3051

/* Clipboard */
#define MV_MID_Cut              3052
#define MV_MID_Copy             3053
#define MV_MID_Paste            3054
#define MV_MID_Clear            3055
#define MV_MID_ClearSel         3056
#define MV_MID_Join             3057

/* Copy special */
#define MV_MID_CopyRef          3058
#define MV_MID_CopyWidth        3059
#define MV_MID_CopyLBearing     3060
#define MV_MID_CopyRBearing     3061
#define MV_MID_CopyVWidth       3062

/* References */
#define MV_MID_UnlinkRef        3063

/* Selection */
#define MV_MID_SelAll           3064

/* Character operations */
#define MV_MID_ReplaceChar      3065
#define MV_MID_InsertCharB      3066
#define MV_MID_InsertCharA      3067

/* ===== Element Menu (3100-3149) ===== */

/* Info */
#define MV_MID_CharInfo         3100
#define MV_MID_FindProblems     3101
#define MV_MID_ShowDependents   3102

/* Transform */
#define MV_MID_Transform        3103

/* Path operations */
#define MV_MID_Stroke           3104
#define MV_MID_RmOverlap        3105
#define MV_MID_Simplify         3106
#define MV_MID_SimplifyMore     3107
#define MV_MID_Correct          3108
#define MV_MID_Intersection     3109
#define MV_MID_FindInter        3110

/* Build */
#define MV_MID_BuildAccent      3111
#define MV_MID_BuildComposite   3112

/* Bitmap */
#define MV_MID_AvailBitmaps     3113
#define MV_MID_RegenBitmaps     3114
#define MV_MID_Autotrace        3115

/* Point operations */
#define MV_MID_Round            3116
#define MV_MID_AddExtrema       3117
#define MV_MID_AddInflections   3118
#define MV_MID_CleanupGlyph     3119
#define MV_MID_TilePath         3120

/* Effects/Styles */
#define MV_MID_Effects          3121

/* ===== Metrics Menu (3150-3199) ===== */

#define MV_MID_Center           3150
#define MV_MID_Thirds           3151
#define MV_MID_SetWidth         3152
#define MV_MID_SetLBearing      3153
#define MV_MID_SetRBearing      3154
#define MV_MID_SetBearings      3155
#define MV_MID_SetVWidth        3156
#define MV_MID_RemoveKerns      3157
#define MV_MID_RemoveVKerns     3158
#define MV_MID_VKernClass       3159
#define MV_MID_VKernFromHKern   3160

/* Display modes */
#define MV_MID_KernOnly         3161
#define MV_MID_WidthOnly        3162
#define MV_MID_BothKernWidth    3163

/* ===== File Menu (3200-3249) ===== */

#define MV_MID_OpenBitmap       3200
#define MV_MID_OpenOutline      3201
#define MV_MID_Recent           3202

/* ===== Other (3900-3999) ===== */

#define MV_MID_Warnings         3900
#define MV_MID_NextLineInWordList 3901
#define MV_MID_PrevLineInWordList 3902

#endif /* FONTFORGEEXE_MV_MIDS_H */
