/*
 * BitmapView Menu IDs
 *
 * Range: 4000-4999
 * All IDs prefixed with BV_MID_ to avoid collisions with other views.
 *
 * This file is the single source of truth for BitmapView menu IDs.
 */

#ifndef FONTFORGEEXE_BV_MIDS_H
#define FONTFORGEEXE_BV_MIDS_H

/* ===== View Menu (4000-4049) ===== */

/* Zoom/Fit */
#define BV_MID_Fit              4000
#define BV_MID_ZoomIn           4001
#define BV_MID_ZoomOut          4002
#define BV_MID_Bigger           4003
#define BV_MID_Smaller          4004

/* Navigation */
#define BV_MID_Next             4005
#define BV_MID_Prev             4006
#define BV_MID_NextDef          4007
#define BV_MID_PrevDef          4008

/* ===== Edit Menu (4050-4099) ===== */

/* Undo/Redo */
#define BV_MID_Undo             4050
#define BV_MID_Redo             4051
#define BV_MID_RemoveUndoes     4052

/* Clipboard */
#define BV_MID_Cut              4053
#define BV_MID_Copy             4054
#define BV_MID_Paste            4055
#define BV_MID_Clear            4056

/* References */
#define BV_MID_CopyRef          4057
#define BV_MID_UnlinkRef        4058

/* Selection */
#define BV_MID_SelAll           4059

/* ===== Element Menu (4100-4149) ===== */

#define BV_MID_GetInfo          4100
#define BV_MID_AvailBitmaps     4101
#define BV_MID_RegenBitmaps     4102

/* ===== Metrics Menu (4150-4199) ===== */

#define BV_MID_SetWidth         4150
#define BV_MID_SetVWidth        4151

/* ===== View Palettes (4200-4249) ===== */

#define BV_MID_Tools            4200
#define BV_MID_Layers           4201
#define BV_MID_Shades           4202
#define BV_MID_DockPalettes     4203

/* ===== File Menu (4250-4299) ===== */

#define BV_MID_Revert           4250
#define BV_MID_Recent           4251

/* ===== Other (4900-4999) ===== */

#define BV_MID_Warnings         4900

#endif /* FONTFORGEEXE_BV_MIDS_H */
