/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
 *
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "basics.h"
#include "metrics.h"

typedef struct splinechar SplineChar;
typedef struct splinechar_ttf_map SplineCharTTFMap;
typedef struct splinefont SplineFont;
typedef struct metricsview MetricsView;
typedef struct encmap EncMap;

static const int INVALID_KERN_OFFSET = 0x7ffffff;
static const int FAKE_UNICODE_BASE = 0x110000;

char* u2utf8_copy(const unichar_t* ubuf);
int _WriteTTFFont(FILE* ttf, SplineFont* sf, int /*enum fontformat*/ format,
                  int32_t* bsizes, int /*enum bitmapformat*/ bf, int flags,
                  EncMap* enc, int layer);
extern SplineCharTTFMap* MakeGlyphTTFMap(SplineFont* sf);

/* Dummy incomplete type which can be casted to C++ type ff::shapers::IShaper */
typedef struct cpp_IShaper cpp_IShaper;

typedef struct shaper_context {
    SplineFont* sf;
    MetricsView* mv;

    // Apply OpenType lookups and compute positional info
    struct opentype_str* (*apply_ticked_features)(
        SplineFont* sf, uint32_t* flist, uint32_t script, uint32_t lang,
        bool gpos_only, int pixelsize, SplineChar** glyphs);

    // Get encoding map
    EncMap* (*get_enc_map)(SplineFont* sf);

    // Case-specific glyph width computation, not to be used in general case.
    int16_t (*get_char_width)(MetricsView* mv, SplineChar* sc);

    // Accessor for MetricsView::metrics
    MetricsCore* (*get_metrics)(MetricsView* mv, int* p_glyphcnt);

    // Retrieve current kerning value from glyph data
    int (*get_kern_offset)(struct opentype_str* glyph);

    // Legacy check of RTL script tag
    int (*script_is_rtl)(uint32_t script);

    // Make glyph - should be used only to create ".notdef" glyph, if it's
    // absent and necessary
    SplineChar* (*get_or_make_char)(SplineFont* sf, int unienc,
                                    const char* name);

} ShaperContext;

typedef struct shaper_def {
    /* Internal shaper name */
    const char* name;

    /* Shaper label, as shown to the user */
    const char* label;
} ShaperDef;

typedef struct feature_map {
    uint32_t feature_tag;
    bool enabled;
} FeatureMap;

struct shaper_out {
    /* Memory owned by this structure */
    struct opentype_str* glyphs;
    struct metrics_core* metrics;
};

const ShaperDef* get_shaper_defs();

/* The internal name of the default shaper */
void set_default_shaper(const char* name);
const char* get_default_shaper();

/* Create a new shaper object */
cpp_IShaper* shaper_factory(const char* name, ShaperContext* r_context);

/* Release the shaper object and nullify the pointer */
void shaper_free(cpp_IShaper** p_shaper);

/* Get the internal name of the shaper */
const char* shaper_name(cpp_IShaper* shaper);

/* Perform shaping: apply font features to the input string and compute position
 * of each glyph.
 *
 * Arguments:
 *     flist - zero-terminated list of OpenType features
 */
struct shaper_out shaper_apply_features(cpp_IShaper* shaper,
                                        SplineChar** glyphs,
                                        FeatureMap* feat_map, uint32_t script,
                                        uint32_t lang, int pixelsize,
                                        bool vertical);

void shaper_scale_metrics(cpp_IShaper* shaper, MetricsView* mv, double iscale,
                          double scale, bool vertical);

uint32_t* shaper_default_features(cpp_IShaper* shaper, uint32_t script,
                                  uint32_t lang, bool vertical);

#ifdef __cplusplus
}
#endif
