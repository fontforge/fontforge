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

#include <cassert>
#include <fstream>

extern "C" {
#include "splinechar.h"
#include "utype.h"
}

namespace ff::shapers {

HarfBuzzShaper::HarfBuzzShaper(std::shared_ptr<ShaperContext> context)
    : context_(context) {
    char temporary_ttf[200] = "\0";
    tmpnam(temporary_ttf);

    WriteTTFFont(
        temporary_ttf, context_->sf, 13 /* ff_ttf */, NULL, 1 /*bf_ttf*/,
        32 | (1 << 29) /* ttf_flag_otmode | ttf_flag_oldkernmappedonly */,
        context_->get_enc_map(context_->sf), 1 /*ly_fore*/);

    // Read file contents into memory
    std::ifstream ttf_stream(temporary_ttf);
    std::istreambuf_iterator<char> ttf_stream_it{ttf_stream}, end;
    std::vector<char> ttf_blob{ttf_stream_it, end};

    hb_ttf_blob = hb_blob_create(ttf_blob.data(), ttf_blob.size(),
                                 HB_MEMORY_MODE_DUPLICATE, NULL, NULL);

    hb_ttf_face = hb_face_create(hb_ttf_blob, 0);

    hb_ttf_font = hb_font_create(hb_ttf_face);
}

HarfBuzzShaper::~HarfBuzzShaper() {
    hb_font_destroy(hb_ttf_font);
    hb_face_destroy(hb_ttf_face);
    hb_blob_destroy(hb_ttf_blob);
}

struct opentype_str* HarfBuzzShaper::extract_shaped_data(
    hb_buffer_t* hb_buffer) {
    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info_arr =
        hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
    hb_glyph_position_t* glyph_pos_arr =
        hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

    // Return data as a raw C-style array
    struct opentype_str* ots_arr = (struct opentype_str*)calloc(
        glyph_count + 1, sizeof(struct opentype_str));

    // Adjust metrics buffer size
    metrics.resize(glyph_count + 1);

    // Process the glyphs and positions
    int total_x_advance = 0, total_y_advance = 0;
    for (int i = 0; i < glyph_count; ++i) {
        char glyph_name[64];
        hb_glyph_info_t& glyph_info = glyph_info_arr[i];
        hb_glyph_position_t& glyph_pos = glyph_pos_arr[i];
        struct opentype_str& ots = ots_arr[i];

        // Warning: after the shaping glyph_info->codepoint is not a Unicode
        // point, but rather an internal glyph index. We can't use it in our
        // functions.
        hb_bool_t found =
            hb_font_get_glyph_name(hb_ttf_font, glyph_info.codepoint,
                                   glyph_name, sizeof(glyph_name) - 1);
        SplineChar* glyph_out =
            context_->get_glyph_by_name(context_->sf, -1, glyph_name);

        ots.sc = glyph_out;

        // Fill unscaled metrics in font units
        hb_position_t h_advance =
            hb_font_get_glyph_h_advance(hb_ttf_font, glyph_info.codepoint);

        hb_glyph_extents_t extents;
        hb_bool_t res = hb_font_get_glyph_extents(
            hb_ttf_font, glyph_info.codepoint, &extents);
        assert(res);

        metrics[i].dwidth = h_advance;
        metrics[i].dheight = glyph_out->vwidth;

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

    return ots_arr;
}

struct opentype_str* HarfBuzzShaper::apply_features(
    SplineChar** glyphs, const std::vector<Tag>& feature_list, Tag script,
    Tag lang, int pixelsize) {
    std::vector<unichar_t> u_vec;
    for (size_t len = 0; glyphs[len] != NULL; ++len) {
        u_vec.push_back(
            (glyphs[len]->unicodeenc > 0)
                ? glyphs[len]->unicodeenc
                : context_->fake_unicode(context_->mv, glyphs[len]));
    }
    u_vec.push_back(0);

    char* utf8_str = u2utf8_copy(u_vec.data());

    hb_buffer_t* hb_buffer = hb_buffer_create();
    hb_buffer_add_utf8(hb_buffer, utf8_str, -1, 0, -1);

    // Set script and language
    hb_script_t hb_script = hb_script_from_iso15924_tag((uint32_t)script);
    hb_buffer_set_script(hb_buffer, hb_script);
    hb_language_t hb_lang = hb_language_from_string((const char*)lang, -1);
    hb_buffer_set_language(hb_buffer, hb_lang);

    bool rtl = !u_vec.empty() && isrighttoleft(u_vec[0]);
    hb_buffer_set_direction(hb_buffer,
                            rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);

    // Shape the text
    hb_shape(hb_ttf_font, hb_buffer, NULL, 0);

    // Retrieve the results
    struct opentype_str* ots_arr = extract_shaped_data(hb_buffer);

    // Perhaps counterintuitively, when setting RTL direction for RTL
    // languages, HarfBuzz would reverse the glyph order in the output
    // buffer. We therefore need to recompute metrics in reverse direction
    if (rtl) {
        std::vector<ShapeMetrics> reverse_metrics = metrics;

        // Note: metrics contain a trailing element for C compatibility
        int glyph_count = metrics.size() - 1;
        int16_t total_x_advance = metrics.back().dx;
        int16_t total_y_advance = metrics.back().dy;

        for (int i = 0; i < glyph_count; ++i) {
            int rev_idx = glyph_count - i - 1;
            metrics[i].dwidth = reverse_metrics[rev_idx].dwidth;
            metrics[i].dheight = reverse_metrics[rev_idx].dheight;

            metrics[i].xoff = -reverse_metrics[rev_idx].xoff;
            metrics[i].yoff = -reverse_metrics[rev_idx].yoff;

            metrics[i].dx = total_x_advance - reverse_metrics[rev_idx].dx -
                            reverse_metrics[rev_idx].dwidth;
            metrics[i].dy = total_y_advance - reverse_metrics[rev_idx].dy;
        }
        for (int i = 0; i < glyph_count / 2; ++i) {
            std::swap(ots_arr[i].sc, ots_arr[glyph_count - i - 1].sc);
        }
    }

    // Cleanup
    hb_buffer_destroy(hb_buffer);
    free(utf8_str);

    return ots_arr;
}

void HarfBuzzShaper::scale_metrics(MetricsView* mv, double iscale, double scale,
                                   bool vertical) {
    int x0 = 10, y0 = 10;
    for (auto& m : metrics) {
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

}  // namespace ff::shapers
