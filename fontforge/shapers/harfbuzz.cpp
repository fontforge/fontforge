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
#include "harfbuzz.hpp"

#include <algorithm>
#include <cassert>
#include <string>

#include <hb-ot.h>

extern "C" {
#include "gfile.h"
}

namespace ff::shapers {

static hb_bool_t resolve_fake_encoding(hb_font_t* font, void* font_data,
                                       hb_codepoint_t unicode,
                                       hb_codepoint_t* glyph, void* user_data) {
    hb_font_t* parent = (hb_font_t*)font_data;

    if (unicode >= FAKE_UNICODE_BASE) {
        *glyph = unicode - FAKE_UNICODE_BASE;
        return true;
    }
    return hb_font_get_nominal_glyph(parent, unicode, glyph);
}

HarfBuzzShaper::HarfBuzzShaper(std::shared_ptr<ShaperContext> context)
    : context_(context) {
    FILE* ttf_file = GFileTmpfile();

    SplineCharTTFMap* ttf_map =
        context_->write_font_into_memory(ttf_file, context_->sf);

    for (SplineCharTTFMap* entry = ttf_map; entry->glyph != NULL; ++entry) {
        ttf_map_[entry->ttf_glyph] = entry->glyph;
    }
    free(ttf_map);

    // Calculate file length
    fseek(ttf_file, 0L, SEEK_END);
    long bufsize = ftell(ttf_file);
    fseek(ttf_file, 0L, SEEK_SET);

    // Read the entire file into memory
    blob = new char[bufsize + 1];
    size_t blob_size = fread(blob, sizeof(char), bufsize, ttf_file);

    hb_ttf_blob =
        hb_blob_create(blob, blob_size, HB_MEMORY_MODE_WRITABLE, NULL, NULL);

    hb_ttf_face = hb_face_create(hb_ttf_blob, 0);
    hb_ttf_raw_font = hb_font_create(hb_ttf_face);

    // To access unencoded glyphs with HarfBuzz, we need to assign them fake
    // encodings above the canonic Unicode range. We also need to create a
    // subfont with custom encoding resolution callback.
    hb_ttf_font = hb_font_create_sub_font(hb_ttf_raw_font);
    hb_font_funcs_t* funcs = hb_font_funcs_create();
    hb_font_funcs_set_nominal_glyph_func(funcs, resolve_fake_encoding, NULL,
                                         NULL);
    hb_font_set_funcs(hb_ttf_font, funcs, hb_ttf_raw_font, NULL);

    // Contrary to the name, it just decreases the reference.
    hb_font_funcs_destroy(funcs);

    fclose(ttf_file);
}

HarfBuzzShaper::~HarfBuzzShaper() {
    hb_font_destroy(hb_ttf_font);
    hb_font_destroy(hb_ttf_raw_font);
    hb_face_destroy(hb_ttf_face);
    hb_blob_destroy(hb_ttf_blob);
    delete[] blob;
}

std::vector<hb_feature_t> HarfBuzzShaper::hb_features(
    Tag script, Tag lang, bool vertical,
    const std::map<Tag, bool>& feature_map) const {
    std::vector<hb_feature_t> hb_feature_vec;

    const std::set<Tag> default_feats =
        default_features(script, lang, vertical);

    for (const auto& [feature_tag, enabled] : feature_map) {
        // [khaledhosny] Some OpenType features like init, medi, etc. are
        // enabled by default and HarfBuzz applies them selectively based on
        // text analysis, but when enabled manually they will be applied
        // unconditionally which will break their intended use.
        //
        // If a default feature is selected in the UI, it should not be in the
        // features list, but if it is unselected it should be in the features
        // list with value set to 0 (disable).
        bool include_feature = !default_feats.count(feature_tag) || !enabled;
        if (include_feature) {
            hb_feature_t hb_feat{feature_tag, enabled, HB_FEATURE_GLOBAL_START,
                                 HB_FEATURE_GLOBAL_END};
            hb_feature_vec.push_back(hb_feat);
        }
    }

    return hb_feature_vec;
}

std::vector<MetricsCore> HarfBuzzShaper::extract_shaped_data(
    hb_buffer_t* hb_buffer) {
    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info_arr =
        hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
    hb_glyph_position_t* glyph_pos_arr =
        hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

    std::vector<MetricsCore> metrics(glyph_count + 1);

    // Process the glyphs and positions
    int total_x_advance = 0, total_y_advance = 0;
    for (int i = 0; i < glyph_count; ++i) {
        hb_glyph_info_t& glyph_info = glyph_info_arr[i];
        hb_glyph_position_t& glyph_pos = glyph_pos_arr[i];

        // Warning: after the shaping glyph_info->codepoint is not a Unicode
        // point, but rather an internal glyph index. We can't use it in our
        // functions.
        auto ttf_map_it = ttf_map_.find(glyph_info.codepoint);
        SplineChar* glyph_out = (ttf_map_it != ttf_map_.end())
                                    ? ttf_map_it->second
                                    : get_notdef_glyph();
        int16_t width, vwidth;
        context_->get_char_metrics(NULL, glyph_out, &width, &vwidth);

        metrics[i].sc = glyph_out;
        metrics[i].codepoint = glyph_info.codepoint;

        // Fill unscaled metrics in font units
        metrics[i].dwidth = width;
        metrics[i].dheight = vwidth;

        metrics[i].xoff = glyph_pos.x_offset;
        metrics[i].yoff = glyph_pos.y_offset;

        metrics[i].dx = total_x_advance;
        metrics[i].dy = total_y_advance;

        total_x_advance += glyph_pos.x_advance;
        total_y_advance += glyph_pos.y_advance;

        metrics[i].kernafter = 0;
        metrics[i].scaled = false;
    }

    // Fill the trailing empty object with auxiliaty data
    metrics.back().dx = total_x_advance;
    metrics.back().dy = total_y_advance;
    metrics.back().scaled = false;

    return metrics;
}

std::vector<MetricsCore> HarfBuzzShaper::reverse_rtl_metrics(
    const std::vector<MetricsCore>& reverse_metrics) const {
    std::vector<MetricsCore> fixed_metrics(reverse_metrics.size());

    // Note: metrics contain a trailing element for C compatibility
    int glyph_count = reverse_metrics.size() - 1;
    int16_t total_x_advance = reverse_metrics.back().dx;
    int16_t total_y_advance = reverse_metrics.back().dy;

    for (int i = 0; i < glyph_count; ++i) {
        int rev_idx = glyph_count - i - 1;
        fixed_metrics[i].sc = reverse_metrics[rev_idx].sc;
        fixed_metrics[i].codepoint = reverse_metrics[rev_idx].codepoint;
        fixed_metrics[i].dwidth = reverse_metrics[rev_idx].dwidth;
        fixed_metrics[i].dheight = reverse_metrics[rev_idx].dheight;

        fixed_metrics[i].xoff = -reverse_metrics[rev_idx].xoff;
        fixed_metrics[i].yoff = reverse_metrics[rev_idx].yoff;

        fixed_metrics[i].dx = total_x_advance - reverse_metrics[rev_idx].dx -
                              reverse_metrics[rev_idx].dwidth;
        fixed_metrics[i].dy = total_y_advance - reverse_metrics[rev_idx].dy;
    }

    return fixed_metrics;
}

std::vector<MetricsCore> HarfBuzzShaper::reverse_ttb_metrics(
    const std::vector<MetricsCore>& bottom_up_metrics) const {
    // Duplicate metrics
    std::vector<MetricsCore> fixed_metrics(bottom_up_metrics);

    hb_font_extents_t font_extents;
    hb_font_get_h_extents(hb_ttf_font, &font_extents);

    // Harfbuzz centers vertically one under another, but the legacy display
    // aligns them to the left. We apply uniform shift to keep the glyph
    // alignment and still place them roughly in the viewport.
    // NOTE: X offsets are normally negative.
    hb_position_t max_xoff =
        -std::min_element(bottom_up_metrics.begin(), bottom_up_metrics.end(),
                          [](const MetricsCore& a, const MetricsCore& b) {
                              return a.xoff < b.xoff;
                          })
             ->xoff;

    for (auto& m : fixed_metrics) {
        m.xoff = m.xoff + max_xoff;
        m.yoff = -m.yoff - font_extents.ascender;
        m.dy = -m.dy;
    }

    return fixed_metrics;
}

std::vector<int> HarfBuzzShaper::compute_kerning_deltas(
    const std::vector<MetricsCore>& metrics, struct opentype_str* ots_arr) {
    size_t glyph_count = metrics.size() - 1;  // Ignore auxiliary data

    // Retrieve the current kerning offsets and apply them manually if they
    // differ from their initial value. The initial value doesn't need
    // adjustment, since it is hopefully present in the generated font, so that
    // HarfBuzz takes it into account automatically.
    std::vector<int> kerning_deltas;
    for (int i = 0; i + 1 < glyph_count; ++i) {
        int kerning_offset = context_->get_kern_offset(ots_arr + i);
        if (kerning_offset == INVALID_KERN_OFFSET) {
            kerning_offset = 0;
        }

        // Keep initial kerning offsets when they are first encountered
        auto key =
            std::make_pair(metrics[i].codepoint, metrics[i + 1].codepoint);
        // Insert only if absent
        initial_kerning_.insert({key, kerning_offset});

        // Compute kerning deltas. Any existing kerning which was not manually
        // changed by the user should give zero delta.
        kerning_deltas.push_back(kerning_offset - initial_kerning_[key]);
    }

    return kerning_deltas;
}

std::vector<int> HarfBuzzShaper::compute_width_deltas(
    const std::vector<MetricsCore>& metrics) {
    size_t glyph_count = metrics.size() - 1;  // Ignore auxiliary data

    // Compute width deltas for glyphs which might have changed.
    std::vector<int> width_deltas;
    for (int i = 0; i < glyph_count; ++i) {
        int16_t width = metrics[i].dwidth;  // Glyph width before adjustments.

        // Keep initial widths when they are first encountered
        auto key = metrics[i].codepoint;
        // Insert only if absent
        initial_width_.insert({key, width});

        // Compute kerning deltas. Any existing kerning which was not manually
        // changed by the user should give zero delta.
        width_deltas.push_back(width - initial_width_[key]);
    }

    return width_deltas;
}

std::vector<MetricsCore> HarfBuzzShaper::apply_features(
    const std::vector<unichar_t>& ubuf, const std::map<Tag, bool>& feature_map,
    Tag script, Tag lang, bool vertical) {
    hb_buffer_t* hb_buffer = hb_buffer_create();
    hb_buffer_add_codepoints(hb_buffer, ubuf.data(), -1, 0, -1);

    // Set script and language
    hb_script_t hb_script = hb_ot_tag_to_script(script);
    hb_buffer_set_script(hb_buffer, hb_script);
    hb_language_t hb_lang = hb_ot_tag_to_language(lang);
    hb_buffer_set_language(hb_buffer, hb_lang);

    bool rtl = false;
    if (vertical) {
        hb_buffer_set_direction(hb_buffer, HB_DIRECTION_TTB);
    } else {
        // Script and language are set from UI, just guess the direction
        hb_buffer_guess_segment_properties(hb_buffer);
        rtl = (hb_buffer_get_direction(hb_buffer) == HB_DIRECTION_RTL);
    }

    auto hb_feature_vec = hb_features(script, lang, vertical, feature_map);

    // Shape the text
    hb_shape(hb_ttf_font, hb_buffer, hb_feature_vec.data(),
             hb_feature_vec.size());

    // Retrieve the results
    std::vector<MetricsCore> metrics = extract_shaped_data(hb_buffer);
    int glyph_count = metrics.size() - 1;

    // Perhaps counterintuitively, when setting RTL direction for RTL
    // languages, HarfBuzz would reverse the glyph order in the output
    // buffer. We therefore need to recompute metrics in reverse direction
    if (rtl) {
        metrics = reverse_rtl_metrics(metrics);
    }

    // Reverse vertical metrics from bottom-up direction (HarfBuzz convention)
    // to top-down (MetricsView convention)
    if (vertical) {
        metrics = reverse_ttb_metrics(metrics);
    }

    // Cleanup
    hb_buffer_destroy(hb_buffer);

    return metrics;
}

ShaperOutput HarfBuzzShaper::mv_apply_features(
    SplineChar** glyphs, const std::map<Tag, bool>& feature_map, Tag script,
    Tag lang, int pixelsize, bool vertical) {
    std::vector<unichar_t> u_vec;
    // Assigned fake encodings will be interpreted by resolve_fake_encoding().
    for (size_t len = 0; glyphs[len] != NULL; ++len) {
        int unicodeenc = -1, ttf_glyph = -1;
        context_->get_encoding(glyphs[len], &unicodeenc, &ttf_glyph);
        u_vec.push_back((unicodeenc > 0) ? unicodeenc
                                         : (ttf_glyph + FAKE_UNICODE_BASE));
    }
    u_vec.push_back(0);

    std::vector<MetricsCore> metrics =
        apply_features(u_vec, feature_map, script, lang, vertical);

    std::vector<int> width_deltas = compute_width_deltas(metrics);

    // glyphs_after_gpos is NULL-terminated thanks to metrics auxiliary data
    std::vector<SplineChar*> glyphs_after_gpos;
    std::transform(metrics.begin(), metrics.end(),
                   std::back_inserter(glyphs_after_gpos),
                   [](const MetricsCore& m) { return m.sc; });

    // Zero-terminated list of features
    std::vector<uint32_t> flist;
    for (const auto& [feature, enabled] : feature_map) {
        if (enabled) {
            flist.push_back(feature);
        }
    }
    flist.push_back(0);

    // Apply legacy shaper for GPOS to retrieve kerning pair references. Metrics
    // calculated by the legacy shaper are ignored, except for kerning deltas.
    struct opentype_str* ots_arr = context_->apply_ticked_features(
        context_->sf, flist.data(), (uint32_t)script, (uint32_t)lang, true,
        pixelsize, glyphs_after_gpos.data());

    // For simplicity, all characters are marked with the same direction.
    size_t glyph_count = metrics.size() - 1;  // Ignore auxiliary data
    hb_script_t hb_script = hb_ot_tag_to_script(script);
    bool rtl =
        (hb_script_get_horizontal_direction(hb_script) == HB_DIRECTION_RTL);
    for (int i = 0; i < glyph_count; ++i) {
        ots_arr[i].r2l = rtl;
    }

    std::vector<int> kerning_deltas = compute_kerning_deltas(metrics, ots_arr);

    // Compute the accumulated shifts dx for each glyph as partial sums of
    // kerning and width deltas. Adjust glyph widths as appropriate.
    int shift = 0;
    for (int i = 0; i < glyph_count; ++i) {
        if (i > 0) {
            shift += (kerning_deltas[i - 1] + width_deltas[i - 1]);
        }
        metrics[i].dx += shift;
        metrics[i].dwidth += width_deltas[i];
    }

    return {ots_arr, metrics};
}

void HarfBuzzShaper::scale_metrics(MetricsView* mv, MetricsCore* metrics,
                                   double iscale, double scale, bool vertical) {
    int glyphcnt = 0;
    int x0 = 10, y0 = 10;
    for (int i = 0; metrics[i].sc != NULL; ++i) {
        MetricsCore& m = metrics[i];
        assert(!m.scaled);
        m.dx = x0 + m.dx * scale;
        m.dy = y0 + m.dy * scale;
        m.dwidth *= scale;
        m.dheight *= scale;
        m.xoff *= scale;
        m.yoff *= scale;
        m.scaled = true;
    }
}

// From HarfBuzz hb_ot_shape_collect_features(), simplified and arranged by
// direction
const std::set<Tag>& HarfBuzzShaper::default_features_by_direction(
    hb_direction_t dir) const {
    static const std::set<Tag> features_ltr = {
        // basic
        "rvrn", "frac", "numr", "dnom", "rand", "trak",
        // ltr
        "ltra", "ltrm",
        // common_features[]
        "abvm", "blwm", "ccmp", "locl", "mark", "mkmk", "rlig",
        // horizontal_features[]
        "calt", "clig", "curs", "dist", "kern", "liga", "rclt"};

    static const std::set<Tag> features_rtl = {
        // basic
        "rvrn", "frac", "numr", "dnom", "rand", "trak",
        // rtl
        "rtla", "rtlm",
        // common_features[]
        "abvm", "blwm", "ccmp", "locl", "mark", "mkmk", "rlig",
        // horizontal_features[]
        "calt", "clig", "curs", "dist", "kern", "liga", "rclt"};

    static const std::set<Tag> features_ttb = {
        // basic
        "rvrn", "frac", "numr", "dnom", "rand", "trak",
        // common_features[]
        "abvm", "blwm", "ccmp", "locl", "mark", "mkmk", "rlig",
        // vertical
        "vert"};

    static const std::set<Tag> features_fallback = {
        // basic
        "rvrn", "frac", "numr", "dnom", "rand", "trak",
        // common_features[]
        "abvm", "blwm", "ccmp", "locl", "mark", "mkmk", "rlig"};

    switch (dir) {
        case HB_DIRECTION_LTR:
            return features_ltr;
        case HB_DIRECTION_RTL:
            return features_rtl;
        case HB_DIRECTION_TTB:
            return features_ttb;
        default:
            return features_fallback;
    }
}

const std::set<Tag>& HarfBuzzShaper::default_features_by_script(
    Tag script) const {
    // From HarfBuzz collect_features_arabic(), simplified:
    static const std::set<Tag> features_arabic = {
        "stch", "ccmp", "locl", "isol", "fina", "fin2", "fin3", "medi",
        "med2", "init", "rlig", "calt", "liga", "clig", "mset",
    };

    // From HarfBuzz collect_features_hangul(), simplified:
    static const std::set<Tag> features_hangul = {"ljmo", "vjmo", "tjmo"};

    // From HarfBuzz collect_features_use(), simplified:
    static const std::set<Tag> features_use = {
        "locl", "ccmp", "nukt", "akhn", "rphf", "pref", "rkrf", "abvf",
        "blwf", "half", "pstf", "vatu", "cjct", "isol", "init", "medi",
        "fina", "abvs", "blws", "haln", "pres", "psts",
    };

    // From HarfBuzz collect_features_indic(), simplified:
    static const std::set<Tag> features_indic = {
        "locl", "ccmp", "nukt", "akhn", "rphf", "rkrf", "pref",
        "blwf", "abvf", "half", "pstf", "vatu", "cjct", "init",
        "pres", "abvs", "blws", "psts", "haln",
    };

    // From HarfBuzz collect_features_khmer(), simplified:
    static const std::set<Tag> features_khmer = {
        "locl", "ccmp", "pref", "blwf", "abvf", "pstf",
        "cfar", "pres", "abvs", "blws", "psts",
    };

    // From HarfBuzz collect_features_myanmar(), simplified:
    static const std::set<Tag> features_myanmar = {
        "locl", "ccmp", "rphf", "pref", "blwf",
        "pstf", "pres", "abvs", "blws", "psts",
    };

    static const std::set<Tag> empty;

    if (script == "DFLT" || script == "Latn") {
        // No script-specific features
        return empty;
    } else if (script == "Arab" || script == "Syrc") {
        return features_arabic;
    } else if (script == "Hang") {
        return features_hangul;
    } else if (script == "Beng" || script == "Deva" || script == "Gujr" ||
               script == "Guru" || script == "Knda" || script == "Mlym" ||
               script == "Orya" || script == "Taml" || script == "Telu") {
        return features_indic;
    } else if (script == "Khmr") {
        return features_khmer;
    } else if (script == "Mymr") {
        return features_khmer;
    } else {
        return features_use;
    }
}

std::set<Tag> HarfBuzzShaper::default_features_collect(
    Tag script, hb_direction_t dir) const {
    std::set<Tag> features = default_features_by_direction(dir);
    const std::set<Tag>& features_by_script =
        default_features_by_script(script);

    features.insert(features_by_script.begin(), features_by_script.end());

    // Disable specific features according to HarfBuzz override_features_*
    if (script == "Hang") {
        features.erase("calt");
    } else if (script == "Khmr") {
        features.erase("liga");
    }

    return features;
}

#ifdef HB_OT_SHAPE_PLAN_GET_FEATURE_TAGS
// Code adapted from HarfBuzz test/api/test-shape-plan.c
std::set<Tag> HarfBuzzShaper::default_features_from_plan(
    hb_script_t hb_script, hb_language_t hb_lang, hb_direction_t dir) const {
    hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
    props.direction = dir;
    props.script = hb_script;
    props.language = hb_lang;

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_set_segment_properties(buffer, &props);
    hb_buffer_add_utf8(buffer, u8" ", -1, 0, -1);

    hb_shape_plan_t* shape_plan =
        hb_shape_plan_create(hb_ttf_face, &props, NULL, 0, NULL);
    hb_bool_t ret =
        hb_shape_plan_execute(shape_plan, hb_ttf_font, buffer, NULL, 0);

    // dummy call to check the total projected number of tags
    unsigned int count =
        hb_ot_shape_plan_get_feature_tags(shape_plan, 0, nullptr, nullptr);

    // actually retrieve the tags
    std::vector<hb_tag_t> features(count);
    hb_ot_shape_plan_get_feature_tags(shape_plan, 0, &count, features.data());

    return std::set<Tag>(features.begin(), features.end());
}
#endif

std::set<Tag> HarfBuzzShaper::default_features(Tag script, Tag lang,
                                               bool vertical) const {
    hb_script_t hb_script = hb_ot_tag_to_script(script);
    hb_language_t hb_lang = hb_ot_tag_to_language(lang);
    hb_direction_t dir = vertical
                             ? HB_DIRECTION_TTB
                             : hb_script_get_horizontal_direction(hb_script);

#ifdef HB_OT_SHAPE_PLAN_GET_FEATURE_TAGS
    return default_features_from_plan(hb_script, hb_lang, dir);
#else
    return default_features_collect(script, dir);
#endif
}

SplineChar* HarfBuzzShaper::get_notdef_glyph() {
    if (notdef_glyph_ != nullptr) {
        return notdef_glyph_;
    }

    // .notdef glyph should normally be at index 0. If the font doesn't contain
    // zero index, look for alternative options.
    //
    // Look for special glyph names, as appear in AssignNotdefNull().
    static const std::vector<std::string> notdef_names = {
        ".notdef", ".null",  "uni0000", "nonmarkingreturn",
        "uni000D", "glyph1", "glyph2"};
    for (const std::string& name : notdef_names) {
        for (const auto& [idx, sc] : ttf_map_) {
            if (name == context_->get_name(sc)) {
                notdef_glyph_ = sc;
                return notdef_glyph_;
            }
        }
    }

    // No special glyph found, create it.
    notdef_glyph_ = context_->get_or_make_char(context_->sf, -1, ".notdef");
    return notdef_glyph_;
}

}  // namespace ff::shapers
