/*
 * FontView Menu IDs
 *
 * Range: 1000-1999
 * All IDs prefixed with FV_MID_ to avoid collisions with other views.
 *
 * This file is the single source of truth for FontView menu IDs.
 * Include this header in both fontview.c and any code that needs
 * to reference FontView menu IDs (e.g., menuchecks.cpp).
 */

#ifndef FONTFORGEEXE_FV_MIDS_H
#define FONTFORGEEXE_FV_MIDS_H

/* ===== View Menu (1000-1049) ===== */

/* Pixel sizes */
#define FV_MID_24               1000
#define FV_MID_36               1001
#define FV_MID_48               1002
#define FV_MID_72               1003
#define FV_MID_96               1004
#define FV_MID_128              1005

/* Display options */
#define FV_MID_AntiAlias        1006
#define FV_MID_FitToBbox        1007
#define FV_MID_DisplaySubs      1008

/* Cell layout */
#define FV_MID_32x8             1009
#define FV_MID_16x4             1010
#define FV_MID_8x2              1011

/* Metrics display */
#define FV_MID_ShowHMetrics     1012
#define FV_MID_ShowVMetrics     1013

/* Combinations */
#define FV_MID_Ligatures        1014
#define FV_MID_KernPairs        1015
#define FV_MID_AnchorPairs      1016

/* Navigation */
#define FV_MID_Next             1017
#define FV_MID_Prev             1018
#define FV_MID_NextDef          1019
#define FV_MID_PrevDef          1020

/* Other view options */
#define FV_MID_BitmapMag        1021
#define FV_MID_Layers           1022

/* ===== Edit Menu (1050-1099) ===== */

/* Undo/Redo */
#define FV_MID_Undo             1050
#define FV_MID_Redo             1051
#define FV_MID_UndoFontLevel    1052
#define FV_MID_RemoveUndoes     1053

/* Clipboard */
#define FV_MID_Cut              1054
#define FV_MID_Copy             1055
#define FV_MID_Paste            1056
#define FV_MID_Clear            1057
#define FV_MID_PasteInto        1058
#define FV_MID_PasteAfter       1059

/* Copy special */
#define FV_MID_CopyRef          1060
#define FV_MID_CopyWidth        1061
#define FV_MID_CopyLBearing     1062
#define FV_MID_CopyRBearing     1063
#define FV_MID_CopyVWidth       1064
#define FV_MID_CopyFgToBg       1065
#define FV_MID_CopyL2L          1066
#define FV_MID_CopyLookupData   1067

/* References */
#define FV_MID_UnlinkRef        1068
#define FV_MID_RplRef           1069
#define FV_MID_CorrectRefs      1070

/* Selection */
#define FV_MID_SelAll           1071
#define FV_MID_AllFonts         1072
#define FV_MID_DisplayedFont    1073

/* Other edit */
#define FV_MID_Join             1074
#define FV_MID_SameGlyphAs      1075
#define FV_MID_ClearBackground  1076
#define FV_MID_CharName         1077
#define FV_MID_TTFInstr         1078

/* ===== Font/Element Menu (1100-1199) ===== */

/* Info dialogs */
#define FV_MID_FontInfo         1100
#define FV_MID_CharInfo         1101

/* Transforms */
#define FV_MID_Transform        1102
#define FV_MID_NLTransform      1103
#define FV_MID_POV              1104

/* Path operations */
#define FV_MID_Stroke           1105
#define FV_MID_RmOverlap        1106
#define FV_MID_Simplify         1107
#define FV_MID_SimplifyMore     1108
#define FV_MID_Correct          1109
#define FV_MID_Intersection     1110
#define FV_MID_FindInter        1111

/* Build operations */
#define FV_MID_BuildAccent      1112
#define FV_MID_BuildComposite   1113
#define FV_MID_BuildDuplicates  1114

/* Bitmap operations */
#define FV_MID_AvailBitmaps     1115
#define FV_MID_RegenBitmaps     1116
#define FV_MID_RemoveBitmaps    1117
#define FV_MID_Autotrace        1118

/* Point operations */
#define FV_MID_Round            1119
#define FV_MID_AddExtrema       1120
#define FV_MID_AddInflections   1121
#define FV_MID_Balance          1122
#define FV_MID_Harmonize        1123
#define FV_MID_CleanupGlyph     1124
#define FV_MID_CanonicalStart   1125
#define FV_MID_CanonicalContours 1126
#define FV_MID_TilePath         1127

/* Font operations */
#define FV_MID_MergeFonts       1128
#define FV_MID_InterpolateFonts 1129
#define FV_MID_FontCompare      1130

/* Validation */
#define FV_MID_FindProblems     1131
#define FV_MID_Validate         1132

/* Styles */
#define FV_MID_Styles           1133
#define FV_MID_Embolden         1134
#define FV_MID_Condense         1135
#define FV_MID_Italic           1136
#define FV_MID_SmallCaps        1137
#define FV_MID_SubSup           1138
#define FV_MID_ChangeXHeight    1139
#define FV_MID_ChangeGlyph      1140

