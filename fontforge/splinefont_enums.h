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
#pragma once

/* Any additions to this enum should be accounted for in
 * splinechar.c:VSMaskFromFormat() . There are also tables
 * indexed by values of this enum scattered throughout the
 * code
 */
enum fontformat {
    ff_pfa,
    ff_pfb,
    ff_pfbmacbin,
    ff_multiple,
    ff_mma,
    ff_mmb,
    ff_ptype3,
    ff_ptype0,
    ff_cid,
    ff_cff,
    ff_cffcid,
    ff_type42,
    ff_type42cid,
    ff_ttf,
    ff_ttfsym,
    ff_ttfmacbin,
    ff_ttc,
    ff_ttfdfont,
    ff_otf,
    ff_otfdfont,
    ff_otfcid,
    ff_otfciddfont,
    ff_svg,
    ff_ufo,
    ff_ufo2,
    ff_ufo3,
    ff_woff_ttf,
    ff_woff_otf,
    ff_woff2_ttf,
    ff_woff2_otf,
    ff_none
};
#define isttf_ff(ff) ((ff) >= ff_ttf && (ff) <= ff_ttfdfont)
#define isttflike_ff(ff)                                               \
    (((ff) >= ff_ttf && (ff) <= ff_otfdfont) || (ff) == ff_woff_ttf || \
     (ff) == ff_woff2_ttf)

enum bitmapformat {
    bf_bdf,
    bf_ttf,
    bf_sfnt_dfont,
    bf_sfnt_ms,
    bf_otb,
    bf_nfntmacbin,
    /*bf_nfntdfont, */ bf_fon,
    bf_fnt,
    bf_palm,
    bf_ptype3,
    bf_none
};

enum ttf_flags {
    ttf_flag_shortps = 1 << 0,
    ttf_flag_nohints = 1 << 1,
    ttf_flag_applemode = 1 << 2,
    ttf_flag_pfed_comments = 1 << 3,
    ttf_flag_pfed_colors = 1 << 4,
    ttf_flag_otmode = 1 << 5,
    ttf_flag_glyphmap = 1 << 6,
    ttf_flag_TeXtable = 1 << 7,
    ttf_flag_ofm = 1 << 8,
    ttf_flag_oldkern = 1 << 9,  // never set in conjunction with applemode
    ttf_flag_noFFTMtable = 1 << 10,
    ttf_flag_pfed_lookupnames = 1 << 11,
    ttf_flag_pfed_guides = 1 << 12,
    ttf_flag_pfed_layers = 1 << 13,
    ttf_flag_symbol = 1 << 14,
    ttf_flag_dummyDSIG = 1 << 15,
    ttf_native_kern = 1 << 16,  // This applies mostly to U. F. O. right now.
    ttf_flag_fake_map =
        1 << 17,  // Set fake unicode mappings for unmapped glyphs
    ttf_flag_no_outlines =
        1 << 18,  // HarfBuzz: Skip "glyf" and "CFF " tables for performance
    ttf_flag_oldkernmappedonly =
        1 << 29,  // Allow only mapped glyphs in the old-style "kern" table,
                  // required for Windows compatibility
    ttf_flag_nomacnames = 1 << 30  // Don't autogenerate mac name entries
};

enum ttc_flags { ttc_flag_trymerge = 0x1, ttc_flag_cff = 0x2 };

enum openflags {
    of_fstypepermitted = 1,
    /*of_askcmap=2,*/ of_all_glyphs_in_ttc = 4,
    of_fontlint = 8,
    of_hidewindow = 0x10,
    of_all_tables = 0x20
};

enum ps_flags {
    ps_flag_nohintsubs = 0x10000,
    ps_flag_noflex = 0x20000,
    ps_flag_nohints = 0x40000,
    ps_flag_restrict256 = 0x80000,
    ps_flag_afm = 0x100000,
    ps_flag_pfm = 0x200000,
    ps_flag_tfm = 0x400000,
    ps_flag_round = 0x800000,
    /* CFF fonts are wrapped up in some postscript sugar -- unless they are to
     */
    /*  go into a pdf file or an otf font */
    ps_flag_nocffsugar = 0x1000000,
    /* in type42 cid fonts we sometimes want an identity map from gid to cid */
    ps_flag_identitycidmap = 0x2000000,
    ps_flag_afmwithmarks = 0x4000000,
    ps_flag_noseac = 0x8000000,
    ps_flag_outputfontlog = 0x10000000,
    ps_flag_mask = (ps_flag_nohintsubs | ps_flag_noflex | ps_flag_afm |
                    ps_flag_pfm | ps_flag_tfm | ps_flag_round)
};

enum layer_type {
    ly_all = -2,
    ly_grid = -1,
    ly_back = 0,
    ly_fore = 1,
    /* Possibly other foreground layers for type3 things */
    /* Possibly other background layers for normal fonts */
    ly_none = -3
};
