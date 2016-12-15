#ifndef FONTFORGE_GLYPHCOMP_H
#define FONTFORGE_GLYPHCOMP_H

#include "splinefont.h"
#include "scripting.h"

#include <stdio.h>

enum font_compare_flags {
	fcf_outlines             = 1,
	fcf_exact                = 2,
	fcf_warn_not_exact       = 4,
	fcf_hinting              = 8,
	fcf_hintmasks            = 0x10,
	fcf_hmonlywithconflicts  = 0x20,
	fcf_warn_not_ref_exact   = 0x40,
	fcf_bitmaps              = 0x80,
	fcf_names                = 0x100,
	fcf_gpos                 = 0x200,
	fcf_gsub                 = 0x400,
	fcf_adddiff2sf1          = 0x800,
	fcf_addmissing           = 0x1000
};

enum Compare_Ret {
	SS_DiffContourCount     = 1,
	SS_MismatchOpenClosed   = 2,
	SS_DisorderedContours   = 4,
	SS_DisorderedStart      = 8,
	SS_DisorderedDirection  = 16,
	SS_PointsMatch          = 32,
	SS_ContourMatch         = 64,
	SS_NoMatch              = 128,
	SS_RefMismatch          = 256,
	SS_WidthMismatch        = 512,
	SS_VWidthMismatch       = 1024,
	SS_HintMismatch         = 2048,
	SS_HintMaskMismatch     = 4096,
	SS_LayerCntMismatch     = 8192,
	SS_ContourMismatch      = 16384,
	SS_UnlinkRefMatch       = 32768,

	BC_DepthMismatch        = 1<<16,
	BC_BoundingBoxMismatch  = 2<<16,
	BC_BitmapMismatch       = 4<<16,
	BC_NoMatch              = 8<<16,
	BC_Match                = 16<<16,

	SS_RefPtMismatch        = 32<<16
};

extern enum Compare_Ret BitmapCompare(BDFChar *bc1, BDFChar *bc2, int err, int bb_err);
extern enum Compare_Ret SSsCompare(const SplineSet *ss1, const SplineSet *ss2, real pt_err, real spline_err, SplinePoint **hmfail);
extern int CompareFonts(SplineFont *sf1, EncMap *map1, SplineFont *sf2, FILE *diffs, int flags);
extern int CompareGlyphs(Context *c, real pt_err, real spline_err, real pixel_off_frac, int bb_err, int comp_hints, int diffs_are_errors);
extern int CompareLayer(Context *c, const SplineSet *ss1, const SplineSet *ss2, const RefChar *refs1, const RefChar *refs2, real pt_err, real spline_err, const char *name, int diffs_are_errors, SplinePoint **_hmfail);
extern int LayersSimilar(Layer *ly1, Layer *ly2, double spline_err);

#endif /* FONTFORGE_GLYPHCOMP_H */