/* Dependencies */
#define FV_MID_ShowDependentRefs 1141
#define FV_MID_ShowDependentSubs 1142

/* Other element */
#define FV_MID_DefaultATT       1143
#define FV_MID_StrikeInfo       1144
#define FV_MID_MassRename       1145
#define FV_MID_SetColor         1146
#define FV_MID_SetExtremumBound 1147

/* ===== Hints Menu (1200-1249) ===== */

/* Auto hints */
#define FV_MID_AutoHint         1200
#define FV_MID_HintSubsPt       1201
#define FV_MID_AutoCounter      1202
#define FV_MID_DontAutoHint     1203

/* Clear */
#define FV_MID_ClearHints       1204
#define FV_MID_ClearWidthMD     1205
#define FV_MID_ClearInstrs      1206

/* Instructions */
#define FV_MID_AutoInstr        1207
#define FV_MID_EditInstructions 1208
#define FV_MID_Deltas           1209

/* Table editors */
#define FV_MID_Editfpgm         1210
#define FV_MID_Editprep         1211
#define FV_MID_Editcvt          1212
#define FV_MID_Editmaxp         1213
#define FV_MID_RmInstrTables    1214

/* Histograms */
#define FV_MID_HStemHist        1215
#define FV_MID_VStemHist        1216
#define FV_MID_BlueValuesHist   1217

/* ===== Metrics Menu (1250-1299) ===== */

/* Positioning */
#define FV_MID_Center           1250
#define FV_MID_Thirds           1251

/* Width/Bearings */
#define FV_MID_SetWidth         1252
#define FV_MID_SetLBearing      1253
#define FV_MID_SetRBearing      1254
#define FV_MID_SetBearings      1255
#define FV_MID_SetVWidth        1256

/* Kerning */
#define FV_MID_RmHKern          1257
#define FV_MID_RmVKern          1258
#define FV_MID_VKernByClass     1259
#define FV_MID_VKernFromH       1260

/* ===== File Menu (1300-1349) ===== */

/* Open */
#define FV_MID_OpenBitmap       1300
#define FV_MID_OpenOutline      1301
#define FV_MID_OpenMetrics      1302

/* Revert */
#define FV_MID_Revert           1303
#define FV_MID_RevertGlyph      1304
#define FV_MID_RevertToBackup   1305

/* Other file */
#define FV_MID_Recent           1306
#define FV_MID_Print            1307
#define FV_MID_ScriptMenu       1308
#define FV_MID_GenerateTTC      1309
#define FV_MID_ClearSpecialData 1310

/* ===== CID Menu (1350-1399) ===== */

/* Convert */
#define FV_MID_Convert2CID      1350
#define FV_MID_ConvertByCMap    1351

/* Flatten */
#define FV_MID_Flatten          1352
#define FV_MID_FlattenByCMap    1353

/* Insert/Remove */
#define FV_MID_InsertFont       1354
#define FV_MID_InsertBlank      1355
#define FV_MID_RemoveFromCID    1356

/* Other CID */
#define FV_MID_CIDFontInfo      1357
#define FV_MID_ChangeSupplement 1358

/* ===== Encoding Menu (1400-1449) ===== */

/* Reencode */
#define FV_MID_Reencode         1400
#define FV_MID_ForceReencode    1401

/* Add/Remove */
#define FV_MID_AddUnencoded     1402
#define FV_MID_RemoveUnused     1403
#define FV_MID_DetachGlyphs     1404
#define FV_MID_DetachAndRemoveGlyphs 1405

/* Encoding management */
#define FV_MID_LoadEncoding     1406
#define FV_MID_MakeFromFont     1407
#define FV_MID_RemoveEncoding   1408

/* Display */
#define FV_MID_DisplayByGroups  1409
#define FV_MID_Compact          1410
#define FV_MID_HideNoGlyphSlots 1411

/* Namelist */
#define FV_MID_SaveNamelist     1412
#define FV_MID_RenameGlyphs     1413
#define FV_MID_NameGlyphs       1414

/* ===== MM Menu (1450-1499) ===== */

#define FV_MID_CreateMM         1450
#define FV_MID_MMInfo           1451
#define FV_MID_MMValid          1452
#define FV_MID_ChangeMMBlend    1453
#define FV_MID_BlendToNew       1454

/* ===== Hangul (1500-1549) ===== */

#define FV_MID_ModifyComposition 1500
#define FV_MID_BuildSyllables   1501

/* ===== Other (1550-1599) ===== */

#define FV_MID_Warnings         1550

/*
 * Glyph label modes (0-3)
 *
 * These small values serve dual purpose:
 * 1. As enum values for the glyphlabel state
 * 2. As MIDs for the radio button menu items
 */
#define FV_MID_LabelGlyph       0
#define FV_MID_LabelName        1
#define FV_MID_LabelUnicode     2
#define FV_MID_LabelEncoding    3

#endif /* FONTFORGEEXE_FV_MIDS_H */
