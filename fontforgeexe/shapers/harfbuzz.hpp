/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <map>
#include <memory>
#include <hb.h>

#include "i_shaper.hpp"
#include "shaper_shim.hpp"

namespace ff::shapers {

class HarfBuzzShaper : public IShaper {
 public:
    HarfBuzzShaper(std::shared_ptr<ShaperContext> context);
    ~HarfBuzzShaper();

    const char* name() const override { return "harfbuzz"; }

    ShaperOutput apply_features(SplineChar** glyphs,
                                const std::map<Tag, bool>& feature_map,
                                Tag script, Tag lang, int pixelsize,
                                bool vertical) override;

    void scale_metrics(MetricsView* mv, double iscale, double scale,
                       bool vertical) override;

    std::set<Tag> default_features(Tag script, Tag lang,
                                   bool vertical) const override;

 private:
    std::shared_ptr<ShaperContext> context_;

    char* blob = nullptr;
    hb_blob_t* hb_ttf_blob = nullptr;
    hb_face_t* hb_ttf_face = nullptr;
    hb_font_t* hb_ttf_font = nullptr;

    // Map glyph indexes in TTF file to SplineChar objects
    std::map<int, SplineChar*> ttf_map_;

    // Glyph used for missing characters.
    //
    // NOTE: normally HarfBuzz uses codepoint 0 ".notdef" for missing glyphs,
    // but its availability can't be guaranteed. Don't access this member
    // directly, use get_notdef_glyph() instead.
    SplineChar* notdef_glyph_ = nullptr;

    // Initial kerning state at font generation. For a pair of (left_glyph,
    // right_glyph) the shaper manually applies the difference between initial
    // and latest value to avoid regenerating the font at each change.
    //
    // NOTE: For completeness, this cache should have been keeping initial
    // kerning for each combination of features, but this would lead to
    // exponential storage, so we limit ourselves to the current combination, in
    // hope that it would be the same for all the other feature combinations.
    std::map<std::pair<hb_codepoint_t, hb_codepoint_t>, hb_position_t>
        initial_kerning_;

    // Initial width at font generation. The difference is applied similarly to
    // kerning deltas.
    //
    // NOTE: There is no need to keep glyph bearings, since HarfBuzz is not
    // responsible for glyph drawing, and the relative position of glyphs is
    // only affected by widths.
    std::map<hb_codepoint_t, hb_position_t> initial_width_;

    // Convert feature tags to HarfBuzz feature structures
    std::vector<hb_feature_t> hb_features(
        Tag script, Tag lang, bool vertical,
        const std::map<Tag, bool>& feature_map) const;

    // Retrieve data from shaped buffer and fill metrics.
    std::pair<SplineChar**, std::vector<MetricsCore>> extract_shaped_data(
        hb_buffer_t* hb_buffer);

    // RTL HarfBuzz shaping returns metrics end-to-start. This method reverses
    // them.
    std::vector<MetricsCore> reverse_rtl_metrics(
        const std::vector<MetricsCore>& reverse_metrics) const;

    std::vector<MetricsCore> reverse_ttb_metrics(
        const std::vector<MetricsCore>& bottom_up_metrics) const;

    // Compute changes in kerning due to user's input after the font was
    // generated.
    std::vector<int> compute_kerning_deltas(hb_buffer_t* hb_buffer,
                                            struct opentype_str* ots_arr);

    // Compute changes in glyph width due to user's input after the font was
    // generated.
    std::vector<int> compute_width_deltas(hb_buffer_t* hb_buffer,
                                          SplineChar** glyphs);

    const std::set<Tag>& default_features_by_direction(
        hb_direction_t dir) const;

    const std::set<Tag>& default_features_by_script(Tag script) const;

    std::set<Tag> default_features_collect(Tag script,
                                           hb_direction_t dir) const;

#ifdef HB_OT_SHAPE_PLAN_GET_FEATURE_TAGS
    std::set<Tag> default_features_from_plan(hb_script_t hb_script,
                                             hb_language_t hb_lang,
                                             hb_direction_t dir) const;
#endif

    SplineChar* get_notdef_glyph();
};

}  // namespace ff::shapers
