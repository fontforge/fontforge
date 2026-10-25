/* Copyright (C) 2003-2012 by George Williams */
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

#include <math.h>
#include <algorithm>

#include "fontforge.h"
#include "autohint.h"
#include "dumppfa.h"
#include "splineutil.h"

#include "gtk/show_histogram_shim.hpp"

/* This operations are designed to work on a single font. NOT a CID collection*/
/*  A CID collection must be treated one sub-font at a time */

using HistogramMap = std::map<int, ff::dlg::HistogramBarRecord>;

static HistogramMap HistFindBlues(SplineFont* sf, int layer, uint8_t* selected,
                                  EncMap* map) {
    HistogramMap blues_map;

    for (int i = 0; i < (selected == NULL ? sf->glyphcnt : map->enccount);
         ++i) {
        int top, bottom;
        SplineChar* sc;
        DBounds b;

        int gid = selected == NULL ? i : map->map[i];
        if (gid != -1 && (sc = sf->glyphs[gid]) != NULL &&
            sc->layers[ly_fore].splines != NULL &&
            sc->layers[ly_fore].refs == NULL &&
            (selected == NULL || selected[i])) {
            SplineCharLayerFindBounds(sc, layer, &b);
            bottom = rint(b.miny);
            top = rint(b.maxy);
            if (top == bottom) continue;

            blues_map[top].count++;
            blues_map[top].glyph_names.push_back(sc->name);

            blues_map[bottom].count++;
            blues_map[bottom].glyph_names.push_back(sc->name);
        }
    }
    return blues_map;
}

static HistogramMap HistFindStemWidths(SplineFont* sf, int layer,
                                       uint8_t* selected, EncMap* map,
                                       int hor) {
    HistogramMap stems_map;

    for (int i = 0; i < (selected == NULL ? sf->glyphcnt : map->enccount);
         ++i) {
        int gid = selected == NULL ? i : map->map[i];
        SplineChar* sc;
        if (gid != -1 && (sc = sf->glyphs[gid]) != NULL &&
            sc->layers[ly_fore].splines != NULL &&
            sc->layers[ly_fore].refs == NULL &&
            (selected == NULL || selected[i])) {
            if (autohint_before_generate && sc->changedsincelasthinted &&
                !sc->manualhints)
                SplineCharAutoHint(sc, layer, NULL);
            for (StemInfo* stem = hor ? sc->hstem : sc->vstem; stem != NULL;
                 stem = stem->next) {
                if (stem->ghost) continue;
                int val = rint(stem->width);
                if (val <= 0) val = -val;

                stems_map[val].count++;
                stems_map[val].glyph_names.push_back(sc->name);
            }
        }
    }
    return stems_map;
}

static HistogramMap HistFindHStemWidths(SplineFont* sf, int layer,
                                        uint8_t* selected, EncMap* map) {
    return HistFindStemWidths(sf, layer, selected, map, true);
}

static HistogramMap HistFindVStemWidths(SplineFont* sf, int layer,
                                        uint8_t* selected, EncMap* map) {
    return HistFindStemWidths(sf, layer, selected, map, false);
}

static void HistSet(SplineFont* sf, struct psdict* private_dict,
                    const ff::dlg::UiStrings& ui_strings,
                    const ff::dlg::PrivateDictValues& result) {
    if ((result.primary.empty() || result.primary == "[]") &&
        (result.secondary.empty() || result.secondary == "[]") &&
        private_dict == NULL)
        return;

    if (private_dict == NULL) {
        sf->private_dict = private_dict =
            (psdict*)calloc(1, sizeof(struct psdict));
        private_dict->cnt = 10;
        private_dict->keys = (char**)calloc(10, sizeof(char*));
        private_dict->values = (char**)calloc(10, sizeof(char*));
    }
    PSDictChangeEntry(private_dict, ui_strings.primary_label.c_str(),
                      result.primary.c_str());
    PSDictChangeEntry(private_dict, ui_strings.secondary_label.c_str(),
                      result.secondary.c_str());
}

static bool CheckSmallSelection(uint8_t* selected, EncMap* map,
                                SplineFont* sf) {
    int cnt, tot;

    if (selected == NULL)
        // All glyphs are considered selected, so no "small selection" warning.
        return (false);

    for (int i = cnt = tot = 0; i < map->enccount; ++i) {
        int gid = map->map[i];
        if (gid != -1 && sf->glyphs[gid] != NULL) {
            ++tot;
            if (selected[i]) ++cnt;
        }
    }
    return ((cnt == 1 && tot > 1) || (cnt < 8 && tot > 30));
}

void SFHistogram(GWindow parent, SplineFont* sf, int layer,
                 struct psdict* private_dict, uint8_t* selected, EncMap* map,
                 enum hist_type which) {
    int j, upper_bound;
    HistogramMap values_map;
    ff::dlg::HistogramData dlg_data;

    if (private_dict == NULL) private_dict = sf->private_dict;
    switch (which) {
        case hist_hstem:
            values_map = HistFindHStemWidths(sf, layer, selected, map);
            break;
        case hist_vstem:
            values_map = HistFindVStemWidths(sf, layer, selected, map);
            break;
        case hist_blues:
            values_map = HistFindBlues(sf, layer, selected, map);
            break;
    }

    if (values_map.empty()) { /* Found nothing */
        dlg_data.lower_bound = upper_bound = 0;
    } else {
        dlg_data.lower_bound = values_map.begin()->first;
        upper_bound = values_map.rbegin()->first;
    }

    // Convert map to array for use in the histogram. Array of bars is
    // initialized with zeros.
    dlg_data.bars.resize(upper_bound - dlg_data.lower_bound + 1);
    for (auto& [key, value] : values_map) {
        // Purge duplicate glyph names from glyph list.
        auto last =
            std::unique(value.glyph_names.begin(), value.glyph_names.end());
        value.glyph_names.erase(last, value.glyph_names.end());

        dlg_data.bars[key - dlg_data.lower_bound] = value;
    }

    dlg_data.type = which;
    dlg_data.small_selection_warning = CheckSmallSelection(selected, map, sf);

    const ff::dlg::UiStrings& ui_strings =
        ff::dlg::kHistogramUiStrings.at(dlg_data.type);

    if ((j = PSDictFindEntry(private_dict, ui_strings.primary_label.c_str())) !=
        -1)
        dlg_data.initial_values.primary = private_dict->values[j];
    if ((j = PSDictFindEntry(private_dict,
                             ui_strings.secondary_label.c_str())) != -1)
        dlg_data.initial_values.secondary = private_dict->values[j];

    std::optional<ff::dlg::PrivateDictValues> result =
        ff::dlg::show_histogram_dialog(parent, dlg_data);

    if (result.has_value())
        HistSet(sf, private_dict, ui_strings, result.value());
}
