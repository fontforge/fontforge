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
#include "builtin.hpp"

#include <assert.h>
#include <math.h>

extern "C" {
#include "splinechar.h"
}

namespace ff::shapers {

ShaperOutput BuiltInShaper::apply_features(
    SplineChar** glyphs, const std::map<Tag, bool>& feature_map, Tag script,
    Tag lang, int pixelsize, bool vertical) {
    // Zero-terminated list of enabled features
    std::vector<uint32_t> flist;
    for (const auto& [feature, enabled] : feature_map) {
        if (enabled) {
            flist.push_back(feature);
        }
    }
    flist.push_back(0);

    bool rtl = context_->script_is_rtl((uint32_t)script);

    ots_arr_ = context_->apply_ticked_features(context_->sf, flist.data(),
                                               (uint32_t)script, (uint32_t)lang,
                                               false, pixelsize, glyphs);

    // Count output glyphs and assign direction
    int cnt;
    for (cnt = 0; ots_arr_[cnt].sc != NULL; ++cnt) {
        ots_arr_[cnt].r2l = rtl;
    }

    // Make metrics C-style zero-terminated array to support legacy code
    std::vector<MetricsCore> metrics(cnt + 1);
    for (auto& m : metrics) {
        m.scaled = false;
    }

    return {ots_arr_, metrics};
}

void BuiltInShaper::scale_metrics(MetricsView* mv, double iscale, double scale,
                                  bool vertical) {
    MetricsCore* metrics = context_->get_metrics(mv, NULL);
    // Calculate positions.
    int x = 10;
    int y = 10;
    for (int i = 0; ots_arr_[i].sc != NULL; ++i) {
        assert(!metrics[i].scaled);
        SplineChar* sc = ots_arr_[i].sc;
        metrics[i].dwidth = rint(iscale * context_->get_char_width(mv, sc));
        metrics[i].dx = x;
        metrics[i].xoff = rint(iscale * ots_arr_[i].vr.xoff);
        metrics[i].yoff = rint(iscale * ots_arr_[i].vr.yoff);
        metrics[i].kernafter = rint(iscale * ots_arr_[i].vr.h_adv_off);
        x += metrics[i].dwidth + metrics[i].kernafter;

        metrics[i].dheight = rint(sc->vwidth * scale);
        metrics[i].dy = y;
        if (vertical) {
            metrics[i].kernafter = rint(iscale * ots_arr_[i].vr.v_adv_off);
            y += metrics[i].dheight + metrics[i].kernafter;
        }
        metrics[i].scaled = true;
    }
}

// Duplicates StdFeaturesOfScript()
std::set<Tag> BuiltInShaper::default_features(Tag script, Tag /*lang*/,
                                              bool /*vertical*/) const {
    static const std::set<Tag> simple_stdfeatures = {
        "ccmp", "loca", "kern", "liga", "calt", "mark", "mkmk"};
    static const std::set<Tag> arabic_stdfeatures = {
        "ccmp", "loca", "isol", "init", "medi", "fina", "rlig",
        "liga", "calt", "kern", "curs", "mark", "mkmk"};
    static const std::set<Tag> hebrew_stdfeatures = {
        "ccmp", "loca", "liga", "calt", "kern", "mark", "mkmk"};

    static const std::map<Tag, std::set<Tag>> script_2_std = {
        {"latn", simple_stdfeatures}, {"DFLT", simple_stdfeatures},
        {"cyrl", simple_stdfeatures}, {"grek", simple_stdfeatures},
        {"arab", arabic_stdfeatures}, {"hebr", hebrew_stdfeatures}};

    if (script_2_std.count(script)) {
        return script_2_std.at(script);
    } else {
        return simple_stdfeatures;
    }
}

}  // namespace ff::shapers
